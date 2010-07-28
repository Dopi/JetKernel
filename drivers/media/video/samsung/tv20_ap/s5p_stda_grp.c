/* linux/drivers/media/video/samsung/tv20/s5p_stda_grp.c
 *
 * Graphic Layer ftn. file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioctl.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "s5p_tv.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_GRP_DEBUG 1
#endif

#ifdef S5P_GRP_DEBUG
#define GRPPRINTK(fmt, args...)	\
	printk("\t[GRP] %s: " fmt, __FUNCTION__ , ## args)
#else
#define GRPPRINTK(fmt, args...)
#endif


bool _s5p_grp_start(s5p_tv_vmx_layer vm_layer)
{
	s5p_tv_vmx_err merr;
	s5p_tv_status *st = &s5ptv_status;

	if (!(st->grp_layer_enable[0] || st->grp_layer_enable[1])) {
		
		merr = __s5p_vm_init_status_reg(st->grp_burst,
					st->grp_endian);

		if (merr != VMIXER_NO_ERROR) {
			return false;
		}
	}

	merr = __s5p_vm_init_layer(vm_layer,
			  true,
			  s5ptv_overlay[vm_layer].win_blending,
			  s5ptv_overlay[vm_layer].win.global_alpha,
			  s5ptv_overlay[vm_layer].priority,
			  s5ptv_overlay[vm_layer].fb.fmt.pixelformat,
			  s5ptv_overlay[vm_layer].blank_change,
			  s5ptv_overlay[vm_layer].pixel_blending,
			  s5ptv_overlay[vm_layer].pre_mul,
			  s5ptv_overlay[vm_layer].blank_color,
			  s5ptv_overlay[vm_layer].base_addr,
			  s5ptv_overlay[vm_layer].fb.fmt.bytesperline,
			  s5ptv_overlay[vm_layer].win.w.width,
			  s5ptv_overlay[vm_layer].win.w.height,
			  s5ptv_overlay[vm_layer].win.w.left,
			  s5ptv_overlay[vm_layer].win.w.top,
			  s5ptv_overlay[vm_layer].dst_rect.left,
			  s5ptv_overlay[vm_layer].dst_rect.top);

	if (merr != VMIXER_NO_ERROR) {
		GRPPRINTK("can't initialize layer(%d)\n\r", merr);
		return false;
	}

	__s5p_vm_start();


	st->grp_layer_enable[vm_layer] = true;

	GRPPRINTK("()\n\r");

	return true;
}

bool _s5p_grp_stop(s5p_tv_vmx_layer vm_layer)
{
	s5p_tv_vmx_err merr;
	s5p_tv_status *st = &s5ptv_status;

	GRPPRINTK("()\n\r");

	merr = __s5p_vm_set_layer_show(vm_layer, false);

	if (merr != VMIXER_NO_ERROR) {
		return false;
	}

	merr = __s5p_vm_set_layer_priority(vm_layer, 0);

	if (merr != VMIXER_NO_ERROR) {
		return false;
	}

	__s5p_vm_start();


	st->grp_layer_enable[vm_layer] = false;

	GRPPRINTK("()\n\r");

	return true;
}
