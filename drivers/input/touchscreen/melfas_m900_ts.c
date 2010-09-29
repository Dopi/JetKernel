/* drivers/input/touchscreen/melfas_ts_i2c_tsi.c
 *
 * Copyright (C) 2007 Google, Inc.
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
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>

#include <asm/io.h>
#include <linux/irq.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>

#include <linux/dprintk.h>

#define MELFAS_I2C_ADDR 0x40
#define IRQ_TOUCH_INT IRQ_EINT(8)

#ifdef CONFIG_CPU_FREQ
#include <plat/s3c64xx-dvfs.h>
#endif
 
extern int mcsdl_download_binary_data(void);
extern int get_sending_oj_event();

struct melfas_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct  work;
	int irq;
	int fw_ver;
	struct early_suspend	early_suspend;
};

struct melfas_ts_driver *melfas_ts = NULL;
struct i2c_driver melfas_ts_i2c;
struct workqueue_struct *melfas_ts_wq;

#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h);
void melfas_ts_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#define TOUCH_HOME	KEY_HOME
#define TOUCH_MENU	KEY_MENU
#define TOUCH_BACK	KEY_BACK

int melfas_ts_tk_keycode[] =
{ TOUCH_HOME, TOUCH_MENU, TOUCH_BACK, };

extern struct class *sec_class;
struct device *ts_dev;

static ssize_t registers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int status, op_mode, hw_rev, fw_ver;
	
	status  = i2c_smbus_read_byte_data(melfas_ts->client, 0x00);
	if (status < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	op_mode = i2c_smbus_read_byte_data(melfas_ts->client, 0x01);
	if (op_mode < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	hw_rev = i2c_smbus_read_byte_data(melfas_ts->client, 0x1E);
	if (hw_rev < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	fw_ver = i2c_smbus_read_byte_data(melfas_ts->client, 0x20);
	if (fw_ver < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	
	sprintf(buf, "[TOUCH] Melfas Tsp Register Info.\n");
	sprintf(buf, "%sRegister 0x00 (status) : 0x%08x\n", buf, status);
	sprintf(buf, "%sRegister 0x01 (op_mode): 0x%08x\n", buf, op_mode);
	sprintf(buf, "%sRegister 0x1E (hw_rev) : 0x%08x\n", buf, hw_rev);
	sprintf(buf, "%sRegister 0x20 (fw_ver) : 0x%08x\n", buf, fw_ver);

	return sprintf(buf, "%s", buf);
}

static ssize_t registers_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
		
	    ret = i2c_smbus_write_byte_data(melfas_ts->client, 0x01, 0x01);
		if (ret < 0) {
			printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		}
		printk("[TOUCH] software reset.\n");
	}
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	sprintf(buf, "[TOUCH] Melfas Tsp Gpio Info.\n");
	sprintf(buf, "%sGPIO TOUCH_EN  : %s\n", buf, gpio_get_value(GPIO_TOUCH_EN)? "HIGH":"LOW");
	sprintf(buf, "%sGPIO TOUCH_INT : %s\n", buf, gpio_get_value(GPIO_TOUCH_INT)? "HIGH":"LOW"); 
	return sprintf(buf, "%s", buf);
}

static ssize_t gpio_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "ON", 2) == 0 || strncmp(buf, "on", 2) == 0) {
		gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);
		printk("[TOUCH] enable.\n");
		mdelay(200);
	}

	if(strncmp(buf, "OFF", 3) == 0 || strncmp(buf, "off", 3) == 0) {
		gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_LOW);
		printk("[TOUCH] disable.\n");
	}
	
	if(strncmp(buf, "RESET", 5) == 0 || strncmp(buf, "reset", 5) == 0) {
		gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_LOW);
		mdelay(500);
		gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);
		printk("[TOUCH] reset.\n");
		mdelay(200);
	}
	return size;
}

static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int ret;;
	
	int hw_rev, fw_ver;
	
	hw_rev = i2c_smbus_read_byte_data(melfas_ts->client, 0x1E);
	if (hw_rev < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}
	fw_ver = i2c_smbus_read_byte_data(melfas_ts->client, 0x20);
	if (fw_ver < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");;
	}

	sprintf(buf, "H/W rev. 0x%x F/W ver. 0x%x\n", hw_rev, fw_ver);
	return sprintf(buf, "%s", buf);
}

static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
#ifdef CONFIG_TOUCHSCREEN_MELFAS_FIRMWARE_UPDATE	
	int ret;
	if(strncmp(buf, "UPDATE", 6) == 0 || strncmp(buf, "update", 6) == 0) {
//		if(melfas_ts->fw_ver > 0x03) {
			printk("[TOUCH] previous firmware version: 0x%x.\n", melfas_ts->fw_ver);
			s3c_gpio_cfgpin(GPIO_I2C0_SDA, S3C_GPIO_SFN(1));
			s3c_gpio_cfgpin(GPIO_I2C0_SCL, S3C_GPIO_SFN(1));
			ret = mcsdl_download_binary_data();
			s3c_gpio_cfgpin(GPIO_I2C0_SDA, S3C_GPIO_SFN(GPIO_I2C0_SDA_AF));
			s3c_gpio_cfgpin(GPIO_I2C0_SCL, S3C_GPIO_SFN(GPIO_I2C0_SCL_AF));
			if(ret)
				printk("[TOUCH] firmware update success!\n");
			else {
				printk("[TOUCH] firmware update failed.. RESET!\n");
				gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_LOW);
				mdelay(500);
				gpio_set_value(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);
				mdelay(200);
			}
//		}
	}
#endif

	return size;
}

static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(registers, S_IRUGO | S_IWUSR, registers_show, registers_store);
static DEVICE_ATTR(firmware, S_IRUGO | S_IWUSR, firmware_show, firmware_store);

void melfas_ts_work_func(struct work_struct *work)
{
	int ret;
	struct i2c_msg msg[2];

	struct i2c_msg msg1[2];
	uint8_t buf2[1];
	int button;
	unsigned int keycode, keypress;
    
	uint8_t start_reg;
	uint8_t buf1[8];
	int	x_old, y_old;
	x_old = y_old = 0;

	msg[0].addr = melfas_ts->client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &start_reg;
	start_reg = 0x10;
	msg[1].addr = melfas_ts->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = sizeof(buf1);
	msg[1].buf = buf1;

	ret = i2c_transfer(melfas_ts->client->adapter, msg, 2);

	if (ret < 0) {
		printk(KERN_ERR "melfas_ts_work_func: i2c_transfer failed\n");
	} else {

		int x = buf1[2] | (uint16_t)(buf1[1] & 0x03) << 8; 
		int y = buf1[4] | (uint16_t)(buf1[3] & 0x0f) << 8; 
		int z = buf1[5];
		int finger = buf1[0] & 0x01;
		int width = buf1[6];

#ifdef CONFIG_CPU_FREQ
			set_dvfs_perf_level();
#endif

		if(buf1[0] & 0xC0) {
			msg1[0].addr = melfas_ts->client->addr;
			msg1[0].flags = 0;
			msg1[0].len = 1;
			msg1[0].buf = &start_reg;
			start_reg = 0x25;
			msg1[1].addr = melfas_ts->client->addr;
			msg1[1].flags = I2C_M_RD;
			msg1[1].len = sizeof(buf2);
			msg1[1].buf = buf2;
    
			ret = i2c_transfer(melfas_ts->client->adapter, msg1, 2);

			button = buf2[0]; //key:1 home key:2 menu key:3 back
			switch(button) {
			case 0x01 : 
			case 0x09 :
#if defined(CONFIG_MACH_VINSQ)
                                keycode = TOUCH_MENU; break;
#else
				keycode = TOUCH_HOME; break;
#endif
			case 0x02 : 
			case 0x0A :
#if defined(CONFIG_MACH_VINSQ)
                                keycode = TOUCH_HOME; break;
#else
				keycode = TOUCH_MENU; break;
#endif
			case 0x03 : 
			case 0x0B :
//			case 0x04 : 
//			case 0x0C :
				keycode = TOUCH_BACK; break;
			default :
				printk("[TOUCH_KEY] undefined button: 0x%02x.\n", button);
				enable_irq(melfas_ts->irq);
				return;
			}

			if(!(keycode ==TOUCH_MENU && get_sending_oj_event()))
			{
				if(button & 0x08) 
					keypress = 0;
				else 
					keypress = 1;
//				printk("[TOUCH_KEY] keycode: %4d, keypress: %4d\n", keycode, keypress); 
				dprintk(TSP_KEY," keycode: %4d, keypress: %4d\n", keycode, keypress); 
				input_report_key(melfas_ts->input_dev, keycode, keypress);
			}
		}
		else {

//			printk("[TOUCH_ABS] x: %4d, y: %4d, z: %4d, finger: %4d, width: %4d\n", x, y, z, finger, width); 
			dprintk(TSP_ABS," x: %4d, y: %4d, z: %4d, finger: %4d, width: %4d\n", x, y, z, finger, width); 
			if (x) 	input_report_abs(melfas_ts->input_dev, ABS_X, x);
			if (y)	input_report_abs(melfas_ts->input_dev, ABS_Y, y);

			input_report_abs(melfas_ts->input_dev, ABS_PRESSURE, z);
			input_report_abs(melfas_ts->input_dev, ABS_TOOL_WIDTH, width);
			input_report_key(melfas_ts->input_dev, BTN_TOUCH, finger);

		}

		input_sync(melfas_ts->input_dev);
	}
	enable_irq(melfas_ts->irq);
}

irqreturn_t melfas_ts_irq_handler(int irq, void *dev_id)
{
	disable_irq(melfas_ts->irq);
	queue_work(melfas_ts_wq, &melfas_ts->work);
	return IRQ_HANDLED;
}

//int melfas_ts_probe(struct i2c_client *client)
int melfas_ts_probe()
{
	int ret = 0;
	uint16_t max_x, max_y;
	int fw_ver = 0;

	if (!i2c_check_functionality(melfas_ts->client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "melfas_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	INIT_WORK(&melfas_ts->work, melfas_ts_work_func);

#if 0	
    ret = i2c_smbus_write_byte_data(melfas_ts->client, 0x01, 0x01); /* device command = reset */
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_write_byte_data failed\n");
		/* fail? */
	}
#endif
	fw_ver = i2c_smbus_read_byte_data(melfas_ts->client, 0x20);
	if (fw_ver < 0) {
		printk(KERN_ERR "i2c_smbus_read_byte_data failed\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "melfas_ts_probe: Firmware Version %x\n", fw_ver);

	melfas_ts->fw_ver = fw_ver;
	melfas_ts->irq = IRQ_TOUCH_INT;

	ret = i2c_smbus_read_word_data(melfas_ts->client, 0x08);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_word_data failed\n");
		goto err_detect_failed;
	}
	max_x = (ret >> 8 & 0xff) | ((ret & 0x03) << 8);

	ret = i2c_smbus_read_word_data(melfas_ts->client, 0x0a);
	if (ret < 0) {
		printk(KERN_ERR "i2c_smbus_read_word_data failed\n");
		goto err_detect_failed;
	}
	max_y = (ret >> 8 & 0xff) | ((ret & 0x03) << 8);

	melfas_ts->input_dev = input_allocate_device();
	if (melfas_ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "melfas_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	melfas_ts->input_dev->name = "melfas_ts_input";
	
	set_bit(EV_SYN, melfas_ts->input_dev->evbit);
	set_bit(EV_KEY, melfas_ts->input_dev->evbit);
	set_bit(TOUCH_HOME, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_MENU, melfas_ts->input_dev->keybit);
	set_bit(TOUCH_BACK, melfas_ts->input_dev->keybit);

	melfas_ts->input_dev->keycode = melfas_ts_tk_keycode;	
	set_bit(BTN_TOUCH, melfas_ts->input_dev->keybit);
	set_bit(EV_ABS, melfas_ts->input_dev->evbit);
#if defined(CONFIG_MACH_VINSQ)
        max_x = 240;
        max_y = 400;
#endif
	input_set_abs_params(melfas_ts->input_dev, ABS_X, 0, max_x, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_Y, 0, max_y, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(melfas_ts->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

    printk("melfas_ts_probe: max_x %d, max_y %d\n", max_x, max_y);

	/* melfas_ts->input_dev->name = melfas_ts->keypad_info->name; */
	ret = input_register_device(melfas_ts->input_dev);
	if (ret) {
		printk(KERN_ERR "melfas_ts_probe: Unable to register %s input device\n", melfas_ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	ret = request_irq(melfas_ts->irq, melfas_ts_irq_handler, IRQF_DISABLED, "melfas_ts irq", 0);
	if(ret == 0) {
		printk(KERN_INFO "melfas_ts_probe: Start touchscreen %s \n", melfas_ts->input_dev->name);
	}
	else {
		printk("request_irq failed\n");
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	melfas_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	melfas_ts->early_suspend.suspend = melfas_ts_early_suspend;
	melfas_ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	return 0;

err_input_register_device_failed:
	input_free_device(melfas_ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
err_check_functionality_failed:
	return ret;
}

int melfas_ts_remove(struct i2c_client *client)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&melfas_ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	free_irq(melfas_ts->irq, 0);
	input_unregister_device(melfas_ts->input_dev);
	return 0;
}

//int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
int melfas_ts_suspend(pm_message_t mesg)
{
	disable_irq(melfas_ts->irq);
    gpio_set_value(GPIO_TOUCH_EN, 0);  // TOUCH EN
    
	return 0;
}

//int melfas_ts_resume(struct i2c_client *client)
int melfas_ts_resume()
{
    gpio_set_value(GPIO_TOUCH_EN, 1);  // TOUCH EN
	msleep(300);
	enable_irq(melfas_ts->irq);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void melfas_ts_early_suspend(struct early_suspend *h)
{
	melfas_ts_suspend(PMSG_SUSPEND);
}

void melfas_ts_late_resume(struct early_suspend *h)
{
	melfas_ts_resume();
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

unsigned short melfas_ts_i2c_normal[] = {(MELFAS_I2C_ADDR >> 1), I2C_CLIENT_END};
unsigned short melfas_ts_i2c_ignore[] = {1, (MELFAS_I2C_ADDR >> 1), I2C_CLIENT_END};
unsigned short melfas_ts_i2c_probe[] = {I2C_CLIENT_END};

struct i2c_client_address_data melfas_ts_addr_data = {
	.normal_i2c = melfas_ts_i2c_normal,
	.ignore = melfas_ts_i2c_ignore,
	.probe = melfas_ts_i2c_probe,
};

int melfas_ts_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if(!c)
		return -ENOMEM;
	memset(c, 0, sizeof(struct i2c_client));
	
	strcpy(c->name, "melfas_ts");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &melfas_ts_i2c;
	
	if((ret = i2c_attach_client(c)) < 0)
		goto error;

	melfas_ts->client = c;
	printk("melfas_ts_i2c is attached..\n");
error:
	return ret;
}

int melfas_ts_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &melfas_ts_addr_data, melfas_ts_attach);
}

struct i2c_driver melfas_ts_i2c = {
	.driver = {
		.name	= "melfas_ts_i2c",
	},
	.id	= MELFAS_I2C_ADDR,
	.attach_adapter = melfas_ts_attach_adapter,
};

void init_hw_setting(void)
{

	s3c_gpio_cfgpin(GPIO_TOUCH_INT, S3C_GPIO_SFN(GPIO_TOUCH_INT_AF));
	s3c_gpio_setpull(GPIO_TOUCH_INT, S3C_GPIO_PULL_NONE); 
	
	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_EDGE_FALLING);
//	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_LEVEL_LOW);

	if (gpio_is_valid(GPIO_TOUCH_EN)) {
		if (gpio_request(GPIO_TOUCH_EN, S3C_GPIO_LAVEL(GPIO_TOUCH_EN)))
			printk(KERN_ERR "Filed to request GPIO_TOUCH_EN!\n");
		gpio_direction_output(GPIO_TOUCH_EN, GPIO_LEVEL_LOW);
	}
//mk93.lee Phone power on prob.	s3c_gpio_setpull(GPIO_PHONE_ON, S3C_GPIO_PULL_UP); 
}

struct platform_driver melfas_ts_driver =  {
	.probe	= melfas_ts_probe,
	.remove = melfas_ts_remove,
	.driver = {
		.name = "melfas-ts",
		.owner	= THIS_MODULE,
	},
};


int __init melfas_ts_init(void)
{
	int ret;

	init_hw_setting();

    gpio_set_value(GPIO_TOUCH_EN, 1);  // TOUCH EN
	mdelay(300);

	ts_dev = device_create(sec_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_registers) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_registers.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware.attr.name);

	melfas_ts = kzalloc(sizeof(struct melfas_ts_driver), GFP_KERNEL);
	if(melfas_ts == NULL) {
		return -ENOMEM;
	}

	ret = i2c_add_driver(&melfas_ts_i2c);
	if(ret) printk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);

	if(!melfas_ts->client) {
		printk("###################################################\n");
		printk("##                                               ##\n");
		printk("##    WARNING! TOUCHSCREEN DRIVER CAN'T WORK.    ##\n");
		printk("##    PLEASE CHECK YOUR TOUCHSCREEN CONNECTOR!   ##\n");
		printk("##                                               ##\n");
		printk("###################################################\n");
		i2c_del_driver(&melfas_ts_i2c);
		return 0;
	}
	melfas_ts_wq = create_singlethread_workqueue("melfas_ts_wq");
	if (!melfas_ts_wq)
		return -ENOMEM;

	return platform_driver_register(&melfas_ts_driver);

}

void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_i2c);
	if (melfas_ts_wq)
		destroy_workqueue(melfas_ts_wq);
}
late_initcall(melfas_ts_init);
//module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

MODULE_DESCRIPTION("Melfas Touchscreen Driver");
MODULE_LICENSE("GPL");
