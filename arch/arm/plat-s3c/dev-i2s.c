/* linux/arch/arm/plat-s3c/dev-i2s.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for hsmmc devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/map.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <asm/irq.h>


/* IIS */
static struct resource s3c_iis_resource[] = {
	[0] = {
		.start = S3C_PA_IIS,
		.end   = S3C_PA_IIS + S3C_SZ_IIS -1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_S3C6410_IIS,
		.end   = IRQ_S3C6410_IIS,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_iis_dmamask = 0xffffffffUL;

struct platform_device s3c_device_iis = {
	.name		  = "s3c-iis",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_iis_resource),
	.resource	  = s3c_iis_resource,
	.dev              = {
		.dma_mask = &s3c_device_iis_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

