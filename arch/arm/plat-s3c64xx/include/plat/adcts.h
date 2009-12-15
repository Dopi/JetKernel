/* arch/arm/plat-s3c/include/plat/adc.h
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_ADCTS_H
#define __ASM_ARCH_ADCTS_H __FILE__

#include <plat/regs-adc.h>

enum s3c_adcts_type {
	ADC_TYPE_0,
	ADC_TYPE_1,	/* S3C2416, S3C2450 */
	ADC_TYPE_2,	/* S3C64XX, S5PC1XX */
};

#define MAX_SCAN_TIME	(32)
#define TS_CHANNEL	(8)
#define MAX_ADC_CHANNEL	(8)

enum s3c_adcts_state {
	TS_STATUS_UP=0,
	TS_STATUS_DOWN_NOW,
	TS_STATUS_DOWN,
};

struct s3c_adcts_channel_info {
        int delay;
        int presc;
        int resol;	/* S3C_ADCCON_RESSEL_10BIT or S3C_ADCCON_RESSEL_12BIT */
};

struct s3c_adcts_plat_info {
	struct s3c_adcts_channel_info channel[MAX_ADC_CHANNEL];
};

struct s3c_adcts_value{
	unsigned int	status;			/* 0:UP 1:DOWN NOW 2:DOWN */
	unsigned int	xp[MAX_SCAN_TIME];
	unsigned int	yp[MAX_SCAN_TIME];
};

extern struct s3c_ts_mach_info;
extern int s3c_adcts_register_ts (struct s3c_ts_mach_info *ts,
			void (*done_callback)(struct s3c_adcts_value *ts_value));
extern int s3c_adcts_unregister_ts (void);
extern int s3c_adc_get_adc_data(int channel);

extern void __init s3c_adcts_set_platdata(struct s3c_adcts_plat_info *pd);
#endif /* __ASM_ARCH_ADCTS_H */
