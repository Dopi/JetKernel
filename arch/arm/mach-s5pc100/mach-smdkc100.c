/* linux/arch/arm/mach-s3c6410/mach-smdk6410.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>

#include <plat/s5pc100.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>


#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

static struct s3c2410_uartcfg smdk6410_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = 0x3c5,
		.ulcon	     = 0x03,
		.ufcon	     = 0x51,
	},
};

struct map_desc smdkc100_iodesc[] = {};

static struct platform_device *smdkc100_devices[] __initdata = {
};

static void __init smdkc100_map_io(void)
{
	s5pc1xx_init_io(smdkc100_iodesc, ARRAY_SIZE(smdkc100_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdk6410_uartcfgs, ARRAY_SIZE(smdk6410_uartcfgs));
}

static void __init smdkc100_machine_init(void)
{
	platform_add_devices(smdkc100_devices, ARRAY_SIZE(smdkc100_devices));
}

MACHINE_START(SMDKC100, "SMDKC100")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5PC1XX_PA_SDRAM + 0x100,

	.init_irq	= s5pc100_init_irq,
	.map_io		= smdkc100_map_io,
	.init_machine	= smdkc100_machine_init,
	.timer		= &s3c24xx_timer,
MACHINE_END
