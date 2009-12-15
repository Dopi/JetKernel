/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h2.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank A register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5PC1XX_GPH2CON			(S5PC1XX_GPH2_BASE + 0x00)
#define S5PC1XX_GPH2DAT			(S5PC1XX_GPH2_BASE + 0x04)
#define S5PC1XX_GPH2PUD			(S5PC1XX_GPH2_BASE + 0x08)
#define S5PC1XX_GPH2DRV			(S5PC1XX_GPH2_BASE + 0x0c)
#define S5PC1XX_GPH2CONPDN		(S5PC1XX_GPH2_BASE + 0x10)
#define S5PC1XX_GPH2PUDPDN		(S5PC1XX_GPH2_BASE + 0x14)

#define S5PC1XX_GPH2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPH2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPH2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPH2_0_WAKEUP_INT_16	(0x2 << 0)
#define S5PC1XX_GPH2_0_KEYPAD_COL_0	(0x3 << 0)
#define S5PC1XX_GPH2_0_CAM_B_DATA_0	(0x4 << 0)

#define S5PC1XX_GPH2_1_WAKEUP_INT_17	(0x2 << 4)
#define S5PC1XX_GPH2_1_KEYPAD_COL_1	(0x3 << 4)
#define S5PC1XX_GPH2_1_CAM_B_DATA_1	(0x4 << 4)

#define S5PC1XX_GPH2_2_WAKEUP_INT_18	(0x2 << 8)
#define S5PC1XX_GPH2_2_KEYPAD_COL_2	(0x3 << 8)
#define S5PC1XX_GPH2_2_CAM_B_DATA_2	(0x4 << 8)

#define S5PC1XX_GPH2_3_WAKEUP_INT_19	(0x2 << 12)
#define S5PC1XX_GPH2_3_KEYPAD_COL_3	(0x3 << 12)
#define S5PC1XX_GPH2_3_CAM_B_DATA_3	(0x4 << 12)

#define S5PC1XX_GPH2_4_WAKEUP_INT_20	(0x2 << 16)
#define S5PC1XX_GPH2_4_KEYPAD_COL_4	(0x3 << 16)
#define S5PC1XX_GPH2_4_CAM_B_DATA_4	(0x4 << 16)

#define S5PC1XX_GPH2_5_WAKEUP_INT_21	(0x2 << 20)
#define S5PC1XX_GPH2_5_KEYPAD_COL_5	(0x3 << 20)
#define S5PC1XX_GPH2_5_CAM_B_DATA_5	(0x4 << 20)

#define S5PC1XX_GPH2_6_WAKEUP_INT_22	(0x2 << 24)
#define S5PC1XX_GPH2_6_KEYPAD_COL_6	(0x3 << 24)
#define S5PC1XX_GPH2_6_CAM_B_DATA_6	(0x4 << 24)

#define S5PC1XX_GPH2_7_WAKEUP_INT_23	(0x2 << 28)
#define S5PC1XX_GPH2_7_KEYPAD_COL_7	(0x3 << 28)
#define S5PC1XX_GPH2_7_CAM_B_DATA_7	(0x4 << 28)

