/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-k1.h
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

#define S5PC1XX_GPK1CON			(S5PC1XX_GPK1_BASE + 0x00)
#define S5PC1XX_GPK1DAT			(S5PC1XX_GPK1_BASE + 0x04)
#define S5PC1XX_GPK1PUD			(S5PC1XX_GPK1_BASE + 0x08)
#define S5PC1XX_GPK1DRV			(S5PC1XX_GPK1_BASE + 0x0c)
#define S5PC1XX_GPK1CONPDN		(S5PC1XX_GPK1_BASE + 0x10)
#define S5PC1XX_GPK1PUDPDN		(S5PC1XX_GPK1_BASE + 0x14)

#define S5PC1XX_GPK1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPK1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPK1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPK1_0_EBI_BEn0		(0x2 << 0)
#define S5PC1XX_GPK1_1_EBI_BEn1		(0x2 << 4)
#define S5PC1XX_GPK1_2_SROM_WAITn	(0x2 << 8)
#define S5PC1XX_GPK1_3_EBI_DATA_RDn	(0x2 << 12)
#define S5PC1XX_GPK1_4_CF_OEn		(0x2 << 16)
#define S5PC1XX_GPK1_5_CF_WEn		(0x2 << 20)

