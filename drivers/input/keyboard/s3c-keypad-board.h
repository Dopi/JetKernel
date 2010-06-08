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

#define KEYPAD_COLUMNS  (8)     /* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_7 */
#define KEYPAD_ROWS     (7)     /* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_6 */
#define MAX_KEYPAD_NR   (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR  32

#define KEYCODE_SYM	251
#define KEYCODE_MENU	252
#define KEYCODE_TAB	253

//u32 g_board_num = CONFIG_INSTINCTQ_REV;
u32 g_board_num = CONFIG_JET_REV;

struct s3c_keypad_slide slide_instinctq = {IRQ_EINT(4), GPIO_HALL_SW, GPIO_HALL_SW_AF, 1};

struct s3c_keypad_special_key special_key_instinctq_00_01[] = {
	{0x00000000, 0x00010000, KEYCODE_FOCUS},
	{0x01000100, 0x00000000, KEYCODE_DUMPKEY},
};

struct s3c_keypad_special_key special_key_instinctq_01a_40[] = {
	{0x01000100, 0x00000000, KEYCODE_DUMPKEY},
	{0x00000000, 0x04200000, KEYCODE_SYM},
	{0x00000000, 0x20200000, KEYCODE_MENU},
	{0x00000000, 0x00280000, KEYCODE_TAB},
};
struct s3c_keypad_gpio_key gpio_key_instinctq_01[] = {
	{IRQ_EINT(5),  GPIO_POWER_N     , GPIO_POWER_N_AF     , KEYCODE_ENDCALL, 1},
};

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, &slide_instinctq,  &special_key_instinctq_00_01[0], 2,   &gpio_key_instinctq_01[0] , 1, 0},
	{0x0010, &slide_instinctq,  &special_key_instinctq_00_01[0], 2,   &gpio_key_instinctq_01[0] , 1, 0},
	{0x001a, &slide_instinctq,  &special_key_instinctq_01a_40[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},
	{0x0020, &slide_instinctq,  &special_key_instinctq_01a_40[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},
	{0x0030, &slide_instinctq,  &special_key_instinctq_01a_40[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},
	{0x0040, &slide_instinctq,  &special_key_instinctq_01a_40[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},
};

#endif				/* _S3C_KEYIF_H_ */
