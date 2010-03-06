/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j3.h
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

#define S5PC1XX_GPJ3CON			(S5PC1XX_GPJ3_BASE + 0x00)
#define S5PC1XX_GPJ3DAT			(S5PC1XX_GPJ3_BASE + 0x04)
#define S5PC1XX_GPJ3PUD			(S5PC1XX_GPJ3_BASE + 0x08)
#define S5PC1XX_GPJ3DRV			(S5PC1XX_GPJ3_BASE + 0x0c)
#define S5PC1XX_GPJ3CONPDN		(S5PC1XX_GPJ3_BASE + 0x10)
#define S5PC1XX_GPJ3PUDPDN		(S5PC1XX_GPJ3_BASE + 0x14)

#define S5PC1XX_GPJ3_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPJ3_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPJ3_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPJ3_0_MSM_DATA_8	(0x2 << 0)
#define S5PC1XX_GPJ3_0_CF_DATA_8	(0x4 << 0)
#define S5PC1XX_GPJ3_0_GPIO_INT19_0	(0xf << 0)

#define S5PC1XX_GPJ3_1_MSM_DATA_9	(0x2 << 4)
#define S5PC1XX_GPJ3_1_CF_DATA_9	(0x4 << 4)
#define S5PC1XX_GPJ3_1_GPIO_INT19_1	(0xf << 4)

#define S5PC1XX_GPJ3_2_MSM_DATA_10	(0x2 << 8)
#define S5PC1XX_GPJ3_2_CF_DATA_10	(0x4 << 8)
#define S5PC1XX_GPJ3_2_GPIO_INT19_2	(0xf << 8)

#define S5PC1XX_GPJ3_3_MSM_DATA_11	(0x2 << 12)
#define S5PC1XX_GPJ3_3_CF_DATA_11	(0x4 << 12)
#define S5PC1XX_GPJ3_3_GPIO_INT19_3	(0xf << 12)

#define S5PC1XX_GPJ3_4_MSM_DATA_12	(0x2 << 16)
#define S5PC1XX_GPJ3_4_CF_DATA_12	(0x4 << 16)
#define S5PC1XX_GPJ3_4_GPIO_INT19_4	(0xf << 16)

#define S5PC1XX_GPJ3_5_MSM_DATA_13	(0x2 << 20)
#define S5PC1XX_GPJ3_5_CF_DATA_13	(0x4 << 20)
#define S5PC1XX_GPJ3_5_GPIO_INT19_5	(0xf << 20)

#define S5PC1XX_GPJ3_6_MSM_DATA_14	(0x2 << 24)
#define S5PC1XX_GPJ3_6_CF_DATA_14	(0x4 << 24)
#define S5PC1XX_GPJ3_6_GPIO_INT19_6	(0xf << 24)

#define S5PC1XX_GPJ3_7_MSM_DATA_15	(0x2 << 28)
#define S5PC1XX_GPJ3_7_CF_DATA_15	(0x4 << 28)
#define S5PC1XX_GPJ3_7_GPIO_INT19_7	(0xf << 28)

