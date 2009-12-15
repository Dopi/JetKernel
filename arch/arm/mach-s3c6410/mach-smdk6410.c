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
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/gpio-core.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>

#include <plat/regs-rtc.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/pm.h>

#if defined(CONFIG_USB_GADGET_S3C_OTGD) || defined(CONFIG_USB_OHCI_HCD)
#include <plat/regs-otg.h>
#endif


#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE
extern struct sys_timer s3c64xx_timer;	// hskang
static struct s3c2410_uartcfg smdk6410_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
};

struct map_desc smdk6410_iodesc[] = {};

static struct platform_device *smdk6410_devices[] __initdata = {
#ifdef CONFIG_S3C6410_SD_CH0
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C6410_SD_CH1
	&s3c_device_hsmmc1,
#endif
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_ts,
	//&s3c_device_adc,
	&s3c_device_smc911x,
	&s3c_device_lcd,
	&s3c_device_nand,
	&s3c_device_keypad,
        &s3c_device_usbgadget,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
//	{ I2C_BOARD_INFO("WM8580", 0x1b), },
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	/* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("WM8580", 0x1b), },
};


static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
                .delay 			= 10000,
                .presc 			= 49,
                .oversampling_shift	= 2,
		.resol_bit 		= 12,
		.s3c_cpu_type		= TYPE_S3C6410_TS,
};

static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s3c6410 support 12-bit resolution */
	.delay		= 10000,
	.presc 		= 49,
	.resolution 	= 12,
};

static void __init smdk6410_map_io(void)
{
	s3c_device_nand.name = "s3c6410-nand";

	s3c64xx_init_io(smdk6410_iodesc, ARRAY_SIZE(smdk6410_iodesc));
	s3c64xx_gpiolib_init();
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(smdk6410_uartcfgs, ARRAY_SIZE(smdk6410_uartcfgs));
}

static void __init smdk6410_smc911x_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S3C64XX_SROM_BW);
	tmp &=~(0xF<<4);
	tmp |= (1<<7) | (1<<6) | (1<<4);
	__raw_writel(tmp, S3C64XX_SROM_BW);

	__raw_writel((0x0<<28)|(0x4<<24)|(0xd<<16)|(0x1<<12)|(0x4<<8)|(0x6<<4)|(0x0<<0), S3C64XX_SROM_BC1);
}

static void __init smdk6410_machine_init(void)
{
	s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;
	
	smdk6410_smc911x_set();

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

	s3c_ts_set_platdata(&s3c_ts_platform);
//	s3c_adc_set_platdata(&s3c_adc_platform);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	platform_add_devices(smdk6410_devices, ARRAY_SIZE(smdk6410_devices));
	s3c6410_pm_init();
}

MACHINE_START(SMDK6410, "SMDK6410")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,

	.init_irq	= s3c6410_init_irq,
	.map_io		= smdk6410_map_io,
	.init_machine	= smdk6410_machine_init,
	.timer		= &s3c64xx_timer,
MACHINE_END

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	if (port == 0) {
	}
	else if (port == 1) {
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
