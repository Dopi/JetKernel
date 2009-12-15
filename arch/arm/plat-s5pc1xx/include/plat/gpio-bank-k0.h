/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-k0.h
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

#define S5PC1XX_GPK0CON			(S5PC1XX_GPK0_BASE + 0x00)
#define S5PC1XX_GPK0DAT			(S5PC1XX_GPK0_BASE + 0x04)
#define S5PC1XX_GPK0PUD			(S5PC1XX_GPK0_BASE + 0x08)
#define S5PC1XX_GPK0DRV			(S5PC1XX_GPK0_BASE + 0x0c)
#define S5PC1XX_GPK0CONPDN		(S5PC1XX_GPK0_BASE + 0x10)
#define S5PC1XX_GPK0PUDPDN		(S5PC1XX_GPK0_BASE + 0x14)

#define S5PC1XX_GPK0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPK0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPK0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPK0_0_SROM_CSn0	(0x2 << 0)

#define S5PC1XX_GPK0_1_SROM_CSn1	(0x2 << 4)

#define S5PC1XX_GPK0_2_SROM_CSn2	(0x2 << 8)
#define S5PC1XX_GPK0_2_NF_CSn0		(0x3 << 8)
#define S5PC1XX_GPK0_2_ONENAND_CSn0	(0x5 << 8)

#define S5PC1XX_GPK0_3_SROM_CSn3	(0x2 << 12)
#define S5PC1XX_GPK0_3_NF_CSn1		(0x3 << 12)
#define S5PC1XX_GPK0_3_ONENAND_CSn1	(0x5 << 12)

#define S5PC1XX_GPK0_4_SROM_CSn4	(0x2 << 16)
#define S5PC1XX_GPK0_4_NF_CSn2		(0x3 << 16)
#define S5PC1XX_GPK0_4_CF_CSn0		(0x4 << 16)

#define S5PC1XX_GPK0_5_SROM_CSn5	(0x2 << 20)
#define S5PC1XX_GPK0_5_NF_CSn3		(0x3 << 20)
#define S5PC1XX_GPK0_5_CF_CSn1		(0x4 << 20)

#define S5PC1XX_GPK0_6_EBI_OEn		(0x2 << 24)

#define S5PC1XX_GPK0_7_EBI_WEn		(0x2 << 28)
