/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h3.h
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

#define S5PC1XX_GPH3CON			(S5PC1XX_GPH3_BASE + 0x00)
#define S5PC1XX_GPH3DAT			(S5PC1XX_GPH3_BASE + 0x04)
#define S5PC1XX_GPH3PUD			(S5PC1XX_GPH3_BASE + 0x08)
#define S5PC1XX_GPH3DRV			(S5PC1XX_GPH3_BASE + 0x0c)
#define S5PC1XX_GPH3CONPDN		(S5PC1XX_GPH3_BASE + 0x10)
#define S5PC1XX_GPH3PUDPDN		(S5PC1XX_GPH3_BASE + 0x14)

#define S5PC1XX_GPH3_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPH3_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPH3_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPH3_0_WAKEUP_INT_24	(0x2 << 0)
#define S5PC1XX_GPH3_0_KEYPAD_ROW_0	(0x3 << 0)
#define S5PC1XX_GPH3_0_CAM_B_PCLK	(0x4 << 0)

#define S5PC1XX_GPH3_1_WAKEUP_INT_25	(0x2 << 4)
#define S5PC1XX_GPH3_1_KEYPAD_ROW_1	(0x3 << 4)
#define S5PC1XX_GPH3_1_CAM_B_VSYNC	(0x4 << 4)

#define S5PC1XX_GPH3_2_WAKEUP_INT_26	(0x2 << 8)
#define S5PC1XX_GPH3_2_KEYPAD_ROW_2	(0x3 << 8)
#define S5PC1XX_GPH3_2_CAM_B_HREF	(0x4 << 8)

#define S5PC1XX_GPH3_3_WAKEUP_INT_27	(0x2 << 12)
#define S5PC1XX_GPH3_3_KEYPAD_ROW_3	(0x3 << 12)
#define S5PC1XX_GPH3_3_CAM_B_FIELD	(0x4 << 12)

#define S5PC1XX_GPH3_4_WAKEUP_INT_28	(0x2 << 16)
#define S5PC1XX_GPH3_4_KEYPAD_ROW_4	(0x3 << 16)
#define S5PC1XX_GPH3_4_CAN0_TX		(0x4 << 16)

#define S5PC1XX_GPH3_5_WAKEUP_INT_29	(0x2 << 20)
#define S5PC1XX_GPH3_5_KEYPAD_ROW_5	(0x3 << 20)
#define S5PC1XX_GPH3_5_CAN0_RX		(0x4 << 20)

#define S5PC1XX_GPH3_6_WAKEUP_INT_30	(0x2 << 24)
#define S5PC1XX_GPH3_6_KEYPAD_ROW_6	(0x3 << 24)
#define S5PC1XX_GPH3_6_CAN1_TX		(0x4 << 24)

#define S5PC1XX_GPH3_7_WAKEUP_INT_31	(0x2 << 28)
#define S5PC1XX_GPH3_7_KEYPAD_ROW_7	(0x3 << 28)
#define S5PC1XX_GPH3_7_CAN1_RX		(0x4 << 28)

