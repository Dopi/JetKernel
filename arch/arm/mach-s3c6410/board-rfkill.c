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

/* Control bluetooth power for infobowlq platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/gpio.h>
#include <mach/infobowlq.h>	/*Updated by kumar.gvs 22 Apr 2009*/
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

void rfkill_switch_all(enum rfkill_type type, enum rfkill_user_states state);

extern void s3c_setup_uart_cfg_gpio(unsigned char port);
extern void s3c_reset_uart_cfg_gpio(unsigned char port);

static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4325";

static int bluetooth_set_power(void *data, enum rfkill_user_states state)
{
	unsigned int ret = 0; 
	switch (state) {

		case RFKILL_USER_STATE_UNBLOCKED:
			printk("[BT] Device Powering ON \n");
			s3c_setup_uart_cfg_gpio(1);
			//wake_unlock(&bt_wake_lock); 

			if (gpio_is_valid(GPIO_BT_WLAN_REG_ON))
			{
				ret = gpio_request(GPIO_BT_WLAN_REG_ON, S3C_GPIO_LAVEL(GPIO_BT_WLAN_REG_ON));
				if (ret < 0) {
					printk("[BT] Failed to request GPIO_BT_WLAN_REG_ON!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);
			}

			if (gpio_is_valid(GPIO_BT_RST_N))
			{
				ret = gpio_request(GPIO_BT_RST_N, S3C_GPIO_LAVEL(GPIO_BT_RST_N));
				if (ret < 0) {
					gpio_free(GPIO_BT_WLAN_REG_ON);
					printk("[BT] Failed to request GPIO_BT_RST_N!\n");
					return ret;			
				}
				gpio_direction_output(GPIO_BT_RST_N, GPIO_LEVEL_LOW);
			}

			/* Set GPIO_BT_WLAN_REG_ON high */ 
			s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT1);  
			s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

			printk("[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));		

			mdelay(150);  // 100msec, delay  between reg_on & rst. (bcm4325 powerup sequence)

			/* Set GPIO_BT_RST_N high */
			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT1);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);

			printk("[BT] GPIO_BT_RST_N = %d\n", gpio_get_value(GPIO_BT_RST_N));

			gpio_free(GPIO_BT_RST_N);
			gpio_free(GPIO_BT_WLAN_REG_ON);

			break;

		case RFKILL_USER_STATE_SOFT_BLOCKED:
			printk("[BT] Device Powering OFF \n");
			//wake_unlock(&bt_wake_lock);  //if not, it doesn't go to sleep after BT on->off. There's lock somewhere.
			s3c_reset_uart_cfg_gpio(1); 

			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_LOW);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);

			printk("[BT] GPIO_BT_RST_N = %d\n",gpio_get_value(GPIO_BT_RST_N));

			if(gpio_get_value(GPIO_WLAN_RST_N) == 0)
			{		
				s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
				gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_LOW);

				s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT0);
				s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

				printk("[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));
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
	printk("[BT] GPIO_BT_HOST_WAKE = %d\n", gpio_val);	
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
		printk("[BT] wake_lock timeout = 5 sec\n");	
		wake_lock_timeout(&rfkill_wake_lock, 5*HZ);
	}

	enable_irq(IRQ_BT_HOST_WAKE);
}
static DECLARE_WORK(bt_host_wake_work, bt_host_wake_work_func);


irqreturn_t bt_host_wake_irq_handler(int irq, void *dev_id)
{
	printk(KERN_DEBUG "[BT] bt_host_wake_irq_handler start\n");
	disable_irq_nosync(IRQ_BT_HOST_WAKE);
	schedule_work(&bt_host_wake_work);
		
	return IRQ_HANDLED;
}

static int bt_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret =0;
	
	ret = bluetooth_set_power(data, blocked? RFKILL_USER_STATE_SOFT_BLOCKED : RFKILL_USER_STATE_UNBLOCKED);
		
	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

static int __init infobowlq_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	int irq,ret;

	//Initialize wake locks
	wake_lock_init(&rfkill_wake_lock, WAKE_LOCK_SUSPEND, "board-rfkill");
#ifdef BT_SLEEP_ENABLER
	wake_lock_init(&bt_wake_lock, WAKE_LOCK_SUSPEND, "bt-rfkill");
#endif

	//BT Host Wake IRQ
	irq = IRQ_BT_HOST_WAKE;

//	set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
	ret = request_irq(irq, bt_host_wake_irq_handler, 0, "bt_host_wake_irq_handler", NULL);
	if(ret < 0)
		printk(KERN_ERR "[BT] Request_irq failed \n");
	set_irq_type(irq, IRQ_TYPE_EDGE_BOTH);
	enable_irq(IRQ_BT_HOST_WAKE);

	//RFKILL init - default to bluetooth off
	rfkill_switch_all(RFKILL_TYPE_BLUETOOTH, RFKILL_USER_STATE_SOFT_BLOCKED);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH, &bt_rfkill_ops, NULL);
	if (!bt_rfk)
		return -ENOMEM;

	rfkill_init_sw_state(bt_rfk, 0);

	printk(KERN_DEBUG "[BT] rfkill_register(bt_rfk) \n");

	rc = rfkill_register(bt_rfk);
	if (rc)
	{
		printk ("***********ERROR IN REGISTERING THE RFKILL***********\n");
		rfkill_destroy(bt_rfk);
	}

	rfkill_set_sw_state(bt_rfk, 1);
	bluetooth_set_power(NULL, RFKILL_USER_STATE_SOFT_BLOCKED);

	return rc;
}

static struct platform_driver infobowlq_device_rfkill = {
	.probe = infobowlq_rfkill_probe,
	.driver = {
		.name = "bt_rfkill",
		.owner = THIS_MODULE,
	},
};

#ifdef BT_SLEEP_ENABLER
static struct rfkill *bt_sleep;

static int bluetooth_set_sleep(void *data, enum rfkill_user_states state)
{	
	unsigned int ret =0;
	switch (state) {

		case RFKILL_USER_STATE_UNBLOCKED:
			if (gpio_is_valid(GPIO_BT_WAKE))
			{
				ret = gpio_request(GPIO_BT_WAKE, S3C_GPIO_LAVEL(GPIO_BT_WAKE));
				if(ret < 0) {
					printk(KERN_ERR "[BT] Failed to request GPIO_BT_WAKE!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

			mdelay(50);  // 50msec, why?
			printk("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
			printk("[BT] wake_unlock(bt_wake_lock)\n");
			wake_unlock(&bt_wake_lock);
			break;

		case RFKILL_USER_STATE_SOFT_BLOCKED:
			if (gpio_is_valid(GPIO_BT_WAKE))
			{
				ret = gpio_request(GPIO_BT_WAKE, S3C_GPIO_LAVEL(GPIO_BT_WAKE));
				if(ret < 0) {
					printk("[BT] Failed to request GPIO_BT_WAKE!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);

			mdelay(50);
			printk("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
			printk("[BT] wake_lock(bt_wake_lock)\n");
			wake_lock(&bt_wake_lock);
			break;

		default:
			printk(KERN_ERR "[BT] Bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int btsleep_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret =0;
	
	ret = bluetooth_set_sleep(data, blocked? RFKILL_USER_STATE_SOFT_BLOCKED : RFKILL_USER_STATE_UNBLOCKED);
		
	return ret;
}

static const struct rfkill_ops btsleep_rfkill_ops = {
	.set_block = btsleep_rfkill_set_block,
};

static int __init infobowlq_btsleep_probe(struct platform_device *pdev)
{
	int rc = 0;

	bt_sleep = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH, &btsleep_rfkill_ops, NULL);
	if (!bt_sleep)
		return -ENOMEM;

	rfkill_set_sw_state(bt_sleep, 1);

	rc = rfkill_register(bt_sleep);
	if (rc)
		rfkill_destroy(bt_sleep);

	bluetooth_set_sleep(NULL, RFKILL_USER_STATE_UNBLOCKED);

	return rc;
}

static struct platform_driver infobowlq_device_btsleep = {
	.probe = infobowlq_btsleep_probe,
	.driver = {
		.name = "bt_sleep",
		.owner = THIS_MODULE,
	},
};
#endif

static int __init infobowlq_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&infobowlq_device_rfkill);

#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&infobowlq_device_btsleep);
#endif

	return rc;
}

module_init(infobowlq_rfkill_init);
MODULE_DESCRIPTION("infobowlq rfkill");
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");
