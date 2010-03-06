/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-f2.h
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

#define S5PC1XX_GPF2CON			(S5PC1XX_GPF2_BASE + 0x00)
#define S5PC1XX_GPF2DAT			(S5PC1XX_GPF2_BASE + 0x04)
#define S5PC1XX_GPF2PUD			(S5PC1XX_GPF2_BASE + 0x08)
#define S5PC1XX_GPF2DRV			(S5PC1XX_GPF2_BASE + 0x0c)
#define S5PC1XX_GPF2CONPDN		(S5PC1XX_GPF2_BASE + 0x10)
#define S5PC1XX_GPF2PUDPDN		(S5PC1XX_GPF2_BASE + 0x14)

#define S5PC1XX_GPF2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPF2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPF2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPF2_0_LCD_VD_12	(0x2 << 0)
#define S5PC1XX_GPF2_0_SYS_VD_12	(0x3 << 0)
#define S5PC1XX_GPF2_0_V656_DATA_4	(0x4 << 0)
#define S5PC1XX_GPF2_0_GPIO_INT9_0	(0xf << 0)

#define S5PC1XX_GPF2_1_LCD_VD_13	(0x2 << 4)
#define S5PC1XX_GPF2_1_SYS_VD_13	(0x3 << 4)
#define S5PC1XX_GPF2_1_V656_DATA_5	(0x4 << 4)
#define S5PC1XX_GPF2_1_GPIO_INT9_1	(0xf << 4)

#define S5PC1XX_GPF2_2_LCD_VD_14	(0x2 << 8)
#define S5PC1XX_GPF2_2_SYS_VD_14	(0x3 << 8)
#define S5PC1XX_GPF2_2_V656_DATA_6	(0x4 << 8)
#define S5PC1XX_GPF2_2_GPIO_INT9_2	(0xf << 8)

#define S5PC1XX_GPF2_3_LCD_VD_15	(0x2 << 12)
#define S5PC1XX_GPF2_3_SYS_VD_15	(0x3 << 12)
#define S5PC1XX_GPF2_3_V656_DATA_7	(0x4 << 12)
#define S5PC1XX_GPF2_3_GPIO_INT9_3	(0xf << 12)

#define S5PC1XX_GPF2_4_LCD_VD_16	(0x2 << 16)
#define S5PC1XX_GPF2_4_SYS_VD_16	(0x3 << 16)
#define S5PC1XX_GPF2_4_GPIO_INT9_4	(0xf << 16)

#define S5PC1XX_GPF2_5_LCD_VD_17	(0x2 << 20)
#define S5PC1XX_GPF2_5_SYS_VD_17	(0x3 << 20)
#define S5PC1XX_GPF2_5_GPIO_INT9_5	(0xf << 20)

#define S5PC1XX_GPF2_6_LCD_VD_18	(0x2 << 24)
#define S5PC1XX_GPF2_6_GPIO_INT9_6	(0xf << 24)

#define S5PC1XX_GPF2_7_LCD_VD_19	(0x2 << 28)
#define S5PC1XX_GPF2_7_GPIO_INT9_7	(0xf << 28)

