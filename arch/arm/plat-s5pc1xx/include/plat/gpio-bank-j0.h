/* linux/arch/arm/plat-s5pc1xx/include/plat/gpio-bank-j0.h
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

#define S5PC1XX_GPJ0CON			(S5PC1XX_GPJ0_BASE + 0x00)
#define S5PC1XX_GPJ0DAT			(S5PC1XX_GPJ0_BASE + 0x04)
#define S5PC1XX_GPJ0PUD			(S5PC1XX_GPJ0_BASE + 0x08)
#define S5PC1XX_GPJ0DRV			(S5PC1XX_GPJ0_BASE + 0x0c)
#define S5PC1XX_GPJ0CONPDN		(S5PC1XX_GPJ0_BASE + 0x10)
#define S5PC1XX_GPJ0PUDPDN		(S5PC1XX_GPJ0_BASE + 0x14)

#define S5PC1XX_GPJ0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5PC1XX_GPJ0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5PC1XX_GPJ0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5PC1XX_GPJ0_0_MSM_ADDR_0	(0x2 << 0)
#define S5PC1XX_GPJ0_0_HSI_TX_DATA	(0x3 << 0)
#define S5PC1XX_GPJ0_0_CF_ADDR_0	(0x4 << 0)
#define S5PC1XX_GPJ0_0_GPIO_INT16_0	(0xf << 0)

#define S5PC1XX_GPJ0_1_MSM_ADDR_1	(0x2 << 4)
#define S5PC1XX_GPJ0_1_HSI_TX_FLAG	(0x3 << 4)
#define S5PC1XX_GPJ0_1_CF_ADDR_1	(0x4 << 4)
#define S5PC1XX_GPJ0_1_GPIO_INT16_1	(0xf << 4)

#define S5PC1XX_GPJ0_2_MSM_ADDR_2	(0x2 << 8)
#define S5PC1XX_GPJ0_2_HSI_TX_WAKE	(0x3 << 8)
#define S5PC1XX_GPJ0_2_CF_ADDR_2	(0x4 << 8)
#define S5PC1XX_GPJ0_2_GPIO_INT16_2	(0xf << 8)

#define S5PC1XX_GPJ0_3_MSM_ADDR_3	(0x2 << 12)
#define S5PC1XX_GPJ0_3_HSI_TX_READY	(0x3 << 12)
#define S5PC1XX_GPJ0_3_CF_IORDY		(0x4 << 12)
#define S5PC1XX_GPJ0_3_GPIO_INT16_3	(0xf << 12)

#define S5PC1XX_GPJ0_4_MSM_ADDR_4	(0x2 << 16)
#define S5PC1XX_GPJ0_4_HSI_RX_DATA	(0x3 << 16)
#define S5PC1XX_GPJ0_4_CF_INTRQ		(0x4 << 16)
#define S5PC1XX_GPJ0_4_GPIO_INT16_4	(0xf << 16)

#define S5PC1XX_GPJ0_5_MSM_ADDR_5	(0x2 << 20)
#define S5PC1XX_GPJ0_5_HSI_RX_FLAG	(0x3 << 20)
#define S5PC1XX_GPJ0_5_CF_INPACKn	(0x4 << 20)
#define S5PC1XX_GPJ0_5_GPIO_INT16_5	(0xf << 20)

#define S5PC1XX_GPJ0_6_MSM_ADDR_6	(0x2 << 24)
#define S5PC1XX_GPJ0_6_HSI_RX_WAKE	(0x3 << 24)
#define S5PC1XX_GPJ0_6_CF_RESET		(0x4 << 24)
#define S5PC1XX_GPJ0_6_GPIO_INT16_6	(0xf << 24)

#define S5PC1XX_GPJ0_7_MSM_ADDR_7	(0x2 << 28)
#define S5PC1XX_GPJ0_7_HSI_RX_READY	(0x3 << 28)
#define S5PC1XX_GPJ0_7_CF_REG		(0x4 << 28)
#define S5PC1XX_GPJ0_7_GPIO_INT16_7	(0xf << 28)

