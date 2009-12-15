#ifndef __ICTHM02S_H__
#define __ICTHM02S_H__

/* 00 - Offset register        */
#define REG_OFF   0x00
/* 01 - Threshold              */
#define REG_THOLD 0x01
/* 02 - Function               */
#define REG_FUNC  0x02
/* 03 - Sample Rate            */
#define REG_SRATE 0x03
/* 04 - Calibration Data High  */
#define REG_CDH   0x04
/* 05 - Calibration Data Low   */
#define REG_CDL   0x05
/* 06 - Reserved               */
/* 07 - Sensing Control        */
#define REG_SCTL  0x07


#define IRQ_HM_INT IRQ_EINT(17)
#define MIN_X -7
#define MIN_Y -7
#define MAX_X  7
#define MAX_Y  7

#define MAX_REG_NUMBER	0x8
#define MAX_TYPE_NUMBER 0x3


#define HM_TYPE KEY_TYPE

#define NORMAL_TYPE	 0
#define INVERSE_TYPE 1

#define SPI_TYPE NORMAL_TYPE


static int __devexit hm_spi_remove(struct spi_device *spi);
struct workqueue_struct *hm_wq;

static int inverseBit(u8 value_ori);

static u8 hm_icthm02s_original_image[8] =
{
	0x08,   /* 00 - OFFSET                 */
	0x0c,   /* 01 - Threshold              */
	0x12,   /* 02 - Function               */
	0x30,   /* 03 - Sample Rate            */
	0x00,   /* 04 - Calibration Data High  */
	0x00,   /* 05 - Calibration Data Low   */
	0x00,   /* 06 - Reserved               */
	0x00,   /* 07 - Sensing Control        */
};




typedef enum t_event_type
{
	KEY_TYPE   = 1,
	MOUSE_TYPE = 2,
	ALL_TYPE   = 3,


}event_type;




struct hm_icthm02s_data {
	struct input_dev *input_dev;
	struct work_struct work;
	int             irq;

/* Type is changeable. You need to consider interface with app. */
	event_type		type;
	int time_tick;
    struct hrtimer timer;
	struct spi_device       *spi;
	u8              spi_wbuffer[2];
	u8              spi_rbuffer[2];
	/* Image of the SPI registers in ICTHM02S */
	u8              reg_image[8];

};




#endif

