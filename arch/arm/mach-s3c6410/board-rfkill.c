/*
 * Copyright (C) 2008 Google, Inc.
 * Author: Nick Pelly <npelly@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* Control bluetooth power for S3C6410 platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/gpio.h>
#ifdef CONFIG_MACH_SPICA
#include <mach/spica.h>		/*Updated by kumar.gvs 22 Apr 2009*/
#else 
#ifdef CONFIG_MACH_INSTINCTQ
#include <mach/instinctq.h>	/*Updated by kumar.gvs 22 Apr 2009*/
#else 
#ifdef CONFIG_MACH_JET
#include <mach/jet.h>
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
#include <plat/gpio-cfg.h>
#include <plat/egpio.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <mach/hardware.h>
#include <linux/i2c/pmic.h>

#define BT_SLEEP_ENABLER

#define IRQ_BT_HOST_WAKE      IRQ_EINT(22)

static struct wake_lock rfkill_wake_lock;
static struct wake_lock bt_wake_lock;

void rfkill_switch_all(enum rfkill_type type, enum rfkill_state state);

extern void s3c_setup_uart_cfg_gpio(unsigned char port);
extern void s3c_reset_uart_cfg_gpio(unsigned char port);

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4325";

static int bluetooth_set_power(void *data, enum rfkill_state state)
{
	unsigned int ret = 0; 
	switch (state) {

		case RFKILL_STATE_UNBLOCKED:
#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] Device Powering ON \n");
#else
			printk(KERN_DEBUG "[BT] Device Powering ON \n");
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
			s3c_setup_uart_cfg_gpio(1);

			if (gpio_is_valid(GPIO_BT_WLAN_REG_ON))
			{
				ret = gpio_request(GPIO_BT_WLAN_REG_ON, S3C_GPIO_LAVEL(GPIO_BT_WLAN_REG_ON));
				if (ret < 0) {
#ifdef CONFIG_MACH_INSTINCTQ
					printk("[BT] Failed to request GPIO_BT_WLAN_REG_ON!\n");
#else
                                        printk(KERN_ERR "[BT] Failed to request GPIO_BT_WLAN_REG_ON!\n");
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
					return ret;
				}
				gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);
			}

			if (gpio_is_valid(GPIO_BT_RST_N))
			{
				ret = gpio_request(GPIO_BT_RST_N, S3C_GPIO_LAVEL(GPIO_BT_RST_N));
				if (ret < 0) {
					gpio_free(GPIO_BT_WLAN_REG_ON);
#ifdef CONFIG_MACH_INSTINCTQ
					printk("[BT] Failed to request GPIO_BT_RST_N!\n");
#else
                                        printk(KERN_ERR "[BT] Failed to request GPIO_BT_RST_N!\n");
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
					return ret;			
				}
				gpio_direction_output(GPIO_BT_RST_N, GPIO_LEVEL_LOW);
			}

			/* Set GPIO_BT_WLAN_REG_ON high */ 
			s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT1);  
			s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));		

			mdelay(150);  // 100msec, delay  between reg_on & rst. (bcm4325 powerup sequence)
#else
                        printk(KERN_DEBUG "[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));              
 
                        msleep(150);  // 100msec, delay  between reg_on & rst. (bcm4325 powerup sequence)
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */



			/* Set GPIO_BT_RST_N high */
			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT1);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);

#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] GPIO_BT_RST_N = %d\n", gpio_get_value(GPIO_BT_RST_N));
#else
                        printk(KERN_DEBUG "[BT] GPIO_BT_RST_N = %d\n", gpio_get_value(GPIO_BT_RST_N));
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */

			gpio_free(GPIO_BT_RST_N);
			gpio_free(GPIO_BT_WLAN_REG_ON);

			break;

		case RFKILL_STATE_SOFT_BLOCKED:
#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] Device Powering OFF \n");
#else
                        printk(KERN_DEBUG "[BT] Device Powering OFF \n");
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
			s3c_reset_uart_cfg_gpio(1);

			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_LOW);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] GPIO_BT_RST_N = %d\n",gpio_get_value(GPIO_BT_RST_N));
#else
                        printk(KERN_DEBUG "[BT] GPIO_BT_RST_N = %d\n",gpio_get_value(GPIO_BT_RST_N));
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */

			if(gpio_get_value(GPIO_WLAN_RST_N) == 0)
			{		
				s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
				gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_LOW);

				s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT0);
				s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

#ifdef CONFIG_MACH_INSTINCTQ
				printk("[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));
#else
                                printk(KERN_DEBUG "[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
			}

			gpio_free(GPIO_BT_RST_N);
			gpio_free(GPIO_BT_WLAN_REG_ON);

			break;

		default:
			printk(KERN_ERR "[BT] Bad bluetooth rfkill state %d\n", state);
	}

	return 0;
}


static void bt_host_wake_work_func(struct work_struct *ignored)
{
    int gpio_val;

	gpio_val = gpio_get_value(GPIO_BT_HOST_WAKE);
#ifdef CONFIG_MACH_INSTINCTQ
	printk("[BT] GPIO_BT_HOST_WAKE = %d\n", gpio_val);	
#else 
        printk(KERN_DEBUG "[BT] GPIO_BT_HOST_WAKE = %d\n", gpio_val);
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
/*	
	if(gpio_val == GPIO_LEVEL_LOW)
	{
		//wake_unlock
		printk("[BT]:wake_unlock \n");	
		wake_unlock(&rfkill_wake_lock);	
	}
	else
	{
		//wake_lock
		printk("[BT]:wake_lock \n");	
		wake_lock(&rfkill_wake_lock);
	}
*/

	if(gpio_val == GPIO_LEVEL_HIGH)
	{
#ifdef CONFIG_MACH_INSTINCTQ
		printk("[BT] wake_lock timeout = 5 sec\n");	
#else
                printk(KERN_DEBUG "[BT] wake_lock timeout = 5 sec\n");
#endif /* #ifdef CONFIG_MACH_INSTINCTQ */
		wake_lock_timeout(&rfkill_wake_lock, 5*HZ);
	}

	enable_irq(IRQ_EINT(22));
}
static DECLARE_WORK(bt_host_wake_work, bt_host_wake_work_func);


irqreturn_t bt_host_wake_irq_handler(int irq, void *dev_id)
{
	disable_irq(IRQ_EINT(22));
	schedule_work(&bt_host_wake_work);
		
	return IRQ_HANDLED;
}


#ifdef CONFIG_MACH_INSTINCTQ
static int __init instinctq_rfkill_probe(struct platform_device *pdev)
#else 
#ifdef CONFIG_MACH_SPICA
static int __init spica_rfkill_probe(struct platform_device *pdev)
#else 
#ifdef CONFIG_MACH_JET
static int __init jet_rfkill_probe(struct platform_device *pdev)
#else 
static int __init smdk6410_rfkill_probe(struct platform_device *pdev)
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
{
	int rc = 0;
	int irq,ret;

	//Initialize wake locks
	wake_lock_init(&rfkill_wake_lock, WAKE_LOCK_SUSPEND, "board-rfkill");
	wake_lock_init(&bt_wake_lock, WAKE_LOCK_SUSPEND, "bt-rfkill");

	//BT Host Wake IRQ
	irq = IRQ_BT_HOST_WAKE;

	ret = request_irq(irq, bt_host_wake_irq_handler, 0, "bt_host_wake_irq_handler", NULL);
	if(ret < 0)
#ifdef CONFIG_MACH_INSTINCTQ
		printk("[BT] Request_irq failed \n");
#else
                printk(KERN_DEBUG "[BT] Request_irq failed \n");
#endif	/* #else #ifdef CONFIG_MACH_INSTINCTQ */

	set_irq_type(irq, IRQ_TYPE_EDGE_BOTH);
	enable_irq(IRQ_EINT(22));

	//RFKILL init - default to bluetooth off
	rfkill_switch_all(RFKILL_TYPE_BLUETOOTH, RFKILL_STATE_SOFT_BLOCKED);

	bt_rfk = rfkill_allocate(&pdev->dev, RFKILL_TYPE_BLUETOOTH);
	if (!bt_rfk)
		return -ENOMEM;

	bt_rfk->name = bt_name;
	bt_rfk->state = RFKILL_STATE_SOFT_BLOCKED;
	/* userspace cannot take exclusive control */
	bt_rfk->user_claim_unsupported = 1;
	bt_rfk->user_claim = 0;
	bt_rfk->data = NULL;  // user data
	bt_rfk->toggle_radio = bluetooth_set_power;

#ifdef CONFIG_MACH_INSTINCTQ
	printk("[BT] rfkill_register(bt_rfk) \n");
#else
        printk(KERN_DEBUG "[BT] rfkill_register(bt_rfk) \n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
	rc = rfkill_register(bt_rfk);
	if (rc)
		rfkill_free(bt_rfk);

	bluetooth_set_power(NULL, RFKILL_STATE_SOFT_BLOCKED);

	return rc;
}

#ifdef CONFIG_MACH_INSTINCTQ
static struct platform_driver instinctq_device_rfkill = {
	.probe = instinctq_rfkill_probe,
#else 
#ifdef CONFIG_MACH_SPICA
static struct platform_driver spica_device_rfkill = {
	.probe = spica_rfkill_probe,
#else 
#ifdef CONFIG_MACH_JET
static struct platform_driver jet_device_rfkill = {
	.probe = jet_rfkill_probe,
#else 
static struct platform_driver smdk6410_device_rfkill = {
	.probe = smdk6410_rfkill_probe,
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
	.driver = {
		.name = "bt_rfkill",
		.owner = THIS_MODULE,
	},
};

#ifdef BT_SLEEP_ENABLER
static struct rfkill *bt_sleep;

static int bluetooth_set_sleep(void *data, enum rfkill_state state)
{	unsigned int ret =0;
	switch (state) {

		case RFKILL_STATE_UNBLOCKED:
			if (gpio_is_valid(GPIO_BT_WAKE))
			{
				ret = gpio_request(GPIO_BT_WAKE, S3C_GPIO_LAVEL(GPIO_BT_WAKE));
				if(ret < 0) {
#ifdef CONFIG_MACH_INSTINCTQ
					printk("[BT] Failed to request GPIO_BT_WAKE!\n");
#else
                                        printk(KERN_DEBUG "[BT] Failed to request GPIO_BT_WAKE!\n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

#ifdef CONFIG_MACH_INSTINCTQ
			mdelay(50);  // 50msec, why?
			printk("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
#else
                        printk(KERN_DEBUG "[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] wake_unlock(bt_wake_lock)\n");
#else
                        printk(KERN_DEBUG "[BT] wake_unlock(bt_wake_lock)\n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
			wake_unlock(&bt_wake_lock);
			break;

		case RFKILL_STATE_SOFT_BLOCKED:
			if (gpio_is_valid(GPIO_BT_WAKE))
			{
				ret = gpio_request(GPIO_BT_WAKE, S3C_GPIO_LAVEL(GPIO_BT_WAKE));
				if(ret < 0) {
#ifdef CONFIG_MACH_INSTINCTQ
					printk("[BT] Failed to request GPIO_BT_WAKE!\n");
#else
                                        printk(KERN_DEBUG "[BT] Failed to request GPIO_BT_WAKE!\n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);

#ifdef CONFIG_MACH_INSTINCTQ
			mdelay(50);
			printk("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
#else
                        printk(KERN_DEBUG "[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
#ifdef CONFIG_MACH_INSTINCTQ
			printk("[BT] wake_lock(bt_wake_lock)\n");
#else
                        printk(KERN_DEBUG "[BT] wake_lock(bt_wake_lock)\n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
			wake_lock(&bt_wake_lock);
			break;

		default:
			printk(KERN_ERR "[BT] Bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

#ifdef CONFIG_MACH_INSTINCTQ
static int __init instinctq_btsleep_probe(struct platform_device *pdev)
#else 
#ifdef CONFIG_MACH_SPICA
static int __init spica_btsleep_probe(struct platform_device *pdev)
#else 
#ifdef CONFIG_MACH_JET
static int __init jet_btsleep_probe(struct platform_device *pdev)
#else 
static int __init smdk6410_btsleep_probe(struct platform_device *pdev)
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
{
	int rc = 0;

	bt_sleep = rfkill_allocate(&pdev->dev, RFKILL_TYPE_BLUETOOTH);
	if (!bt_sleep)
		return -ENOMEM;

	bt_sleep->name = bt_name;
	bt_sleep->state = RFKILL_STATE_UNBLOCKED;
	/* userspace cannot take exclusive control */
	bt_sleep->user_claim_unsupported = 1;
	bt_sleep->user_claim = 0;
	bt_sleep->data = NULL;  // user data
	bt_sleep->toggle_radio = bluetooth_set_sleep;

	rc = rfkill_register(bt_sleep);
	if (rc)
		rfkill_free(bt_sleep);
	
#ifdef CONFIG_MACH_INSTINCTQ
	printk("[BT] rfkill_force_state(bt_sleep, RFKILL_STATE_UNBLOCKED) \n");
#else
        printk(KERN_DEBUG "[BT] rfkill_force_state(bt_sleep, RFKILL_STATE_UNBLOCKED) \n");
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
	rfkill_force_state(bt_sleep, RFKILL_STATE_UNBLOCKED);
	
	bluetooth_set_sleep(NULL, RFKILL_STATE_UNBLOCKED);

	return rc;
}

#ifdef CONFIG_MACH_INSTINCTQ
static struct platform_driver instinctq_device_btsleep = {
	.probe = instinctq_btsleep_probe,
#else 
#ifdef CONFIG_MACH_SPICA
static struct platform_driver spica_device_btsleep = {
	.probe = spica_btsleep_probe,
#else 
#ifdef CONFIG_MACH_JET
static struct platform_driver jet_device_btsleep = {
	.probe = jet_btsleep_probe,
#else 
static struct platform_driver smdk6410_device_btsleep = {
	.probe = smdk6410_btsleep_probe,
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
	.driver = {
		.name = "bt_sleep",
		.owner = THIS_MODULE,
	},
};
#endif

#ifdef CONFIG_MACH_INSTINCTQ
static int __init instinctq_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&instinctq_device_rfkill);
#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&instinctq_device_btsleep);
#endif
#else 
#ifdef CONFIG_MACH_SPICA
static int __init spica_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&spica_device_rfkill);
#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&spica_device_btsleep);
#endif
#else 
#ifdef CONFIG_MACH_JET
static int __init jet_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&jet_device_rfkill);
#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&jet_device_btsleep);
#endif
#else 
static int __init smdk6410_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&smdk6410_device_rfkill);
#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&smdk6410_device_btsleep);
#endif
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */

	return rc;
}

#ifdef CONFIG_MACH_INSTINCTQ
module_init(instinctq_rfkill_init);
MODULE_DESCRIPTION("instinctq rfkill");
#else 
#ifdef CONFIG_MACH_SPICA
module_init(spica_rfkill_init);
MODULE_DESCRIPTION("spica rfkill");
#else 
#ifdef CONFIG_MACH_JET
module_init(jet_rfkill_init);
MODULE_DESCRIPTION("jet rfkill");
#else 
module_init(smdk6410_rfkill_init);
MODULE_DESCRIPTION("smdk6410 rfkill");
#endif	/* #ifdef CONFIG_MACH_JET */
#endif	/* #ifdef CONFIG_MACH_SPICA */
#endif	/* #ifdef CONFIG_MACH_INSTINCTQ */
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");
