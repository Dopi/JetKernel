/* linux/arch/arm/mach-s5pc100/cpu.c
 *
 * Copyright 2008 Samsung Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <plat/cpu-freq.h>
#include <plat/regs-serial.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/sdhci.h>
#include <plat/iic-core.h>
#include <plat/s5pc100.h>
#include <mach/map.h>

/* Initial IO mappings */

static struct map_desc s5pc100_iodesc[] __initdata = {
#if 0
	IODESC_ENT(LCD),
	IODESC_ENT(SROMC),
	IODESC_ENT(HOSTIFB),
#endif
};

/* s5pc100_map_io
 *
 * register the standard cpu IO areas
*/

void __init s5pc100_map_io(void)
{
	iotable_init(s5pc100_iodesc, ARRAY_SIZE(s5pc100_iodesc));

#if 0
	/* initialise device information early */
	s3c6410_default_sdhci0();
	s3c6410_default_sdhci1();

	/* the i2c devices are directly compatible with s3c2440 */
	s3c_i2c0_setname("s3c2440-i2c");
	s3c_i2c1_setname("s3c2440-i2c");
#endif
}

void __init s5pc100_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initialising clocks\n", __func__);
	s3c24xx_register_baseclocks(xtal);
	s5pc1xx_register_clocks();
	s5pc100_register_clocks();
	s5pc100_setup_clocks();
}

void __init s5pc100_init_irq(void)
{
	/* VIC0, VIC1, and VIC2 are fully populated. */
	s5pc1xx_init_irq(~0, ~0, ~0);
}

struct sysdev_class s5pc100_sysclass = {
	.name	= "s5pc100-core",
};

static struct sys_device s5pc100_sysdev = {
	.cls	= &s5pc100_sysclass,
};

static int __init s5pc100_core_init(void)
{
	return sysdev_class_register(&s5pc100_sysclass);
}

core_initcall(s5pc100_core_init);

int __init s5pc100_init(void)
{
	printk("S5PC100: Initialising architecture\n");

	return sysdev_register(&s5pc100_sysdev);
}
