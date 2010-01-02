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

#ifdef CONFIG_MACH_INSTINCTQ
#define DRIVER_NAME	"instinctq-battery"

/*
 * InstinctQ Rev01 board Battery Table
 */
#define BATT_CAL		296	/* 3.51V */

#define BATT_MAXIMUM		59	/* 4.19V */
#define BATT_FULL		46	/* 4.04V */
#define BATT_SAFE_RECHARGE	43	/* 4.00V */
#define BATT_ALMOST_FULL	34	/* 3.90V */
#define BATT_HIGH		24	/* 3.79V */
#define BATT_MED		20	/* 3.74V */
#define BATT_LOW		15	/* 3.68V */
#define BATT_CRITICAL		10	/* 3.63V */
#define BATT_MINIMUM		(-2)	/* 3.49V */
#define BATT_OFF		(-8)	/* 3.40V */

/*
 * InstinctQ Rev01 board Temperature Table
 */
const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 235,		-50	},	/* Low block	*/
	{ 242,		0	},	/* Low recover	*/
	{ 300,		100	},
	{ 320,		200	},
	{ 340,		300	},
	{ 372,		350	},
	{ 399,		430	},	/* High recover	*/
	{ 410,		440	},
	{ 440,		450	},	/* High block	*/
};

#define TEMP_HIGH_BLOCK		temper_table[8][0]
#define TEMP_HIGH_RECOVER	temper_table[6][0]
#define TEMP_LOW_BLOCK		temper_table[0][0]
#define TEMP_LOW_RECOVER	temper_table[1][0]

/*
 * InstinctQ Rev00 board ADC channel
 */
typedef enum s3c_adc_channel {
	S3C_ADC_VOLTAGE = 0,
	S3C_ADC_TEMPERATURE,
	S3C_ADC_CHG_CURRENT,
	S3C_ADC_EAR,
	S3C_ADC_V_F,
	S3C_ADC_SSENS,
	S3C_ADC_NC6,
	S3C_ADC_BOARD_REV,
	ENDOFADC
} adc_channel_type;

#define IRQ_TA_CONNECTED_N	IRQ_EINT(19)
#define IRQ_TA_CHG_N		IRQ_EINT(25)

/*
 * InstinctQ GPIO for battery driver
 */
const unsigned int gpio_ta_connected	= GPIO_TA_CONNECTED_N;
const unsigned int gpio_ta_connected_af	= GPIO_TA_CONNECTED_N_AF;
const unsigned int gpio_chg_ing		= GPIO_CHG_ING_N;
const unsigned int gpio_chg_ing_af	= GPIO_CHG_ING_N_AF;
const unsigned int gpio_chg_en		= GPIO_CHG_EN;
const unsigned int gpio_chg_en_af	= GPIO_CHG_EN_AF;

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
/* #define __TEMP_ADC_VALUE__ */
/* #define __USE_EGPIO__ */
#define __CHECK_BATTERY_V_F__
#define __BATTERY_COMPENSATION__
#define __CHECK_BOARD_REV__
#define __BOARD_REV_ADC__
#define __TEST_DEVICE_DRIVER__
/* #define __ALWAYS_AWAKE_DEVICE__ */
#define __TEST_MODE_INTERFACE__
#define __CHECK_CHG_CURRENT__
#define __ADJUST_RECHARGE_ADC__
#define __9BITS_RESOLUTION__
#define __REVERSE_TEMPER_ADC__
#define __ANDROID_BAT_LEVEL_CONCEPT__
/*****************************************************************************/

#define TOTAL_CHARGING_TIME	(5*60*60*1000)	/* 5 hours */
#define TOTAL_RECHARGING_TIME	(2*60*60*1000)	/* 2 hours */

#ifdef __CHECK_BATTERY_V_F__
#define BATT_VF_MAX		28
#define BATT_VF_MIN		12
#endif /* __CHECK_BATTERY_V_F__ */

#ifdef __CHECK_BOARD_REV__
const unsigned int rev_to_check	= 0x10;		/* 0x10 : rev01 */
#endif /* __CHECK_BOARD_REV__ */

#ifdef __CHECK_CHG_CURRENT__
#define CURRENT_OF_FULL_CHG	73
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __ADJUST_RECHARGE_ADC__
#define BATT_RECHARGE_CODE	3		/* 0.03V */
#endif /* __ADJUST_RECHARGE_ADC__ */

#ifdef __BATTERY_COMPENSATION__
#define COMPENSATE_VIBRATOR		2
#define COMPENSATE_CAMERA		2
#define COMPENSATE_MP3			2
#define COMPENSATE_VIDEO		1
#define COMPENSATE_VOICE_CALL_2G	3
#define COMPENSATE_VOICE_CALL_3G	3
#define COMPENSATE_DATA_CALL		3
#define COMPENSATE_LCD			3
#define COMPENSATE_TA			(-9)
#define COMPENSATE_CAM_FALSH		3
#define COMPENSATE_BOOTING		9
#endif /* __BATTERY_COMPENSATION__ */

#elif defined(CONFIG_MACH_SPICA)

#define DRIVER_NAME	"spica-battery"

/*
 * Spica Rev00 board Battery Table
 */
#define BATT_CAL		2447	/* 3.60V */

#define BATT_MAXIMUM		406	/* 4.176V */
#define BATT_FULL		353	/* 4.10V  */
#define BATT_SAFE_RECHARGE 	353	/* 4.10V */
#define BATT_ALMOST_FULL	188 	/* 3.8641V */	//322	/* 4.066V */
#define BATT_HIGH		112 	/* 3.7554V */ 	//221	/* 3.919V */
#define BATT_MED		66 	/* 3.6907V */ 	//146	/* 3.811V */
#define BATT_LOW		43 	/* 3.6566V */	//112	/* 3.763V */
#define BATT_CRITICAL		8 	/* 3.6037V */ 	//(74)	/* 3.707V */
#define BATT_MINIMUM		(-28) 	/* 3.554V */	//(38)	/* 3.655V */
#define BATT_OFF		(-128) 	/* 3.4029V */	//(-103)	/* 3.45V  */

/*
 * Spica Rev00 board Temperature Table
 */
const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 1632,		-50	},
	{ 1616,		-40	},
	{ 1599,		-30	},
	{ 1583,		-20	},
	{ 1566,		-10	},
	{ 1550,		0	},
	{ 1530,		10	},
	{ 1510,		20	},
	{ 1490,		30	},
	{ 1470,		40	},
	{ 1450,		50	},
	{ 1428,		60	},
	{ 1406,		70	},
	{ 1384,		80	},
	{ 1362,		90	},
	{ 1340,		100	},
	{ 1317,		110	},
	{ 1294,		120	},
	{ 1270,		130	},
	{ 1247,		140	},
	{ 1224,		150	},
	{ 1202,		160	},
	{ 1179,		170	},
	{ 1157,		180	},
	{ 1134,		190	},
	{ 1112,		200	},
	{ 1088,		210	},
	{ 1065,		220	},
	{ 1041,		230	},
	{ 1018,		240	},
	{ 994,		250	},
	{ 972,		260	},
	{ 951,		270	},
	{ 929,		280	},
	{ 908,		290	},
	{ 886,		300	},
	{ 866,		310	},
	{ 845,		320	},
	{ 825,		330	},
	{ 804,		340	},
	{ 784,		350	},
	{ 764,		360	},
	{ 744,		370	},
	{ 724,		380	},
	{ 704,		390	},
	{ 684,		400	},
	{ 666,		410	},
	{ 648,		420	},
	{ 629,		430	},
	{ 611,		440	},
	{ 593,		450	},
	{ 577,		460	},
	{ 560,		470	},
	{ 544,		480	},
	{ 527,		490	},
	{ 511,		500	},
	{ 497,		510	},
	{ 482,		520	},
	{ 468,		530	},
	{ 453,		540	},
	{ 439,		550	},
	{ 426,		560	},
	{ 414,		570	},
	{ 401,		580	},
	{ 388,		590	},
	{ 375,		600	},
	{ 364,		610	},
	{ 354,		620	},
	{ 343,		630	},
	{ 334,		640	},
	{ 324,		650	},
};

#define TEMP_HIGH_BLOCK		temper_table[68][0]
#define TEMP_HIGH_RECOVER	temper_table[63][0]
#define TEMP_LOW_BLOCK		temper_table[0][0]
#define TEMP_LOW_RECOVER	temper_table[5][0]
 
/*
 * Spica Rev00 board ADC channel
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
 * Spica GPIO for battery driver
 */
 const unsigned int gpio_ta_connected	= GPIO_TA_CONNECTED_N;
 const unsigned int gpio_ta_connected_af	= GPIO_TA_CONNECTED_N_AF;
 const unsigned int gpio_chg_ing		= GPIO_TA_CHG_N;
 const unsigned int gpio_chg_ing_af	= GPIO_TA_CHG_N_AF;
 const unsigned int gpio_chg_en		= GPIO_TA_EN;
 const unsigned int gpio_chg_en_af	= GPIO_TA_EN_AF;

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
/* #define __TEMP_ADC_VALUE__ */
/* #define __USE_EGPIO__ */
/* #define __CHECK_BATTERY_V_F__ */
#define __BATTERY_V_F__
#define __BATTERY_COMPENSATION__
/* #define __CHECK_BOARD_REV__ */
/* #define __BOARD_REV_ADC__ */
#define __TEST_DEVICE_DRIVER__
/* #define __ALWAYS_AWAKE_DEVICE__ */
#define __TEST_MODE_INTERFACE__
/*****************************************************************************/

#define TOTAL_CHARGING_TIME	(6*60*60*1000)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	(2*60*60*1000)	/* 2 hours */

#ifdef __BATTERY_COMPENSATION__
#define COMPENSATE_VIBRATOR		19
#define COMPENSATE_CAMERA		25
#define COMPENSATE_MP3			17
#define COMPENSATE_VIDEO		28
#define COMPENSATE_VOICE_CALL_2G	13
#define COMPENSATE_VOICE_CALL_3G	14
#define COMPENSATE_DATA_CALL		25
#define COMPENSATE_LCD			0
#define COMPENSATE_TA			0
#define COMPENSATE_CAM_FALSH		0
#define COMPENSATE_BOOTING		52
#endif /* __BATTERY_COMPENSATION__ */

#endif // CONFIG_MACH_SPICA

