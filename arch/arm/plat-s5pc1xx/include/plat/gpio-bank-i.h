/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-i.h
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

#define S5PC1XX_GPICON			(S5PC1XX_GPI_BASE + 0x00)
#define S5PC1XX_GPIDAT			(S5PC1XX_GPI_BASE + 0x04)
#define S5PC1XX_GPIPUD			(S5PC1XX_GPI_BASE + 0x08)
#define S5PC1XX_GPIDRV			(S5PC1XX_GPI_BASE + 0x0c)
#define S5PC1XX_GPICONPDN		(S5PC1XX_GPI_BASE + 0x10)
#define S5PC1XX_GPIPUDPDN		(S5PC1XX_GPI_BASE + 0x14)

#define S5PC1XX_GPI_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPI_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPI_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPI0_IEM_SCLK		(0x2 << 0)
#define S5PC1XX_GPI0_GPIO_INT15_0	(0xf << 0)

#define S5PC1XX_GPI1_IEM_SPWI		(0x2 << 4)
#define S5PC1XX_GPI1_GPIO_INT15_1	(0xf << 4)

#define S5PC1XX_GPI2_BOOT_OPT_0		(0x2 << 8)
#define S5PC1XX_GPI2_GPIO_INT15_2	(0xf << 8)

#define S5PC1XX_GPI3_BOOT_OPT_1		(0x2 << 12)
#define S5PC1XX_GPI3_GPIO_INT15_3	(0xf << 12)

#define S5PC1XX_GPI4_BOOT_OPT_2		(0x2 << 16)
#define S5PC1XX_GPI4_GPIO_INT15_4	(0xf << 16)

#define S5PC1XX_GPI5_BOOT_OPT_3		(0x2 << 20)
#define S5PC1XX_GPI5_GPIO_INT15_5	(0xf << 20)

#define S5PC1XX_GPI6_BOOT_OPT_4		(0x2 << 24)
#define S5PC1XX_GPI6_GPIO_INT15_6	(0xf << 24)

#define S5PC1XX_GPI7_BOOT_OPT_5		(0x2 << 28)
#define S5PC1XX_GPI7_GPIO_INT15_7	(0xf << 28)

