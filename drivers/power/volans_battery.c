/*
 * Battery and Power Management code for the SAMSUNG GT-i6410.
 *  - based on ds2760_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * Author: Geun-Young, Kim <nenggun.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c/max17040.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

/* update interval time. */
#define INTERVAL	30

/* GT-i6410 specific type definitions. */
enum {
	CABLE_TYPE_UNKNOWN = 0,
	CABLE_TYPE_TA,
	CABLE_TYPE_USB,
};

enum {
	PF_EVENT_OFFLINE = 0,
	PF_EVENT_ONLINE,
	PF_EVENT_UNKNOWN,
	PF_EVENT_FULL,
	PF_EVENT_COUNT_FULL,
	PF_EVENT_ABNORMAL_TEMP,
	PF_EVENT_RECOVER_TEMP,
};

struct volans_device_info {
	struct device *dev;

	unsigned long update_time;  /* RTC update time. */
	unsigned long elapsed_time; /* elapsed time. */
	unsigned char raw[MAX17040_DATA_SIZE];
	int voltage_raw;            /* units of 1.25 mV */
	int voltage_uV;             /* units of uV	*/
	int high_capacity;          /* percentage */
	int low_capacity;           /* additional resolution capacity */
	int temp_raw;               /* units of .. */
	int temp_C;                 /* units of C */
	int charge_status;          /* POWER_SUPPLY_STATUS_ */
	int charge_event;           /* PF_EVENT_ */
	int cable_type;             /* CABLE_TYPE_ */
	int full_counter;
	int full_condition;
	int ta_online;
	int vf;
	int slp_event;

	struct power_supply bat;
	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;

	void (*charger_hw_ctrl)(int);
};

/* Temperature - ADC value table. */
static int T_conversion_table[][2] = {
	{-20, 461},
	{-10, 427},
	{ -7, 413},
	{ -5, 401},
	{ -3, 395},
	{ -2, 394},
	{  0, 393},
	{  2, 372},
	{  5, 352},
	{ 10, 327},
	{ 20, 266},
	{ 30, 212},
	{ 40, 156},
	{ 50, 117},
	{ 56,  91},
	{ 58,  84},
	{ 60,  80},
	{ 61,  79},
	{ 63,  78},
	{ 65,  67},
	{ 70,  56},
	{  0,  -1}
};

extern unsigned int s3c_adc_conv(unsigned int channel);
extern unsigned long current_rtc_seconds(void);

static int adc_avr_value(int channel)
{
	unsigned int sample[7];
	unsigned int max_sample = 0, min_sample = 10000;
	unsigned int avr = 0;

	int i, count = (int)ARRAY_SIZE(sample);

	for (i = 0; i < count; i++) {
		sample[i] = s3c_adc_conv(channel);
		avr += sample[i];

		max_sample = max(max_sample, sample[i]);
		min_sample = min(min_sample, sample[i]);
	}

	avr = (avr - (max_sample + min_sample)) / (count - 2);
	return (int)avr;
}

static int T_interpolate(int raw_adc)
{
	int index, dist, dt, dest_C;

	if (raw_adc > T_conversion_table[0][1])
		return INT_MIN;

	for (index = 1; T_conversion_table[index][1] != -1; index++) {
		if (raw_adc > T_conversion_table[index][1]) {
			dist = T_conversion_table[index][1] -
				T_conversion_table[index - 1][1];
			dt = (dist * 1000) / (T_conversion_table[index][0] -
					T_conversion_table[index - 1][0]) - 500; /* dt < 0 */
			dt /= 1000;

			dist = T_conversion_table[index - 1][1];
			dest_C = T_conversion_table[index - 1][0];

			while (raw_adc < dist) {
				dist += dt;
				dest_C++;
			}

			return dest_C;
		}
	}

	return INT_MAX;
}

static void delta_interval_update(struct volans_device_info *di)
{
	int interval = current_rtc_seconds() - di->update_time;

	if (interval < 0 || interval > INTERVAL)
		interval = INTERVAL;

	di->elapsed_time += interval;
}

extern int (*connected_device_status)(void);
static int volans_battery_cable_type(void)
{
	int device_status, cable_type;

	if (!connected_device_status)
		return CABLE_TYPE_UNKNOWN;

	/*
	 * connected_device_status is provided by usb module.
	 *
	 * 0: disconnected
	 * 1: usb
	 * 2: dedicated charger(ta)
	 */
	device_status = connected_device_status();

	if (device_status == 1)
		cable_type = CABLE_TYPE_USB;
	else if (device_status == 2)
		cable_type = CABLE_TYPE_TA;
	else
		cable_type = CABLE_TYPE_UNKNOWN;

	return cable_type;
}

static int volans_battery_read_status(struct volans_device_info *di)
{
	const int adc_ch_temp = 1, adc_ch_vf = 2;
	int ret;

	ret = max17040_get_data(di->raw, sizeof(di->raw));
	if (ret != sizeof(di->raw)) {
		pr_warning("%s - call to max17040_get_info failed.\n", __func__);
		return -1;
	}

	/* MAX17040 reports voltage in units of 1.25mV, but the battery class
	 * reports in units of uV, so convert by multiplying by 1250. */
	di->voltage_raw = (di->raw[MAX17040_VCELL_MSB] << 4) |
		(di->raw[MAX17040_VCELL_LSB] >> 4);
	di->voltage_uV = di->voltage_raw * 1250;

	/* MAX17040 reports capacity in unit of 1.0%. low byte provides
	 * additional resolution in units 1/256%, so convert by multiplying by 1/256. */
	di->high_capacity = di->raw[MAX17040_SOC_MSB];
	di->low_capacity = (di->high_capacity * 100) +
		(di->raw[MAX17040_SOC_LSB] * 100 / 256);

	/* Temperature calculation using S3C6410 ADC. */
	di->temp_raw = adc_avr_value(adc_ch_temp);
	di->temp_C = T_interpolate(di->temp_raw);

	/* Cable type. */
	di->cable_type = volans_battery_cable_type();

	/* Power supply online check. */
	di->ta_online = !gpio_get_value(GPIO_TA_nCONNECTED);

	/* Battery VF. */
	di->vf = adc_avr_value(adc_ch_vf);

	return 0;
}

static void __volans_battery_update_status(struct volans_device_info *di)
{
	volans_battery_read_status(di);

	/* Initial status setting. */
	if (di->charge_status == POWER_SUPPLY_STATUS_UNKNOWN)
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;

	/* Invalid battery check. */
	if (di->vf > 300) {
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->charge_event = PF_EVENT_UNKNOWN;
		di->charger_hw_ctrl(0);
	}

	/* Charging! */
	else if (di->ta_online) {
		if (di->charge_status == POWER_SUPPLY_STATUS_DISCHARGING) {
			di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
			di->charge_event = PF_EVENT_ONLINE;
			di->full_counter = 0;
			di->full_condition = 0;
			di->elapsed_time = 0;
			di->charger_hw_ctrl(1);
		} else if (di->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			if (di->temp_C < 58 && di->temp_C > 0) {
				di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
				di->charge_event = PF_EVENT_RECOVER_TEMP;
				di->charger_hw_ctrl(1);
			}
		} else if (di->charge_status == POWER_SUPPLY_STATUS_FULL) {
			/* Recovery voltage level = 4.15V */
			if (di->voltage_uV < 4150000) {
				di->charge_status = POWER_SUPPLY_STATUS_CHARGING;
				di->full_condition = 0;
				di->elapsed_time = 0;
				di->charger_hw_ctrl(1);
			}
		} else {
			if (di->temp_C > 63 || di->temp_C < -5) {
				di->charge_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
				di->charge_event = PF_EVENT_ABNORMAL_TEMP;
				di->charger_hw_ctrl(0);
			} else if ((di->cable_type == CABLE_TYPE_TA && di->elapsed_time > 21600)
					|| di->full_condition) {
				di->charge_status = POWER_SUPPLY_STATUS_FULL;
				di->charge_event = PF_EVENT_FULL;
				di->full_counter++;
				di->charger_hw_ctrl(0);
			} else
				delta_interval_update(di);
		}
	}

	/* Disconnect! */
	else {
		di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->charge_event = PF_EVENT_OFFLINE;
		di->charger_hw_ctrl(0);
	}

	di->update_time = current_rtc_seconds();
}

static void volans_battery_update_status(struct volans_device_info *di)
{
	int old_status = di->charge_status;

	__volans_battery_update_status(di);

	/* Event processing. */
	if (old_status != di->charge_status) {
		if (di->charge_status == POWER_SUPPLY_STATUS_FULL) {
			if (di->full_counter == 1)
				power_supply_changed(&di->bat);
		} else if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING &&
				old_status == POWER_SUPPLY_STATUS_FULL) {
			/* nothing.. */
		} else
			power_supply_changed(&di->bat);
	}

	pr_info("%s- status: %d / mV: %d / T: %d(%d) / capacity: %d / full counter: %d / "
			"cable type: %d / elapsed: %ld\n", __func__,
			di->charge_status, di->voltage_uV/1000, di->temp_C, di->temp_raw,
			di->high_capacity, di->full_counter, di->cable_type, di->elapsed_time);
}

static int volans_battery_slp_update_status(void *data, int full_condition)
{
	struct volans_device_info *di = (struct volans_device_info *)data;
	int old_status = di->charge_status;

	di->full_condition = full_condition;

	__volans_battery_update_status(di);

	/* Wakeup? */
	if (old_status != di->charge_status) {
		if (di->charge_status == POWER_SUPPLY_STATUS_FULL) {
			if (di->full_counter == 1)
				di->slp_event = 1;
		} else if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING &&
				old_status == POWER_SUPPLY_STATUS_FULL) {
			/* nothing.. */
		} else
			di->slp_event = 1;
	}

	return di->slp_event;
}

static void volans_battery_work(struct work_struct *work)
{
	struct volans_device_info *di = container_of(work,
			struct volans_device_info, monitor_work.work);
	const int interval = HZ * INTERVAL;

	volans_battery_update_status(di);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, interval);
}

#define to_volans_device_info(x) container_of((x), struct volans_device_info, \
		bat);

static int volans_battery_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct volans_device_info *di = to_volans_device_info(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = di->charge_status;
			return 0;
		default:
			break;
	}

	volans_battery_read_status(di);

	switch (psp) {
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = di->voltage_uV;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval = di->high_capacity;
			break;
		case POWER_SUPPLY_PROP_TEMP:
			val->intval = di->temp_C;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

/* Volans battery specific sysfs nodes. */
#define VOLANS_BATTERY_PROP_LOW_CAPACITY	0
#define VOLANS_BATTERY_PROP_TEMP_RAW		1
#define VOLANS_BATTERY_PROP_CHARGE_EVENT	2

static struct device_attribute volans_battery_attrs[];
static ssize_t volans_battery_show_property(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct volans_device_info *di;

	const ptrdiff_t off = attr - volans_battery_attrs;
	int value = 0;

	di = to_volans_device_info(psy);

	if (off == VOLANS_BATTERY_PROP_CHARGE_EVENT) {
		return sprintf(buf, "%d\n", di->charge_event);
	}

	volans_battery_read_status(di);

	switch (off) {
		case VOLANS_BATTERY_PROP_LOW_CAPACITY:
			value = di->low_capacity;
			break;
		case VOLANS_BATTERY_PROP_TEMP_RAW:
			value = di->temp_raw;
			break;
		default:
			value = -EINVAL;
	}

	return sprintf(buf, "%d\n", value);
}

#define VOLANS_BATTERY_ATTR(_name)               \
{                                                \
	.attr = { .name = #_name, .mode = S_IRUGO }, \
	.show = volans_battery_show_property,        \
	.store = NULL,                               \
}

static struct device_attribute volans_battery_attrs[] = {
	VOLANS_BATTERY_ATTR(low_capacity),
	VOLANS_BATTERY_ATTR(temp_raw),
	VOLANS_BATTERY_ATTR(charge_event)
};

static int volans_battery_create_attrs(struct power_supply *psy)
{
	int i, rc = 0;

	for (i = 0; i < ARRAY_SIZE(volans_battery_attrs); i++) {
		rc = device_create_file(psy->dev,
				&volans_battery_attrs[i]);
		if (rc)
			goto attr_failed;
	}

	return 0;

attr_failed:
	while (i--)
		device_remove_file(psy->dev, &volans_battery_attrs[i]);

	return rc;
}

static void volans_battery_remove_attrs(struct power_supply *psy)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(volans_battery_attrs); i++) {
		device_remove_file(psy->dev, &volans_battery_attrs[i]);
	}
}

static irqreturn_t ta_connect_handler(int irq, void *dev_id)
{
	struct volans_device_info *di = (struct volans_device_info *)dev_id;

	if (di->charge_status != POWER_SUPPLY_STATUS_UNKNOWN) {
		cancel_delayed_work(&di->monitor_work);
		queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ/10);

		/*
		pr_info("%s- %s\n", __func__, gpio_get_value(GPIO_TA_nCONNECTED) ?
				"disconnected!" : "connected!");
		*/
	}

	return IRQ_HANDLED;
}

static irqreturn_t charging_handler(int irq, void *dev_id)
{
	struct volans_device_info *di = (struct volans_device_info *)dev_id;

	/* if TA is disconnected, ignore this interrupt! */
	if (gpio_get_value(GPIO_TA_nCONNECTED))
		return IRQ_HANDLED;

	if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING &&
			!di->full_condition) {
		/*
		 * full condition: TA is enabled, Indicator is high!
		 */
		di->full_condition = !gpio_get_value(GPIO_TA_EN) &&
			gpio_get_value(GPIO_TA_nCHG);

		if (di->full_condition) {
			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ/10);

			pr_info("%s- full condition!\n", __func__);
		}
	}

	return IRQ_HANDLED;
}

static void volans_battery_hw_charger_ctrl(int enable)
{
	gpio_set_value(GPIO_TA_EN, !enable);
}

static void volans_battery_hw_dep_init(void)
{
	/* gpio setup. */
	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_SFN(GPIO_TA_nCONNECTED_AF));
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TA_nCHG, S3C_GPIO_SFN(GPIO_TA_nCHG_AF));
	s3c_gpio_setpull(GPIO_TA_nCHG, S3C_GPIO_PULL_NONE);

	gpio_set_value(GPIO_TA_EN, GPIO_LEVEL_HIGH);
	s3c_gpio_cfgpin(GPIO_TA_EN, S3C_GPIO_SFN(GPIO_TA_EN_AF));
	s3c_gpio_setpull(GPIO_TA_EN, S3C_GPIO_PULL_NONE);
}

#define IRQ_TA_nCONNECTED	IRQ_EINT(19)
#define IRQ_TA_nCHG			IRQ_EINT(25)

static int volans_battery_set_isr(void *dev_id)
{
	set_irq_type(IRQ_TA_nCONNECTED, IRQ_TYPE_EDGE_BOTH);
	if (request_irq(IRQ_TA_nCONNECTED, ta_connect_handler, IRQF_DISABLED,
				"TA_nCONNECTED", dev_id)) {
		return -1;
	}

	set_irq_type(IRQ_TA_nCHG, IRQ_TYPE_EDGE_RISING);
	if (request_irq(IRQ_TA_nCHG, charging_handler, IRQF_DISABLED,
				"TA_nCHG", dev_id)) {
		free_irq(IRQ_TA_nCONNECTED, dev_id);
		return -1;
	}

	return 0;
}

static enum power_supply_property volans_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

extern int volans_battery_monitor_register(
		int (*update_status)(void *, int), void *data);
extern void volans_battery_monitor_unregister(void);

static int volans_battery_probe(struct platform_device *pdev)
{
	struct volans_device_info *di;
	int ret = 0;

	volans_battery_hw_dep_init();

	di = kzalloc(sizeof(struct volans_device_info), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	platform_set_drvdata(pdev, di);

	di->dev = &pdev->dev;
	di->bat.name = pdev->dev.bus_id;
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = volans_battery_props;
	di->bat.num_properties = ARRAY_SIZE(volans_battery_props);
	di->bat.get_property = volans_battery_get_property;
	di->bat.external_power_changed = NULL;

	di->charge_status = POWER_SUPPLY_STATUS_UNKNOWN;
	di->charge_event = PF_EVENT_UNKNOWN;
	di->cable_type = CABLE_TYPE_UNKNOWN;
	di->charger_hw_ctrl = volans_battery_hw_charger_ctrl;

	ret = power_supply_register(&pdev->dev, &di->bat);
	if (ret) {
		pr_err("%s - failed to register battery!\n", __func__);
		goto power_supply_failed;
	}

	ret = volans_battery_create_attrs(&di->bat);
	if (ret) {
		pr_err("%s - failed to create attributes!\n", __func__);
		goto create_attr_failed;
	}

	INIT_DELAYED_WORK(&di->monitor_work, volans_battery_work);
	di->monitor_wqueue = create_singlethread_workqueue(pdev->dev.bus_id);
	if (!di->monitor_wqueue) {
		ret = -ESRCH;
		goto workqueue_failed;
	}

	ret = volans_battery_set_isr(di);
	if (ret) {
		pr_err("%s - failed to register isr!\n", __func__);
		goto isr_failed;
	}

	volans_battery_monitor_register(volans_battery_slp_update_status,
			di);

	cancel_delayed_work_sync(&di->monitor_work);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ);

	return 0;

isr_failed:
	destroy_workqueue(di->monitor_wqueue);
workqueue_failed:
	volans_battery_remove_attrs(&di->bat);
create_attr_failed:
	power_supply_unregister(&di->bat);
power_supply_failed:
	kfree(di);

	return ret;
}

static int volans_battery_remove(struct platform_device *pdev)
{
	struct volans_device_info *di = platform_get_drvdata(pdev);

	free_irq(IRQ_TA_nCONNECTED, di);
	free_irq(IRQ_TA_nCHG, di);

	di->charger_hw_ctrl(0);

	cancel_delayed_work_sync(&di->monitor_work);
	destroy_workqueue(di->monitor_wqueue);

	power_supply_unregister(&di->bat);
	volans_battery_remove_attrs(&di->bat);

	kfree(di);
	volans_battery_monitor_unregister();

	return 0;
}

static int volans_battery_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct volans_device_info *di = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&di->monitor_work);

	/* suspend time compensation. */
	if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING)
		delta_interval_update(di);

	di->slp_event = 0;

	return 0;
}

static int volans_battery_resume(struct platform_device *pdev)
{
	struct volans_device_info *di = platform_get_drvdata(pdev);

	if (di->slp_event)
		power_supply_changed(&di->bat);

	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ);

	return 0;
}

static struct platform_driver volans_battery_driver = {
	.driver = {
		.name = "volans-battery",
	},
	.probe    = volans_battery_probe,
	.remove   = volans_battery_remove,
	.suspend  = volans_battery_suspend,
	.resume   = volans_battery_resume,
};

static int __init volans_battery_init(void)
{
	return platform_driver_register(&volans_battery_driver);
}

static void __exit volans_battery_exit(void)
{
	platform_driver_unregister(&volans_battery_driver);
}

module_init(volans_battery_init);
module_exit(volans_battery_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Geun-Young, Kim <nenggun.kim@samsung.com>");
MODULE_DESCRIPTION("SAMSUNG GT-i6410 battery driver");

