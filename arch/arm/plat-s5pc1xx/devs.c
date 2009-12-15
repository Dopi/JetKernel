/* linux/arch/arm/plat-s3c64xx/devs.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Base S3C64XX resource and device definitions
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
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/devs.h>

#if 0
/* SMC9115 LAN via ROM interface */

static struct resource s3c_smc911x_resources[] = {
      [0] = {
              .start  = S3C64XX_PA_SMC9115,
              .end    = S3C64XX_PA_SMC9115 + 0x1fffffff,
              .flags  = IORESOURCE_MEM,
      },
      [1] = {
              .start = IRQ_EINT(10),
              .end   = IRQ_EINT(10),
              .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_smc911x = {
      .name           = "smc911x",
      .id             =  -1,
      .num_resources  = ARRAY_SIZE(s3c_smc911x_resources),
      .resource       = s3c_smc911x_resources,
};
#endif

