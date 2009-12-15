/* linux/arch/arm/plat-s5pc1xx/cpu.c
 *
 * Copyright 2008 Samsung Electonics Co.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5PC1XX CPU Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/regs-serial.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/s5pc100.h>

/* table of supported CPUs */

static const char name_s5pc100[] = "S5PC100";

static struct cpu_table cpu_ids[] __initdata = {
	{
		.idcode		= 0x43100000,
		.idmask		= 0xfffffff0,
		.map_io		= s5pc100_map_io,
		.init_clocks    = s5pc100_init_clocks,
		.init_uarts	= s5pc100_init_uarts,
		.init		= s5pc100_init,
		.name		= name_s5pc100
	},
};

/* minimal IO mapping */

/* see notes on uart map in arch/arm/mach-s3c6400/include/mach/debug-macro.S */
#define UART_OFFS (S3C_PA_UART & 0xfffff)

static struct map_desc s3c_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)S3C_VA_SYS,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_SYSCON),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C_VA_UART + UART_OFFS),
		.pfn		= __phys_to_pfn(S3C_PA_UART),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC0,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_VIC0),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC1,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_VIC1),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC2,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_VIC2),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_TIMER,
		.pfn		= __phys_to_pfn(S3C_PA_TIMER),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PC1XX_VA_GPIO,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_GPIO),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5PC1XX_VA_CHIPID,
		.pfn		= __phys_to_pfn(S5PC1XX_PA_CHIPID),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

/* read cpu identification code */

void __init s5pc1xx_init_io(struct map_desc *mach_desc, int size)
{
	unsigned long idcode;

	/* initialise the io descriptors we need for initialisation */
	iotable_init(s3c_iodesc, ARRAY_SIZE(s3c_iodesc));
	iotable_init(mach_desc, size);

	idcode = __raw_readl(S5PC1XX_VA_CHIPID);
	s3c_init_cpu(idcode, cpu_ids, ARRAY_SIZE(cpu_ids));
}
