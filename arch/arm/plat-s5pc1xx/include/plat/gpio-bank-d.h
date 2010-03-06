/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-d.h
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

#define S5PC1XX_GPDCON			(S5PC1XX_GPD_BASE + 0x00)
#define S5PC1XX_GPDDAT			(S5PC1XX_GPD_BASE + 0x04)
#define S5PC1XX_GPDPUD			(S5PC1XX_GPD_BASE + 0x08)
#define S5PC1XX_GPDDRV			(S5PC1XX_GPD_BASE + 0x0c)
#define S5PC1XX_GPDCONPDN		(S5PC1XX_GPD_BASE + 0x10)
#define S5PC1XX_GPDPUDPDN		(S5PC1XX_GPD_BASE + 0x14)

#define S5PC1XX_GPD_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPD_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPD_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPD0_TOUT_0		(0x2 << 0)
#define S5PC1XX_GPD0_PWM_TCLK 		(0x3 << 0)
#define S5PC1XX_GPD0_GPIO_INT4_0	(0xf << 0)

#define S5PC1XX_GPD1_TOUT_1		(0x2 << 4)
#define S5PC1XX_GPD1_EX_DMA_REQn	(0x3 << 4)
#define S5PC1XX_GPD1_GPIO_INT4_1	(0xf << 4)

#define S5PC1XX_GPD2_TOUT_2		(0x2 << 8)
#define S5PC1XX_GPD2_EX_DMA_ACKn	(0x3 << 8)
#define S5PC1XX_GPD2_GPIO_INT4_2	(0xf << 8)

#define S5PC1XX_GPD3_I2C0_SDA		(0x2 << 12)
#define S5PC1XX_GPD3_GPIO_INT4_3	(0xf << 12)

#define S5PC1XX_GPD4_I2C0_SCL		(0x2 << 16)
#define S5PC1XX_GPD4_GPIO_INT4_4	(0xf << 16)

#define S5PC1XX_GPD5_I2C1_SDA		(0x2 << 20)
#define S5PC1XX_GPD5_SPDIF0_OUT		(0x3 << 20)
#define S5PC1XX_GPD5_GPIO_INT4_5	(0xf << 20)

#define S5PC1XX_GPD6_I2C1_SCL		(0x2 << 24)
#define S5PC1XX_GPD6_SPDIF_EXTCLK	(0x3 << 24)
#define S5PC1XX_GPD6_GPIO_INT4_6	(0xf << 24)

