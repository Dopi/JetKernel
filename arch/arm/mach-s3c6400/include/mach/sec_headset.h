/*
 * Copyright (C) 2008 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_SEC_HEADSET_H
#define __ASM_ARCH_SEC_HEADSET_H

enum {
	SEC_HEADSET_NO_DEVICE		= 0,
	SEC_HEADSET_NORMAL_HEADSET	= 1,
	SEC_HEADSET_SEC_HEADSET		= 2,
};

struct sec_gpio_info 
{
	int	eint;
	int	gpio;
	int	gpio_af;
	int	low_active;
};

struct sec_headset_port
{
	struct sec_gpio_info	det_headset;
	struct sec_gpio_info	send_end;
};

struct sec_headset_platform_data
{
	struct sec_headset_port	*port;
	int			nheadsets;
};

#endif
