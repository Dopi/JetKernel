/*
 * linux/arch/arm/plat-s3c64xx/include/plat/s3c64xx-dvfs.h
 *
 * Geun-Young, Kim. <nenggun.kim@samsung.com>
 *
 * Copyright (c) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __S3C64XX_DVFS_H__
#define __S3C64XX_DVFS_H__

/* definitions. */
#define USE_FREQ_TABLE
#define USE_DVS

#define VERY_HI_RATE	(800 * 1000 * 1000)
#define KHZ_T		1000
#define MPU_CLK		"clk_cpu"

#define DVFS_BGM_UNSET	0
#define DVFS_BGM_SET	1
#define DVFS_MAX_LEVEL	5

/* platform specific data. */
struct __s3c64xx_dvfs_data {
	int performance;
	int vdd_arm;
	int bgm_mode;
};

/* prototypes. */
extern void set_dvfs_perf_level(int set);
extern void get_dvfs_info(struct __s3c64xx_dvfs_data *pdata);
extern void set_dvfs_vdd_arm_level(int level);
extern void set_dvfs_bgm_mode(int mode);
extern unsigned int get_cpuspeed_by_level(int level);

#endif	/* __S3C64XX_DVFS_H__ */

