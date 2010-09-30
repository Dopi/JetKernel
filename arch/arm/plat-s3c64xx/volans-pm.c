/*
 * Samsung GT-i6410 power manager implementation.
 *
 * Author: Geun-Young, Kim <nenggun.kim@samsung.com>
 *
 * Copyright (c) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/i2c/pmic.h>

#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/regs-rtc.h>
#include <plat/regs-watchdog.h>
#include <plat/gpio-cfg.h>
#include <plat/power-domain.h>
#include <plat/s3c64xx-dvfs.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/volans_gpio_table.h>

struct __volans_battery_monitor {
	int (*update_status)(void *data, int full_condition);
	void *data;
};

static struct __volans_battery_monitor *_pbattery_monitor;

static struct timer_list power_off_timer;

extern void volans_machine_halt(char mode);
static void power_off_timer_handler(unsigned long data)
{
	void __iomem *p = (void __iomem *)data;

	if (!gpio_get_value(GPIO_TA_nCONNECTED)) {
		unsigned int *inf = (unsigned int *)__va(VOLANS_PA_BM);
		*inf = VOLANS_BM_MAGIC;

		__raw_writel(S3C2410_WTCNT_CNT, p + S3C2410_WTCNT_OFFSET);
		__raw_writel(S3C2410_WTCNT_CON, p + S3C2410_WTCON_OFFSET);
		__raw_writel(S3C2410_WTCNT_DAT, p + S3C2410_WTDAT_OFFSET);
	} else {
		volans_machine_halt('h');
	}
}

struct __gpio_config sleep_gpio_table[] = {
	/* GPK */
	{ .gpio = GPIO_CAM_EN, .af = GPIO_CAM_EN_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_CAM_CIF_nRST, .af = 1, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_CAM_CIF_nSTBY, .af = 1, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },

	/* GPL */
	{ .gpio = GPIO_TF_EN, .af = GPIO_TF_EN_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_CAM_3M_nSTBY, .af = 0, .level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_VIB_EN, .af = GPIO_VIB_EN_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_PHONE_ON, .af = GPIO_PHONE_ON_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_LCD_EN, .af = GPIO_LCD_EN_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },

	/* GPM */
	{ .gpio = GPIO_PDA_ACTIVE, .af = GPIO_PDA_ACTIVE_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_FM_SCL, .af = 0, .level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_FM_SDA, .af = 0, .level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },

	/* GPN */

	/* ... */
	{ .gpio = GPIO_LED_DRV_EN, .af = GPIO_LED_DRV_EN_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
	{ .gpio = GPIO_LED_DRV_SEL, .af = GPIO_LED_DRV_SEL_AF, .level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE, .slp_con = 0, .slp_pull = 0 },
};

extern int (*connected_device_status)(void);
static void pmic_disable(void)
{
	int symbols[] = {
		POWER_USB_OTGi, POWER_MOVINAND, POWER_MMC,
		POWER_BT,
		POWER_LCD, POWER_USB_OTG
	};

	int i;

	for (i = 0; i < ARRAY_SIZE(symbols); i++) {
		if (symbols[i] == POWER_MMC)
			pmic_power_control(POWER_MMC, PMIC_POWER_ON);
		else if (symbols[i] == POWER_BT)
			continue;
		else
			pmic_power_control(symbols[i], PMIC_POWER_OFF);
	}
}

static void pmic_enable(void)
{
	pmic_power_control(POWER_MMC, PMIC_POWER_ON);
	pmic_power_control(POWER_MOVINAND, PMIC_POWER_ON);

	if (connected_device_status() == 3) {
		pmic_power_control(POWER_USB_OTGi, PMIC_POWER_ON);
		pmic_power_control(POWER_USB_OTG, PMIC_POWER_ON);
	}
}

#define EINT_nONED_INT_AP	(0x1 << 0)
#define EINT_HOST_WAKE		(0x1 << 1)
#define EINT_nPOWER			(0x1 << 5)
#define EINT_INTB			(0x1 << 9)
#define EINT_DET_35			(0x1 << 10)
#define EINT_EAR_SEND_END	(0x1 << 11)
#define EINT_TA_nCONNECTED	(0x1 << 19)
#define EINT_TA_nCHG		(0x1 << 25)

extern void s3c_adc_slp_control(int enable);
extern void s3c_i2c_slp_control(int enable);
extern void __iomem *get_rtc_base(void);

static void wakeup_source_status(unsigned int wakeup_stat,
		unsigned int eint0pend)
{
	char buf[32];

	memset(buf, '\0', sizeof(buf));

	if (wakeup_stat & 0x02)
		sprintf(buf, "%s", "RTC Alarm!");
	else if (wakeup_stat & 0x04)
		sprintf(buf, "%s", "RTC Tick!");
	else if (wakeup_stat & 0x10)
		sprintf(buf, "%s", "Keypad!");
	else if (wakeup_stat & 0x01) {
		if (eint0pend & EINT_nONED_INT_AP)
			sprintf(buf, "%s", "OneDRAM!");
		else if (eint0pend & EINT_nPOWER)
			sprintf(buf, "%s", "Power Key!");
		else if (eint0pend & EINT_INTB)
			sprintf(buf, "%s", "USB!");
		else if (eint0pend & EINT_DET_35)
			sprintf(buf, "%s", "Earjack!");
		else if (eint0pend & EINT_EAR_SEND_END)
			sprintf(buf, "%s", "Earkey!");
		else if (eint0pend & EINT_TA_nCONNECTED)
			sprintf(buf, "%s", "TA(USB) Cable!");
		else if (eint0pend & EINT_TA_nCHG)
			sprintf(buf, "%s", "Full Charging!");
		else if (eint0pend & EINT_HOST_WAKE)
			sprintf(buf, "%s", "BT Host Wake!");
		else
			sprintf(buf, "%s", "None!!!");
	}

	else
		sprintf(buf, "%s", "None!!!");

	if (!(wakeup_stat & 0x04)) {
		printk("WAKEUP_STAT: 0x%08X / EINT0PEND: 0x%08X / Wakeup Source: %s\n",
				wakeup_stat, eint0pend, buf);
	}
}

void platform_pm_sleep(void)
{
	struct __gpio_config *pgpio;

	unsigned int spcon;
	int i;

	pmic_disable();

	for (i = 0; i < ARRAY_SIZE(sleep_gpio_table); i++) {
		pgpio = &sleep_gpio_table[i];

		if (pgpio->level != GPIO_LEVEL_NONE)
			gpio_set_value(pgpio->gpio, pgpio->level);

		s3c_gpio_cfgpin(pgpio->gpio, S3C_GPIO_SFN(pgpio->af));
		s3c_gpio_setpull(pgpio->gpio, pgpio->pull);
	}

	spcon = __raw_readl(S3C64XX_SPC_BASE);
	spcon &= ~(0xFFEC0000);
	__raw_writel(spcon, S3C64XX_SPC_BASE);

	__raw_writel(0x20, S3C64XX_SPCONSLP);

	s3c_set_normal_cfg(S3C64XX_DOMAIN_IROM, S3C64XX_ACTIVE_MODE, S3C64XX_IROM);
}

int platform_pm_wakeup(unsigned int wakeup_stat, unsigned int eint0pend)
{
	void __iomem *rtc_base = get_rtc_base();
	unsigned int tmp;

	int full_condition;
	int ret = 1;

	s3c_set_normal_cfg(S3C64XX_DOMAIN_IROM, S3C64XX_LP_MODE, S3C64XX_IROM);

	pmic_enable();

	full_condition = (wakeup_stat & 0x01) && (eint0pend & EINT_TA_nCHG);

	if (wakeup_stat & 0x04 || full_condition) {
		s3c_adc_slp_control(1);
		s3c_i2c_slp_control(1);

		ret = _pbattery_monitor->update_status(_pbattery_monitor->data,
				full_condition);
	}

	if (__raw_readl(rtc_base + S3C2410_RTCCON) & S3C_RTCCON_TICEN) {
		tmp = __raw_readl(rtc_base + S3C2410_RTCCON);
		__raw_writel(tmp & ~S3C_RTCCON_TICEN, rtc_base + S3C2410_RTCCON);
	}

	wakeup_source_status(wakeup_stat, eint0pend);

	return ret;
}

void platform_config_wakeup_source(void)
{
	unsigned int value;
	unsigned int EXTINT_SOURCES = 0;

	value = __raw_readl(S3C_PWR_CFG) | 0x0001ff80;

	value &= ~((0x1 << 11) | (0x1 << 10) | (0x1 << 8));
	__raw_writel(value, S3C_PWR_CFG);

	if (!gpio_get_value(GPIO_TA_nCONNECTED)) {
		void __iomem *base = get_rtc_base();
		unsigned int interval = (32768 - 1) * 30;

		__raw_writel(interval, base + S3C_RTC_TICNT);
		__raw_writel(__raw_readl(base + S3C2410_RTCCON) | S3C_RTCCON_TICEN,
				base + S3C2410_RTCCON);
	}

	s3c_gpio_cfgpin(GPIO_nPOWER, S3C_GPIO_SFN(GPIO_nPOWER_AF));
	s3c_gpio_setpull(GPIO_nPOWER, S3C_GPIO_PULL_NONE);

	value = __raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 8);
	value |= (S3C64XX_EXTINT_BOTHEDGE << 8);
	__raw_writel(value, S3C64XX_EINT0CON0);

	if (gpio_get_value(GPIO_BT_EN))
		EXTINT_SOURCES |= EINT_HOST_WAKE;

	if (!gpio_get_value(GPIO_TA_nCONNECTED) && !gpio_get_value(GPIO_TA_EN)
			&& !gpio_get_value(GPIO_TA_nCHG))
		EXTINT_SOURCES |= EINT_TA_nCHG;

	value = __raw_readl(S3C64XX_EINT0PEND);
	value |= (EINT_nONED_INT_AP | EINT_nPOWER | EINT_INTB | EINT_DET_35 |
			EINT_EAR_SEND_END | EINT_TA_nCONNECTED | EXTINT_SOURCES);
	__raw_writel(value, S3C64XX_EINT0PEND);

	value = (EINT_nONED_INT_AP | EINT_nPOWER | EINT_INTB | EINT_DET_35 |
			EINT_EAR_SEND_END | EINT_TA_nCONNECTED | EXTINT_SOURCES);
	__raw_writel(~value, S3C64XX_EINT0MASK);

	__raw_writel(0x0FFFFFFF & ~value, S3C_EINT_MASK);

	__raw_writel(0x00005000, S3C64XX_MEM0CONSLP0);
	__raw_writel(0x01451595, S3C64XX_MEM0CONSLP1);
	__raw_writel(0x10055000, S3C64XX_MEM1CONSLP);
}

static const char *gpio_base_port[] = {
	"S3C64XX_GPA", "S3C64XX_GPB", "S3C64XX_GPC", "S3C64XX_GPD", "S3C64XX_GPE",
	"S3C64XX_GPF", "S3C64XX_GPG", "S3C64XX_GPH", "S3C64XX_GPI", "S3C64XX_GPJ",
	"S3C64XX_GPK", "S3C64XX_GPL", "S3C64XX_GPM", "S3C64XX_GPN", "S3C64XX_GPO",
	"S3C64XX_GPP", "S3C64XX_GPQ"
};

static unsigned int gpio_base_num[] = {
	S3C64XX_GPA(0), S3C64XX_GPB(0), S3C64XX_GPC(0), S3C64XX_GPD(0), S3C64XX_GPE(0),
	S3C64XX_GPF(0), S3C64XX_GPG(0), S3C64XX_GPH(0), S3C64XX_GPI(0), S3C64XX_GPJ(0),
	S3C64XX_GPK(0), S3C64XX_GPL(0), S3C64XX_GPM(0), S3C64XX_GPN(0), S3C64XX_GPO(0),
	S3C64XX_GPP(0), S3C64XX_GPQ(0)
};

static int find_gpio_base(unsigned int pin)
{
	int index;

	/* GPQ */
	if (pin >= S3C64XX_GPQ(0))
		return ARRAY_SIZE(gpio_base_num) - 1;

	for (index = 1; index < ARRAY_SIZE(gpio_base_num); index++) {
		/* GPA ~ GPP */
		if (pin < gpio_base_num[index])
			return index - 1;
	}

	return -1;
}

static ssize_t show_gpio(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned int gpio;
	int index, i;

	for (i = 0; i < ARRAY_SIZE(volans_gpio_table); i++) {
		gpio = volans_gpio_table[i].gpio;
		index = find_gpio_base(gpio);

		if (index < 0)
			break;

		pr_info("-- #%03d LEVEL: %d / NAME: %s(%d)\n",
				gpio, gpio_get_value(gpio), gpio_base_port[index],
				gpio - gpio_base_num[index]);
	}

	return 0;
}

static ssize_t store_gpio(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	if (n > 0) {
		int pin, level;

		sscanf(buf, "%d %d", &pin, &level);

		if (pin >= 0 && (level == GPIO_LEVEL_LOW || level == GPIO_LEVEL_HIGH))
			gpio_set_value(pin, level);
	}

	return n;
}

static ssize_t show_pmic(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	const char *symbol_strings[] = {
		"POWER_USB_OTGi", "POWER_MOVINAND", "POWER_MMC",
		"POWER_BT",
		"POWER_LCD", "POWER_USB_OTG"
	};

	int symbols[] = {
		POWER_USB_OTGi, POWER_MOVINAND, POWER_MMC,
		POWER_BT,
		POWER_LCD, POWER_USB_OTG
	};

	int i, data;

	pr_info("-- PMIC Table --\n");

	for (i = 0; i < ARRAY_SIZE(symbols); i++) {
		if (pmic_power_read(symbols[i], &data))
			continue;

		pr_info("%s- %d\n", symbol_strings[i], data);
	}

	return 0;
}

static ssize_t store_pmic(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	int ldo, level;

	if (n > 0) {
		sscanf(buf, "%d %d", &ldo, &level);

		if (ldo < 3 || ldo > 8)
			return 0;

		pmic_power_control(ldo - 1, level);
	}

	return n;
}

static ssize_t show_domain(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned int normal_cfg, domain_stat;

	normal_cfg = __raw_readl(S3C_NORMAL_CFG);
	domain_stat = get_domain_off_stat();

	pr_info("-- NORMAL_CFG : 0x%08x / domain_stat: 0x%08x\n", normal_cfg, domain_stat);
	pr_info("-- 3D       : %d\n", (domain_stat & (1 << S3C64XX_3D)) ? 1 : 0);
	pr_info("-- MFC      : %d\n", (domain_stat & (1 << S3C64XX_MFC)) ? 1 : 0);
	pr_info("-- JPEG     : %d\n", (domain_stat & (1 << S3C64XX_JPEG)) ? 1 : 0);
	pr_info("-- CAM.I/F  : %d\n", (domain_stat & (1 << S3C64XX_CAMERA)) ? 1 : 0);
	pr_info("-- 2D       : %d\n", (domain_stat & (1 << S3C64XX_2D)) ? 1 : 0);
	pr_info("-- TVENC.   : %d\n", (domain_stat & (1 << S3C64XX_TVENC)) ? 1 : 0);
	pr_info("-- SCALER   : %d\n", (domain_stat & (1 << S3C64XX_SCALER)) ? 1 : 0);
	pr_info("-- ROT      : %d\n", (domain_stat & (1 << S3C64XX_ROT)) ? 1 : 0);
	pr_info("-- POST     : %d\n", (domain_stat & (1 << S3C64XX_POST)) ? 1 : 0);
	pr_info("-- LCD      : %d\n", (domain_stat & (1 << S3C64XX_LCD)) ? 1 : 0);
	pr_info("-- SDMA0    : %d\n", (domain_stat & (1 << S3C64XX_SDMA0)) ? 1 : 0);
	pr_info("-- SDMA1    : %d\n", (domain_stat & (1 << S3C64XX_SDMA1)) ? 1 : 0);
	pr_info("-- SECURITY : %d\n", (domain_stat & (1 << S3C64XX_SECURITY)) ? 1 : 0);
	pr_info("-- ETM      : %d\n", (domain_stat & (1 << S3C64XX_ETM)) ? 1 : 0);
	pr_info("-- IROM     : %d\n", (domain_stat & (1 << S3C64XX_IROM)) ? 1 : 0);

	return 0;
}

static ssize_t store_domain(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	int config, flag, deviceID;

	if (n > 0) {
		sscanf(buf, "%d %d %d", &config, &flag, &deviceID);
		s3c_set_normal_cfg((1 << config), flag, deviceID);
	}

	return n;
}

static ssize_t show_vddarm(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	struct __s3c64xx_dvfs_data dvfs_info;
	get_dvfs_info(&dvfs_info);

	return sprintf(buf, "%d\n", dvfs_info.vdd_arm);
}

static ssize_t store_vddarm(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	int lv;

	if (!n) return 0;
	sscanf(buf, "%d", &lv);
	set_dvfs_vdd_arm_level(lv);

	return n;
}

static ssize_t show_bgm_mode(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	struct __s3c64xx_dvfs_data dvfs_info;
	get_dvfs_info(&dvfs_info);

	return sprintf(buf, "%d\n", dvfs_info.bgm_mode);
}

static ssize_t store_bgm_mode(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	int mode;

	if (!n) return 0;
	sscanf(buf, "%d", &mode);
	set_dvfs_bgm_mode(mode);

	return n;
}

static ssize_t show_start_poweroff(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 0);
}

static ssize_t store_start_poweroff(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t n)
{
	const int interval = 35;

	int mode;
	sscanf(buf, "%d", &mode);

	if (mode == 1) {
		power_off_timer.expires = jiffies + (HZ * interval);

		del_timer(&power_off_timer);
		add_timer(&power_off_timer);

		pr_info("%s- start power off!(%d) after %ds..\n", __func__,
				mode, interval);
	}

	return n;
}

#define VOLANS_PM_ATTR(_name)		\
	struct kobj_attribute _name =	\
__ATTR(_name, 0644, show_##_name, store_##_name)

static VOLANS_PM_ATTR(gpio);
static VOLANS_PM_ATTR(pmic);
static VOLANS_PM_ATTR(domain);
static VOLANS_PM_ATTR(vddarm);
static VOLANS_PM_ATTR(bgm_mode);
static VOLANS_PM_ATTR(start_poweroff);

int power_sysfs_add(void)
{
	struct attribute *pm_attrs[] = {
		&gpio.attr, &pmic.attr, &domain.attr, &vddarm.attr, &bgm_mode.attr,
		&start_poweroff.attr
	};

	int i;

	for (i = 0; i < ARRAY_SIZE(pm_attrs); i++) {
		if (power_kobj != NULL) {
			if (sysfs_create_file(power_kobj, pm_attrs[i]) < 0)
				return -ENOMEM;
		}
	}

	power_off_timer.function = power_off_timer_handler;
	power_off_timer.data = (unsigned long)ioremap(S3C64XX_PA_WATCHDOG,
			S3C64XX_SZ_WATCHDOG);

	init_timer(&power_off_timer);

	return 0;
}

int volans_battery_monitor_register(
		int (*update_status)(void *, int), void *data)
{
	_pbattery_monitor = kzalloc(sizeof(struct __volans_battery_monitor),
			GFP_KERNEL);
	if (!_pbattery_monitor)
		return -ENOMEM;

	_pbattery_monitor->update_status = update_status;
	_pbattery_monitor->data = data;

	return 0;
}

void volans_battery_monitor_unregister(void)
{
	if (_pbattery_monitor)
		kfree(_pbattery_monitor);
}
EXPORT_SYMBOL(volans_battery_monitor_register);
EXPORT_SYMBOL(volans_battery_monitor_unregister);


