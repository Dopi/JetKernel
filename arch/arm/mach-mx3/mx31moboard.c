/*
 *  Copyright (C) 2008 Valentin Longchamp, EPFL Mobots group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/init.h>

#include <linux/platform_device.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/partitions.h>
#include <linux/memory.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <mach/common.h>
#include <mach/imx-uart.h>
#include <mach/iomux-mx3.h>

#include "devices.h"

static struct physmap_flash_data mx31moboard_flash_data = {
	.width  	= 2,
};

static struct resource mx31moboard_flash_resource = {
	.start	= 0xa0000000,
	.end	= 0xa1ffffff,
	.flags	= IORESOURCE_MEM,
};

static struct platform_device mx31moboard_flash = {
	.name	= "physmap-flash",
	.id	= 0,
	.dev	= {
		.platform_data  = &mx31moboard_flash_data,
	},
	.resource = &mx31moboard_flash_resource,
	.num_resources = 1,
};

static struct imxuart_platform_data uart_pdata = {
	.flags = IMXUART_HAVE_RTSCTS,
};

static struct platform_device *devices[] __initdata = {
	&mx31moboard_flash,
};

/*
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	platform_add_devices(devices, ARRAY_SIZE(devices));

	mxc_iomux_mode(MX31_PIN_CTS1__CTS1);
	mxc_iomux_mode(MX31_PIN_RTS1__RTS1);
	mxc_iomux_mode(MX31_PIN_TXD1__TXD1);
	mxc_iomux_mode(MX31_PIN_RXD1__RXD1);

	mxc_register_device(&mxc_uart_device0, &uart_pdata);

	mxc_iomux_mode(MX31_PIN_CTS2__CTS2);
	mxc_iomux_mode(MX31_PIN_RTS2__RTS2);
	mxc_iomux_mode(MX31_PIN_TXD2__TXD2);
	mxc_iomux_mode(MX31_PIN_RXD2__RXD2);

	mxc_register_device(&mxc_uart_device1, &uart_pdata);

	mxc_iomux_mode(MX31_PIN_PC_RST__CTS5);
	mxc_iomux_mode(MX31_PIN_PC_VS2__RTS5);
	mxc_iomux_mode(MX31_PIN_PC_BVD2__TXD5);
	mxc_iomux_mode(MX31_PIN_PC_BVD1__RXD5);

	mxc_register_device(&mxc_uart_device4, &uart_pdata);
}

/*
 * This structure defines static mappings for the mx31moboard.
 */
static struct map_desc mx31moboard_io_desc[] __initdata = {
	{
		.virtual	= AIPS1_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(AIPS1_BASE_ADDR),
		.length		= AIPS1_SIZE,
		.type		= MT_DEVICE_NONSHARED
	}, {
		.virtual	= AIPS2_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(AIPS2_BASE_ADDR),
		.length		= AIPS2_SIZE,
		.type		= MT_DEVICE_NONSHARED
	},
};

/*
 * Set up static virtual mappings.
 */
void __init mx31moboard_map_io(void)
{
	mxc_map_io();
	iotable_init(mx31moboard_io_desc, ARRAY_SIZE(mx31moboard_io_desc));
}

static void __init mx31moboard_timer_init(void)
{
	mxc_clocks_init(26000000);
	mxc_timer_init("ipg_clk.0");
}

struct sys_timer mx31moboard_timer = {
	.init	= mx31moboard_timer_init,
};

MACHINE_START(MX31MOBOARD, "EPFL Mobots mx31moboard")
	/* Maintainer: Valentin Longchamp, EPFL Mobots group */
	.phys_io	= AIPS1_BASE_ADDR,
	.io_pg_offst	= ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params    = PHYS_OFFSET + 0x100,
	.map_io         = mx31moboard_map_io,
	.init_irq       = mxc_init_irq,
	.init_machine   = mxc_board_init,
	.timer          = &mx31moboard_timer,
MACHINE_END

