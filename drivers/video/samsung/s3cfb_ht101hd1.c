/* linux/drivers/video/samsung/s3cfb_ht101hd1.c
 *
 * HT101HD1-100 XWVGA Display Panel Support
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s3cfb.h"

static struct s3cfb_lcd ht101hd1 = {
	.width = 1366,
	.height = 768,
	.bpp = 24,
	.freq = 60,

	.timing = {
		.h_fp = 1,
		.h_bp = 1,
		.h_sw = 33,
		.v_fp = 1,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 4,
	},

	.polarity = {
		.rise_vclk = 0,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	ht101hd1.init_ldi = NULL;
	ctrl->lcd = &ht101hd1;
}

