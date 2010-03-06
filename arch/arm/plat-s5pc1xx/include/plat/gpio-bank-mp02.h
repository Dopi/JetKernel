/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp02.h
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

#define S5PC1XX_MP02CON			(S5PC1XX_MP02_BASE + 0x00)
#define S5PC1XX_MP02DAT			(S5PC1XX_MP02_BASE + 0x04)
#define S5PC1XX_MP02PUD			(S5PC1XX_MP02_BASE + 0x08)
#define S5PC1XX_MP02DRV			(S5PC1XX_MP02_BASE + 0x0c)
#define S5PC1XX_MP02CONPDN		(S5PC1XX_MP02_BASE + 0x10)
#define S5PC1XX_MP02PUDPDN		(S5PC1XX_MP02_BASE + 0x14)

#define S5PC1XX_MP02_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_MP02_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_MP02_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_MP02_0_EBI_ADDR_16	(0x2 << 0)
#define S5PC1XX_MP02_1_EBI_ADDR_17	(0x2 << 4)
#define S5PC1XX_MP02_2_EBI_ADDR_18	(0x2 << 8)
#define S5PC1XX_MP02_3_EBI_ADDR_19	(0x2 << 12)
#define S5PC1XX_MP02_4_EBI_ADDR_20	(0x2 << 16)
#define S5PC1XX_MP02_5_EBI_DATA_0	(0x2 << 20)
#define S5PC1XX_MP02_6_EBI_DATA_1	(0x2 << 24)
#define S5PC1XX_MP02_7_EBI_DATA_2	(0x2 << 28)

