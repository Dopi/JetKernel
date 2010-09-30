/*
 * s3c6410 block power manager implementation.
 *
 * Copyright (c) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/power-domain.h>

//#define DEBUG_POWER_DOMAIN

#ifdef DEBUG_POWER_DOMAIN
static int domain_hash_map[15] = {0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 7};
static char *domain_name[] = {"G", "V", "I", "P", "F", "S", "ETM", "IROM"}; 
#endif	/* DEBUG_POWER_DOMAIN */

/* global variables. */
static unsigned int s3c_domain_off_stat = 0x7FFF;
static spinlock_t power_lock;

void s3c_init_domain_power(void)
{
	spin_lock_init(&power_lock);

#ifdef CONFIG_DOMAIN_GATING
	s3c_set_normal_cfg(S3C64XX_DOMAIN_G, S3C64XX_LP_MODE, S3C64XX_3D);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_V, S3C64XX_LP_MODE, S3C64XX_MFC);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_JPEG);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_I, S3C64XX_LP_MODE, S3C64XX_CAMERA);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_2D);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_TVENC);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_SCALER);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_ROT);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_LP_MODE, S3C64XX_POST);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA0);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SDMA1);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_S, S3C64XX_LP_MODE, S3C64XX_SECURITY);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_IROM, S3C64XX_LP_MODE, S3C64XX_IROM);
#endif	/* CONFIG_DOMAIN_GATING */

	/* LCD on. */
	s3c_set_normal_cfg(S3C64XX_DOMAIN_F, S3C64XX_ACTIVE_MODE, S3C64XX_LCD);

	/* ETM on. */
	s3c_set_normal_cfg(S3C64XX_DOMAIN_ETM, S3C64XX_ACTIVE_MODE, S3C64XX_ETM);
}

unsigned int get_domain_off_stat(void)
{
	return s3c_domain_off_stat;
}

int domain_off_check(unsigned int config)
{
	if(config == S3C64XX_DOMAIN_V) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_V_MASK)	
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_G) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_G_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_I) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_I_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_P) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_P_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_F) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_F_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_S) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_S_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_ETM) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_ETM_MASK)
			return 0;
	}
	else if(config == S3C64XX_DOMAIN_IROM) {
		if(s3c_domain_off_stat & S3C64XX_DOMAIN_IROM_MASK)
			return 0;
	}
	return 1;
}
EXPORT_SYMBOL(domain_off_check);

void s3c_set_normal_cfg(unsigned int config, unsigned int flag, unsigned int deviceID)
{
	unsigned int normal_cfg;
	int power_off_flag = 0;

	spin_lock(&power_lock);
	normal_cfg = __raw_readl(S3C_NORMAL_CFG);

	if(flag == S3C64XX_ACTIVE_MODE) {
		s3c_domain_off_stat |= (1 << deviceID);

		if(!(normal_cfg & config)) {
			normal_cfg |= (config);
			__raw_writel(normal_cfg, S3C_NORMAL_CFG);

#ifdef DEBUG_POWER_DOMAIN
			printk("===== Domain-%s Power ON NORMAL_CFG : %x \n",
					domain_name[domain_hash_map[deviceID]], normal_cfg);
#endif	/* DEBUG_POWER_DOMAIN */
		}

	}

	else if(flag == S3C64XX_LP_MODE) {
		s3c_domain_off_stat &= (~( 1 << deviceID));
		power_off_flag = domain_off_check(config);

		if(power_off_flag == 1) {
			if(normal_cfg & config) {
				normal_cfg &= (~config);
				__raw_writel(normal_cfg, S3C_NORMAL_CFG);

#ifdef DEBUG_POWER_DOMAIN
				printk("===== Domain-%s Power OFF NORMAL_CFG : %x \n",
						domain_name[domain_hash_map[deviceID]], normal_cfg);
#endif	/* DEBUG_POWER_DOMAIN */
			}
		}
	}

	spin_unlock(&power_lock);
}
EXPORT_SYMBOL(s3c_set_normal_cfg);

int s3c_wait_blk_pwr_ready(unsigned int config)
{
	unsigned int blk_pwr_stat;
	int timeout;
	int ret = 0;

	/* Wait max 20 ms */
	timeout = 20;
	while (!((blk_pwr_stat = __raw_readl(S3C_BLK_PWR_STAT)) & config)) {
		if (timeout == 0) {
			printk(KERN_ERR "config %x: blk power never ready.\n", config);
			ret = 1;
			goto s3c_wait_blk_pwr_ready_end;
		}
		timeout--;
		mdelay(1);
	}
s3c_wait_blk_pwr_ready_end:
	return ret;
}
EXPORT_SYMBOL(s3c_wait_blk_pwr_ready);

