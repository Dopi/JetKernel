/* linux/drivers/input/keyboard/s3c-keypad.h 
 *
 * Driver header for Samsung SoC keypad.
 *
 * dopi711@googlemail.com, Copyright (c) 2010 JetDroid project
 *      http://code.google.com/p/jetdroid
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "s3c-keypad.h"

#ifndef _S3C_KEYPAD_BOARD_H_
#define _S3C_KEYPAD_BOARD_H_

#if defined(CONFIG_MACH_INSTINCTQ)

#if defined(CONFIG_JET_OPTION)

#define KEYPAD_COLUMNS  (8)     /* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_3 */
#define KEYPAD_ROWS     (7)     /* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_2 */
#define MAX_KEYPAD_NR   (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR  32

u32 g_board_num = CONFIG_INSTINCTQ_REV;
//u32 g_board_num = CONFIG_JET_REV;

struct s3c_keypad_special_key special_key_spica_00[] = {
	{0x08000000, 0x00000000, KEYCODE_FOCUS},
	{0x00040100, 0x00000000, KEYCODE_DUMPKEY},
};

struct s3c_keypad_gpio_key gpio_key_spica_00[] = {
	{IRQ_EINT(5), GPIO_POWER_N, GPIO_POWER_N_AF, KEYCODE_ENDCALL, 1},
};

#if 0 //(CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
struct s3c_keypad_gpio_key gpio_key_spica_02[] = {
	{IRQ_EINT(5), GPIO_POWER_N, GPIO_POWER_N_AF, KEYCODE_ENDCALL, 1},
	{IRQ_EINT(17), GPIO_HOLD_KEY_N, GPIO_HOLD_KEY_N_AF, KEYCODE_HOLDKEY, 1},
};
#endif

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_00[0], 1, 1},
	{0x0010, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_00[0], 1, 1},
#if 0 //(CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
	{0x0020, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_02[0], 2, 1},
#endif
};

#else	/* CONFIG_JET_OPTION */

#define KEYPAD_COLUMNS  (8)     /* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_7 */
#define KEYPAD_ROWS     (7)     /* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_6 */
#define MAX_KEYPAD_NR   (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR  32

u32 g_board_num = CONFIG_INSTINCTQ_REV;

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
#endif	/* CONFIG_JET_OPTION */

#elif defined(CONFIG_MACH_SPICA)

static void __iomem *key_base;

#define KEYPAD_COLUMNS (4) 	/* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_3 */
#define KEYPAD_ROWS (4) 	/* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_3 */
#define MAX_KEYPAD_NR (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR 32

u32 g_board_num = CONFIG_SPICA_REV;

struct s3c_keypad_special_key special_key_spica_00[] = {
	{0x08000000, 0x00000000, KEYCODE_FOCUS},
	{0x00040100, 0x00000000, KEYCODE_DUMPKEY},
};

struct s3c_keypad_gpio_key gpio_key_spica_00[] = {
	{IRQ_EINT(5), GPIO_POWER_N, GPIO_POWER_N_AF, KEYCODE_ENDCALL, 1},
};

#if defined(CONFIG_MACH_SPICA) && (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
struct s3c_keypad_gpio_key gpio_key_spica_02[] = {
	{IRQ_EINT(5), GPIO_POWER_N, GPIO_POWER_N_AF, KEYCODE_ENDCALL, 1},
	{IRQ_EINT(17), GPIO_HOLD_KEY_N, GPIO_HOLD_KEY_N_AF, KEYCODE_HOLDKEY, 1},
};
#endif

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_00[0], 1, 1},
	{0x0010, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_00[0], 1, 1},
#if defined(CONFIG_MACH_SPICA) && (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
	{0x0020, NULL, &special_key_spica_00[0], 2, &gpio_key_spica_02[0], 2, 1},
#endif
};

#endif 				/* CONFIG_MACH_SPICA */

#endif				/* _S3C_KEYIF_H_ */
