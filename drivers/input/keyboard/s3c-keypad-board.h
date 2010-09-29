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


#ifndef _S3C_KEYPAD_BOARD_H_
#define _S3C_KEYPAD_BOARD_H_

static void __iomem *key_base;

#define KEYPAD_COLUMNS	(4)	/* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_3 */
#define KEYPAD_ROWS	(4)	/* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_3 */
#define MAX_KEYPAD_NR   (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR	32

#if 1	// Temporary Code by SYS.LSI
#define ENDCALL_KEY 249
#define HOLD_KEY 17
#endif

u32 g_board_num = CONFIG_SATURN_REV;

struct s3c_keypad_special_key special_key_saturn_00[] = {
	{0x08000000, 0x00000000, KEYCODE_FOCUS},
	{0x00000500, 0x00000000, KEYCODE_DUMPKEY},
};

struct s3c_keypad_gpio_key gpio_key_saturn_00[] = {
	{IRQ_EINT(5),  GPIO_POWER_N,      GPIO_POWER_N_AF,      KEYCODE_ENDCALL, 1},
};


struct s3c_keypad_gpio_key gpio_key_saturn_02[] = {
	{IRQ_EINT(5),  GPIO_POWER_N,      GPIO_POWER_N_AF,      KEYCODE_ENDCALL, 1},
};

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, NULL, &special_key_saturn_00[0], 2,  &gpio_key_saturn_00[0], 1, 1},
	{0x0010, NULL, &special_key_saturn_00[0], 2,  &gpio_key_saturn_00[0], 1, 1},
	{0x0020, NULL, &special_key_saturn_00[0], 2,  &gpio_key_saturn_02[0], 1, 1},
};

#endif				/* _S3C_KEYIF_H_ */
