/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-mp01.h
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

#define S5PC1XX_MP01CON			(S5PC1XX_MP01_BASE + 0x00)
#define S5PC1XX_MP01DAT			(S5PC1XX_MP01_BASE + 0x04)
#define S5PC1XX_MP01PUD			(S5PC1XX_MP01_BASE + 0x08)
#define S5PC1XX_MP01DRV			(S5PC1XX_MP01_BASE + 0x0c)
#define S5PC1XX_MP01CONPDN		(S5PC1XX_MP01_BASE + 0x10)
#define S5PC1XX_MP01PUDPDN		(S5PC1XX_MP01_BASE + 0x14)

#define S5PC1XX_MP01_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_MP01_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_MP01_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_MP01_0_EBI_ADDR_8	(0x2 << 0)
#define S5PC1XX_MP01_1_EBI_ADDR_9	(0x2 << 4)
#define S5PC1XX_MP01_2_EBI_ADDR_10	(0x2 << 8)
#define S5PC1XX_MP01_3_EBI_ADDR_11	(0x2 << 12)
#define S5PC1XX_MP01_4_EBI_ADDR_12	(0x2 << 16)
#define S5PC1XX_MP01_5_EBI_ADDR_13	(0x2 << 20)
#define S5PC1XX_MP01_6_EBI_ADDR_14	(0x2 << 24)
#define S5PC1XX_MP01_7_EBI_ADDR_15	(0x2 << 28)
