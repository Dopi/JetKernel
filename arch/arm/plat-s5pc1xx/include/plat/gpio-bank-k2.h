/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-k2.h
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

#define S5PC1XX_GPK2CON			(S5PC1XX_GPK2_BASE + 0x00)
#define S5PC1XX_GPK2DAT			(S5PC1XX_GPK2_BASE + 0x04)
#define S5PC1XX_GPK2PUD			(S5PC1XX_GPK2_BASE + 0x08)
#define S5PC1XX_GPK2DRV			(S5PC1XX_GPK2_BASE + 0x0c)
#define S5PC1XX_GPK2CONPDN		(S5PC1XX_GPK2_BASE + 0x10)
#define S5PC1XX_GPK2PUDPDN		(S5PC1XX_GPK2_BASE + 0x14)

#define S5PC1XX_GPK2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPK2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPK2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPK2_0_NF_CLE		(0x3 << 0)
#define S5PC1XX_GPK2_0_ONAND_ADDRVALID	(0x5 << 0)

#define S5PC1XX_GPK2_1_NF_ALE		(0x3 << 4)
#define S5PC1XX_GPK2_1_ONAND_SMCLK	(0x5 << 4)

#define S5PC1XX_GPK2_2_NF_FWEn		(0x3 << 8)
#define S5PC1XX_GPK2_2_ONAND_RPn	(0x5 << 8)

#define S5PC1XX_GPK2_3_NF_FREn		(0x3 << 12)

#define S5PC1XX_GPK2_4_NF_RnB_0		(0x2 << 16)
#define S5PC1XX_GPK2_4_ONAND_INT_0	(0x5 << 16)

#define S5PC1XX_GPK2_5_NF_RnB_1		(0x2 << 20)
#define S5PC1XX_GPK2_5_ONAND_INT_1	(0x5 << 20)

#define S5PC1XX_GPK2_6_NF_RnB_2		(0x2 << 24)

#define S5PC1XX_GPK2_7_NF_RnB_3		(0x2 << 28)
