/*
 * linux/drivers/power/s3c6410_battery.h
 *
 * Battery measurement code for S3C6410 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"saturn-battery"

/*
 * Saturn Rev02 board Battery Table
 */
#define BATT_CAL		2472	/* 3.65V */

#define BATT_MAXIMUM		379	/* 4.20V */
#define BATT_FULL		310	/* 4.10V */
#define BATT_SAFE_RECHARGE	310	/* 4.10V */
#define BATT_ALMOST_FULL	248	/* 4.01V */
#define BATT_HIGH		131	/* 3.84V */
#define BATT_MED		69	/* 3.75V */
#define BATT_LOW		41	/* 3.71V */
#define BATT_CRITICAL		0	/* 3.65V */
#define BATT_MINIMUM		(-42)	/* 3.59V */
#define BATT_OFF		(-173)	/* 3.40V */

/*
 * Saturn Rev02 board Temperature Table
 */

#define NUM_TEMP_TBL	69

const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{	1640 	,	-50	},
	{	1626 	,	-40	},
	{	1612 	,	-30	},
	{	1598 	,	-20	},
	{	1584 	,	-10	},
	{	1570 	,	0	},
	{	1545	,	10	},
	{	1520	,	20	},
	{	1495	,	30	},
	{	1470	,	40	},
	{	1445	,	50	},
	{	1420	,	60	},
	{	1395	,	70	},
	{	1370	,	80	},
	{	1345	,	90	},
	{	1321	,	100 },
	{	1296	,	110 },
	{	1271	,	120 },
	{	1246	,	130 },
	{	1221	,	140 },
	{	1196	,	150 },
	{	1171	,	160 },
	{	1146	,	170 },
	{	1121	,	180 },
	{	1096	,	190 },
	{	1071	,	200 },
	{	1046	,	210 },
	{	1021	,	220 },
	{	996 	,	230 },
	{	971 	,	240 },
	{	946 	,	250 },
	{	921 	,	260 },
	{	896 	,	270 },
	{	871 	,	280 },
	{	846 	,	290 },
	{	822 	,	300 },
	{	797 	,	310 },
	{	772 	,	320 },
	{	747 	,	330 },
	{	722 	,	340 },
	{	697 	,	350 },
	{	672 	,	360 },
	{	647 	,	370 },
	{	622 	,	380 },
	{	597 	,	390 },
	{	572 	,	400	},
	{	561 	,	410	},
	{	549 	,	420 },
	{	538 	,	430	},
	{	526 	,	440 },
	{	515 	,	450 },
	{	503 	,	460 },
	{	492 	,	470 },
	{	480 	,	480 },
	{	469 	,	490 },
	{	457 	,	500 },
	{	446 	,	510 },
	{	434 	,	520 },
	{	423 	,	530 },
	{	411 	,	540 },
	{	400 	,	550 },
	{	388 	,	560 },
	{	377 	,	570 },
	{	365 	,	580	},
	{	357 	,	590	},
	{	349 	,	600	},
	{	341 	,	610	},
	{	333 	,	620	},
	{	325 	,	630	},
};

#define TEMP_HIGH_BLOCK		temper_table[NUM_TEMP_TBL-1][0]
#define TEMP_HIGH_RECOVER	temper_table[NUM_TEMP_TBL-6][0]
#define TEMP_LOW_BLOCK		temper_table[0][0]
#define TEMP_LOW_RECOVER	temper_table[5][0]

/*
 * Saturn Rev02 board ADC channel
 */
typedef enum s3c_adc_channel {
	S3C_ADC_VOLTAGE = 0,
	S3C_ADC_TEMPERATURE,
	S3C_ADC_V_F,
	S3C_ADC_EAR,
	S3C_ADC_CHG_CURRENT,
	ENDOFADC
} adc_channel_type;

#define IRQ_TA_CONNECTED_N	IRQ_EINT(19)
#define IRQ_TA_CHG_N		IRQ_EINT(25)

/*
 * Saturn GPIO for battery driver
 */
const unsigned int gpio_ta_connected	= GPIO_TA_nCONNECTED;
const unsigned int gpio_ta_connected_af	= GPIO_TA_nCONNECTED_AF;
const unsigned int gpio_chg_ing		= GPIO_TA_nCHG;
const unsigned int gpio_chg_ing_af	= GPIO_TA_nCHG_AF;
const unsigned int gpio_chg_en		= GPIO_TA_EN;
const unsigned int gpio_chg_en_af	= GPIO_TA_EN_AF;

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
/* #define __TEMP_ADC_VALUE__ */
/* #define __USE_EGPIO__ */
#define __CHECK_BATTERY_V_F__
#define __BATTERY_V_F__ 
/* #define __BATTERY_COMPENSATION__ */
/* #define __CHECK_BOARD_REV__ */
/* #define __BOARD_REV_ADC__ */
#define __TEST_DEVICE_DRIVER__
/* #define __ALWAYS_AWAKE_DEVICE__ */
#define __TEST_MODE_INTERFACE__
/* #define __CHECK_CHG_CURRENT__ */
/* #define __ADJUST_RECHARGE_ADC__ */
#if CONFIG_FUEL_GAUGE_MAX17040 // for Saturn
#define __FUEL_GAUGES_IC__  
#endif 
#define __AVG_TEMP_ADC__
/*****************************************************************************/

#define TOTAL_CHARGING_TIME	(5*60*60*1000)	/* 5 hours */
#define TOTAL_RECHARGING_TIME	(2*60*60*1000)	/* 2 hours */

#ifdef __CHECK_BATTERY_V_F__
#define BATT_VF_MAX		100
#define BATT_VF_MIN		0
#endif /* __CHECK_BATTERY_V_F__ */

#ifdef __CHECK_CHG_CURRENT__
#define CURRENT_OF_FULL_CHG		65
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __ADJUST_RECHARGE_ADC__
#define BATT_RECHARGE_CODE		50	/* 0.05V */
#endif /* __ADJUST_RECHARGE_ADC__ */

#ifdef __FUEL_GAUGES_IC__
#define SOC_LB_FOR_POWER_OFF		27

#define FULL_CHARGE_COND_VOLTAGE	4000
#define RECHARGE_COND_VOLTAGE		4080
#endif /* __FUEL_GAUGES_IC__ */

