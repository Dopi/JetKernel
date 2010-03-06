/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j2.h
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

#define S5PC1XX_GPJ2CON			(S5PC1XX_GPJ2_BASE + 0x00)
#define S5PC1XX_GPJ2DAT			(S5PC1XX_GPJ2_BASE + 0x04)
#define S5PC1XX_GPJ2PUD			(S5PC1XX_GPJ2_BASE + 0x08)
#define S5PC1XX_GPJ2DRV			(S5PC1XX_GPJ2_BASE + 0x0c)
#define S5PC1XX_GPJ2CONPDN		(S5PC1XX_GPJ2_BASE + 0x10)
#define S5PC1XX_GPJ2PUDPDN		(S5PC1XX_GPJ2_BASE + 0x14)

#define S5PC1XX_GPJ2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPJ2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPJ2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPJ2_0_MSM_DATA_0	(0x2 << 0)
#define S5PC1XX_GPJ2_0_CF_DATA_0	(0x4 << 0)
#define S5PC1XX_GPJ2_0_GPIO_INT18_0	(0xf << 0)

#define S5PC1XX_GPJ2_1_MSM_DATA_1	(0x2 << 4)
#define S5PC1XX_GPJ2_1_CF_DATA_1	(0x4 << 4)
#define S5PC1XX_GPJ2_1_GPIO_INT18_1	(0xf << 4)

#define S5PC1XX_GPJ2_2_MSM_DATA_2	(0x2 << 8)
#define S5PC1XX_GPJ2_2_CF_DATA_2	(0x4 << 8)
#define S5PC1XX_GPJ2_2_GPIO_INT18_2	(0xf << 8)

#define S5PC1XX_GPJ2_3_MSM_DATA_3	(0x2 << 12)
#define S5PC1XX_GPJ2_3_CF_DATA_3	(0x4 << 12)
#define S5PC1XX_GPJ2_3_GPIO_INT18_3	(0xf << 12)

#define S5PC1XX_GPJ2_4_MSM_DATA_4	(0x2 << 16)
#define S5PC1XX_GPJ2_4_CF_DATA_4	(0x4 << 16)
#define S5PC1XX_GPJ2_4_GPIO_INT18_4	(0xf << 16)

#define S5PC1XX_GPJ2_5_MSM_DATA_5	(0x2 << 20)
#define S5PC1XX_GPJ2_5_CF_DATA_5	(0x4 << 20)
#define S5PC1XX_GPJ2_5_GPIO_INT18_5	(0xf << 20)

#define S5PC1XX_GPJ2_6_MSM_DATA_6	(0x2 << 24)
#define S5PC1XX_GPJ2_6_CF_DATA_6	(0x4 << 24)
#define S5PC1XX_GPJ2_6_GPIO_INT18_6	(0xf << 24)

#define S5PC1XX_GPJ2_7_MSM_DATA_7	(0x2 << 28)
#define S5PC1XX_GPJ2_7_CF_DATA_7	(0x4 << 28)
#define S5PC1XX_GPJ2_7_GPIO_INT18_7	(0xf << 28)

