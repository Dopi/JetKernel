/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-g3.h
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

#define S5PC1XX_GPG3CON			(S5PC1XX_GPG3_BASE + 0x00)
#define S5PC1XX_GPG3DAT			(S5PC1XX_GPG3_BASE + 0x04)
#define S5PC1XX_GPG3PUD			(S5PC1XX_GPG3_BASE + 0x08)
#define S5PC1XX_GPG3DRV			(S5PC1XX_GPG3_BASE + 0x0c)
#define S5PC1XX_GPG3CONPDN		(S5PC1XX_GPG3_BASE + 0x10)
#define S5PC1XX_GPG3PUDPDN		(S5PC1XX_GPG3_BASE + 0x14)

#define S5PC1XX_GPG3_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPG3_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPG3_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPG3_0_SD_2_CLK		(0x2 << 0)
#define S5PC1XX_GPG3_0_SPI_2_CLK	(0x3 << 0)
#define S5PC1XX_GPG3_0_I2S_2_SCLK	(0x4 << 0)
#define S5PC1XX_GPG3_0_PCM_0_SCLK	(0x5 << 0)
#define S5PC1XX_GPG3_0_GPIO_INT14_0	(0xf << 0)

#define S5PC1XX_GPG3_1_SD_2_CMD		(0x2 << 4)
#define S5PC1XX_GPG3_1_SPI_2_nSS	(0x3 << 4)
#define S5PC1XX_GPG3_1_I2S_2_CDCLK	(0x4 << 4)
#define S5PC1XX_GPG3_1_PCM_0_EXTCLK	(0x5 << 4)
#define S5PC1XX_GPG3_1_GPIO_INT14_1	(0xf << 4)

#define S5PC1XX_GPG3_2_SD_2_DATA0	(0x2 << 8)
#define S5PC1XX_GPG3_2_SPI_2_MISO	(0x3 << 8)
#define S5PC1XX_GPG3_2_I2S_2_LRCK	(0x4 << 8)
#define S5PC1XX_GPG3_2_PCM_0_FSYNC	(0x5 << 8)
#define S5PC1XX_GPG3_2_GPIO_INT14_2	(0xf << 8)

#define S5PC1XX_GPG3_3_SD_2_DATA1	(0x2 << 12)
#define S5PC1XX_GPG3_3_SPI_2_MOSI	(0x3 << 12)
#define S5PC1XX_GPG3_3_I2S_2_SDI	(0x4 << 12)
#define S5PC1XX_GPG3_3_PCM_0_SIN	(0x5 << 12)
#define S5PC1XX_GPG3_3_GPIO_INT14_3	(0xf << 12)

#define S5PC1XX_GPG3_4_SD_2_DATA2	(0x2 << 16)
#define S5PC1XX_GPG3_4_I2S_2_SDO	(0x4 << 16)
#define S5PC1XX_GPG3_4_PCM_0_SOUT	(0x5 << 16)
#define S5PC1XX_GPG3_4_GPIO_INT14_4	(0xf << 16)

#define S5PC1XX_GPG3_5_SD_2_DATA3	(0x2 << 20)
#define S5PC1XX_GPG3_5_SPDIF0_OUT	(0x5 << 20)
#define S5PC1XX_GPG3_5_GPIO_INT14_5	(0xf << 20)

#define S5PC1XX_GPG3_6_SD_2_CDn		(0x2 << 24)
#define S5PC1XX_GPG3_6_SPDIF_EXTCLK	(0x5 << 24)
#define S5PC1XX_GPG3_6_GPIO_INT14_6	(0xf << 24)

