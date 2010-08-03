/* linux/arch/arm/plat-s3c/include/plat/ts.h
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/earlysuspend.h>

#ifndef __ASM_ARCH_TS_H
#define __ASM_ARCH_TS_H __FILE__


enum s3c_adc_type {
	ADC_TYPE_0,
	ADC_TYPE_1,	/* S3C2416, S3C2450 */
	ADC_TYPE_2,	/* S3C64XX, S5PC1XX */
};

struct s3c_ts_mach_info {
	int             	delay;
	int             	presc;
	int             	oversampling_shift;
	int			resol_bit;
	enum s3c_adc_type	s3c_adc_con;
	int			panel_resistance;
	int			threshold;
};

struct s3c_ts_info {
	struct input_dev 	*dev;
	long 			xp;
	long 			yp;
	int 			count;
	int 			shift;
	char 			phys[32];
	int			resol_bit;
	int			resistance;
	int			pressure;
	int			threshold_pressure;
	enum s3c_adc_type	s3c_adc_con;
	struct early_suspend	early_suspend;
	int			ts_switch_claimed;	
};

extern void __init s3c_ts_set_platdata(struct s3c_ts_mach_info *pd);

#endif /* __ASM_ARCH_TS_H */
