#ifndef __BMA020_H__
#define __BMA020_H__

#include <linux/earlysuspend.h>

/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "[BMA020] %s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif 
/*******************************************************************************/

/* don't use SPI interface */
#define BMA020_SPI_RD_MASK 0x80   /* for spi read transactions on SPI the MSB has to be set */


/* They used to connect i2c_acc_bma020_write. */
#define BMA020_WR_FUNC_PTR char (* bma020_bus_write)(unsigned char, unsigned char *)

#define BMA020_BUS_WRITE_FUNC(dev_addr, reg_addr, reg_data, len)\
          bma020_bus_write(reg_addr, reg_data)


/* They used to connect i2c_acc_bma020_read. */
#define BMA020_RD_FUNC_PTR char (* bma020_bus_read)( unsigned char, unsigned char *, unsigned int)

#define BMA020_BUS_READ_FUNC( dev_addr, reg_addr, reg_data, r_len )\
           bma020_bus_read( reg_addr , reg_data, r_len )


/* BMA150 IOCTL */
#define BMA150_IOC_MAGIC 				'B'
#define BMA150_CALIBRATE				_IOW(BMA150_IOC_MAGIC,2, unsigned char)
#define BMA150_SET_RANGE            	_IOWR(BMA150_IOC_MAGIC,4, unsigned char)
#define BMA150_SET_MODE             	_IOWR(BMA150_IOC_MAGIC,6, unsigned char)
#define BMA150_SET_BANDWIDTH            _IOWR(BMA150_IOC_MAGIC,8, unsigned char)
#define BMA150_READ_ACCEL_XYZ           _IOWR(BMA150_IOC_MAGIC,46,short)
#define BMA150_IOC_MAXNR            	48

#define DEBUG							0

/* BMA020 I2C Address */

#define BMA020_I2C_ADDR		0x38


/* BMA020 API error codes */

#define E_BMA020_NULL_PTR		(char)-127
#define E_COMM_RES		    (char)-1
#define E_OUT_OF_RANGE		(char)-2



/* register definitions */

#define BMA020_EEP_OFFSET   0x20
#define BMA020_IMAGE_BASE	0x0b
#define BMA020_IMAGE_LEN	19

#define CHIP_ID_REG			0x00
#define VERSION_REG			0x01
#define X_AXIS_LSB_REG		0x02
#define X_AXIS_MSB_REG		0x03
#define Y_AXIS_LSB_REG		0x04
#define Y_AXIS_MSB_REG		0x05
#define Z_AXIS_LSB_REG		0x06
#define Z_AXIS_MSB_REG		0x07
//#define TEMP_RD_REG			0x08
#define BMA020_STATUS_REG	0x09
#define BMA020_CTRL_REG		0x0a
#define BMA020_CONF1_REG	0x0b
#define LG_THRESHOLD_REG	0x0c
#define LG_DURATION_REG		0x0d
#define HG_THRESHOLD_REG	0x0e
#define HG_DURATION_REG		0x0f
#define MOTION_THRS_REG		0x10
#define HYSTERESIS_REG		0x11
#define CUSTOMER1_REG		0x12
#define CUSTOMER2_REG		0x13
#define RANGE_BWIDTH_REG	0x14
#define BMA020_CONF2_REG	0x15

#if 0
#define OFFS_GAIN_X_REG		0x16
#define OFFS_GAIN_Y_REG		0x17
#define OFFS_GAIN_Z_REG		0x18
#define OFFS_GAIN_T_REG		0x19
#define OFFSET_X_REG		0x1a
#define OFFSET_Y_REG		0x1b
#define OFFSET_Z_REG		0x1c
#define OFFSET_T_REG		0x1d
#endif

/* register write and read delays */

#define MDELAY_DATA_TYPE	unsigned int
#define BMA020_EE_W_DELAY 28	/* delay after EEP write is 28 msec */


typedef struct  {
		short x, /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
			  y, /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
			  z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
} bma020acc_t;


typedef struct  {
		unsigned char	
		bma020_conf1 ,  /**<  image address 0x0b: interrupt enable bits, low-g settings */
		lg_threshold,	/**<  image address 0x0c: low-g threshold, depends on selected g-range */
		lg_duration,	/**<  image address 0x0d: low-g duration in ms */
		hg_threshold,	/**<  image address 0x0e: high-g threshold, depends on selected g-range */
		hg_duration,	/**<  image address 0x0f: high-g duration in ms */
		motion_thrs,	/**<  image address 0x10: any motion threshold */
		hysteresis,		/**<  image address 0x11: low-g and high-g hysteresis register */
		customer1,		/**<  image address 0x12: customer reserved register 1 */
		customer2,		/**<  image address 0x13: customer reserved register 2  */
		range_bwidth,	/**<  image address 0x14: range and bandwidth selection register */
		bma020_conf2;	/**<  image address 0x15: spi4, latched interrupt, auto-wake-up configuration */
#if 0
		offs_gain_x,	/**<  image address 0x16: offset_x LSB and x-axis gain settings */
		offs_gain_y,	/**<  image address 0x17: offset_y LSB and y-axis gain settings */
		offs_gain_z,	/**<  image address 0x18: offset_z LSB and z-axis gain settings */
		offs_gain_t,	/**<  image address 0x19: offset_t LSB and temperature gain settings */
		offset_x,		/**<  image address 0x1a: offset_x calibration MSB register */
		offset_y,		/**<  image address 0x1b: offset_y calibration MSB register */ 
		offset_z,		/**<  image address 0x1c: offset_z calibration MSB register */ 
		offset_t;		/**<  image address 0x1d: temperature calibration MSB register */
#endif
} bma020regs_t;



typedef struct {	
	bma020regs_t * image;		/**< pointer to bma020regs_t structure not mandatory */
	unsigned char mode;			/**< save current BMA020 operation mode */
	unsigned char chip_id,		/**< save BMA020's chip id which has to be 0x02 after calling bma020_init() */
				  ml_version, 	/**< holds the BMA020 ML_version number */	
				  al_version;	/**< holds the BMA020 AL_version number */
	unsigned char dev_addr;   	/**< initializes BMA020's I2C device address 0x38 */
	unsigned char int_mask;	  	/**< stores the current BMA020 API generated interrupt mask */
	BMA020_WR_FUNC_PTR;		  	/**< function pointer to the SPI/I2C write function */
	BMA020_RD_FUNC_PTR;		  	/**< function pointer to the SPI/I2C read function */
	void (*delay_msec)( MDELAY_DATA_TYPE ); /**< function pointer to a pause in mili seconds function */
	struct early_suspend early_suspend;		/**< suspend function */
} bma020_t;




	
/* bit slice positions in registers */

/* cond BITSLICE */

#define CHIP_ID__POS		0
#define CHIP_ID__MSK		0x07
#define CHIP_ID__LEN		3
#define CHIP_ID__REG		CHIP_ID_REG


#define ML_VERSION__POS		0
#define ML_VERSION__LEN		4
#define ML_VERSION__MSK		0x0F
#define ML_VERSION__REG		VERSION_REG



#define AL_VERSION__POS  	4
#define AL_VERSION__LEN  	4
#define AL_VERSION__MSK		0xF0
#define AL_VERSION__REG		VERSION_REG



/* DATA REGISTERS */

#define NEW_DATA_X__POS  	0
#define NEW_DATA_X__LEN  	1
#define NEW_DATA_X__MSK  	0x01
#define NEW_DATA_X__REG		X_AXIS_LSB_REG

#define ACC_X_LSB__POS   	6
#define ACC_X_LSB__LEN   	2
#define ACC_X_LSB__MSK		0xC0
#define ACC_X_LSB__REG		X_AXIS_LSB_REG

#define ACC_X_MSB__POS   	0
#define ACC_X_MSB__LEN   	8
#define ACC_X_MSB__MSK		0xFF
#define ACC_X_MSB__REG		X_AXIS_MSB_REG

#define NEW_DATA_Y__POS  	0
#define NEW_DATA_Y__LEN  	1
#define NEW_DATA_Y__MSK  	0x01
#define NEW_DATA_Y__REG		Y_AXIS_LSB_REG

#define ACC_Y_LSB__POS   	6
#define ACC_Y_LSB__LEN   	2
#define ACC_Y_LSB__MSK   	0xC0
#define ACC_Y_LSB__REG		Y_AXIS_LSB_REG

#define ACC_Y_MSB__POS   	0
#define ACC_Y_MSB__LEN   	8
#define ACC_Y_MSB__MSK   	0xFF
#define ACC_Y_MSB__REG		Y_AXIS_MSB_REG

#define NEW_DATA_Z__POS  	0
#define NEW_DATA_Z__LEN  	1
#define NEW_DATA_Z__MSK		0x01
#define NEW_DATA_Z__REG		Z_AXIS_LSB_REG

#define ACC_Z_LSB__POS   	6
#define ACC_Z_LSB__LEN   	2
#define ACC_Z_LSB__MSK		0xC0
#define ACC_Z_LSB__REG		Z_AXIS_LSB_REG

#define ACC_Z_MSB__POS   	0
#define ACC_Z_MSB__LEN   	8
#define ACC_Z_MSB__MSK		0xFF
#define ACC_Z_MSB__REG		Z_AXIS_MSB_REG

#if 0
#define TEMPERATURE__POS 	0
#define TEMPERATURE__LEN 	8
#define TEMPERATURE__MSK 	0xFF
#define TEMPERATURE__REG	TEMP_RD_REG
#endif



/* STATUS BITS */

#define STATUS_HG__POS		0
#define STATUS_HG__LEN		1
#define STATUS_HG__MSK		0x01
#define STATUS_HG__REG		BMA020_STATUS_REG

#define STATUS_LG__POS		1
#define STATUS_LG__LEN		1
#define STATUS_LG__MSK		0x02
#define STATUS_LG__REG		BMA020_STATUS_REG

#define HG_LATCHED__POS  	2
#define HG_LATCHED__LEN  	1
#define HG_LATCHED__MSK		0x04
#define HG_LATCHED__REG		BMA020_STATUS_REG

#define LG_LATCHED__POS		3
#define LG_LATCHED__LEN		1
#define LG_LATCHED__MSK		8
#define LG_LATCHED__REG		BMA020_STATUS_REG

#define ALERT_PHASE__POS	4
#define ALERT_PHASE__LEN	1
#define ALERT_PHASE__MSK	0x10
#define ALERT_PHASE__REG	BMA020_STATUS_REG


#define ST_RESULT__POS		7
#define ST_RESULT__LEN		1
#define ST_RESULT__MSK		0x80
#define ST_RESULT__REG		BMA020_STATUS_REG


/* CONTROL BITS */

#define SLEEP__POS			0
#define SLEEP__LEN			1
#define SLEEP__MSK			0x01
#define SLEEP__REG			BMA020_CTRL_REG

#define SOFT_RESET__POS		1
#define SOFT_RESET__LEN		1
#define SOFT_RESET__MSK		0x02
#define SOFT_RESET__REG		BMA020_CTRL_REG



#define SELF_TEST__POS		2
#define SELF_TEST__LEN		2
#define SELF_TEST__MSK		0x0C
#define SELF_TEST__REG		BMA020_CTRL_REG



#define SELF_TEST0__POS		2
#define SELF_TEST0__LEN		1
#define SELF_TEST0__MSK		0x04
#define SELF_TEST0__REG		BMA020_CTRL_REG

#define SELF_TEST1__POS		3
#define SELF_TEST1__LEN		1
#define SELF_TEST1__MSK		0x08
#define SELF_TEST1__REG		BMA020_CTRL_REG


#if 0
#define EE_W__POS			4
#define EE_W__LEN			1
#define EE_W__MSK			0x10
#define EE_W__REG			BMA020_CTRL_REG

#define UPDATE_IMAGE__POS	5
#define UPDATE_IMAGE__LEN	1
#define UPDATE_IMAGE__MSK	0x20
#define UPDATE_IMAGE__REG	BMA020_CTRL_REG
#endif

#define RESET_INT__POS		6
#define RESET_INT__LEN		1
#define RESET_INT__MSK		0x40
#define RESET_INT__REG		BMA020_CTRL_REG



/* LOW-G, HIGH-G settings */


#define ENABLE_LG__POS		0
#define ENABLE_LG__LEN		1
#define ENABLE_LG__MSK		0x01
#define ENABLE_LG__REG		BMA020_CONF1_REG


#define ENABLE_HG__POS		1
#define ENABLE_HG__LEN		1
#define ENABLE_HG__MSK		0x02
#define ENABLE_HG__REG		BMA020_CONF1_REG


/* LG/HG counter */
	

#define COUNTER_LG__POS			2
#define COUNTER_LG__LEN			2
#define COUNTER_LG__MSK			0x0C
#define COUNTER_LG__REG			BMA020_CONF1_REG
	
#define COUNTER_HG__POS			4
#define COUNTER_HG__LEN			2
#define COUNTER_HG__MSK			0x30
#define COUNTER_HG__REG			BMA020_CONF1_REG



/* LG/HG duration is in ms */

#define LG_DUR__POS			0
#define LG_DUR__LEN			8
#define LG_DUR__MSK			0xFF
#define LG_DUR__REG			LG_DURATION_REG

#define HG_DUR__POS			0
#define HG_DUR__LEN			8
#define HG_DUR__MSK			0xFF
#define HG_DUR__REG			HG_DURATION_REG

			
#define LG_THRES__POS		0
#define LG_THRES__LEN		8
#define LG_THRES__MSK		0xFF
#define LG_THRES__REG		LG_THRESHOLD_REG


#define HG_THRES__POS		0
#define HG_THRES__LEN		8
#define HG_THRES__MSK		0xFF
#define HG_THRES__REG		HG_THRESHOLD_REG


#define LG_HYST__POS			0
#define LG_HYST__LEN			3
#define LG_HYST__MSK			0x07
#define LG_HYST__REG			HYSTERESIS_REG


#define HG_HYST__POS			3
#define HG_HYST__LEN			3
#define HG_HYST__MSK			0x38
#define HG_HYST__REG			HYSTERESIS_REG



/* ANY MOTION and ALERT settings */

#define EN_ANY_MOTION__POS		6
#define EN_ANY_MOTION__LEN		1
#define EN_ANY_MOTION__MSK		0x40
#define EN_ANY_MOTION__REG		BMA020_CONF1_REG


/* ALERT settings */


#define ALERT__POS			7
#define ALERT__LEN			1
#define ALERT__MSK			0x80
#define ALERT__REG			BMA020_CONF1_REG


/* ANY MOTION Duration */


#define ANY_MOTION_THRES__POS	0
#define ANY_MOTION_THRES__LEN	8
#define ANY_MOTION_THRES__MSK	0xFF
#define ANY_MOTION_THRES__REG	MOTION_THRS_REG


#define ANY_MOTION_DUR__POS		6
#define ANY_MOTION_DUR__LEN		2
#define ANY_MOTION_DUR__MSK		0xC0	
#define ANY_MOTION_DUR__REG		HYSTERESIS_REG


#define CUSTOMER_RESERVED1__POS		0
#define CUSTOMER_RESERVED1__LEN	 	8
#define CUSTOMER_RESERVED1__MSK		0xFF
#define CUSTOMER_RESERVED1__REG		CUSTOMER1_REG

#define CUSTOMER_RESERVED2__POS		0
#define CUSTOMER_RESERVED2__LEN	 	8
#define CUSTOMER_RESERVED2__MSK		0xFF
#define CUSTOMER_RESERVED2__REG		CUSTOMER2_REG



/* BANDWIDTH dependend definitions */

#define BANDWIDTH__POS				0
#define BANDWIDTH__LEN			 	3
#define BANDWIDTH__MSK			 	0x07
#define BANDWIDTH__REG				RANGE_BWIDTH_REG



/* RANGE */

#define RANGE__POS				3
#define RANGE__LEN				2
#define RANGE__MSK				0x18	
#define RANGE__REG				RANGE_BWIDTH_REG


/* WAKE UP */

#define WAKE_UP__POS			0
#define WAKE_UP__LEN			1
#define WAKE_UP__MSK			0x01
#define WAKE_UP__REG			BMA020_CONF2_REG


#define WAKE_UP_PAUSE__POS		1
#define WAKE_UP_PAUSE__LEN		2
#define WAKE_UP_PAUSE__MSK		0x06
#define WAKE_UP_PAUSE__REG		BMA020_CONF2_REG



/* ACCELERATION DATA SHADOW */

#define SHADOW_DIS__POS			3
#define SHADOW_DIS__LEN			1
#define SHADOW_DIS__MSK			0x08
#define SHADOW_DIS__REG			BMA020_CONF2_REG



/* LATCH Interrupt */

#define LATCH_INT__POS			4
#define LATCH_INT__LEN			1
#define LATCH_INT__MSK			0x10
#define LATCH_INT__REG			BMA020_CONF2_REG


/* new data interrupt */

#define NEW_DATA_INT__POS		5
#define NEW_DATA_INT__LEN		1
#define NEW_DATA_INT__MSK		0x20
#define NEW_DATA_INT__REG		BMA020_CONF2_REG


#define ENABLE_ADV_INT__POS		6
#define ENABLE_ADV_INT__LEN		1
#define ENABLE_ADV_INT__MSK		0x40
#define ENABLE_ADV_INT__REG		BMA020_CONF2_REG


#define BMA020_SPI4_OFF	0
#define BMA020_SPI4_ON	1

#define SPI4__POS				7
#define SPI4__LEN				1
#define SPI4__MSK				0x80
#define SPI4__REG				BMA020_CONF2_REG


#if 0
#define OFFSET_X_LSB__POS	6
#define OFFSET_X_LSB__LEN	2
#define OFFSET_X_LSB__MSK	0xC0
#define OFFSET_X_LSB__REG	OFFS_GAIN_X_REG

#define GAIN_X__POS			0
#define GAIN_X__LEN			6
#define GAIN_X__MSK			0x3f
#define GAIN_X__REG			OFFS_GAIN_X_REG

#define OFFSET_Y_LSB__POS	6
#define OFFSET_Y_LSB__LEN	2
#define OFFSET_Y_LSB__MSK	0xC0
#define OFFSET_Y_LSB__REG	OFFS_GAIN_Y_REG

#define GAIN_Y__POS			0
#define GAIN_Y__LEN			6
#define GAIN_Y__MSK			0x3f
#define GAIN_Y__REG			OFFS_GAIN_Y_REG


#define OFFSET_Z_LSB__POS	6
#define OFFSET_Z_LSB__LEN	2
#define OFFSET_Z_LSB__MSK	0xC0
#define OFFSET_Z_LSB__REG	OFFS_GAIN_Z_REG

#define GAIN_Z__POS			0
#define GAIN_Z__LEN			6
#define GAIN_Z__MSK			0x3f
#define GAIN_Z__REG			OFFS_GAIN_Z_REG

#define OFFSET_T_LSB__POS	6
#define OFFSET_T_LSB__LEN	2
#define OFFSET_T_LSB__MSK	0xC0
#define OFFSET_T_LSB__REG	OFFS_GAIN_T_REG

#define GAIN_T__POS			0
#define GAIN_T__LEN			6
#define GAIN_T__MSK			0x3f
#define GAIN_T__REG			OFFS_GAIN_T_REG

#define OFFSET_X_MSB__POS	0
#define OFFSET_X_MSB__LEN	8
#define OFFSET_X_MSB__MSK	0xFF
#define OFFSET_X_MSB__REG	OFFSET_X_REG


#define OFFSET_Y_MSB__POS	0
#define OFFSET_Y_MSB__LEN	8
#define OFFSET_Y_MSB__MSK	0xFF
#define OFFSET_Y_MSB__REG	OFFSET_Y_REG

#define OFFSET_Z_MSB__POS	0
#define OFFSET_Z_MSB__LEN	8
#define OFFSET_Z_MSB__MSK	0xFF
#define OFFSET_Z_MSB__REG	OFFSET_Z_REG

#define OFFSET_T_MSB__POS	0
#define OFFSET_T_MSB__LEN	8
#define OFFSET_T_MSB__MSK	0xFF
#define OFFSET_T_MSB__REG	OFFSET_T_REG
#endif

#define BMA020_GET_BITSLICE(regvar, bitname)\
			(regvar & bitname##__MSK) >> bitname##__POS


#define BMA020_SET_BITSLICE(regvar, bitname, val)\
		  (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)  


/** endcond */


/* CONSTANTS */


/* range and bandwidth */

#define BMA020_RANGE_2G			0 /**< sets range to 2G mode \see bma020_set_range() */
#define BMA020_RANGE_4G			1 /**< sets range to 4G mode \see bma020_set_range() */
#define BMA020_RANGE_8G			2 /**< sets range to 8G mode \see bma020_set_range() */


#define BMA020_BW_25HZ		0	/**< sets bandwidth to 25HZ \see bma020_set_bandwidth() */
#define BMA020_BW_50HZ		1	/**< sets bandwidth to 50HZ \see bma020_set_bandwidth() */
#define BMA020_BW_100HZ		2	/**< sets bandwidth to 100HZ \see bma020_set_bandwidth() */
#define BMA020_BW_190HZ		3	/**< sets bandwidth to 190HZ \see bma020_set_bandwidth() */
#define BMA020_BW_375HZ		4	/**< sets bandwidth to 375HZ \see bma020_set_bandwidth() */
#define BMA020_BW_750HZ		5	/**< sets bandwidth to 750HZ \see bma020_set_bandwidth() */
#define BMA020_BW_1500HZ	6	/**< sets bandwidth to 1500HZ \see bma020_set_bandwidth() */

/* mode settings */

#define BMA020_MODE_NORMAL      0
#define BMA020_MODE_SLEEP       2
#define BMA020_MODE_WAKE_UP     3

/* wake up */

#define BMA020_WAKE_UP_PAUSE_20MS		0
#define BMA020_WAKE_UP_PAUSE_80MS		1
#define BMA020_WAKE_UP_PAUSE_320MS		2
#define BMA020_WAKE_UP_PAUSE_2560MS		3


/* LG/HG thresholds are in LSB and depend on RANGE setting */
/* no range check on threshold calculation */

#define BMA020_SELF_TEST0_ON		1
#define BMA020_SELF_TEST1_ON		3

#define BMA020_EE_W_OFF			0
#define BMA020_EE_W_ON			1



/* low-g, high-g, any_motion */

#define BMA020_COUNTER_LG_RST		0
#define BMA020_COUNTER_LG_0LSB		BMA020_COUNTER_LG_RST
#define BMA020_COUNTER_LG_1LSB		1
#define BMA020_COUNTER_LG_2LSB		2
#define BMA020_COUNTER_LG_3LSB		3

#define BMA020_COUNTER_HG_RST		0
#define BMA020_COUNTER_HG_0LSB		BMA020_COUNTER_HG_RST
#define BMA020_COUNTER_HG_1LSB		1
#define BMA020_COUNTER_HG_2LSB		2
#define BMA020_COUNTER_HG_3LSB		3

#define BMA020_COUNTER_RST			0
#define BMA020_COUNTER_0LSB			BMA020_COUNTER_RST
#define BMA020_COUNTER_1LSB			1
#define BMA020_COUNTER_2LSB			2
#define BMA020_COUNTER_3LSB			3




#define BMA020_LG_THRES_IN_G( gthres, range)			((256 * gthres ) / range)
#define BMA020_HG_THRES_IN_G(gthres, range)				((256 * gthres ) / range)
#define BMA020_LG_HYST_IN_G( ghyst, range )				((32 * ghyst) / range)
#define BMA020_HG_HYST_IN_G( ghyst, range )				((32 * ghyst) / range)
#define BMA020_ANY_MOTION_THRES_IN_G( gthres, range)	((128 * gthres ) / range)


#define BMA020_ANY_MOTION_DUR_1		0
#define BMA020_ANY_MOTION_DUR_3		1
#define BMA020_ANY_MOTION_DUR_5		2
#define BMA020_ANY_MOTION_DUR_7		3



#define BMA020_SHADOW_DIS_OFF	0
#define BMA020_SHADOW_DIS_ON	1

#define BMA020_LATCH_INT_OFF	0
#define BMA020_LATCH_INT_ON		1

#define BMA020_NEW_DATA_INT_OFF	0
#define BMA020_NEW_DATA_INT_ON	1

#define BMA020_ENABLE_ADV_INT_OFF	0
#define BMA020_ENABLE_ADV_INT_ON	1

#define BMA020_EN_ANY_MOTION_OFF 	0
#define BMA020_EN_ANY_MOTION_ON 	1


#define BMA020_ALERT_OFF	0
#define BMA020_ALERT_ON		1

#define BMA020_ENABLE_LG_OFF	0
#define BMA020_ENABLE_LG_ON		1

#define BMA020_ENABLE_HG_OFF	0
#define BMA020_ENABLE_HG_ON		1



#define BMA020_INT_ALERT		(1<<7)
#define BMA020_INT_ANY_MOTION	(1<<6)
#define BMA020_INT_EN_ADV_INT	(1<<5)
#define BMA020_INT_NEW_DATA		(1<<4)
#define BMA020_INT_LATCH		(1<<3)
#define BMA020_INT_HG			(1<<1)
#define BMA020_INT_LG			(1<<0)


#define BMA020_INT_STATUS_HG			(1<<0)
#define BMA020_INT_STATUS_LG			(1<<1)
#define BMA020_INT_STATUS_HG_LATCHED	(1<<2)
#define BMA020_INT_STATUS_LG_LATCHED	(1<<3)
#define BMA020_INT_STATUS_ALERT			(1<<4)
#define BMA020_INT_STATUS_ST_RESULT		(1<<7)


#define BMA020_CONF1_INT_MSK	((1<<ALERT__POS) | (1<<EN_ANY_MOTION__POS) | (1<<ENABLE_HG__POS) | (1<<ENABLE_LG__POS))
#define BMA020_CONF2_INT_MSK	((1<<ENABLE_ADV_INT__POS) | (1<<NEW_DATA_INT__POS) | (1<<LATCH_INT__POS))




/* Function prototypes */

int bma020_init(bma020_t *);

int bma020_set_image (bma020regs_t *);

int bma020_get_image(bma020regs_t *);

int bma020_get_offset(unsigned char, unsigned short *); 

int bma020_set_offset(unsigned char, unsigned short ); 

int bma020_set_offset_eeprom(unsigned char, unsigned short);

int bma020_soft_reset(void); 

int bma020_update_image(void); 

int bma020_write_ee(unsigned char , unsigned char ) ;

int bma020_set_ee_w(unsigned char);

int bma020_selftest(unsigned char);

int bma020_get_selftest_result(unsigned char *);

int bma020_set_range(char); 

int bma020_get_range(unsigned char*);

int bma020_set_mode(unsigned char); 

unsigned char bma020_get_mode(void);

int bma020_set_wake_up_pause(unsigned char);

int bma020_get_wake_up_pause(unsigned char *);

int bma020_set_bandwidth(char);

int bma020_get_bandwidth(unsigned char *);

int bma020_set_low_g_threshold(unsigned char);

int bma020_get_low_g_threshold(unsigned char*);

int bma020_set_low_g_hysteresis(unsigned char);

int bma020_set_low_g_countdown(unsigned char);

int bma020_get_low_g_countdown(unsigned char *);

int bma020_get_low_g_hysteresis(unsigned char*);

int bma020_set_low_g_duration(unsigned char);

int bma020_get_low_g_duration(unsigned char*);

int bma020_set_high_g_threshold(unsigned char);

int bma020_get_high_g_threshold(unsigned char*);

int bma020_set_high_g_hysteresis(unsigned char);

int bma020_set_high_g_countdown(unsigned char);

int bma020_get_high_g_countdown(unsigned char *);

int bma020_get_high_g_hysteresis(unsigned char*);

int bma020_set_high_g_duration(unsigned char);

int bma020_get_high_g_duration(unsigned char*);

int bma020_set_any_motion_threshold(unsigned char);

int bma020_get_any_motion_threshold(unsigned char*);

int bma020_set_any_motion_count(unsigned char);

int bma020_get_any_motion_count(unsigned char *);

int bma020_read_accel_x(short *);

int bma020_read_accel_y(short *);

int bma020_read_accel_z(short *);

int bma020_read_temperature(unsigned char*);

int bma020_read_accel_xyz(bma020acc_t *);

int bma020_get_interrupt_status(unsigned char *);

int bma020_reset_interrupt(void);

int bma020_set_interrupt_mask(unsigned char);

int bma020_get_interrupt_mask(unsigned char *);

int bma020_set_low_g_int(unsigned char);

int bma020_set_high_g_int(unsigned char);

int bma020_set_any_motion_int(unsigned char);

int bma020_set_alert_int(unsigned char);

int bma020_set_advanced_int(unsigned char);

int bma020_latch_int(unsigned char);

int bma020_set_new_data_int(unsigned char onoff);

int bma020_pause(int);

int bma020_read_reg(unsigned char , unsigned char *, unsigned char);

int bma020_write_reg(unsigned char , unsigned char*, unsigned char );

bma020acc_t bma020_calibrate();



#endif   // __BMA380_H__





