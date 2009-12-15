/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h1.h
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

#define S5PC1XX_GPH1CON			(S5PC1XX_GPH1_BASE + 0x00)
#define S5PC1XX_GPH1DAT			(S5PC1XX_GPH1_BASE + 0x04)
#define S5PC1XX_GPH1PUD			(S5PC1XX_GPH1_BASE + 0x08)
#define S5PC1XX_GPH1DRV			(S5PC1XX_GPH1_BASE + 0x0c)
#define S5PC1XX_GPH1CONPDN		(S5PC1XX_GPH1_BASE + 0x10)
#define S5PC1XX_GPH1PUDPDN		(S5PC1XX_GPH1_BASE + 0x14)

#define S5PC1XX_GPH1_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPH1_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPH1_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPH1_0_WAKEUP_INT_8	(0x2 << 0)

#define S5PC1XX_GPH1_1_WAKEUP_INT_9	(0x2 << 4)

#define S5PC1XX_GPH1_2_WAKEUP_INT_10	(0x2 << 8)
#define S5PC1XX_GPH1_2_CG_REALIN	(0x3 << 8)

#define S5PC1XX_GPH1_3_WAKEUP_INT_11	(0x2 << 12)
#define S5PC1XX_GPH1_3_CG_IMGIN		(0x3 << 12)

#define S5PC1XX_GPH1_4_WAKEUP_INT_12	(0x2 << 16)
#define S5PC1XX_GPH1_4_CG_GPO0		(0x3 << 16)

#define S5PC1XX_GPH1_5_WAKEUP_INT_13	(0x2 << 20)
#define S5PC1XX_GPH1_5_CG_GPO1		(0x3 << 20)

#define S5PC1XX_GPH1_6_WAKEUP_INT_14	(0x2 << 24)
#define S5PC1XX_GPH1_6_CG_GPO2		(0x3 << 24)

#define S5PC1XX_GPH1_7_WAKEUP_INT_15	(0x2 << 28)
#define S5PC1XX_GPH1_7_CG_GPO3		(0x3 << 28)

