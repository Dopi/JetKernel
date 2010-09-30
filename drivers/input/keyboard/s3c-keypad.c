/* drivers/input/keyboard/s3c-keypad.c
 *
 * Driver core for Samsung SoC onboard UARTs.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>

#include <linux/io.h>
#include <linux/irq.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/irq.h>

#include <plat/regs-gpio.h>
#include <plat/regs-keypad.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/s3c64xx-dvfs.h>

#include <message.h>
#include <dprintk.h>
#include "s3c-keypad.h"		

#undef S3C_KEYPAD_DEBUG 
//#define S3C_KEYPAD_DEBUG 

//#ifdef S3C_KEYPAD_DEBUG
#define DPRINTK(x...) printk(KERN_INFO "S3C-Keypad " x)
//#else
//#define DPRINTK(x...)		/* !!!! */
//#endif

#define DEVICE_NAME "s3c-keypad"	

#define TRUE 1
#define FALSE 0

#define KEYPAD_COREDUMP		0x111 
#define KEYPAD_CHANGEUART	0x10a 

static struct timer_list keypad_timer;
static struct timer_list powerkey_timer;
static int is_timer_on = FALSE;
static struct clk *keypad_clock;
struct resource *keypad_mem = NULL;		
struct resource *keypad_irq = NULL;	

struct resource *res = NULL;
struct input_dev *input_dev;
struct s3c_keypad *s3c_keypad;

extern struct task_struct *find_task_by_pid(pid_t nr);
static int keylock = 0;
static unsigned int process_id = 0;

static inline void input_report_keypad(struct input_dev *dev, unsigned int code, int value)
{
	if(keylock)
	{
		input_report_switch(input_dev, SW_LID, value);
		dprintk(DCM_INP, "[KEY] keylock is ON, so we report this key using EV_SW\n");
	}
	else
		input_report_key(input_dev, code, value);
}

static ssize_t keylock_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (!buf || (count == 0))
		return 0;

	if (strncmp(buf,"on", 2) == 0)
		keylock = 1;
	else if (strncmp(buf, "off", 3) == 0)
		keylock = 0;
	else if (strncmp(buf, "1", 1) == 0)
		keylock = 1;
	else if (strncmp(buf, "0", 1) == 0)
		keylock = 0;
	dprintk(DCM_INP, "[KEY] keylock setting is changed : %s\n", keylock ? "on" : "off");
	return count;
}

static ssize_t keylock_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	return sprintf(buf, "%s\n", keylock ? "on" : "off");
}

static DEVICE_ATTR(keylock, S_IRUGO | S_IWUSR, keylock_show, keylock_store);

static ssize_t storepid_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{	
	if (!buf || (count == 0))
		return 0;

	process_id = simple_strtol(buf, NULL, 10);

	return count;
}

static ssize_t storepid_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	return sprintf(buf, "PID:%d\n", process_id);
}

static DEVICE_ATTR(storepid, S_IRUGO | S_IWUSR, storepid_show, storepid_store);

void earjack_report_key(unsigned int keycode, int value)
{
    input_report_keypad(s3c_keypad->dev, KEY_SEND, value);
    dprintk(DCM_INP, "[KEY] earjack key %s\n", value ? "pressed" : "released");   
}
EXPORT_SYMBOL(earjack_report_key);

static int keypad_scan(u32 *keymask_low, u32 *keymask_high)
{
	int i,j = 0;
	u32 cval,rval;

	for (i=0; i<KEYPAD_COLUMNS; i++) {
		cval = readl(key_base+S3C_KEYIFCOL) | KEYCOL_DMASK;
		cval &= ~(1 << i);
		writel(cval, key_base+S3C_KEYIFCOL);

		udelay(KEYPAD_DELAY);

		rval = ~(readl(key_base+S3C_KEYIFROW)) & KEYROW_DMASK;
		
		if ((i*KEYPAD_ROWS) < MAX_KEYMASK_NR)
			*keymask_low |= (rval << (i * KEYPAD_ROWS));
		else {
			*keymask_high |= (rval << (j * KEYPAD_ROWS));
			j = j +1;
		}
	}

	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	return 0;
}

static unsigned change_uart_flag = 0;
static unsigned prevmask_low = 0, prevmask_high = 0, coredump_prevmask = 0, change_uart_prevmask = 0;
extern volans_set_uart_path(u32 uartpathValue);

static void keypad_timer_handler(unsigned long data)
{
	u32 keymask_low = 0, keymask_high = 0;
	u32 press_mask_low, press_mask_high;
	u32 release_mask_low, release_mask_high;
	int i;
	struct s3c_keypad *pdata = (struct s3c_keypad *)data;
	struct input_dev *dev = pdata->dev;

	keypad_scan(&keymask_low, &keymask_high);

	if (keymask_low != prevmask_low) {
		press_mask_low = ((keymask_low ^ prevmask_low) & keymask_low); 
		release_mask_low = ((keymask_low ^ prevmask_low) & prevmask_low); 

		i = 0;
		while (press_mask_low) {
			if (press_mask_low & 1) {
				input_report_keypad(dev,pdata->keycodes[i],1);
				dprintk(DCM_INP, "[KEY] L scancode[ %d (0x%x) ] press[%d].\n",  pdata->keycodes[i],pdata->keycodes[i], KEY_PRESSED);	
				DPRINTK("low Pressed  : %d\n",i);
			}
			press_mask_low >>= 1;
			i++;
		}

		i = 0;
		while (release_mask_low) {
			if (release_mask_low & 1) {
				input_report_keypad(dev,pdata->keycodes[i],0);
				dprintk(DCM_INP, "[KEY] L scancode[ %d (0x%x) ] press[%d].\n",  pdata->keycodes[i],pdata->keycodes[i], KEY_RELEASED);	
				DPRINTK("low Released : %d\n",i);
			}
			release_mask_low >>= 1;
			i++;
		}
		prevmask_low = keymask_low;
	}

	if (keymask_high != prevmask_high) {
		press_mask_high = ((keymask_high ^ prevmask_high) & keymask_high); 
		release_mask_high = ((keymask_high ^ prevmask_high) & prevmask_high);

		i = 0;
		while (press_mask_high) {
			if (press_mask_high & 1) {
				input_report_keypad(dev,pdata->keycodes[i+MAX_KEYMASK_NR],1);
				dprintk(DCM_INP, "[KEY] H scancode[ %d (0x%x) ] press[%d].\n",  pdata->keycodes[i],pdata->keycodes[i], KEY_PRESSED);	
				DPRINTK("high Pressed  : %d %d\n",pdata->keycodes[i+MAX_KEYMASK_NR],i);
			}
			press_mask_high >>= 1;
			i++;
		}

		i = 0;
		while (release_mask_high) {
			if (release_mask_high & 1) {
				input_report_keypad(dev,pdata->keycodes[i+MAX_KEYMASK_NR],0);
				dprintk(DCM_INP, "[KEY] H scancode[ %d (0x%x) ] press[%d].\n",  pdata->keycodes[i],pdata->keycodes[i], KEY_RELEASED);	
				DPRINTK("high Released : %d\n",pdata->keycodes[i+MAX_KEYMASK_NR]);
			}
			release_mask_high >>= 1;
			i++;
		}
		prevmask_high = keymask_high;
	}

	if (keymask_low | keymask_high) {
		mod_timer(&keypad_timer,jiffies + HZ/10);
	} else {
#ifdef	__CONFIG_WAKEUP_FAKE_KEY__
		input_report_keypad(dev, KEY_WAKEUP,1);
		input_report_keypad(dev, KEY_WAKEUP,0);

#endif
		writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
		is_timer_on = FALSE;
	}	
	coredump_prevmask = 0;
	change_uart_prevmask = 0;
}

static irqreturn_t s3c_keypad_isr(int irq, void *dev_id)
{
#ifdef CONFIG_CPU_FREQ
	//bss set_dvfs_perf_level(1);
#endif	/* CONFIG_CPU_FREQ */

	writel(readl(key_base+S3C_KEYIFCON) & ~(INT_F_EN|INT_R_EN), key_base+S3C_KEYIFCON);

	keypad_timer.expires = jiffies + (HZ/100);
	if ( is_timer_on == FALSE) {
		del_timer(&keypad_timer);
		add_timer(&keypad_timer);
		is_timer_on = TRUE;
	} else {
		mod_timer(&keypad_timer,keypad_timer.expires);
	}
	writel(KEYIFSTSCLR_CLEAR, key_base+S3C_KEYIFSTSCLR);

	return IRQ_HANDLED;
}

static int is_this_longkey = 0;
static void powerkey_timer_handler(unsigned long data)
{
	struct s3c_keypad *pdata = (struct s3c_keypad *)data;
	struct input_dev *dev = pdata->dev;

	int level = gpio_get_value(GPIO_nPOWER);

	if (level == GPIO_LEVEL_LOW) {
		input_report_key(dev, KEY_POWER2, 1);
		is_this_longkey = 1;

	}

	else {
		is_this_longkey = 0;
	}

	del_timer(&powerkey_timer);
}

static irqreturn_t powerkey_handler(int irq, void *dev_id)
{
	static struct timeval tv;
	static int time_check = 0;

	struct s3c_keypad *pdata = (struct s3c_keypad *)dev_id;
	int level = gpio_get_value(GPIO_nPOWER);

	if (!time_check) {
		do_gettimeofday(&tv);
		time_check = 1;
	}
	else {
		struct timeval tv2;
		do_gettimeofday(&tv2);

		if (tv2.tv_usec - tv.tv_usec > 0 && tv2.tv_usec - tv.tv_usec < 1000) {
			do_gettimeofday(&tv);
			return IRQ_HANDLED;
		}

		do_gettimeofday(&tv);
	}

#ifdef CONFIG_CPU_FREQ
	//bss set_dvfs_perf_level(1);
#endif	/* CONFIG_CPU_FREQ */


	if (level == GPIO_LEVEL_LOW) {
		del_timer(&powerkey_timer);
		powerkey_timer.expires = (jiffies + (2*HZ));
		add_timer(&powerkey_timer);
	}

	else {
		if (is_this_longkey) {
			input_report_key(pdata->dev, KEY_POWER2, 0);
			is_this_longkey = 0;

		}

		else {
			del_timer(&powerkey_timer);

			input_report_keypad(pdata->dev, KEY_POWER, 1);

			input_report_keypad(pdata->dev, KEY_POWER, 0);

		}
	}

	return IRQ_HANDLED;
}

static int __init s3c_keypad_probe(struct platform_device *pdev)
{

int ret, size;
	int key, code;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev,"no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;

	keypad_mem = request_mem_region(res->start, size, pdev->name);
	if (keypad_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	key_base = ioremap(res->start, size);
	if (key_base == NULL) {
		printk(KERN_ERR "Failed to remap register block\n");
		ret = -ENOMEM;
		goto err_map;
	}

	keypad_clock = clk_get(&pdev->dev, "keypad");
	//if (IS_ERR(keypad_clock)) {
	//	dev_err(&pdev->dev, "failed to find keypad clock source\n");
	//	ret = PTR_ERR(keypad_clock);
	//	goto err_clk;
	//}

	clk_enable(keypad_clock);
	
	s3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL);
	input_dev = input_allocate_device();

	if (!s3c_keypad || !input_dev) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	platform_set_drvdata(pdev, s3c_keypad);
	s3c_keypad->dev = input_dev;
	
	writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
	writel(KEYIFFC_DIV, key_base+S3C_KEYIFFC);

	s3c_setup_keypad_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);

	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);
	
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SW, input_dev->evbit);
    //set_bit(KEY_EXT_ROTATE_0, input_dev->keybit);
    //set_bit(KEY_EXT_ROTATE_90, input_dev->keybit);
    //set_bit(KEY_EXT_ROTATE_180, input_dev->keybit);
    //set_bit(KEY_EXT_ROTATE_270, input_dev->keybit);

	//set_bit(KEY_POWER, input_dev->keybit);
	//set_bit(KEY_POWER2, input_dev->keybit);

        //set_bit(SW_LID, input_dev->swbit);

	s3c_keypad->nr_rows = KEYPAD_ROWS;
	s3c_keypad->no_cols = KEYPAD_COLUMNS;
	s3c_keypad->total_keys = MAX_KEYPAD_NR;

	for(key = 0; key < s3c_keypad->total_keys; key++){
		code = s3c_keypad->keycodes[key] = keypad_keycode[key];
		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, input_dev->keybit);
		
	}

	input_dev->name = DEVICE_NAME;
	input_dev->phys = "s3c-keypad/input0";
	
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;

	input_dev->keycode = keypad_keycode;
#ifdef __CONFIG_KEYLOCK__
	input_dev->keycodesize = sizeof(int);
	input_dev->keycodemax = s3c_keypad->total_keys;
#endif

	init_timer(&keypad_timer);
	keypad_timer.function = keypad_timer_handler;
	keypad_timer.data = (unsigned long)s3c_keypad;

	init_timer(&powerkey_timer);
	powerkey_timer.function = powerkey_timer_handler;
	powerkey_timer.data = (unsigned long)s3c_keypad;

	keypad_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (keypad_irq == NULL) {
		dev_err(&pdev->dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq;
	}

	ret = request_irq(IRQ_KEYPAD, s3c_keypad_isr, IRQF_SAMPLE_RANDOM,
		DEVICE_NAME, (void *) pdev);
	if (ret) {
		printk("request_irq failed (IRQ_KEYPAD) !!!\n");
		ret = -EIO;
		goto err_irq;
	}

	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
		goto out;
	}

	keypad_timer.expires = jiffies + (HZ/10);

	if (is_timer_on == FALSE) {
		add_timer(&keypad_timer);
		is_timer_on = TRUE;
	} else {
		mod_timer(&keypad_timer,keypad_timer.expires);
	}

	s3c_gpio_cfgpin(GPIO_nPOWER, S3C_GPIO_SFN(GPIO_nPOWER_AF));
	s3c_gpio_setpull(GPIO_nPOWER, S3C_GPIO_PULL_NONE);

	set_irq_type(IRQ_EINT(5), IRQ_TYPE_EDGE_BOTH);
	if (request_irq(IRQ_EINT(5), powerkey_handler, IRQF_SAMPLE_RANDOM,
				"keypad", s3c_keypad)) {
		// TODO: ...
	}

	ret = device_create_file(&input_dev->dev, &dev_attr_keylock);
	ret = device_create_file(&input_dev->dev, &dev_attr_storepid);

	printk( DEVICE_NAME " Initialized\n");
	return 0;

out:
	free_irq(keypad_irq->start, input_dev );
	free_irq(keypad_irq->end, input_dev );
	
err_irq:
	input_free_device(input_dev);
	kfree(s3c_keypad);
	
err_alloc:
	clk_disable(keypad_clock);
	clk_put(keypad_clock);

err_clk:
	iounmap(key_base);

err_map:
	release_resource(keypad_mem);
	kfree(keypad_mem);

err_req:
	return ret;
}

static int s3c_keypad_remove(struct platform_device *pdev)
{
	struct input_dev *input_dev = platform_get_drvdata(pdev);
	writel(KEYIFCON_CLEAR, key_base+S3C_KEYIFCON);

	if(keypad_clock) {
		clk_disable(keypad_clock);
		clk_put(keypad_clock);
		keypad_clock = NULL;
	}

	input_unregister_device(input_dev);
	iounmap(key_base);

	free_irq(IRQ_EINT(5), pdev->dev.platform_data);

	kfree(pdev->dev.platform_data);
	free_irq(IRQ_KEYPAD, (void *) pdev);

	del_timer(&keypad_timer);	
	printk(DEVICE_NAME " Removed.\n");

	return 0;
}

#ifdef CONFIG_PM
static unsigned int keyifcon, keyiffc;

static int s3c_keypad_suspend(struct platform_device *dev, pm_message_t state)
{
	keyifcon = readl(key_base+S3C_KEYIFCON);
	keyiffc = readl(key_base+S3C_KEYIFFC);
	
	disable_irq(IRQ_KEYPAD);

	clk_disable(keypad_clock);

	return 0;
}

static int s3c_keypad_resume(struct platform_device *dev)
{
	clk_enable(keypad_clock);

	writel(keyifcon, key_base+S3C_KEYIFCON);
	writel(keyiffc, key_base+S3C_KEYIFFC);

	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL); 
	
	enable_irq(IRQ_KEYPAD);

	return 0;
}
#else
#define s3c_keypad_suspend NULL
#define s3c_keypad_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver s3c_keypad_driver = {
	.probe		= s3c_keypad_probe,
	.remove		= s3c_keypad_remove,
	.suspend	= s3c_keypad_suspend,
	.resume		= s3c_keypad_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DEVICE_NAME,
	},
};

static int __init s3c_keypad_init(void)
{
	int ret;

	ret = platform_driver_register(&s3c_keypad_driver);
	
	if(!ret)
	   printk(KERN_INFO "S3C Keypad Driver\n");

	return ret;
}

static void __exit s3c_keypad_exit(void)
{
	platform_driver_unregister(&s3c_keypad_driver);
}

module_init(s3c_keypad_init);
module_exit(s3c_keypad_exit);

MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("KeyPad interface for Samsung S3C");
