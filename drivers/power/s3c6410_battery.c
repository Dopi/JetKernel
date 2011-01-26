/*
 * linux/drivers/power/s3c6410_battery.c
 *
 * Battery measurement code for S3C6410 platform.
 *
 * based on palmtx_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>

#include "s3c6410_battery.h"

#define DEBUG

static struct wake_lock vbus_wake_lock;
static struct wake_lock rf_cal_wake_lock;

#if (defined __TEST_DEVICE_DRIVER__  || defined __ALWAYS_AWAKE_DEVICE__)
static struct wake_lock wake_lock_for_dev;
#endif /* __TEST_DEVICE_DRIVER__ || __ALWAYS_AWAKE_DEVICE__ */

#ifdef __USE_EGPIO__
#include <plat/egpio.h>
#endif /* __USE_EGPIO__ */

#ifdef __TEST_DEVICE_DRIVER__ 
#include <linux/i2c/pmic.h>
#endif /* __TEST_DEVICE_DRIVER__ */

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
#include <linux/i2c.h>
#include "fuel_gauge.c"
#endif /* __FUEL_GAUGES_IC__ */

/* Prototypes */
int first_soc =0;
//extern int orig_temp;		// added by kimjh
extern int s3c_adc_get_adc_data(int channel);
extern int get_usb_power_state(void);

static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count);
static void s3c_set_chg_en(int enable);

#ifdef __TEST_DEVICE_DRIVER__
extern int amp_enable(int);
extern int audio_power(int);

static ssize_t s3c_test_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t s3c_test_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count);
static int bat_temper_state = 0; // for test 1: overheat, 2: freeze
#endif /* __TEST_DEVICE_DRIVER__ */

#ifdef __TEST_MODE_INTERFACE__
static struct power_supply *s3c_power_supplies_test = NULL;
static void polling_timer_func(unsigned long unused);
static void s3c_bat_status_update(struct power_supply *bat_ps);
#endif /* __TEST_MODE_INTERFACE__ */

#define ADC_DATA_ARR_SIZE	6
#define ADC_TOTAL_COUNT		10
#define POLLING_INTERVAL	2000
#ifdef __TEST_MODE_INTERFACE__
#define POLLING_INTERVAL_TEST	1000
#endif /* __TEST_MODE_INTERFACE__ */

#ifdef __BATTERY_COMPENSATION__
/* Offset Bit Value */
#define OFFSET_VIBRATOR_ON		(0x1 << 0)
#define OFFSET_CAMERA_ON		(0x1 << 1)
#define OFFSET_MP3_PLAY			(0x1 << 2)
#define OFFSET_VIDEO_PLAY		(0x1 << 3)
#define OFFSET_VOICE_CALL_2G		(0x1 << 4)
#define OFFSET_VOICE_CALL_3G		(0x1 << 5)
#define OFFSET_DATA_CALL		(0x1 << 6)
#define OFFSET_LCD_ON			(0x1 << 7)
#define OFFSET_TA_ATTACHED		(0x1 << 8)
#define OFFSET_CAM_FLASH		(0x1 << 9)
#define OFFSET_BOOTING			(0x1 << 10)
#endif /* __BATTERY_COMPENSATION__ */

#ifdef __9BITS_RESOLUTION__
#define INVALID_VOL_ADC		20
#else /* __9BITS_RESOLUTION__ */
#define INVALID_VOL_ADC		160
#endif /* __9BITS_RESOLUTION__ */

#define ENABLE		1
#define DISABLE		0

#ifndef convert_adc2voltage
	#ifdef __9BITS_RESOLUTION__
	#define convert_adc2voltage(x)		(x * 10000 / 848)
	#else /* __9BITS_RESOLUTION__ */
	#define convert_adc2voltage(x)		((x - 2170 ) * 10 / 7 + 3200)
	#endif /* __9BITS_RESOLUTION__ */
#endif /* convert_adc2voltage */

static struct work_struct bat_work;
static struct work_struct cable_work;

#ifdef COMPENSATE_BOOTING
static struct workqueue_struct *batt_drv_wqueue;
static struct work_struct compensate_boot_work;
#endif /* COMPENSATE_BOOTING */

static struct device *dev;
static struct timer_list polling_timer;
static struct timer_list cable_timer;
static int cable_intr_cnt = 0;

static int s3c_battery_initial;
static int force_update;
static int full_charge_flag;

static int batt_max;
static int batt_full;
static int batt_safe_rech;
static int batt_almost;
static int batt_high;
static int batt_medium;
static int batt_low;
static int batt_critical;
static int batt_min;
static int batt_off;
#ifdef __ADJUST_RECHARGE_ADC__
static int batt_recharging;
#endif /* __ADJUST_RECHARGE_ADC__ */
#ifdef __BATTERY_COMPENSATION__
static int batt_compensation;
#endif /* __BATTERY_COMPENSATION__ */

static unsigned int start_time_msec;
static unsigned int total_time_msec;

#ifdef __BOARD_REV_ADC__
static int board_rev_adc;
static int is_end_board_rev_adc;
#endif /* __BOARD_REV_ADC__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
static int is_batt_test=0;
#endif
static char *status_text[] = {
	[POWER_SUPPLY_STATUS_UNKNOWN] =		"Unknown",
	[POWER_SUPPLY_STATUS_CHARGING] =	"Charging",
	[POWER_SUPPLY_STATUS_DISCHARGING] =	"Discharging",
	[POWER_SUPPLY_STATUS_NOT_CHARGING] =	"Not Charging",
	[POWER_SUPPLY_STATUS_FULL] =		"Full",
};

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
} charger_type_t;

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	s32 batt_vol;		/* Battery voltage from ADC */
	s32 batt_vol_adc;	/* Battery ADC value */
	s32 batt_vol_adc_cal;	/* Battery ADC value (calibrated)*/
	s32 batt_temp;		/* Battery Temperature (C) from ADC */
	s32 batt_temp_adc;	/* Battery Temperature ADC value */
	s32 batt_temp_adc_cal;	/* Battery Temperature ADC value (calibrated) */
	s32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_svc;		/* Battery svc*/
	u32 batt_is_full;       /* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	s32 batt_vol_adc_aver;	/* batt vol adc average */
#ifdef __TEST_MODE_INTERFACE__
	u32 batt_test_mode;	/* test mode */
	s32 batt_vol_aver;	/* batt vol average */
	s32 batt_temp_aver;	/* batt temp average */
	s32 batt_temp_adc_aver;	/* batt temp adc average */
	s32 batt_v_f_adc;	/* batt V_F adc */
#endif /* __TEST_MODE_INTERFACE__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	s32 batt_temp_full_notify; //Full battery notify to platform
	s32 batt_temp_chg_cut_off_on; // cut off / on by temperature... 
#endif
    s32 batt_slate_mode; /* slate mode */
};

/* lock to protect the battery info */
static DEFINE_MUTEX(work_lock);

struct s3c_battery_info {
	int present;
	int polling;
	unsigned int polling_interval;
	unsigned int device_state;

	struct battery_info bat_info;
};
static struct s3c_battery_info s3c_bat_info;

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};
static struct adc_sample_info adc_sample[ENDOFADC];

static inline int s3c_adc_get_adc_data_ex(int channel) {
#ifndef __9BITS_RESOLUTION__
	return s3c_adc_get_adc_data(channel);
#else
#ifdef __REVERSE_TEMPER_ADC__
	if (channel == S3C_ADC_TEMPERATURE) 
		return (4096 - s3c_adc_get_adc_data(channel)) / 8;
	else
#endif /* __REVERSE_TEMPER_ADC__ */
		return s3c_adc_get_adc_data(channel) / 8;
#endif /* __9BITS_RESOLUTION__ */
}

#ifdef __CHECK_CHG_CURRENT__
static inline void clear_adc_sample(adc_channel_type channel)
{
	adc_sample[channel].cnt = 0;
	adc_sample[channel].total_adc = 0;
}
#endif /* __CHECK_CHG_CURRENT__ */

static unsigned long calculate_average_adc(adc_channel_type channel, int adc)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;

	cnt = adc_sample[channel].cnt;
	total_adc = adc_sample[channel].total_adc;

	if (adc < 0 || adc == 0) {
		dev_err(dev, "%s: invalid adc : %d(ch:%d)\n", __func__,
				adc, channel);
		adc = adc_sample[channel].average_adc;
	}

	if( cnt < ADC_TOTAL_COUNT ) {
		adc_sample[channel].adc_arr[cnt] = adc;
		adc_sample[channel].index = cnt;
		adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {
#if 0
		if (channel == S3C_ADC_VOLTAGE &&
				!s3c_bat_info.bat_info.charging_enabled && 
				adc > adc_sample[channel].average_adc) {
			dev_dbg(dev, "%s: adc over avg : %d\n", __func__, adc);
			return adc_sample[channel].average_adc;
		}
#endif
		index = adc_sample[channel].index;
		if (++index >= ADC_TOTAL_COUNT)
			index = 0;

		total_adc = (total_adc - adc_sample[channel].adc_arr[index]) + adc;
		average_adc = total_adc / ADC_TOTAL_COUNT;

		adc_sample[channel].adc_arr[index] = adc;
		adc_sample[channel].index = index;
	}

	adc_sample[channel].total_adc = total_adc;
	adc_sample[channel].average_adc = average_adc;

	dev_dbg(dev, "%s: ch:%d adc=%d, avg_adc=%d\n",
			__func__, channel, adc, average_adc);
	return average_adc;
}

static int s3c_bat_get_adc_data(adc_channel_type adc_ch)
{
	int adc_arr[ADC_DATA_ARR_SIZE];
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++) {
		adc_arr[i] = s3c_adc_get_adc_data_ex(adc_ch);
		dev_dbg(dev, "%s: adc_arr = %d\n", __func__, adc_arr[i]);
		if (i != 0) {
			if (adc_arr[i] > adc_max) 
				adc_max = adc_arr[i];
			else if (adc_arr[i] < adc_min)
				adc_min = adc_arr[i];
		} else {
			adc_max = adc_arr[0];
			adc_min = adc_arr[0];
		}
		adc_total += adc_arr[i];
	}

	dev_dbg(dev, "%s: adc_max = %d, adc_min = %d\n",
			__func__, adc_max, adc_min);
	
	
	return (adc_total - adc_max - adc_min) / (ADC_DATA_ARR_SIZE - 2);
}

//#ifndef __FUEL_GAUGES_IC__
static unsigned long s3c_read_bat(struct power_supply *bat_ps)
{
	int adc = 0;
	int adc_data = 0;
	static int cnt = 0;

#if 0 //def CONFIG_MACH_INFOBOWLQ	
   	adc_data = s3c_adc_get_adc_data(REVISION_ADC_CHANNEL); 			// added by kimjh
	printk("%s, adc_data : %d\n",__FUNCTION__, adc_data);			// added by kimjh
#endif
	
	dev_dbg(dev, "%s\n", __func__);

	adc = s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);
	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);
	
#ifdef __BATTERY_COMPENSATION__
	adc += batt_compensation;
#endif /* __BATTERY_COMPENSATION__ */
	if (adc < s3c_bat_info.bat_info.batt_vol_adc_aver - INVALID_VOL_ADC
			&& cnt < 10) {
		dev_err(dev, "%s: invalid adc = %d\n", __func__, adc);
		adc = s3c_bat_info.bat_info.batt_vol_adc_aver;
		cnt++;
	} else {
		cnt = 0;
	}
	s3c_bat_info.bat_info.batt_vol_adc =  adc;

	return calculate_average_adc(S3C_ADC_VOLTAGE, adc);
}
//#endif /* __FUEL_GAUGES_IC__ */

static unsigned long s3c_read_temp(struct power_supply *bat_ps)
{
	int adc = 0;

	dev_dbg(dev, "%s\n", __func__);

	adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);

#ifdef __TEST_DEVICE_DRIVER__
	switch (bat_temper_state) {
		case 0:
			break;
		case 1:
			adc = TEMP_HIGH_BLOCK;
			break;
		case 2:
			adc = TEMP_LOW_BLOCK;
			break;
		default:
			break;
	}
#endif /* __TEST_DEVICE_DRIVER__ */

	s3c_bat_info.bat_info.batt_temp_adc = adc;

#ifdef __AVG_TEMP_ADC__
	return calculate_average_adc(S3C_ADC_TEMPERATURE, adc);
#else /* __AVG_TEMP_ADC__ */
	return adc;
#endif /* __AVG_TEMP_ADC__ */
}

static u32 s3c_get_bat_health(void)
{
	return s3c_bat_info.bat_info.batt_health;
}

static void s3c_set_bat_health(u32 batt_health)
{
	s3c_bat_info.bat_info.batt_health = batt_health;
}

static int is_over_abs_time(void)
{
	unsigned int total_time;

	if (!start_time_msec)
		return 0;

	if (s3c_bat_info.bat_info.batt_is_recharging)
		total_time = TOTAL_RECHARGING_TIME;
	else
		total_time = TOTAL_CHARGING_TIME;

	total_time_msec = jiffies_to_msecs(jiffies) - start_time_msec;
	if (total_time_msec > total_time)
		return 1;
	else
		return 0;
}

#ifdef __BATTERY_COMPENSATION__
static void s3c_bat_set_compesation(int mode, 
				    int offset,
				    int compensate_value)
{
	if (mode) {
		if (!(s3c_bat_info.device_state & offset)) {
			s3c_bat_info.device_state |= offset;
			batt_compensation += compensate_value;
		}
	} else {
		if (s3c_bat_info.device_state & offset) {
			s3c_bat_info.device_state &= ~offset;
			batt_compensation -= compensate_value;
		}
	}
	dev_dbg(dev, "%s: device_state=0x%x, compensation=%d\n", __func__,
			s3c_bat_info.device_state, batt_compensation);
}
#endif /* __BATTERY_COMPENSATION__ */

#ifdef __CHECK_CHG_CURRENT__
static int s3c_read_current(struct power_supply *bat_ps)
{
	int adc = 0;
	adc = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);

	return calculate_average_adc(S3C_ADC_CHG_CURRENT, adc);
}

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
static void check_chg_current_fuel(struct power_supply *bat_ps)
{
	static int cnt = 0;
	int chg_current = 0; 
      int full_current_cond = 0;
	  
	chg_current = s3c_read_current(bat_ps);
	s3c_bat_info.bat_info.batt_current = chg_current;

       full_current_cond = CURRENT_OF_FULL_CHG ;
	if ((chg_current <= full_current_cond)/* && (s3c_bat_info.bat_info.level >= 100)*/) {
		cnt++;
		if (cnt >= 20) {
			dev_printk("<6>",dev, "%s: battery full(%d)\n",
					__func__, chg_current);
			s3c_set_chg_en(DISABLE);
			s3c_bat_info.bat_info.batt_is_full = 1;
			force_update = 1;
			full_charge_flag = 1;
			cnt = 0;
			clear_adc_sample(S3C_ADC_CHG_CURRENT);
		}
	} else {
		cnt = 0;
	}
#ifdef log_block	
//	dev_dbg(dev, "%s: chg_current=%d\n", __func__, chg_current);
	dev_printk("<6>",dev, "%s: chg_current=%d\n", __func__, chg_current);
#endif
}
#endif
static void check_chg_current(struct power_supply *bat_ps)
{
	static int cnt = 0;
	int chg_current = 0; 

	chg_current = s3c_read_current(bat_ps);
	s3c_bat_info.bat_info.batt_current = chg_current;
	if (chg_current <= CURRENT_OF_FULL_CHG) {
		cnt++;
		if (cnt >= 10) {
			dev_info(dev, "%s: battery full(%d)\n",
					__func__, chg_current);
			s3c_set_chg_en(DISABLE);
			s3c_bat_info.bat_info.batt_is_full = 1;
			force_update = 1;
			full_charge_flag = 1;
			cnt = 0;
			clear_adc_sample(S3C_ADC_CHG_CURRENT);
		}
	} else {
		cnt = 0;
	}
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	dev_info(dev, "%s: chg_current=%d\n", __func__, chg_current);
#else
	dev_dbg(dev, "%s: chg_current=%d\n", __func__, chg_current);
#endif
}
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __ADJUST_RECHARGE_ADC__
#ifdef __FUEL_GAUGES_IC_MAX17043__
static void check_recharging_bat_fuel(int fg_vcell)				// added by kimjh
{
	static int cnt = 0; 
	int adc_data = 0;
#if 0
   	adc_data = s3c_adc_get_adc_data(REVISION_ADC_CHANNEL); 		// added by kimjh
	printk("%s, adc_data :%d\n",__FUNCTION__, adc_data);		// added by kimjh
#endif
	if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
#ifdef log_block
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);
#endif
		cnt = 0; 
		return;
	}    

#ifdef log_block
	printk("%s: recharging(vcell:%d)\n", __func__, fg_vcell);
#endif
	if (s3c_bat_info.bat_info.batt_is_full &&
			!s3c_bat_info.bat_info.charging_enabled &&
			(fg_vcell <= RECHARGE_COND_VOLTAGE  
			 || fg_vcell <= EX_FULL_CHARGE_COND_VOLTAGE) ) {

		if (++cnt >= 20) {
			dev_printk("<6>",dev, "%s: recharging(vcell:%d)\n", __func__, fg_vcell);
			s3c_bat_info.bat_info.batt_is_recharging = 1; 
			s3c_set_chg_en(ENABLE);
			cnt = 0; 
		}    
	} else {
		cnt = 0; 
	}    
}

static void check_recharging_bat(int fg_vcell)
{
	static int cnt = 0; 
	int adc_data = 0;
#if 0
   	adc_data = s3c_adc_get_adc_data(REVISION_ADC_CHANNEL); 		// added by kimjh
	printk("%s, adc_data :%d\n",__FUNCTION__, adc_data);		// added by kimjh
#endif
	if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);
		cnt = 0; 
		return;
	}    

	printk("%s: recharging(vcell:%d)\n", __func__, fg_vcell);
/*
	if (s3c_bat_info.bat_info.batt_is_full &&
			!s3c_bat_info.bat_info.charging_enabled &&
			(fg_vcell >= RECHARGE_COND_VOLTAGE))                                                                     
	{    
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);                                                                
		re_cnt++;
		if(re_cnt <= RECHAGE_FIRST_COUNT)
		{    
			printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);
			s3c_bat_info.bat_info.batt_is_recharging = 0; 
			s3c_set_chg_en(DISABLE);                                                                            
			//      re_cnt = 0;  
		}    
		//  else
		//      re_cnt = 0;  
	}   
 */	
	if (s3c_bat_info.bat_info.batt_is_full &&
			!s3c_bat_info.bat_info.charging_enabled &&
			(fg_vcell <= RECHARGE_COND_VOLTAGE 
			 /*|| fg_vcell <= FULL_CHARGE_COND_VOLTAGE*/) ) {

		if (++cnt >= 10) {
			dev_info(dev, "%s: recharging(vcell:%d)\n", __func__, fg_vcell);
			s3c_bat_info.bat_info.batt_is_recharging = 1; 
			s3c_set_chg_en(ENABLE);
			cnt = 0; 
		}    
	} else {
		cnt = 0; 
	}    
}
#else /* __FUEL_GAUGES_IC__ */
static void check_recharging_bat(int bat_vol)
{
	static int cnt = 0;

	if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD)
		goto out;

	if (!s3c_bat_info.bat_info.batt_is_full || 
		s3c_bat_info.bat_info.charging_enabled) 
		goto out;

	if (batt_recharging != -1 && bat_vol <= batt_recharging) {
		if (++cnt >= 3) {
			dev_info(dev, "%s: recharging(adj):%d\n", __func__,
					bat_vol);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(ENABLE);
			goto out;
		}
	} else {
		cnt = 0;
	}

	if (bat_vol <= batt_safe_rech ) {
		dev_info(dev, "%s: recharging(safe):%d\n", __func__, bat_vol);
		s3c_bat_info.bat_info.batt_is_recharging = 1;
		s3c_set_chg_en(ENABLE);
		goto out;
	}
	return;
out:
	cnt = 0;
	return;
}
#endif
#else /* __ADJUST_RECHARGE_ADC__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
static void check_recharging_bat(int fg_vcell)
{
	static int cnt = 0;

	if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
		cnt = 0;
		return;
	}

	if (s3c_bat_info.bat_info.batt_is_full &&
		!s3c_bat_info.bat_info.charging_enabled &&
		(fg_vcell <= RECHARGE_COND_VOLTAGE || 
			fg_vcell <= FULL_CHARGE_COND_VOLTAGE)) {
		if (++cnt >= 10) {
			dev_info(dev, "%s: recharging(vcell:%d)\n", __func__,
					fg_vcell);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(ENABLE);
			cnt = 0;
		}
	} else {
		cnt = 0;
	}
}
#else /* __FUEL_GAUGES_IC__ */
static void check_recharging_bat(int bat_vol)
{
	static int cnt = 0;

	if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
		cnt = 0;
		return;
	}

	if (bat_vol > batt_safe_rech ) {
		cnt = 0;
		return;
	}

	if (s3c_bat_info.bat_info.batt_is_full &&
		!s3c_bat_info.bat_info.charging_enabled) {
		if (++cnt >= 10) {
			dev_info(dev, "%s: recharging(safe), (adc:%d)\n",
					__func__, bat_vol);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(ENABLE);
			cnt = 0;
		}
	} else {
		cnt = 0;
	}
}
#endif /* __FUEL_GAUGES_IC__ */
#endif /* __ADJUST_RECHARGE_ADC__ */



static int s3c_get_bat_level_concept(struct power_supply *bat_ps)
{
	int bat_level = 0;
	int bat_vol = s3c_read_bat(bat_ps);

	s3c_bat_info.bat_info.batt_vol_adc_aver = bat_vol;

	if(is_over_abs_time()) {
		bat_level = 100;
		s3c_bat_info.bat_info.batt_is_full = 1;
		dev_info(dev, "%s: charging time is over\n", __func__);
		s3c_set_chg_en(DISABLE);
		goto __end__;
	}

#ifdef __BATTERY_COMPENSATION__
	if (s3c_bat_info.bat_info.charging_enabled) {
		if (bat_vol > batt_full - COMPENSATE_TA) {
			s3c_bat_set_compesation(0, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
		}
	}
#endif /* __BATTERY_COMPENSATION__ */

	check_recharging_bat(bat_vol);

	if (bat_vol > batt_full) {
		int temp = 5;	/* 100% : 4.03V ADC : 341  Offset : 45 */
		if (bat_vol > (batt_full + temp) || 
				s3c_bat_info.bat_info.batt_is_full)
			bat_level = 100;
		else
			bat_level = 90;

#ifdef __CHECK_CHG_CURRENT__
		if (s3c_bat_info.bat_info.charging_enabled) {
			check_chg_current(bat_ps);
			if (!s3c_bat_info.bat_info.batt_is_full)
				bat_level = 90;
		}
#endif /* __CHECK_CHG_CURRENT__ */
		dev_dbg(dev, "%s: (full)level = %d\n", __func__, bat_level );
	} else if (batt_full >= bat_vol && bat_vol > batt_almost) {
		int temp = (batt_full - batt_almost) / 2;
		if (bat_vol > (batt_almost + temp))
			bat_level = 80;
		else
			bat_level = 70;

		if (s3c_bat_info.bat_info.batt_is_recharging)
			bat_level = 100;

		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);		// added by kimjh
		dev_dbg(dev, "%s: (almost)level = %d\n", __func__, bat_level);
	} else if (batt_almost >= bat_vol && bat_vol > batt_high) {
		int temp = (batt_almost - batt_high) / 2;
		if (bat_vol > (batt_high + temp))
			bat_level = 60;
		else
			bat_level = 50;
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);		// added by kimjh
		dev_dbg(dev, "%s: (high)level = %d\n", __func__, bat_level );
	} else if (batt_high >= bat_vol && bat_vol > batt_medium) {
		int temp = (batt_high - batt_medium) / 2;
		if (bat_vol > (batt_medium + temp))
			bat_level = 40;
		else
			bat_level = 30;
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);		// added by kimjh
		dev_dbg(dev, "%s: (med)level = %d\n", __func__, bat_level);
	} else if (batt_medium >= bat_vol && bat_vol > batt_low) {
		bat_level = 15;
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);		// added by kimjh
		dev_dbg(dev, "%s: (low)level = %d\n", __func__, bat_level);
	} else if (batt_low >= bat_vol && bat_vol > batt_critical) {
		bat_level = 5;
		printk("func : %s, line : %d\n",__FUNCTION__,__LINE__);		// added by kimjh
		dev_dbg(dev, "%s: (cri)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_critical >= bat_vol && bat_vol > batt_min) {
		bat_level = 3;
		dev_info(dev, "%s: (min)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_min >= bat_vol && bat_vol > batt_off) {
		bat_level = 1;
		dev_info(dev, "%s: (off)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_off >= bat_vol)  {
		bat_level = 0;
		dev_info(dev, "%s: (off)level = %d, vol = %d", __func__,
				bat_level, bat_vol);
	}
	dev_dbg(dev, "%s: level = %d\n", __func__, bat_level);

__end__:
	dev_dbg(dev, "%s: bat_vol = %d, level = %d, is_full = %d\n",
			__func__, bat_vol, bat_level, 
			s3c_bat_info.bat_info.batt_is_full);
#ifdef __TEMP_ADC_VALUE__
	return 80;
#else
	return bat_level;
#endif /* __TEMP_ADC_VALUE__ */
}

static int s3c_get_bat_level(struct power_supply *bat_ps)
{
	int bat_level = 0;
	int bat_vol = s3c_read_bat(bat_ps);

	s3c_bat_info.bat_info.batt_vol_adc_aver = bat_vol;

	if(is_over_abs_time()) {
		bat_level = 100;
		s3c_bat_info.bat_info.batt_is_full = 1;
		dev_info(dev, "%s: charging time is over\n", __func__);
		s3c_set_chg_en(DISABLE);
		goto __end__;
	}

#ifdef __BATTERY_COMPENSATION__
	if (s3c_bat_info.bat_info.charging_enabled) {
		if (bat_vol > batt_almost - COMPENSATE_TA) {
			s3c_bat_set_compesation(0, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
		}
	}
#endif /* __BATTERY_COMPENSATION__ */

	if (bat_vol > batt_full) {
		bat_level = 100;
#ifdef __CHECK_CHG_CURRENT__
		if (s3c_bat_info.bat_info.charging_enabled) {
			check_chg_current(bat_ps);
			if (!s3c_bat_info.bat_info.batt_is_full)
				bat_level = 90;
		}
#endif /* __CHECK_CHG_CURRENT__ */
#ifdef __ADJUST_RECHARGE_ADC__
		check_recharging_bat(bat_vol);
#endif /* __ADJUST_RECHARGE_ADC__ */
		dev_dbg(dev, "%s: (full)level = %d\n", __func__, bat_level );
	} else if (batt_full >= bat_vol && bat_vol > batt_almost) {
		int temp = (batt_full - batt_almost) / 3;
		if (bat_vol > (batt_almost + temp * 2))
			bat_level = 90;
		else if (bat_vol > (batt_almost + temp))
			bat_level = 80;
		else
			bat_level = 70;

		if (s3c_bat_info.bat_info.batt_is_recharging)
			bat_level = 100;

		if (s3c_bat_info.bat_info.batt_is_full &&
			!s3c_bat_info.bat_info.charging_enabled) {
			dev_info(dev, "%s: recharging(under full)\n", __func__);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(ENABLE);
			bat_level = 100;
		}
		dev_dbg(dev, "%s: (almost)level = %d\n", __func__, bat_level);
	} else if (batt_almost >= bat_vol && bat_vol > batt_high) {
		bat_level = 50;
		dev_dbg(dev, "%s: (high)level = %d\n", __func__, bat_level );
	} else if (batt_high >= bat_vol && bat_vol > batt_medium) {
		bat_level = 30;
		dev_dbg(dev, "%s: (med)level = %d\n", __func__, bat_level);
	} else if (batt_medium >= bat_vol && bat_vol > batt_low) {
		bat_level = 15;
		dev_dbg(dev, "%s: (low)level = %d\n", __func__, bat_level);
	} else if (batt_low >= bat_vol && bat_vol > batt_critical) {
		bat_level = 5;
		dev_info(dev, "%s: (cri)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_critical >= bat_vol && bat_vol > batt_min) {
		bat_level = 3;
		dev_info(dev, "%s: (min)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_min >= bat_vol && bat_vol > batt_off) {
		bat_level = 1;
		dev_info(dev, "%s: (off)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	} else if (batt_off >= bat_vol)  {
		bat_level = 0;
		dev_info(dev, "%s: (off)level = %d, vol = %d", __func__,
				bat_level, bat_vol);
	}
	dev_dbg(dev, "%s: level = %d\n", __func__, bat_level);

__end__:
	dev_dbg(dev, "%s: bat_vol = %d, level = %d, is_full = %d\n",
			__func__, bat_vol, bat_level, 
			s3c_bat_info.bat_info.batt_is_full);
#ifdef __TEMP_ADC_VALUE__
	return 80;
#else
	return bat_level;
#endif /* __TEMP_ADC_VALUE__ */
}


static int s3c_get_bat_vol(struct power_supply *bat_ps)
{
	int bat_vol = 0;
	int adc = s3c_bat_info.bat_info.batt_vol_adc;
#ifdef __TEST_MODE_INTERFACE__
	int batt_vol_adc_aver = s3c_bat_info.bat_info.batt_vol_adc_aver;
	s3c_bat_info.bat_info.batt_vol_aver = 
		convert_adc2voltage(batt_vol_adc_aver);
#endif /* __TEST_MODE_INTERFACE__ */

	bat_vol = convert_adc2voltage(adc);

	dev_dbg(dev, "%s: adc = %d, bat_vol = %d\n",
			__func__, adc, bat_vol);
	return bat_vol;
}
#if defined(CONFIG_MACH_INFOBOWLQ) && defined __FUEL_GAUGES_IC_MAX17043__
static int s3c_get_bat_level_fuel(struct power_supply *bat_ps)
{
	int fg_soc = -1;
	int fg_vcell = -1;

	int fg_soc_max = 970;
	int fg_soc_min = 8;
//rf cal cmd delay issue 	static int fg_reset_cnt = 0;	
#ifdef log_block
	printk("[Start] %s(), line : %d\n",__FUNCTION__,__LINE__);	
#endif
	if ((fg_soc = fg_read_soc()) < 0) {
		dev_err(dev, "%s: Can't read soc!!!\n", __func__);
#ifdef CONFIG_MACH_INFOBOWLQ		// added by kimjh
		fg_soc = s3c_bat_info.bat_info.level;
#endif
	}
#ifdef CONFIG_MACH_INFOBOWLQ		
       if (fg_soc == -1) //fg_reset -> pwroff
		fg_soc = s3c_bat_info.bat_info.level;
#endif

#ifdef log_block
 	printk("[SOC] %s(), line : %d\n",__FUNCTION__,__LINE__);	
#endif
	if ((fg_vcell = fg_read_vcell()) < 0) {
		dev_err(dev, "%s: Can't read vcell!!!\n", __func__);
		fg_vcell = s3c_bat_info.bat_info.batt_vol;
	} else
		s3c_bat_info.bat_info.batt_vol = fg_vcell;
#ifdef log_block
	printk("[VCELL] %s(), line : %d\n",__FUNCTION__,__LINE__);	
#endif
	if (is_over_abs_time()) {
		fg_soc = 100;
		s3c_bat_info.bat_info.batt_is_full = 1;
		dev_info(dev, "%s: charging time is over\n", __func__);
		s3c_set_chg_en(DISABLE);
		goto __end__;
	}
#ifndef CONFIG_MACH_INFOBOWLQ
	if (fg_soc > 80) {
		fg_soc += fg_soc - 80;
#ifdef log_block
		printk("%s(), fg_soc : %d, line : %d\n",__FUNCTION__, fg_soc, __LINE__);
#endif
		if (fg_soc > 100)
			fg_soc = 100;
	}
#endif	
#ifdef log_block
	printk("%s(), fg_soc : %d, line : %d\n",__FUNCTION__, fg_soc, __LINE__);
#endif
/* soc value compensation algorithm	*/		
#ifdef CONFIG_MACH_INFOBOWLQ		// added by kimjh
	fg_soc = ((fg_soc*10-fg_soc_min)*100)/(fg_soc_max-fg_soc_min);

#ifdef log_block
	dev_printk("<6>",dev,"%s(), fg_soc : %d,vcell : %d, line : %d\n",__FUNCTION__, fg_soc,fg_vcell, __LINE__);
#endif
	if(fg_soc > 100)
		fg_soc = 100;
	
	else if(fg_soc < 0)
		fg_soc = 0;
/* soc value compensation algorithm	*/		
#endif
#ifdef __CHECK_CHG_CURRENT__
	if (fg_vcell >= EX_FULL_CHARGE_COND_VOLTAGE) {
		if (s3c_bat_info.bat_info.charging_enabled) {
			check_chg_current_fuel(bat_ps);
		}
		if (s3c_bat_info.bat_info.batt_is_full) {
			fg_soc = 100;
		}
#if	0	// comment by kimjh
		else 
			fg_soc = 100;		// added by kimjh
#endif
	}
#endif /* __CHECK_CHG_CURRENT__ */

//  Move SOC Complement Code => (bootloader -> Kernle)
#if 0	/* SOC Complement Code*/
	if (fg_vcell >= FG_COMPER_COND_VOLTAGE) {
		if (fg_soc == 0) {
			fg_reset_soc();
			fg_soc = fg_read_soc();
		}
	}
#if 0 //rf cal cmd delay issue def CONFIG_MACH_INFOBOWLQ
/*SOC Compensation Reset command*/			// added by kimjh
	if (first_soc != fg_soc)
	{
		if(((first_soc - fg_soc) > 15) || ((fg_soc - first_soc) < -15))
		{
			printk("%s, line : %d\n",__FUNCTION__,__LINE__);
			if(fg_reset_cnt < 1)
			{
			fg_reset_cnt++;
			printk("fg_reset_cnt2 : %d\n",fg_reset_cnt);
			fg_reset_soc();
			}
		}

	}
#endif
	printk("%s(), fg_soc : %d, line : %d\n",__FUNCTION__, fg_soc, __LINE__);
/*SOC Compensation Reset command*/
#endif
		check_recharging_bat_fuel(fg_vcell);

__end__:
#ifdef log_block
	dev_dbg(dev, "%s: fg_vcell = %d, fg_soc = %d, is_full = %d\n",
			__func__, fg_vcell, fg_soc, 
			s3c_bat_info.bat_info.batt_is_full);
#endif
	return fg_soc;
}
static int s3c_get_bat_vol_fuel(struct power_supply *bat_ps)
{
	return s3c_bat_info.bat_info.batt_vol;
}
#endif


#ifdef __BOARD_REV_ADC__
static void s3c_get_board_rev_adc(struct power_supply *bat_ps)
{
	static unsigned int cnt = 0;
	int adc = 0;

	adc = s3c_bat_get_adc_data(S3C_ADC_BOARD_REV);
	board_rev_adc = calculate_average_adc(S3C_ADC_BOARD_REV, adc);
	dev_dbg(dev, "%s: adc=%d, avg_adc=%d\n", __func__, adc, board_rev_adc);
	if (++cnt >= 10)
		is_end_board_rev_adc = 1;
}
#endif /* __BOARD_REV_ADC__ */

#if (defined __TEST_MODE_INTERFACE__ && defined __BATTERY_V_F__)
static void s3c_get_v_f_adc(void)
{
	s3c_bat_info.bat_info.batt_v_f_adc
		= s3c_bat_get_adc_data(S3C_ADC_V_F);
	dev_info(dev, "%s: vf=%d\n", __func__,
			s3c_bat_info.bat_info.batt_v_f_adc);
}
#endif /* __TEST_MODE_INTERFACE__ && __BATTERY_V_F__ */

static inline int gpio_get_value_ex(unsigned int pin)
{
#ifndef __USE_EGPIO__
	return gpio_get_value(pin);
#else
	return egpio_get_value(pin);
#endif /* __USE_EGPIO__ */
}

static inline void gpio_set_value_ex(unsigned int pin, unsigned int level)
{
#ifndef __USE_EGPIO__
	gpio_set_value(pin, level);
#else
	egpio_set_value(pin, level);
#endif /* __USE_EGPIO__ */
}

static void s3c_set_time_for_charging(int mode) {
	if (mode) {
		/* record start time for abs timer */
		start_time_msec = jiffies_to_msecs(jiffies);
		dev_info(dev, "%s: start_time(%d)\n", __func__,
				start_time_msec);
	} else {
		/* initialize start time for abs timer */
		start_time_msec = 0;
		total_time_msec = 0;
		dev_info(dev, "%s: reset abs timer\n", __func__);
	}
}

static void s3c_set_chg_en(int enable)
{
	int chg_en_val = gpio_get_value_ex(gpio_chg_en);

	if (enable) {
//wall charger wrong detect issue		if (chg_en_val == GPIO_LEVEL_HIGH) {
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_LOW);
//.i [VinsQ] [Start] Charger IC Changed to MAX8922    Pineone hgwoo    2010.01.05
#if defined(CONFIG_MACH_INFOBOWLQ)
			mdelay(5);			// retouch by pineone kimjh 	udelay(2000)->mdelay(5) 	2010.01.07
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_HIGH);
			udelay(10);
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_LOW);
			udelay(2000);
#endif
//.i [VinsQ] [End] Charger IC Changed to MAX8922    Pineone hgwoo    2010.01.05
			dev_printk("<6>",dev, "%s: gpio_chg_en(0) / Charger ON\n", __func__);
			s3c_set_time_for_charging(1);
#ifdef __BATTERY_COMPENSATION__
			s3c_bat_set_compesation(1, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
#endif /* __BATTERY_COMPENSATION__ */
//wall charger wrong detect issue		}
	} else {
//wall charger wrong detect issue		if (chg_en_val == GPIO_LEVEL_LOW) {
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_HIGH);
//.i [VinsQ] [Start] Charger IC Changed to MAX8922    Pineone hgwoo    2010.01.05
#if defined(CONFIG_MACH_INFOBOWLQ)
			udelay(2000);
#endif
//.i [VinsQ] [End] Charger IC Changed to MAX8922    Pineone hgwoo    2010.01.05
			dev_printk("<6>",dev, "%s: gpio_chg_en(1) / Charger OFF\n", __func__);
			s3c_set_time_for_charging(0);
			s3c_bat_info.bat_info.batt_is_recharging = 0;
#ifdef __BATTERY_COMPENSATION__
			s3c_bat_set_compesation(0, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
#endif /* __BATTERY_COMPENSATION__ */
//wall charger wrong detect issue		}
	}
	s3c_bat_info.bat_info.charging_enabled = enable;
}

static void s3c_temp_control(int mode) {
	int adc = s3c_bat_info.bat_info.batt_temp_adc_aver;
	dev_info(dev, "%s: temp_adc=%d\n", __func__, adc);

	switch (mode) {
	case POWER_SUPPLY_HEALTH_GOOD:
		dev_info(dev, "%s: GOOD\n", __func__);
		s3c_set_bat_health(mode);
		break;
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		dev_info(dev, "%s: OVERHEAT\n", __func__);
		s3c_set_bat_health(mode);
		break;
	case POWER_SUPPLY_HEALTH_COLD:
		dev_info(dev, "%s: COLD\n", __func__);
		s3c_set_bat_health(mode);
		break;
	default:
		break;
	}
	schedule_work(&cable_work);
}

static int s3c_get_bat_temp(struct power_supply *bat_ps)
{
#ifdef log_block
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
#endif
	int temp = 0;
	int array_size = 0;
	int i = 0;
	int temp_adc = s3c_read_temp(bat_ps);
	int health = s3c_get_bat_health();
#ifdef __BATTERY_COMPENSATION__
	unsigned int ex_case = 0;
#endif /* __BATTERY_COMPENSATION__ */
#ifdef __TEST_MODE_INTERFACE__
	s3c_bat_info.bat_info.batt_temp_adc_aver = temp_adc;
#endif /* __TEST_MODE_INTERFACE__ */

#ifdef __BATTERY_COMPENSATION__
	ex_case = OFFSET_MP3_PLAY | OFFSET_VOICE_CALL_2G | OFFSET_VOICE_CALL_3G
		| OFFSET_DATA_CALL | OFFSET_VIDEO_PLAY;
	if (s3c_bat_info.device_state & ex_case)
		goto __map_temperature__;
#endif /* __BATTERY_COMPENSATION__ */

	if (s3c_bat_info.bat_info.charging_source == CHARGER_BATTERY) 
		goto __map_temperature__;

#ifndef __REVERSE_TEMPER_ADC__
	if (temp_adc <= TEMP_HIGH_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
	} else if (temp_adc >= TEMP_HIGH_RECOVER &&
			temp_adc <= TEMP_LOW_RECOVER) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				health == POWER_SUPPLY_HEALTH_COLD)
			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
	} else if (temp_adc >= TEMP_LOW_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_COLD);
	}

__map_temperature__:	
	array_size = ARRAY_SIZE(temper_table);
	for (i = 0; i < (array_size - 1); i++) {
		if (i == 0) {
			if (temp_adc >= temper_table[0][0]) {
				temp = temper_table[0][1];
				break;
			} else if (temp_adc <= temper_table[array_size-1][0]) {
				temp = temper_table[array_size-1][1];
				break;
			}
		}

		if (temper_table[i][0] > temp_adc &&
				temper_table[i+1][0] <= temp_adc) {
			temp = temper_table[i+1][1];
		}
	}
#else /* __REVERSE_TEMPER_ADC__ */
	if (temp_adc >= TEMP_HIGH_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
	} else if (temp_adc <= TEMP_HIGH_RECOVER &&
			temp_adc >= TEMP_LOW_RECOVER) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				health == POWER_SUPPLY_HEALTH_COLD)
			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
	} else if (temp_adc <= TEMP_LOW_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_COLD);
	}

__map_temperature__:	
	array_size = ARRAY_SIZE(temper_table);
	for (i = 0; i < (array_size - 1); i++) {
		if (i == 0) {
			if (temp_adc <= temper_table[0][0]) {
				temp = temper_table[0][1];
				break;
			} else if (temp_adc >= temper_table[array_size-1][0]) {
				temp = temper_table[array_size-1][1];
				break;
			}
		}

		if (temper_table[i][0] < temp_adc &&
				temper_table[i+1][0] >= temp_adc) {
			temp = temper_table[i+1][1];
		}
	}

#if 0 //defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	/* Test Temp RCOMP Return*/
	int rcomp_temp = temp;

	if (orig_temp - rcomp_temp > 100 || rcomp_temp - orig_temp < -100 )
	{
		if(rcomp_temp < TEMP_RCOMP)		/* TEMP_RCOMP = 200(20.0C) */
		{
			fg_rcomp_low_temp(rcomp_temp);
		}
		else if (rcomp_temp > TEMP_RCOMP)
		{
			fg_rcomp_high_temp(rcomp_temp);	
		}
	}
#endif
#endif /* __REVERSE_TEMPER_ADC__ */
#ifdef log_block
	printk("%s: temp = %d, adc = %d\n",
			__FUNCTION__, temp, temp_adc);
#endif
#ifdef __TEST_MODE_INTERFACE__
       	s3c_bat_info.bat_info.batt_temp_aver = temp;
#endif /* __TEST_MODE_INTERFACE__ */
	return temp;
}

#if	0	/* Battery Event Temperature Block disable */ 	// added by kimjh
static int s3c_bat_event_temp_status(void)
{
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
	int event_health = s3c_get_bat_health();
	unsigned int event_case = 0;

	event_case = OFFSET_MP3_PLAY | OFFSET_VOICE_CALL_2G | OFFSET_VOICE_CALL_3G
		| OFFSET_DATA_CALL | OFFSET_VIDEO_PLAY;

	if (s3c_bat_info.device_state & event_case)
	{
		printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
		if(event_health == POWER_SUPPLY_HEALTH_OVERHEAT 
				|| event_health == POWER_SUPPLY_HEALTH_COLD)
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD);
	}
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
}
#endif

static int s3c_bat_get_charging_status(void)
{
        charger_type_t charger = CHARGER_BATTERY; 
        int ret = 0;
        
        charger = s3c_bat_info.bat_info.charging_source;
        
        switch (charger) {
        case CHARGER_BATTERY:
                ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
                break;
        case CHARGER_USB:
			if(s3c_bat_info.bat_info.batt_slate_mode){
			charger = CHARGER_BATTERY;
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		}
        case CHARGER_AC:
		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD)
			ret = POWER_SUPPLY_STATUS_DISCHARGING;
		else {
			if (s3c_bat_info.bat_info.batt_is_full)
				ret = POWER_SUPPLY_STATUS_FULL;
			else
				ret = POWER_SUPPLY_STATUS_CHARGING;
		}
                break;
	case CHARGER_DISCHARGE:
		ret = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
        default:
                ret = POWER_SUPPLY_STATUS_UNKNOWN;
        }

	dev_dbg(dev, "%s: %s\n", __func__, status_text[ret]);
        return ret;
}

static int s3c_bat_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	dev_dbg(bat_ps->dev, "%s: psp = %d\n", __func__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s3c_bat_get_charging_status();
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s3c_get_bat_health();
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s3c_bat_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = s3c_bat_info.bat_info.level;
		dev_dbg(dev, "%s: level = %d\n", __func__, 
				val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = s3c_bat_info.bat_info.batt_temp;
		dev_dbg(bat_ps->dev, "%s: temp = %d\n", __func__, 
				val->intval);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s3c_power_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp, 
		union power_supply_propval *val)
{
	charger_type_t charger;
	
	dev_dbg(bat_ps->dev, "%s: psp = %d\n", __func__, psp);

	charger = s3c_bat_info.bat_info.charging_source;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
			val->intval = (charger == CHARGER_AC ? 1 : 0);
		else if (bat_ps->type == POWER_SUPPLY_TYPE_USB){
			if(s3c_bat_info.bat_info.batt_slate_mode){
			    charger = CHARGER_BATTERY;			
	        }
			val->intval = (charger == CHARGER_USB ? 1 : 0);
		}else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

#define SEC_BATTERY_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_bat_show_property,							\
        .store = s3c_bat_store,								\
}

static struct device_attribute s3c_battery_attrs[] = {
        SEC_BATTERY_ATTR(batt_vol),
        SEC_BATTERY_ATTR(batt_vol_adc),
        SEC_BATTERY_ATTR(batt_vol_adc_cal),
        SEC_BATTERY_ATTR(batt_temp),
        SEC_BATTERY_ATTR(batt_temp_adc),
        SEC_BATTERY_ATTR(batt_temp_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
#ifdef __BOARD_REV_ADC__
        SEC_BATTERY_ATTR(board_rev_adc),
#endif /* __BOARD_REV_ADC__ */
#ifdef __TEST_MODE_INTERFACE__
	/* test mode */
	SEC_BATTERY_ATTR(batt_test_mode),
	/* average */
	SEC_BATTERY_ATTR(batt_vol_aver),
	SEC_BATTERY_ATTR(batt_temp_aver),
	SEC_BATTERY_ATTR(batt_temp_adc_aver),
	SEC_BATTERY_ATTR(batt_v_f_adc),
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	SEC_BATTERY_ATTR(batt_chg_current),
	SEC_BATTERY_ATTR(batt_chg_current_aver),
#endif /* __CHECK_CHG_CURRENT__ */
	SEC_BATTERY_ATTR(charging_source),
#ifdef __BATTERY_COMPENSATION__
	SEC_BATTERY_ATTR(vibrator),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(mp3),
	SEC_BATTERY_ATTR(svc),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(device_state),
	SEC_BATTERY_ATTR(batt_compensation),
	SEC_BATTERY_ATTR(is_booting),
#endif /* __BATTERY_COMPENSATION__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	SEC_BATTERY_ATTR(fg_soc),
	SEC_BATTERY_ATTR(reset_soc),
	SEC_BATTERY_ATTR(full_notify),
	SEC_BATTERY_ATTR(chg_temp_cut_off_on),	
#endif /* __FUEL_GAUGES_IC__ */
    SEC_BATTERY_ATTR(batt_slate_mode),
};

enum {
        BATT_VOL = 0,
        BATT_VOL_ADC,
        BATT_VOL_ADC_CAL,
        BATT_TEMP,
        BATT_TEMP_ADC,
        BATT_TEMP_ADC_CAL,
	BATT_VOL_ADC_AVER,
#ifdef __BOARD_REV_ADC__
        BOARD_REV_ADC,
#endif /* __BOARD_REV_ADC__ */
#ifdef __TEST_MODE_INTERFACE__
	BATT_TEST_MODE,
	BATT_VOL_AVER,
	BATT_TEMP_AVER,
	BATT_TEMP_ADC_AVER,
	BATT_V_F_ADC,
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	BATT_CHG_CURRENT,	
	BATT_CHG_CURRENT_AVER,	
#endif /* __CHECK_CHG_CURRENT__ */
	BATT_CHARGING_SOURCE,
#ifdef __BATTERY_COMPENSATION__
	BATT_VIBRATOR,
	BATT_CAMERA,
	BATT_MP3,
	BATT_SVC,
	BATT_VIDEO,
	BATT_VOICE_CALL_2G,
	BATT_VOICE_CALL_3G,
	BATT_DATA_CALL,
	BATT_DEV_STATE,
	BATT_COMPENSATION,
	BATT_BOOTING,
#endif /* __BATTERY_COMPENSATION__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	BATT_FG_SOC,
	BATT_RESET_SOC,
	BATT_FULL_NOTIFY,
	BATT_CHG_CUT_OFF_ON,
#endif /* __FUEL_GAUGES_IC__ */
    BATT_SLATE_MODE,
};

static int s3c_bat_create_attrs(struct device * dev)
{
        int i, rc;
        
        for (i = 0; i < ARRAY_SIZE(s3c_battery_attrs); i++) {
                rc = device_create_file(dev, &s3c_battery_attrs[i]);
                if (rc)
                        goto s3c_attrs_failed;
        }
        goto succeed;
        
s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_battery_attrs[i]);
succeed:        
        return rc;
}

#ifdef CONFIG_MACH_INFOBOWLQ
#ifdef __CHECK_CHG_CURRENT__
int s3c_ichg_average(void)
{
	int adc = 0;
	adc = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);

	return calculate_average_adc(S3C_ADC_CHG_CURRENT, adc);
}
#endif
#endif
static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol);
                break;
        case BATT_VOL_ADC:
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
				s3c_bat_info.bat_info.batt_vol_adc = 0;
#else
				s3c_bat_info.bat_info.batt_vol_adc = s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);
#endif
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc);
                break;
        case BATT_VOL_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc_cal);
                break;
        case BATT_TEMP:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp);
                break;
        case BATT_TEMP_ADC:
		s3c_bat_info.bat_info.batt_temp_adc = 
			s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc);
                break;	
        case BATT_SVC:
        		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_svc); 
        		break;        
#ifdef __TEST_MODE_INTERFACE__
	case BATT_TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		s3c_bat_info.bat_info.batt_test_mode);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
        case BATT_TEMP_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc_cal);
                break;
        case BATT_VOL_ADC_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_vol_adc_aver);
		break;
#ifdef __BOARD_REV_ADC__
        case BOARD_REV_ADC:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", board_rev_adc);
                break;
#endif /* __BOARD_REV_ADC__ */
#ifdef __TEST_MODE_INTERFACE__
	case BATT_VOL_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_vol_aver);
		break;
	case BATT_TEMP_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_temp_aver);
		break;
	case BATT_TEMP_ADC_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_temp_adc_aver);
		break;
	case BATT_V_F_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_v_f_adc);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	case BATT_CHG_CURRENT:
		s3c_bat_info.bat_info.batt_current = 
			s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				s3c_bat_info.bat_info.batt_current);
		break;
	case BATT_CHG_CURRENT_AVER:
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				s3c_ichg_average());
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				adc_sample[S3C_ADC_CHG_CURRENT].average_adc);
#endif
		break;

#endif /* __CHECK_CHG_CURRENT__ */
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.charging_source);
		break;
#ifdef __BATTERY_COMPENSATION__
	case BATT_DEV_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%08x\n",
			s3c_bat_info.device_state);
		break;
	case BATT_COMPENSATION:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			batt_compensation);
		break;
#endif /* __BATTERY_COMPENSATION__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	case BATT_FG_SOC:
#ifdef 	__FUEL_GAUGES_IC_MAX17043__
#ifdef log_block
		printk("%s(), s3c_bat_info.bat_info.level : %d, line : %d\n",__FUNCTION__, s3c_bat_info.bat_info.level, __LINE__);
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",	s3c_bat_info.bat_info.level);
	
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			fg_read_soc());
#endif
		break;

	case BATT_FULL_NOTIFY:
#ifdef _DEBUG_	//just for debug
			printk("%s(), s3c_bat_info.bat_info.batt_temp_full_notify : %d, line : %d\n",__FUNCTION__, s3c_bat_info.bat_info.batt_temp_full_notify, __LINE__);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",	s3c_bat_info.bat_info.batt_temp_full_notify);
#else
#ifdef log_block
			printk("%s(), s3c_bat_info.bat_info.batt_is_full : %d, line : %d\n",__FUNCTION__, s3c_bat_info.bat_info.batt_is_full, __LINE__);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",	s3c_bat_info.bat_info.batt_is_full);
#endif
			break;	

		case BATT_CHG_CUT_OFF_ON:
#ifdef log_block
			printk("%s(), s3c_bat_info.bat_info.batt_temp_chg_cut_off_on : %d, line : %d\n",__FUNCTION__, s3c_bat_info.bat_info.batt_temp_chg_cut_off_on, __LINE__);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",	s3c_bat_info.bat_info.batt_temp_chg_cut_off_on);		
		 break;
#endif /* __FUEL_GAUGES_IC__ */
		case BATT_SLATE_MODE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_slate_mode);
			break;

        default:
                i = -EINVAL;
        }       
        
        return i;
}

static void s3c_bat_set_vol_cal(int batt_cal)
{
	int max_cal = 4096;
#ifdef __9BITS_RESOLUTION__
	max_cal = 512;
#endif /* __9BITS_RESOLUTION__ */

	if (!batt_cal)
		return;

	if (batt_cal >= max_cal) {
		dev_err(dev, "%s: invalid battery_cal(%d)\n", __func__, batt_cal);
		return;
	}

	batt_max = batt_cal + BATT_MAXIMUM;
	batt_full = batt_cal + BATT_FULL;
	batt_safe_rech = batt_cal + BATT_SAFE_RECHARGE;
	batt_almost = batt_cal + BATT_ALMOST_FULL;
	batt_high = batt_cal + BATT_HIGH;
	batt_medium = batt_cal + BATT_MED;
	batt_low = batt_cal + BATT_LOW;
	batt_critical = batt_cal + BATT_CRITICAL;
	batt_min = batt_cal + BATT_MINIMUM;
	batt_off = batt_cal + BATT_OFF;
}

#ifdef COMPENSATE_BOOTING
static void s3c_bat_compensate_booting(int mode)
{
	if (mode)
		s3c_bat_set_compesation(1, OFFSET_BOOTING, COMPENSATE_BOOTING);
	else
		queue_work(batt_drv_wqueue, &compensate_boot_work);
}
#endif /* COMPENSATE_BOOTING */

static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_vol_adc_cal = x;
			s3c_bat_set_vol_cal(x);
			ret = count;
		}
		dev_info(dev, "%s: batt_vol_adc_cal = %d\n", __func__, x);
                break;
        case BATT_TEMP_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_temp_adc_cal = x;
			ret = count;
		}
		dev_info(dev, "%s: batt_temp_adc_cal = %d\n", __func__, x);
                break;
#ifdef __TEST_MODE_INTERFACE__
	case BATT_TEST_MODE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_test_mode = x;
			ret = count;
		}
		if (s3c_bat_info.bat_info.batt_test_mode) {
			s3c_bat_info.polling_interval = POLLING_INTERVAL_TEST;			
			if (s3c_bat_info.polling) {
				del_timer_sync(&polling_timer);
				mod_timer(&polling_timer, jiffies + 
					msecs_to_jiffies(s3c_bat_info.polling_interval));
			}
			s3c_bat_status_update(
				&s3c_power_supplies_test[CHARGER_BATTERY]);
		} else {
			s3c_bat_info.polling_interval = POLLING_INTERVAL;		
			if (s3c_bat_info.polling) {
				del_timer_sync(&polling_timer);
				mod_timer(&polling_timer,jiffies + 
					msecs_to_jiffies(s3c_bat_info.polling_interval));
			}
			s3c_bat_status_update(
				&s3c_power_supplies_test[CHARGER_BATTERY]);
		}
		dev_info(dev, "%s: batt_test_mode = %d\n", __func__, x);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __BATTERY_COMPENSATION__
	case BATT_VIBRATOR:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VIBRATOR_ON,
					COMPENSATE_VIBRATOR);
			ret = count;
		}
		dev_info(dev, "%s: vibrator = %d\n", __func__, x);
                break;
	case BATT_CAMERA:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_CAMERA_ON,
					COMPENSATE_CAMERA);
			ret = count;
		}
		dev_info(dev, "%s: camera = %d\n", __func__, x);
                break;
	case BATT_MP3:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_MP3_PLAY,
					COMPENSATE_MP3);
			ret = count;
		}
		dev_info(dev, "%s: mp3 = %d\n", __func__, x);
                break;
	case BATT_SVC:
		if (sscanf(buf, "%d\n", &x) == 1) 
		{
			if (x == 1) 
		    {
#ifdef log_block
				printk(" BATT_SVC ====> 1");
#endif
				s3c_bat_info.bat_info.batt_svc = 1;			
			}
			else if (x == 0)
			{
#ifdef log_block
				printk(" BATT_SVC ====> 0");
#endif
				s3c_bat_info.bat_info.batt_svc = 0;	
			}
			else
			{
#ifdef log_block
				printk("BATT_SVC ====>Error ==> %s -- %x",buf,x);
#endif
			} 
		}
		else
		{
#ifdef log_block
			printk("BATT_SVC ====>Error ==> %s -- %x -- %d",buf,x,count);
#endif
		}		
#ifdef log_block
		dev_info(dev, "%s: svc = %d\n", __func__, x);
#endif
			break;
	case BATT_VIDEO:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VIDEO_PLAY,
					COMPENSATE_VIDEO);
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: video = %d\n", __func__, x);
#endif
                break;
	case BATT_VOICE_CALL_2G:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_2G,
					COMPENSATE_VOICE_CALL_2G);
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: voice call 2G = %d\n", __func__, x);
#endif
                break;
	case BATT_VOICE_CALL_3G:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_3G,
					COMPENSATE_VOICE_CALL_3G);
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: voice call 3G = %d\n", __func__, x);
#endif
                break;
	case BATT_DATA_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_DATA_CALL,
					COMPENSATE_DATA_CALL);
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: data call = %d\n", __func__, x);
#endif
                break;
#ifdef COMPENSATE_BOOTING
	case BATT_BOOTING:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_compensate_booting(x);
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: boot complete = %d\n", __func__, x);
#endif
                break;
#endif /* COMPENSATE_BOOTING */
#endif /* __BATTERY_COMPENSATION__ */
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	case BATT_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
							 is_batt_test=1;
#endif			
			if (x == 1)
				{
				wake_lock_timeout(&rf_cal_wake_lock, 4*HZ); 		// rf cal wake lock enable.
				fg_reset_soc();
				}
			ret = count;
		}
#ifdef log_block
		dev_info(dev, "%s: Reset SOC:%d\n", __func__, x);
#endif
		break;
#endif /* __FUEL_GAUGES_IC__ */
		case BATT_SLATE_MODE:
			if (sscanf(buf, "%d\n", &x) == 1) {
				s3c_bat_info.bat_info.batt_slate_mode = x;
				ret = count;
			}
				dev_info(dev, "%s: batt_slate_mode = %d\n", __func__, x);
			break;

        default:
                ret = -EINVAL;
        }       

	return ret;
}

#ifdef __BATTERY_COMPENSATION__
void s3c_bat_set_compensation_for_drv(int mode, int offset)
{
	switch(offset) {
	case OFFSET_VIBRATOR_ON:
		dev_dbg(dev, "%s: vibrator = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_VIBRATOR);
		break;
	case OFFSET_LCD_ON:
		dev_info(dev, "%s: LCD On = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_LCD);
		break;
	case OFFSET_CAM_FLASH:
		dev_info(dev, "%s: flash = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_CAM_FALSH);
		break;
	default:
		break;
	}

}
EXPORT_SYMBOL(s3c_bat_set_compensation_for_drv);
#endif /* __BATTERY_COMPENSATION__ */

#ifdef __TEST_DEVICE_DRIVER__
#define SEC_TEST_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_test_show_property,							\
        .store = s3c_test_store,							\
}

static struct device_attribute s3c_test_attrs[] = {
        SEC_TEST_ATTR(pm),
        SEC_TEST_ATTR(usb),
        SEC_TEST_ATTR(bt_wl),
        SEC_TEST_ATTR(tflash),
        SEC_TEST_ATTR(audio),
        SEC_TEST_ATTR(lcd),
        SEC_TEST_ATTR(suspend_lock),
        SEC_TEST_ATTR(control_tmp),
};

enum {
        TEST_PM = 0,
	USB_OFF,
	BT_WL_OFF,
	TFLASH_OFF,
	AUDIO_OFF,
	LCD_CHECK,
	SUSPEND_LOCK,
	CTRL_TMP,
};

static int s3c_test_create_attrs(struct device * dev)
{
        int i, rc;
        
        for (i = 0; i < ARRAY_SIZE(s3c_test_attrs); i++) {
                rc = device_create_file(dev, &s3c_test_attrs[i]);
                if (rc)
                        goto s3c_attrs_failed;
        }
        goto succeed;
        
s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_test_attrs[i]);
succeed:        
        return rc;
}

static void s3c_lcd_check(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO6, &reg_buff)) {
		pr_info("%s: VLCD 1.8V (%d)\n", __func__, reg_buff);
	}
	if ((Get_MAX8698_PM_REG(ELDO7, &reg_buff))) {
		pr_info("%s: VLCD 2.8V (%d)\n", __func__, reg_buff);
	}
}

static void s3c_usb_off(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO3, &reg_buff)) {
		pr_info("%s: OTGI 1.2V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO3, 0);
	}
	if ((Get_MAX8698_PM_REG(ELDO8, &reg_buff))) {
		pr_info("%s: OTG 3.3V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO8, 0);
	}
}

static void s3c_bt_wl_off(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO4, &reg_buff)) {
		pr_info("%s: BT_WL 2.6V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO4, 0);
	}
}

static void s3c_tflash_off(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: TF 3.0V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
}

static void s3c_audio_off(void)
{
	pr_info("%s: Turn off audio power, amp\n", __func__);
	amp_enable(0);
	audio_power(0);
}

static void s3c_test_pm(void)
{
	/* PMIC */
	s3c_usb_off();
	s3c_bt_wl_off();
	s3c_tflash_off();
	s3c_lcd_check();

	/* AUDIO */
	s3c_audio_off();

	/* GPIO */
}

static ssize_t s3c_test_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_test_attrs;

        switch (off) {
        case TEST_PM:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		s3c_test_pm();
                break;
        case USB_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 1);
		s3c_usb_off();
                break;
        case BT_WL_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 2);
		s3c_bt_wl_off();
                break;
        case TFLASH_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 3);
		s3c_tflash_off();
                break;
        case AUDIO_OFF:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 4);
		s3c_audio_off();
                break;
        case LCD_CHECK:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 5);
		s3c_lcd_check();
                break;
        default:
                i = -EINVAL;
        }       
        
        return i;
}

static ssize_t s3c_test_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int mode = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_test_attrs;

        switch (off) {
        case SUSPEND_LOCK:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			dev_dbg(dev, "%s: suspend_lock(%d)\n", __func__, mode);
			if (mode) 
                		wake_lock(&wake_lock_for_dev);
			else
                		wake_lock_timeout(
						&wake_lock_for_dev, HZ / 2);
			ret = count;
		}
                break;
	case CTRL_TMP:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			dev_info(dev, "%s: control tmp(%d)\n", __func__, mode);
			bat_temper_state = mode;
			ret = count;
		}
		break;
        default:
                ret = -EINVAL;
        }       

	return ret;
}
#endif /* __TEST_DEVICE_DRIVER__ */

static enum power_supply_property s3c_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply s3c_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = s3c_battery_properties,
		.num_properties = ARRAY_SIZE(s3c_battery_properties),
		.get_property = s3c_bat_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
};
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	int s3c_bat_temp_read(void)		// added by kimjh
	{
		return s3c_get_bat_temp(&s3c_power_supplies[CHARGER_BATTERY]);
}
int s3c_bat_soc_read(void)		// added by kimjh
{
		int fg_soc = -1;
		if ((fg_soc = fg_read_soc()) < 0) {
			dev_err(dev, "%s: Can't read soc!!!\n", __func__);
			fg_soc = s3c_bat_info.bat_info.level;
		}
		   if (fg_soc == -1) //fg_reset -> pwroff
			fg_soc = s3c_bat_info.bat_info.level;

      force_update = 1;
	return s3c_bat_info.bat_info.level = fg_soc;
}	
#endif
static int s3c_cable_status_update(int status)
{
#ifdef log_block
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
#endif
	int ret = 0;
	charger_type_t source = CHARGER_BATTERY;

	dev_dbg(dev, "%s\n", __func__);

	if(!s3c_battery_initial)
		return -EPERM;

	switch(status) {
	case CHARGER_BATTERY:
		dev_dbg(dev, "%s: cable NOT PRESENT\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		dev_dbg(dev, "%s: cable USB\n", __func__);
		if(s3c_bat_info.bat_info.batt_slate_mode){
			s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;			
		}else{
		s3c_bat_info.bat_info.charging_source = CHARGER_USB;
		}
		break;
	case CHARGER_AC:
		dev_dbg(dev, "%s: cable AC\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_AC;
		break;
#if 0
	case CHARGER_DISCHARGE:
		dev_dbg(dev, "%s: Discharge\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_DISCHARGE;
		break;
#endif
	default:
		dev_err(dev, "%s: Nat supported status(%d)\n", __func__, status);
		ret = -EINVAL;
	}
	source = s3c_bat_info.bat_info.charging_source;

        if (source == CHARGER_USB || source == CHARGER_AC) {
                wake_lock(&vbus_wake_lock);
        } else {
                /* give userspace some time to see the uevent and update
                 * LED state or whatnot...
                 */
		if (gpio_get_value(gpio_ta_connected)) 
			wake_lock_timeout(&vbus_wake_lock, HZ / 2);
        }
        /* if the power source changes, all power supplies may change state */
        power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);
	/*
        power_supply_changed(&s3c_power_supplies[CHARGER_USB]);
        power_supply_changed(&s3c_power_supplies[CHARGER_AC]);
	*/
	dev_dbg(dev, "%s: call power_supply_changed\n", __func__);
	return ret;
}

#ifdef __CHECK_BATTERY_V_F__
static unsigned int s3c_bat_check_v_f(void)
{
	unsigned int rc = 0;
	int adc = 0;
	
	adc = s3c_bat_get_adc_data(S3C_ADC_V_F);
#ifdef CONFIG_MACH_INFOBOWLQ	
	s3c_bat_info.bat_info.batt_v_f_adc = adc;
#endif
	dev_dbg(dev, "%s: V_F ADC = %d\n", __func__, adc);

	if (adc <= BATT_VF_MAX && adc >= BATT_VF_MIN) {
		/* s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD); */
		rc = 1;
	} else {
		dev_info(dev, "%s: Unauthorized battery!\n", __func__);
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
		s3c_set_chg_en(DISABLE);
		force_update = 1;
		rc = 0;
	}
	return rc;
}
#endif /* __CHECK_BATTERY_V_F__ */

static void s3c_bat_status_update(struct power_supply *bat_ps)
{
#ifdef log_block
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
#endif
	int old_level, old_temp, old_is_full;
	dev_dbg(dev, "%s ++\n", __func__);

	if(!s3c_battery_initial)
		return;

	mutex_lock(&work_lock);
	old_temp = s3c_bat_info.bat_info.batt_temp;
	old_level = s3c_bat_info.bat_info.level; 
	old_is_full = s3c_bat_info.bat_info.batt_is_full;
	s3c_bat_info.bat_info.batt_temp = s3c_get_bat_temp(bat_ps); 
#if 0
	s3c_bat_event_temp_status();		// added by kimjh
#endif
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	first_soc = fg_read_soc();		// added by kimjh
      s3c_bat_info.bat_info.level = s3c_get_bat_level_fuel(bat_ps);
#else
#ifdef __ANDROID_BAT_LEVEL_CONCEPT__
	s3c_bat_info.bat_info.level = s3c_get_bat_level_concept(bat_ps);
#else
	s3c_bat_info.bat_info.level = s3c_get_bat_level(bat_ps);
#endif
#endif

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
if(!is_batt_test)
{
#endif
	if (!s3c_bat_info.bat_info.charging_enabled &&
			!s3c_bat_info.bat_info.batt_is_full) {
		if (s3c_bat_info.bat_info.level > old_level)
			s3c_bat_info.bat_info.level = old_level;
	}
	
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
}
	if(is_batt_test) is_batt_test=0;
#endif

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
      s3c_bat_info.bat_info.batt_vol = s3c_get_bat_vol_fuel(bat_ps); 
#else
      s3c_bat_info.bat_info.batt_vol = s3c_get_bat_vol(bat_ps);
#endif

#ifdef __BOARD_REV_ADC__
	if (!is_end_board_rev_adc)
		s3c_get_board_rev_adc(bat_ps);
#endif /* __BOARD_REV_ADC__ */

#if (defined __TEST_MODE_INTERFACE__ && defined __BATTERY_V_F__)
	if (s3c_bat_info.bat_info.batt_test_mode == 1)
		s3c_get_v_f_adc();
#endif /* __TEST_MODE_INTERFACE__ && __BATTERY_V_F__ */

#ifdef __CHECK_BATTERY_V_F__
	if(s3c_bat_info.bat_info.charging_enabled &&
		s3c_get_bat_health() != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
		s3c_bat_check_v_f();
#endif /* __CHECK_BATTERY_V_F__ */

	if (old_level != s3c_bat_info.bat_info.level 
			|| old_temp != s3c_bat_info.bat_info.batt_temp
			|| old_is_full != s3c_bat_info.bat_info.batt_is_full
			|| force_update) {
		force_update = 0;
		power_supply_changed(bat_ps);
		dev_dbg(dev, "%s: call power_supply_changed\n", __func__);
	}

	mutex_unlock(&work_lock);
	dev_dbg(dev, "%s --\n", __func__);
}

static void s3c_cable_check_status(void)
{
#ifdef log_block
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
#endif
	charger_type_t status = 0;

	mutex_lock(&work_lock);

	if (!gpio_get_value(gpio_ta_connected)) {
		if (get_usb_power_state())
			status = CHARGER_USB;
		else
			status = CHARGER_AC;

		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
			dev_info(dev, "%s: Unhealth battery state!\n", __func__);
			s3c_set_chg_en(DISABLE);
		    } else {
		        if((status == CHARGER_USB)&&(s3c_bat_info.bat_info.batt_slate_mode)){
			        status = CHARGER_BATTERY;
			        s3c_set_chg_en(DISABLE);
		} else 
			s3c_set_chg_en(ENABLE);
		    }

		dev_dbg(dev, "%s: status : %s\n", __func__, 
				(status == CHARGER_USB) ? "USB" : "AC");
	} else {
		u32 health = s3c_get_bat_health();

		status = CHARGER_BATTERY;
		s3c_set_chg_en(DISABLE);

		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				health == POWER_SUPPLY_HEALTH_COLD) {
			s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD);
		}
	}
	dev_dbg(dev, "%s: gpio_chg_en %s\n", __func__, 
			gpio_get_value_ex(gpio_chg_en)?"disabled":"enabled");

	s3c_cable_status_update(status);
	mutex_unlock(&work_lock);
}

static void s3c_bat_work(struct work_struct *work)
{
#ifdef __ADJUST_RECHARGE_ADC__
	static int pre_rechar = 0;
	static int cnt = 0;
	if (s3c_bat_info.bat_info.batt_is_full == 1 && full_charge_flag == 1) {
		batt_recharging = -1;
		pre_rechar = 1;
		full_charge_flag = 0;
	}

	if (pre_rechar) {
		if (++cnt > 10) {
			int adc = s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);
			batt_recharging = adc - BATT_RECHARGE_CODE;
			dev_info(dev, "%s: batt_recharging=%d\n", __func__,
					batt_recharging);
			pre_rechar = 0;
			cnt = 0;
		}
	}
#endif /* __ADJUST_RECHARGE_ADC__ */

	dev_dbg(dev, "%s\n", __func__);

	s3c_bat_status_update(
			&s3c_power_supplies[CHARGER_BATTERY]);
}

static void s3c_cable_work(struct work_struct *work)
{
#ifdef log_block
	printk("%s(), line : %d\n",__FUNCTION__,__LINE__);
	dev_dbg(dev, "%s\n", __func__);
#endif
	s3c_cable_check_status();
}

#ifdef COMPENSATE_BOOTING
static void s3c_compensate_boot_work(struct work_struct *work)
{
	dev_info(dev, "%s ++\n", __func__);
	msleep(50000); /* 50sec */
	s3c_bat_set_compesation(0, OFFSET_BOOTING, COMPENSATE_BOOTING);
	dev_info(dev, "%s --\n", __func__);
}
#endif /* COMPENSATE_BOOTING */

#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev, 
		pm_message_t state)
{
	dev_info(dev, "%s\n", __func__);

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

	flush_scheduled_work();
	disable_irq(IRQ_TA_CONNECTED_N);
	disable_irq(IRQ_TA_CHG_N);
	return 0;
}

static int s3c_bat_resume(struct platform_device *pdev)
{
	dev_info(dev, "%s\n", __func__);
	wake_lock(&vbus_wake_lock);
//	wake_lock_timeout(&rf_cal_wake_lock, 4*HZ);			// rf cal wake lock enable.
	enable_irq(IRQ_TA_CONNECTED_N);
	enable_irq(IRQ_TA_CHG_N);
	schedule_work(&bat_work);
	schedule_work(&cable_work);

	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	return 0;
}
#else
#define s3c_bat_suspend NULL
#define s3c_bat_resume NULL
#endif /* CONFIG_PM */

static void polling_timer_func(unsigned long unused)
{
	dev_dbg(dev, "%s\n", __func__);
	schedule_work(&bat_work);

	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

static void cable_timer_func(unsigned long unused)
{
	dev_info(dev, "%s : intr cnt = %d\n", __func__, cable_intr_cnt);
	cable_intr_cnt = 0;
	schedule_work(&cable_work);
}

static irqreturn_t s3c_cable_changed_isr(int irq, void *power_supply)
{
	dev_dbg(dev, "%s: irq=0x%x, gpio_ta_connected=%x\n", __func__, irq,
			gpio_get_value(gpio_ta_connected));

	if (!s3c_battery_initial)
		return IRQ_HANDLED;

	if(gpio_get_value(gpio_ta_connected)) {
		s3c_bat_info.bat_info.batt_is_full = 0;
	}
#ifdef __ADJUST_RECHARGE_ADC__
	batt_recharging = -1;
#endif /* __ADJUST_RECHARGE_ADC__ */
#ifdef __CHECK_CHG_CURRENT__
	clear_adc_sample(S3C_ADC_CHG_CURRENT);
#endif /* __CHECK_CHG_CURRENT__ */

	cable_intr_cnt++;
	if (timer_pending(&cable_timer))
		del_timer(&cable_timer);

	cable_timer.expires = jiffies + msecs_to_jiffies(50);
	add_timer(&cable_timer);

	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return IRQ_HANDLED;
}

static irqreturn_t s3c_cable_charging_isr(int irq, void *power_supply)
{
	int chg_ing = gpio_get_value(gpio_chg_ing);
	dev_dbg(dev, "%s: irq=0x%x, gpio_chg_ing=%d\n", __func__, irq, chg_ing);

	if (!s3c_battery_initial)
		return IRQ_HANDLED;
#ifndef __DISABLE_CHG_ING_INTR__
	if (chg_ing && !gpio_get_value(gpio_ta_connected) &&
			s3c_bat_info.bat_info.charging_enabled &&
			s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD) {
		s3c_set_chg_en(DISABLE);
		s3c_bat_info.bat_info.batt_is_full = 1;
		force_update = 1;
		full_charge_flag = 1;
	}

	schedule_work(&bat_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
#endif /* __DISABLE_CHG_ING_INTR__ */

	return IRQ_HANDLED;
}

static int __devinit s3c_bat_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;

	dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);

	s3c_bat_info.present = 1;
	s3c_bat_info.polling = 1;
	s3c_bat_info.polling_interval = POLLING_INTERVAL;
	s3c_bat_info.device_state = 0;

	s3c_bat_info.bat_info.batt_vol_adc_aver = 0;
#ifdef __TEST_MODE_INTERFACE__
	s3c_bat_info.bat_info.batt_vol_aver = 0;
	s3c_bat_info.bat_info.batt_temp_aver = 0;
	s3c_bat_info.bat_info.batt_temp_adc_aver = 0;
	s3c_bat_info.bat_info.batt_v_f_adc = 0;

	s3c_bat_info.bat_info.batt_test_mode = 0;
 	s3c_power_supplies_test = s3c_power_supplies;
#endif /* __TEST_MODE_INTERFACE__ */
	s3c_bat_info.bat_info.batt_id = 0;
	s3c_bat_info.bat_info.batt_vol = 0;
	s3c_bat_info.bat_info.batt_vol_adc = 0;
	s3c_bat_info.bat_info.batt_vol_adc_cal = 0;
	s3c_bat_info.bat_info.batt_temp = 0;
	s3c_bat_info.bat_info.batt_temp_adc = 0;
	s3c_bat_info.bat_info.batt_temp_adc_cal = 0;
	s3c_bat_info.bat_info.batt_current = 0;
	s3c_bat_info.bat_info.level = 100;
	s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
	s3c_bat_info.bat_info.charging_enabled = 0;
	s3c_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
	s3c_bat_info.bat_info.batt_slate_mode = 0;

	memset(adc_sample, 0x00, sizeof adc_sample);

	batt_max = BATT_CAL + BATT_MAXIMUM;
	batt_full = BATT_CAL + BATT_FULL;
	batt_safe_rech = BATT_CAL + BATT_SAFE_RECHARGE;
	batt_almost = BATT_CAL + BATT_ALMOST_FULL;
	batt_high = BATT_CAL + BATT_HIGH;
	batt_medium = BATT_CAL + BATT_MED;
	batt_low = BATT_CAL + BATT_LOW;
	batt_critical = BATT_CAL + BATT_CRITICAL;
	batt_min = BATT_CAL + BATT_MINIMUM;
	batt_off = BATT_CAL + BATT_OFF;
#ifdef __ADJUST_RECHARGE_ADC__
	batt_recharging = -1;
#endif /* __ADJUST_RECHARGE_ADC__ */

#ifdef __BATTERY_COMPENSATION__
	batt_compensation = 0;
#ifdef COMPENSATE_BOOTING
	s3c_bat_set_compesation(1, OFFSET_BOOTING, COMPENSATE_BOOTING);
	s3c_bat_set_compesation(1, OFFSET_LCD_ON, COMPENSATE_LCD);
#endif /* COMPENSATE_BOOTING */
#endif /* __BATTERY_COMPENSATION__ */

	INIT_WORK(&bat_work, s3c_bat_work);
	INIT_WORK(&cable_work, s3c_cable_work);

#ifdef COMPENSATE_BOOTING
	INIT_WORK(&compensate_boot_work, s3c_compensate_boot_work);
	batt_drv_wqueue = create_singlethread_workqueue("batt_drv_workqueue");
#endif /* COMPENSATE_BOOTING */

#ifdef __ALWAYS_AWAKE_DEVICE__
	dev_info(dev, "%s: always awake(wake_lock)\n", __func__);
	wake_lock(&wake_lock_for_dev);
#endif /* __ALWAYS_AWAKE_DEVICE__ */

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		ret = power_supply_register(&pdev->dev, 
				&s3c_power_supplies[i]);
		if (ret) {
			dev_err(dev, "Failed to register"
					"power supply %d,%d\n", i, ret);
			goto __end__;
		}
	}

	/* create sec detail attributes */
	s3c_bat_create_attrs(s3c_power_supplies[CHARGER_BATTERY].dev);

#ifdef __TEST_DEVICE_DRIVER__
	s3c_test_create_attrs(s3c_power_supplies[CHARGER_AC].dev);
#endif /* __TEST_DEVICE_DRIVER__ */

	/* Request IRQ */ 
	set_irq_type(IRQ_TA_CONNECTED_N, IRQ_TYPE_EDGE_BOTH);
	ret = request_irq(IRQ_TA_CONNECTED_N, s3c_cable_changed_isr,
			  IRQF_DISABLED,
			  DRIVER_NAME,
			  &s3c_power_supplies[CHARGER_BATTERY]);
	if(ret)
		goto __end__;

	set_irq_type(IRQ_TA_CHG_N, IRQ_TYPE_EDGE_BOTH);
	ret = request_irq(IRQ_TA_CHG_N, s3c_cable_charging_isr,
			  IRQF_DISABLED,
			  DRIVER_NAME,
			  &s3c_power_supplies[CHARGER_BATTERY]);

	if (ret)
		goto __ta_connected_irq_failed__;

	if (s3c_bat_info.polling) {
		dev_dbg(dev, "%s: will poll for status\n", 
				__func__);
		setup_timer(&polling_timer, polling_timer_func, 0);
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	}

	setup_timer(&cable_timer, cable_timer_func, 0);

	s3c_battery_initial = 1;
	force_update = 0;
	full_charge_flag = 0;


#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
		{
			if (i2c_add_driver(&fg_i2c_driver))
			pr_err("%s: Can't add fg i2c drv\n", __func__);
		
			struct i2c_client *client = fg_i2c_client;
			u8 data[2];

			if (fg_i2c_read(client, RCOMP0_REG, &data[0]) < 0) {
				pr_err("%s: Failed to read rcomp0\n", __func__);
				return -1;
			}
			if (fg_i2c_read(client, RCOMP1_REG, &data[1]) < 0) {
				pr_err("%s: Failed to read rcomp1\n", __func__);
				return -1;
			}
#ifdef log_block
			printk("%s: RCOMP [0]= 0x%02x [1]=0x%02x\n", __func__, data[0], data[1]);
#endif		
			fg_rcomp_init();

			if (fg_i2c_read(client, RCOMP0_REG, &data[0]) < 0) {
				pr_err("%s: Failed to read rcomp0\n", __func__);
				return -1;
			}
			if (fg_i2c_read(client, RCOMP1_REG, &data[1]) < 0) {
				pr_err("%s: Failed to read rcomp1\n", __func__);
				return -1;
			}
#ifdef log_block
			printk("%s: RCOMP [0]=0x%02x [1]=0x%02x\n", __func__, data[0], data[1]);
#endif		
            }
#endif

	s3c_bat_status_update(
			&s3c_power_supplies[CHARGER_BATTERY]);

#ifdef __CHECK_BATTERY_V_F__
#ifdef __CHECK_BOARD_REV__
       //HYH_20100526	if (system_rev >= rev_to_check)
#endif /* __CHECK_BOARD_REV__ */
		s3c_bat_check_v_f();
#endif /* __CHECK_BATTERY_V_F__ */
	s3c_cable_check_status();

#ifdef log_block	
	printk("func : %s, rev%02x\n",__FUNCTION__, system_rev);
#endif
__end__:
	return ret;
__ta_connected_irq_failed__:
	free_irq(IRQ_TA_CONNECTED_N, 
			&s3c_power_supplies[CHARGER_BATTERY]);
	return ret;
}

static int __devexit s3c_bat_remove(struct platform_device *pdev)
{
	int i;
	dev_info(dev, "%s\n", __func__);

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

	free_irq(IRQ_TA_CONNECTED_N, 
			&s3c_power_supplies[CHARGER_BATTERY]);
	free_irq(IRQ_TA_CHG_N, &s3c_power_supplies[CHARGER_BATTERY]);

	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		power_supply_unregister(&s3c_power_supplies[i]);
	}
 
	return 0;
}

static struct platform_driver s3c_bat_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= s3c_bat_probe,
	.remove		= __devexit_p(s3c_bat_remove),
	.suspend	= s3c_bat_suspend,
	.resume		= s3c_bat_resume,
};

/* Initailize GPIO */
static void s3c_bat_init_hw(void)
{
	s3c_gpio_cfgpin(gpio_ta_connected, 
			S3C_GPIO_SFN(gpio_ta_connected_af));
	s3c_gpio_setpull(gpio_ta_connected, S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(gpio_chg_ing, 
			S3C_GPIO_SFN(gpio_chg_ing_af));
	s3c_gpio_setpull(gpio_chg_ing, S3C_GPIO_PULL_UP);
#ifndef __USE_EGPIO__
	s3c_gpio_cfgpin(gpio_chg_en, 
			S3C_GPIO_SFN(gpio_chg_en_af));
	s3c_gpio_setpull(gpio_chg_en, S3C_GPIO_PULL_NONE); 
#endif /* __USE_EGPIO__ */
	gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_HIGH);
}

static int __init s3c_bat_init(void)
{
	pr_info("%s\n", __func__);
	s3c_bat_init_hw();

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	wake_lock_init(&rf_cal_wake_lock, WAKE_LOCK_SUSPEND, "rf_present");
#if (defined __TEST_DEVICE_DRIVER__  || defined __ALWAYS_AWAKE_DEVICE__)
	wake_lock_init(&wake_lock_for_dev, WAKE_LOCK_SUSPEND, "wake_lock_dev");
#endif /* __TEST_DEVICE_DRIVER__ || __ALWAYS_AWAKE_DEVICE__ */

#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
#ifndef CONFIG_MACH_INFOBOWLQ
	if (i2c_add_driver(&fg_i2c_driver))
		pr_err("%s: Can't add fg i2c drv\n", __func__);
#endif	
#endif /* __FUEL_GAUGES_IC__ */
	return platform_driver_register(&s3c_bat_driver);
}

static void __exit s3c_bat_exit(void)
{
	pr_info("%s\n", __func__);
#if defined(__FUEL_GAUGES_IC__) || defined(__FUEL_GAUGES_IC_MAX17043__)
	i2c_del_driver(&fg_i2c_driver);
#endif /* __FUEL_GAUGES_IC__ */
	platform_driver_unregister(&s3c_bat_driver);
}

module_init(s3c_bat_init);
module_exit(s3c_bat_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");
