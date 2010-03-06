/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j1.h
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

#define S5PC1XX_GPJ1CON			(S5PC1XX_GPJ1_BASE + 0x00)
#define S5PC1XX_GPJ1DAT			(S5PC1XX_GPJ1_BASE + 0x04)
#define S5PC1XX_GPJ1PUD			(S5PC1XX_GPJ1_BASE + 0x08)
#define S5PC1XX_GPJ1DRV			(S5PC1XX_GPJ1_BASE + 0x0c)
#define S5PC1XX_GPJ1CONPDN		(S5PC1XX_GPJ1_BASE + 0x10)
#define S5PC1XX_GPJ1PUDPDN		(S5PC1XX_GPJ1_BASE + 0x14)

#define S5PC1XX_GPJ1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPJ1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPJ1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPJ1_0_MSM_ADDR_8	(0x2 << 0)
#define S5PC1XX_GPJ1_0_GPIO_INT17_0	(0xf << 0)

#define S5PC1XX_GPJ1_1_MSM_ADDR_9	(0x2 << 4)
#define S5PC1XX_GPJ1_1_GPIO_INT17_1	(0xf << 4)

#define S5PC1XX_GPJ1_2_MSM_ADDR_10	(0x2 << 8)
#define S5PC1XX_GPJ1_2_GPIO_INT17_2	(0xf << 8)

#define S5PC1XX_GPJ1_3_MSM_ADDR_11	(0x2 << 12)
#define S5PC1XX_GPJ1_3_GPIO_INT17_3	(0xf << 12)

#define S5PC1XX_GPJ1_4_MSM_ADDR_12	(0x2 << 16)
#define S5PC1XX_GPJ1_4_GPIO_INT17_4	(0xf << 16)

