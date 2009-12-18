/*
 * Header for Crucialtec Optical Joystick.
 */
#ifndef __OPT_JOY_H__
#define __OPT_JOY_H__

// optical joystick address
#define OJT_MOT             0x02  // Motion Occurred & Data Stored(R/W)
#define OJT_DELT_Y          0x03  // Y movement, absolute Y values
#define OJT_DELT_X          0x04  // X movement, absolute X values
#define OJT_SQ				0x05  // squal
#define OJT_SHUTTER_UP		0x06  // shutter_up
#define OJT_SHUTTER_DOWN	0x07  // shutter_down
#define OJT_PIXEL_SUM       0x09  // pixel_sum
#define OJT_POWER_UP_RESET  0x3A  // power up reset

#define OJT_HI			1
#define OJT_LO			0

#define OJT_GPIO_OUT	1
#define OJT_GPIO_IN		0

#define ACTIVE			1
#define INACTIVE		0


/* original value */
//#define SUM_X_THRESHOLD		100
//#define SUM_Y_THRESHOLD		100

#define SUM_X_THRESHOLD		50
#define SUM_Y_THRESHOLD		50

#define OJT_POLLING_PERIOD 		12500000  // 0.0125sec
//#define OJT_POLLING_PERIOD 		500000000  // 0.5 sec

#define IRQ_OJT_INT IRQ_EINT(27)

/* refer to Android's kl file */
//TODO: move to instinctq_rev03.h?
#define SEC_KEY_UP 		KEY_UP
#define SEC_KEY_DOWN	KEY_DOWN
#define SEC_KEY_LEFT	KEY_LEFT
#define SEC_KEY_RIGHT	KEY_RIGHT

int optjoy_keycode[] = {
	SEC_KEY_UP,
	SEC_KEY_DOWN,
	SEC_KEY_LEFT,
	SEC_KEY_RIGHT,
};

/*********** for debug **********************************************************/
#if 0
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif 
/*******************************************************************************/

#endif   // __OPT_JOY_H__
