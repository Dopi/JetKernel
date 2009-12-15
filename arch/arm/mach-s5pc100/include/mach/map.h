/* linux/arch/arm/mach-s5pc100/include/mach/map.h
 *
 * Copyright 2008 Samsung Electronics Co.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5PC1XX - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>

#define S3C_PA_UART		(0xEC000000)
#define S3C_PA_UART0		(S3C_PA_UART + 0x00)
#define S3C_PA_UART1		(S3C_PA_UART + 0x400)
#define S3C_PA_UART2		(S3C_PA_UART + 0x800)
#define S3C_PA_UART3		(S3C_PA_UART + 0xC00)
#define S3C_UART_OFFSET		(0x400)

/* See notes on UART VA mapping in debug-macro.S */
#define S3C_VA_UARTx(x)		(S3C_VA_UART + (S3C_PA_UART & 0xfffff) + ((x) * S3C_UART_OFFSET))

#define S3C_VA_UART0		S3C_VA_UARTx(0)
#define S3C_VA_UART1		S3C_VA_UARTx(1)
#define S3C_VA_UART2		S3C_VA_UARTx(2)
#define S3C_VA_UART3		S3C_VA_UARTx(3)

#define S5PC1XX_PA_SYSCON	(0xE0100000)
#define S5PC1XX_PA_TIMER	(0xEA000000)
#define S5PC1XX_PA_IIC0		(0xEC100000)
#define S5PC1XX_PA_IIC1		(0xEC200000)

#define S5PC1XX_PA_GPIO		(0xE0300000)
#define S5PC1XX_VA_GPIO		S3C_ADDR(0x00500000)
#define S5PC1XX_SZ_GPIO		SZ_4K

#define S5PC1XX_PA_SDRAM	(0x20000000)
#define S5PC1XX_PA_VIC0		(0xE4000000)
#define S5PC1XX_PA_VIC1		(0xE4100000)
#define S5PC1XX_PA_VIC2		(0xE4200000)

#define S5PC1XX_VA_SROMC	S3C_VA_SROMC
#define S5PC1XX_PA_SROMC	(0xE7000000)
#define S5PC1XX_SZ_SROMC	SZ_4K

#define S5PC1XX_VA_LCD	   	S3C_VA_LCD
#define S5PC1XX_PA_LCD	   	(0xEE000000)
#define S5PC1XX_SZ_LCD		SZ_1M

#define S5PC1XX_PA_ADC		(0xF3000000)

#define S5PC1XX_PA_IIS	   	(0xF2000000)
#define S3C_SZ_IIS		SZ_4K

#define S5PC1XX_PA_CHIPID	(0xE0000000)
#define S5PC1XX_VA_CHIPID	S3C_ADDR(0x00700000)

/* DMA controller */
#define S5PC1XX_PA_DMA   	(0xE8000000)

/* place VICs close together */
#define S3C_VA_VIC0		(S3C_VA_IRQ + 0x0)
#define S3C_VA_VIC1		(S3C_VA_IRQ + 0x10000)
#define S3C_VA_VIC2		(S3C_VA_IRQ + 0x20000)

/* compatibiltiy defines. */
#define S3C_PA_TIMER		S5PC1XX_PA_TIMER
#define S3C_PA_IIC		S5PC1XX_PA_IIC0
#define S3C_PA_IIC1		S5PC1XX_PA_IIC1

#define S3C_PA_IIS		S5PC1XX_PA_IIS
#define S3C_PA_ADC		S5PC1XX_PA_ADC
#define S3C_PA_LCD		S5PC1XX_PA_LCD

#endif /* __ASM_ARCH_6400_MAP_H */
