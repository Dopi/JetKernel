/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp03.h
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

#define S5PC1XX_MP03CON			(S5PC1XX_MP03_BASE + 0x00)
#define S5PC1XX_MP03DAT			(S5PC1XX_MP03_BASE + 0x04)
#define S5PC1XX_MP03PUD			(S5PC1XX_MP03_BASE + 0x08)
#define S5PC1XX_MP03DRV			(S5PC1XX_MP03_BASE + 0x0c)
#define S5PC1XX_MP03CONPDN		(S5PC1XX_MP03_BASE + 0x10)
#define S5PC1XX_MP03PUDPDN		(S5PC1XX_MP03_BASE + 0x14)

#define S5PC1XX_MP03_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_MP03_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_MP03_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_MP03_0_EBI_DATA_3	(0x2 << 0)
#define S5PC1XX_MP03_1_EBI_DATA_4	(0x2 << 4)
#define S5PC1XX_MP03_2_EBI_DATA_5	(0x2 << 8)
#define S5PC1XX_MP03_3_EBI_DATA_6	(0x2 << 12)
#define S5PC1XX_MP03_4_EBI_DATA_7	(0x2 << 16)
#define S5PC1XX_MP03_5_EBI_DATA_8	(0x2 << 20)
#define S5PC1XX_MP03_6_EBI_DATA_9	(0x2 << 24)
#define S5PC1XX_MP03_7_EBI_DATA_10	(0x2 << 28)
