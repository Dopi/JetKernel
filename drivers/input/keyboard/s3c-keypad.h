/* linux/drivers/input/keyboard/s3c-keypad.h 
 *
 * Driver header for Samsung SoC keypad.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _S3C_KEYPAD_H_
#define _S3C_KEYPAD_H_

#define GPIO_nPOWER             S3C64XX_GPN(5)
#define GPIO_nPOWER_AF          (2)
#define GPIO_LEVEL_LOW          0


#define __CONFIG_KEYLOCK__				1
#define __CONFIG_WAKEUP_FAKE_KEY__	1
#define __CONFIG_EARKEY__				1
static void __iomem *key_base;

#define KEYPAD_COLUMNS	3	
#define KEYPAD_ROWS	3		

#if (__CONFIG_KEYLOCK__ + __CONFIG_WAKEUP_FAKE_KEY__ + __CONFIG_EARKEY__ == 3)
#define MAX_KEYPAD_NR 	12
#elif (__CONFIG_KEYLOCK__ + __CONFIG_WAKEUP_FAKE_KEY__ + __CONFIG_EARKEY__ == 2)
#define MAX_KEYPAD_NR 	11
#elif (__CONFIG_KEYLOCK__ + __CONFIG_WAKEUP_FAKE_KEY__ + __CONFIG_EARKEY__ == 1)
#define MAX_KEYPAD_NR 	10
#else
#define MAX_KEYPAD_NR 	9
#endif

#define MAX_KEYMASK_NR	32

#define KEY_PRESSED 1
#define KEY_RELEASED 0



#ifdef __CONFIG_WAKEUP_FAKE_KEY__
	#define KEY_WAKEUP		KEY_NEW


int keypad_keycode[] = {
	KEY_POWER,		KEY_SEARCH,			KEY_CONFIG,
	KEY_SEND,		KEY_CONFIG,			KEY_VOLUMEUP,
	KEY_MENU,		KEY_BACK,			KEY_VOLUMEDOWN,
#ifdef __CONFIG_KEYLOCK__
	KEY_POWER,
#endif
	KEY_POWER,
#ifdef __CONFIG_WAKEUP_FAKE_KEY__
	KEY_POWER,
#endif
};

//int keypad_keycode[] = {
//	KEY_FRONT,		KEY_EXIT,			KEY_CONFIG,
//	KEY_PHONE,		KEY_VOLUMEDOWN,		KEY_VOLUMEUP,
//	KEY_SEARCH,		KEY_CAMERA,			KEY_SEARCH,
//#ifdef __CONFIG_KEYLOCK__
//	KEY_SCREENLOCK,
//#endif
//	KEY_SEND,
//#ifdef __CONFIG_WAKEUP_FAKE_KEY__
//	KEY_WAKEUP,
//#endif
//};

#else
int keypad_keycode[] = {
		 1,  2,  3,  
		 4,  5,  6,  
		 7,  8,	 9,
	};
#endif

#ifdef CONFIG_CPU_S3C6410
#define KEYPAD_DELAY		(50)
#endif

#define	KEYIFCOL_CLEAR		(readl(key_base+S3C_KEYIFCOL) & ~0xffff)
#define	KEYIFCON_CLEAR		(readl(key_base+S3C_KEYIFCON) & ~0x1f)
#define KEYIFFC_DIV		(readl(key_base+S3C_KEYIFFC) | 0x1)

struct s3c_keypad {
	struct input_dev *dev;
	int nr_rows;	
	int no_cols;
	int total_keys; 
	int keycodes[MAX_KEYPAD_NR];
};

extern void s3c_setup_keypad_cfg_gpio(int rows, int columns);

#endif				/* _S3C_KEYIF_H_ */
