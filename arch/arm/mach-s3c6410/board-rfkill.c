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

/* Control bluetooth power for spica platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <mach/gpio.h>
#include <mach/spica.h>	/*Updated by kumar.gvs 22 Apr 2009*/
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
			printk(KERN_DEBUG "[BT] Device Powering ON \n");
			s3c_setup_uart_cfg_gpio(1);

			if (gpio_is_valid(GPIO_BT_WLAN_REG_ON))
			{
				ret = gpio_request(GPIO_BT_WLAN_REG_ON, S3C_GPIO_LAVEL(GPIO_BT_WLAN_REG_ON));
				if (ret < 0) {
					printk(KERN_ERR "[BT] Failed to request GPIO_BT_WLAN_REG_ON!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);
			}

			if (gpio_is_valid(GPIO_BT_RST_N))
			{
				ret = gpio_request(GPIO_BT_RST_N, S3C_GPIO_LAVEL(GPIO_BT_RST_N));
				if (ret < 0) {
					gpio_free(GPIO_BT_WLAN_REG_ON);
					printk(KERN_ERR "[BT] Failed to request GPIO_BT_RST_N!\n");
					return ret;			
				}
				gpio_direction_output(GPIO_BT_RST_N, GPIO_LEVEL_LOW);
			}

			/* Set GPIO_BT_WLAN_REG_ON high */ 
			s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT1);  
			s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

			printk(KERN_DEBUG "[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));		

			msleep(150);  // 100msec, delay  between reg_on & rst. (bcm4325 powerup sequence)

			/* Set GPIO_BT_RST_N high */
			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_HIGH);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT1);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);

			printk(KERN_DEBUG "[BT] GPIO_BT_RST_N = %d\n", gpio_get_value(GPIO_BT_RST_N));

			gpio_free(GPIO_BT_RST_N);
			gpio_free(GPIO_BT_WLAN_REG_ON);

			break;

		case RFKILL_STATE_SOFT_BLOCKED:
			printk(KERN_DEBUG "[BT] Device Powering OFF \n");
			s3c_reset_uart_cfg_gpio(1);

			s3c_gpio_setpull(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_RST_N, GPIO_LEVEL_LOW);

			s3c_gpio_slp_cfgpin(GPIO_BT_RST_N, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_BT_RST_N, S3C_GPIO_PULL_NONE);

			printk(KERN_DEBUG "[BT] GPIO_BT_RST_N = %d\n",gpio_get_value(GPIO_BT_RST_N));

			if(gpio_get_value(GPIO_WLAN_RST_N) == 0)
			{		
				s3c_gpio_setpull(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
				gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_LOW);

				s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT0);
				s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

				printk(KERN_DEBUG "[BT] GPIO_BT_WLAN_REG_ON = %d\n", gpio_get_value(GPIO_BT_WLAN_REG_ON));
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
	printk(KERN_DEBUG "[BT] GPIO_BT_HOST_WAKE = %d\n", gpio_val);
/*
	if(gpio_val == GPIO_LEVEL_LOW)
	{
		//wake_unlock^M
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
		printk(KERN_DEBUG "[BT] wake_lock timeout = 5 sec\n");
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


static int __init spica_rfkill_probe(struct platform_device *pdev)
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
		printk(KERN_DEBUG "[BT] Request_irq failed \n");

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

	printk(KERN_DEBUG "[BT] rfkill_register(bt_rfk) \n");
	rc = rfkill_register(bt_rfk);
	if (rc)
		rfkill_free(bt_rfk);

	bluetooth_set_power(NULL, RFKILL_STATE_SOFT_BLOCKED);

	return rc;
}

static struct platform_driver spica_device_rfkill = {
	.probe = spica_rfkill_probe,
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
					printk(KERN_DEBUG "[BT] Failed to request GPIO_BT_WAKE!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

			printk(KERN_DEBUG "[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
			printk(KERN_DEBUG "[BT] wake_unlock(bt_wake_lock)\n");
			wake_unlock(&bt_wake_lock);
			break;

		case RFKILL_STATE_SOFT_BLOCKED:
			if (gpio_is_valid(GPIO_BT_WAKE))
			{
				ret = gpio_request(GPIO_BT_WAKE, S3C_GPIO_LAVEL(GPIO_BT_WAKE));
				if(ret < 0) {
					printk(KERN_DEBUG "[BT] Failed to request GPIO_BT_WAKE!\n");
					return ret;
				}
				gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);
			}

			s3c_gpio_setpull(GPIO_BT_WAKE, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_BT_WAKE, GPIO_LEVEL_HIGH);

			printk(KERN_DEBUG "[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			gpio_free(GPIO_BT_WAKE);
			//billy's changes
			printk(KERN_DEBUG "[BT] wake_lock(bt_wake_lock)\n");
			wake_lock(&bt_wake_lock);
			break;

		default:
			printk(KERN_ERR "[BT] bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int __init spica_btsleep_probe(struct platform_device *pdev)
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

	printk(KERN_DEBUG "[BT] rfkill_force_state(bt_sleep, RFKILL_STATE_UNBLOCKED) \n");
	rfkill_force_state(bt_sleep, RFKILL_STATE_UNBLOCKED);

	bluetooth_set_sleep(NULL, RFKILL_STATE_UNBLOCKED);

	return rc;
}

static struct platform_driver spica_device_btsleep = {
	.probe = spica_btsleep_probe,
	.driver = {
		.name = "bt_sleep",
		.owner = THIS_MODULE,
	},
};
#endif

static int __init spica_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&spica_device_rfkill);

#ifdef BT_SLEEP_ENABLER
	platform_driver_register(&spica_device_btsleep);
#endif

	return rc;
}

module_init(spica_rfkill_init);
MODULE_DESCRIPTION("spica rfkill");
MODULE_AUTHOR("Nick Pelly <npelly@google.com>");
MODULE_LICENSE("GPL");
