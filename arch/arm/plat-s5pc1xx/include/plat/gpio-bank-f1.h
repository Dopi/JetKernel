/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-f1.h
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

#define S5PC1XX_GPF1CON			(S5PC1XX_GPF1_BASE + 0x00)
#define S5PC1XX_GPF1DAT			(S5PC1XX_GPF1_BASE + 0x04)
#define S5PC1XX_GPF1PUD			(S5PC1XX_GPF1_BASE + 0x08)
#define S5PC1XX_GPF1DRV			(S5PC1XX_GPF1_BASE + 0x0c)
#define S5PC1XX_GPF1CONPDN		(S5PC1XX_GPF1_BASE + 0x10)
#define S5PC1XX_GPF1PUDPDN		(S5PC1XX_GPF1_BASE + 0x14)

#define S5PC1XX_GPF1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPF1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPF1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPF1_0_LCD_VD_4		(0x2 << 0)
#define S5PC1XX_GPF1_0_SYS_VD_4		(0x3 << 0)
#define S5PC1XX_GPF1_0_VEN_DATA_4	(0x4 << 0)
#define S5PC1XX_GPF1_0_GPIO_INT8_0	(0xf << 0)

#define S5PC1XX_GPF1_1_LCD_VD_5		(0x2 << 4)
#define S5PC1XX_GPF1_1_SYS_VD_5		(0x3 << 4)
#define S5PC1XX_GPF1_1_VEN_DATA_5	(0x4 << 4)
#define S5PC1XX_GPF1_1_GPIO_INT8_1	(0xf << 4)

#define S5PC1XX_GPF1_2_LCD_VD_6		(0x2 << 8)
#define S5PC1XX_GPF1_2_SYS_VD_6		(0x3 << 8)
#define S5PC1XX_GPF1_2_VEN_DATA_6	(0x4 << 8)
#define S5PC1XX_GPF1_2_GPIO_INT8_2	(0xf << 8)

#define S5PC1XX_GPF1_3_LCD_VD_7		(0x2 << 12)
#define S5PC1XX_GPF1_3_SYS_VD_7		(0x3 << 12)
#define S5PC1XX_GPF1_3_VEN_DATA_7	(0x4 << 12)
#define S5PC1XX_GPF1_3_GPIO_INT8_3	(0xf << 12)

#define S5PC1XX_GPF1_4_LCD_VD_8		(0x2 << 16)
#define S5PC1XX_GPF1_4_SYS_VD_8		(0x3 << 16)
#define S5PC1XX_GPF1_4_V656_DATA_0	(0x4 << 16)
#define S5PC1XX_GPF1_4_GPIO_INT8_4	(0xf << 16)

#define S5PC1XX_GPF1_5_LCD_VD_9		(0x2 << 20)
#define S5PC1XX_GPF1_5_SYS_VD_9		(0x3 << 20)
#define S5PC1XX_GPF1_5_V656_DATA_1	(0x4 << 20)
#define S5PC1XX_GPF1_5_GPIO_INT8_5	(0xf << 20)

#define S5PC1XX_GPF1_6_LCD_VD_10	(0x2 << 24)
#define S5PC1XX_GPF1_6_SYS_VD_10	(0x3 << 24)
#define S5PC1XX_GPF1_6_V656_DATA_2	(0x4 << 24)
#define S5PC1XX_GPF1_6_GPIO_INT8_6	(0xf << 24)

#define S5PC1XX_GPF1_7_LCD_VD_11	(0x2 << 28)
#define S5PC1XX_GPF1_7_SYS_VD_11	(0x3 << 28)
#define S5PC1XX_GPF1_7_V656_DATA_3	(0x4 << 28)
#define S5PC1XX_GPF1_7_GPIO_INT8_7	(0xf << 28)

