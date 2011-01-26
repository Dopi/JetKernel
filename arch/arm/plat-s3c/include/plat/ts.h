/* linux/arch/arm/plat-s3c/include/plat/ts.h
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_TS_H
#define __ASM_ARCH_TS_H __FILE__

struct s3c_ts_mach_info {
	struct s3c_adcts_channel_info adcts;
	int             	sampling_time;
	int			remove_max_min_sampling;
	int			sampling_interval_ms;
	int			screen_size_x;
	int			screen_size_y;
	int			use_tscal;
	int			tscal[7];
};

extern void __init s3c_ts_set_platdata(struct s3c_ts_mach_info *pd);

#endif /* __ASM_ARCH_TS_H */
