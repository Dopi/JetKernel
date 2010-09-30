/* drivers/power/smartq_battery.c
 *
 * Power supply driver for the SmartQ5/7
 *
 * Copyright (C) 2009 Riversky Fang.
 * Author: Riversky Fang <riverskyfang@126.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>

#define GPIO_CHARGER_EN  	S3C64XX_GPK(6)
#define GPIO_DC_DETE  		S3C64XX_GPL(13)

struct smartq_battery_data {
	int irq;
	spinlock_t lock;
	struct power_supply battery;
	struct power_supply ac;
	struct power_supply usb; 
	
	struct task_struct 	*kpwsd_tsk;
	struct semaphore	thread_sem;
};

static struct smartq_battery_data *smartq_battery_data_p;

static void set_charge_mode(int enable)
{
	if (enable) 
		gpio_direction_output(GPIO_CHARGER_EN, 1);		/* 860ma */
	else 
		gpio_direction_output(GPIO_CHARGER_EN, 0);		/* 200ma */
}

static int smartq_get_ac_status(void)
{
#if defined (CONFIG_LCD_4)
	return gpio_get_value(GPIO_DC_DETE) ? 1 : 0 ;	
#else
	return gpio_get_value(GPIO_DC_DETE) ? 0 : 1 ;
#endif
	return 0;
}

static int global_battery_life;
extern unsigned int get_s3c_adc_value(unsigned int port);

static inline int read_battery_life(void)
{
	int battery_life = 0, reference_value = 0;
	static int old_battery_life = 0, old_reference_value = 0;

	/* ref voltage:2.4V, battery max :4.2V */
	if (unlikely((battery_life = get_s3c_adc_value(0)) == 0)) {
	    if (old_battery_life == 0) {
		while (!(battery_life = get_s3c_adc_value(0)))
			;
		old_battery_life = battery_life;
	    } else
		battery_life = old_battery_life;
	}
	
	if(unlikely((reference_value = get_s3c_adc_value(1)) == 0)) {
	    if (old_reference_value == 0) {
		while (!(reference_value = get_s3c_adc_value(1)))
			;
		old_reference_value = reference_value;
	    } else
		reference_value = old_reference_value;
	}
	
	battery_life = (battery_life * 24000) / (reference_value * 42);
	
	pr_debug("ADC voltage value=%u,conference  voltage =%u \n",
			get_s3c_adc_value(0), get_s3c_adc_value(1)); 
	
	return battery_life ;
}


static int smartq_get_battery_voltage(void)
{
	int i;
	int count = 0;
	int battery_life = global_battery_life;
	int battery_life_sum = 0;
	
	for (i = 0; i < 10; i++) {	
		int tmp = read_battery_life();
		if (unlikely(tmp < 600 || tmp > 1100)) {
			printk(KERN_DEBUG "battery read error, voltage = %d\n", tmp);
			continue;
		}
		battery_life_sum += tmp;
		count++;
		if (count >= 5)
			break;
	}

	if (count)
		battery_life = battery_life_sum / count;

	return battery_life;
}


static int smartq_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smartq_get_ac_status() ;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int smartq_usb_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 0 ;		/* nothing to do */
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int smartq_battery_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
	int ret = 0;
	int capacity = 0;

	if (unlikely(global_battery_life < 700 || global_battery_life > 1000))
		global_battery_life = smartq_get_battery_voltage();
	
	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
			val->intval = 1;
			break;
	case POWER_SUPPLY_PROP_STATUS:
		if (smartq_get_ac_status()) {
			if (global_battery_life > 950){
				set_charge_mode(0);
				if (global_battery_life > 980)
					val->intval = POWER_SUPPLY_STATUS_FULL;
				else
					val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else {
				set_charge_mode(1);
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			}
		} else {
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
    		capacity = global_battery_life;
            	if (unlikely(capacity > 1000))
                		capacity = 1000;
            	if (likely(capacity > 785))					/* 3.402V */
                		capacity = (capacity - 785);
            	else
                		capacity = 0;
                	val->intval = ((capacity * 100) / 215);		/* 3.518V */
		//val->intval = (global_battery_life - 750) * 10 / 25;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_BATT_VOL:
		val->intval = (global_battery_life  * 42 / 10) ;
		break;
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_BATT_TEMP:
		val->intval = 300;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property smartq_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_BATT_VOL,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_BATT_TEMP,
};

static enum power_supply_property smartq_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property smartq_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static irqreturn_t smartq_battery_interrupt(int irq, void *dev_id)
{
	struct smartq_battery_data *battery_data = dev_id;

	pr_debug("^^^^^^^^^ Enter battery interrupt!!\n");
	
	global_battery_life = smartq_get_battery_voltage();

	wake_up_process(battery_data->kpwsd_tsk);

	return IRQ_HANDLED ;
}

static int kpwsd(void *arg)
{
	struct smartq_battery_data *battery_data = arg;

	down(&battery_data->thread_sem);
	do {

		if (kthread_should_stop()) {
			break;
		}

		global_battery_life = smartq_get_battery_voltage();
		if (likely(global_battery_life > 700 && global_battery_life <= 1000)) {
			power_supply_changed(&battery_data->battery);
			power_supply_changed(&battery_data->ac);
		}
		
		up(&battery_data->thread_sem);
		schedule_timeout_interruptible(msecs_to_jiffies(1000 * 60));
		down(&battery_data->thread_sem);

	} while (1);
	up(&battery_data->thread_sem);

	return 0;
}


static int smartq_battery_probe(struct platform_device *pdev)
{
	int ret;
	struct smartq_battery_data *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		ret = -ENOMEM;
		goto err_data_alloc_failed;
	}
	
	spin_lock_init(&data->lock);

	platform_set_drvdata(pdev, data);
	smartq_battery_data_p = data;

	data->battery.properties = smartq_battery_props;
	data->battery.num_properties = ARRAY_SIZE(smartq_battery_props);
	data->battery.get_property = smartq_battery_get_property;
	data->battery.name = "battery";
	data->battery.type = POWER_SUPPLY_TYPE_BATTERY;

	data->ac.properties = smartq_ac_props;
	data->ac.num_properties = ARRAY_SIZE(smartq_ac_props);
	data->ac.get_property = smartq_ac_get_property;
	data->ac.name = "ac";
	data->ac.type = POWER_SUPPLY_TYPE_MAINS;

	data->usb.properties = smartq_usb_props;
	data->usb.num_properties = ARRAY_SIZE(smartq_usb_props);
	data->usb.get_property = smartq_usb_get_property;
	data->usb.name = "usb";
	data->usb.type = POWER_SUPPLY_TYPE_USB;

	/* init global val */
	global_battery_life = smartq_get_battery_voltage();

#if 0
	ret = gpio_request(GPIO_DC_DETE, "DC_DETE");
	if (ret < 0) {
		pr_err("DC-DETE: failed to request GPIO %d,"
			" error %d\n", GPIO_DC_DETE, ret);
		goto err_no_irq;
	}
#endif	

	ret = gpio_direction_input(GPIO_DC_DETE);
	if (ret < 0) {
		pr_err("DC-DETE: failed to configure input"
			" direction for GPIO %d, error %d\n",
			GPIO_DC_DETE, ret);
		gpio_free(GPIO_DC_DETE);
		goto err_no_irq;
	}
	
	ret = s3c_gpio_setpull(GPIO_DC_DETE, S3C_GPIO_PULL_NONE);
	if (ret < 0) {
		pr_err("DC-DETE: failed to setpull"
			" pull request for GPIO %d, error %d\n",
			S3C_GPIO_PULL_NONE, ret);
		gpio_free(GPIO_DC_DETE);
		goto err_no_irq;
	}

	data->irq = gpio_to_irq(GPIO_DC_DETE); 
	if (data->irq < 0) {
		printk(KERN_ERR "%s: get_irq failed\n", pdev->name);
		ret = -ENODEV;
		gpio_free(GPIO_DC_DETE);
		goto err_no_irq;
	}

	ret = request_irq(data->irq, smartq_battery_interrupt, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		pdev->name, data);
	
	if (ret )
		goto err_request_irq_failed;

	ret = power_supply_register(&pdev->dev, &data->ac);
	if (ret)
		goto err_ac_failed;


	ret = power_supply_register(&pdev->dev, &data->battery);
	if (ret)
		goto err_battery_failed;

	ret = power_supply_register(&pdev->dev, &data->usb);
	if (ret)
		goto err_usb_failed;
	
	init_MUTEX(&data->thread_sem);

	data->kpwsd_tsk = kthread_create(kpwsd, data, "kpwsd");
	if (IS_ERR(data->kpwsd_tsk)) {
		ret = PTR_ERR(data->kpwsd_tsk);
		data->kpwsd_tsk = NULL;
		goto out;
	}
	
	wake_up_process(data->kpwsd_tsk);


	return 0;
	
out:
err_usb_failed:
	power_supply_unregister(&data->battery);
err_battery_failed:
	power_supply_unregister(&data->ac);
err_ac_failed:
	if (data->irq > 0)
		free_irq(data->irq, data);
err_request_irq_failed:
err_no_irq:
	kfree(data);
err_data_alloc_failed:
	return ret;
}

static int smartq_battery_remove(struct platform_device *pdev)
{
	struct smartq_battery_data *data = platform_get_drvdata(pdev);
	
	kthread_stop(data->kpwsd_tsk);

	power_supply_unregister(&data->battery);
	power_supply_unregister(&data->ac);
	power_supply_unregister(&data->usb);

	if (data->irq > 0)
		free_irq(data->irq, data);
	kfree(data);
	smartq_battery_data_p = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int smartq_battery_suspend(struct platform_device *dev, pm_message_t state)
{
	down(&smartq_battery_data_p->thread_sem);
        	return 0;
}

static int smartq_battery_resume(struct platform_device *dev)
{
	up(&smartq_battery_data_p->thread_sem);
        	return 0;
}
#else
#define smartq_battery_suspend NULL
#define smartq_battery_resume NULL
#endif

static struct platform_driver smartq_battery_driver = {
	.probe		= smartq_battery_probe,
	.remove		= smartq_battery_remove,
	.suspend		= smartq_battery_suspend,
	.resume		= smartq_battery_resume,
	.driver = {
		.name = "smartq-battery"
	}
};

static int __init smartq_battery_init(void)
{
	return platform_driver_register(&smartq_battery_driver);
}

static void __exit smartq_battery_exit(void)
{
	platform_driver_unregister(&smartq_battery_driver);
}

module_init(smartq_battery_init);
module_exit(smartq_battery_exit);

MODULE_AUTHOR("Riversky Fang <riverskyfang@126.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Battery driver for the SmartQ5/7");
