/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-f0.h
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

#define S5PC1XX_GPF0CON			(S5PC1XX_GPF0_BASE + 0x00)
#define S5PC1XX_GPF0DAT			(S5PC1XX_GPF0_BASE + 0x04)
#define S5PC1XX_GPF0PUD			(S5PC1XX_GPF0_BASE + 0x08)
#define S5PC1XX_GPF0DRV			(S5PC1XX_GPF0_BASE + 0x0c)
#define S5PC1XX_GPF0CONPDN		(S5PC1XX_GPF0_BASE + 0x10)
#define S5PC1XX_GPF0PUDPDN		(S5PC1XX_GPF0_BASE + 0x14)

#define S5PC1XX_GPF0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPF0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPF0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPF0_0_LCD_HSYNC	(0x2 << 0)
#define S5PC1XX_GPF0_0_SYS_CS0		(0x3 << 0)
#define S5PC1XX_GPF0_0_VEN_HSYNC	(0x4 << 0)
#define S5PC1XX_GPF0_0_GPIO_INT7_0	(0xf << 0)

#define S5PC1XX_GPF0_1_LCD_VSYNC	(0x2 << 4)
#define S5PC1XX_GPF0_1_SYS_CS1		(0x3 << 4)
#define S5PC1XX_GPF0_1_VEN_VSYNC	(0x4 << 4)
#define S5PC1XX_GPF0_1_GPIO_INT7_1	(0xf << 4)

#define S5PC1XX_GPF0_2_LCD_VDEN		(0x2 << 8)
#define S5PC1XX_GPF0_2_SYS_RS		(0x3 << 8)
#define S5PC1XX_GPF0_2_VEN_HREF		(0x4 << 8)
#define S5PC1XX_GPF0_2_GPIO_INT7_2	(0xf << 8)

#define S5PC1XX_GPF0_3_LCD_VCLK		(0x2 << 12)
#define S5PC1XX_GPF0_3_SYS_WE		(0x3 << 12)
#define S5PC1XX_GPF0_3_V601_CLK		(0x4 << 12)
#define S5PC1XX_GPF0_3_GPIO_INT7_3	(0xf << 12)

#define S5PC1XX_GPF0_4_LCD_VD_0		(0x2 << 16)
#define S5PC1XX_GPF0_4_SYS_VD_0		(0x3 << 16)
#define S5PC1XX_GPF0_4_VEN_DATA_0	(0x4 << 16)
#define S5PC1XX_GPF0_4_GPIO_INT7_4	(0xf << 16)

#define S5PC1XX_GPF0_5_LCD_VD_1		(0x2 << 20)
#define S5PC1XX_GPF0_5_SYS_VD_1		(0x3 << 20)
#define S5PC1XX_GPF0_5_VEN_DATA_1	(0x4 << 20)
#define S5PC1XX_GPF0_5_GPIO_INT7_5	(0xf << 20)

#define S5PC1XX_GPF0_6_LCD_VD_2		(0x2 << 24)
#define S5PC1XX_GPF0_6_SYS_VD_2		(0x3 << 24)
#define S5PC1XX_GPF0_6_VEN_DATA_2	(0x4 << 24)
#define S5PC1XX_GPF0_6_GPIO_INT7_6	(0xf << 24)

#define S5PC1XX_GPF0_7_LCD_VD_3		(0x2 << 28)
#define S5PC1XX_GPF0_7_SYS_VD_3		(0x3 << 28)
#define S5PC1XX_GPF0_7_VEN_DATA_3	(0x4 << 28)
#define S5PC1XX_GPF0_7_GPIO_INT7_7	(0xf << 28)

