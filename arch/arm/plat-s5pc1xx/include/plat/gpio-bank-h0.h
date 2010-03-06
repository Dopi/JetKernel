/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-h0.h
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

#define S5PC1XX_GPH0CON			(S5PC1XX_GPH0_BASE + 0x00)
#define S5PC1XX_GPH0DAT			(S5PC1XX_GPH0_BASE + 0x04)
#define S5PC1XX_GPH0PUD			(S5PC1XX_GPH0_BASE + 0x08)
#define S5PC1XX_GPH0DRV			(S5PC1XX_GPH0_BASE + 0x0c)
#define S5PC1XX_GPH0CONPDN		(S5PC1XX_GPH0_BASE + 0x10)
#define S5PC1XX_GPH0PUDPDN		(S5PC1XX_GPH0_BASE + 0x14)

#define S5PC1XX_GPH0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPH0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPH0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPH0_0_WAKEUP_INT_0	(0x2 << 0)
#define S5PC1XX_GPH0_1_WAKEUP_INT_1	(0x2 << 4)
#define S5PC1XX_GPH0_2_WAKEUP_INT_2	(0x2 << 8)
#define S5PC1XX_GPH0_3_WAKEUP_INT_3	(0x2 << 12)
#define S5PC1XX_GPH0_4_WAKEUP_INT_4	(0x2 << 16)
#define S5PC1XX_GPH0_5_WAKEUP_INT_5	(0x2 << 20)
#define S5PC1XX_GPH0_6_WAKEUP_INT_6	(0x2 << 24)
#define S5PC1XX_GPH0_7_WAKEUP_INT_7	(0x2 << 28)
