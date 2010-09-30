/*
 *  linux/arch/arm/plat-s3c64xx/s3c64xx-cpufreq.c
 *
 *  CPU frequency scaling for S3C64XX
 *
 *  Copyright (C) 2008 Samsung Electronics
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/i2c/pmic.h>
#include <plat/s3c64xx-dvfs.h>
#include <asm/system.h>

#include "dprintk.h"

#define dvfs_dev_dbg(fmt, ...)	dprintk(DVFS_DBG, fmt, ##__VA_ARGS__)
#define VOLTAGE_TABLE_SEL(x, i)	(x == DVFS_BGM_SET ? voltage_table_BGM[i] :\
		voltage_table[i])

/* Frequency Table. */
static struct cpufreq_frequency_table s3c6410_freq_table[] = {
	{ 0, 800*KHZ_T },
	{ 1, 400*KHZ_T },
	{ 2, 266*KHZ_T },
	{ 3, 133*KHZ_T },
	{ 4,  66*KHZ_T },
	{ 5, CPUFREQ_TABLE_END },
};

/* Normal Table: index / ARMCLK / VDD_ARM / VDD_INT */
static const unsigned int voltage_table[][4] = {
	{ 0, 800*KHZ_T, 1350, 1250 },	/* L0 */
	{ 1, 400*KHZ_T, 1150, 1250 },	/* L1 */
	{ 2, 266*KHZ_T, 1100, 1250 },	/* L2 */
	{ 3, 133*KHZ_T, 1050, 1250 },	/* L3 */
	{ 4,  66*KHZ_T, 1050, 1100 },	/* L4 */
};

/* BGM Table: index / ARMCLK / VDD_ARM / VDD_INT */
static const unsigned int voltage_table_BGM[][4] = {
	{ 0, 800*KHZ_T, 1350, 1250 },	/* L0 */
	{ 1, 400*KHZ_T, 1150, 1250 },	/* L1 */
	{ 2, 266*KHZ_T, 1100, 1250 },	/* L2 */
	{ 3, 133*KHZ_T, 1050, 1100 },	/* L3-BGM */
	{ 4,  66*KHZ_T, 1050, 1100 },	/* L4 */
};

/* global variables. */
static DEFINE_MUTEX(dvfs_lock);
static struct __s3c64xx_dvfs_data dvfs_priv_info = {
	.performance = 0,   /* performance level (800MHz) flag. */
	.vdd_arm = 1,       /* VDD_ARM voltage minimum limitation. */
	.bgm_mode = 0,      /* mp3 play mode. */
};

int bgm_sync_indicator = 0;

/* ------------------------------------------------- */
/*           E X P O R T  F U N C T I O N S          */
/* ------------------------------------------------- */
void set_dvfs_perf_level(int set)
{
	dvfs_priv_info.performance = set;
}

void get_dvfs_info(struct __s3c64xx_dvfs_data *pdata)
{
	pdata->performance = dvfs_priv_info.performance;

	mutex_lock(&dvfs_lock);
	pdata->vdd_arm = dvfs_priv_info.vdd_arm;
	pdata->bgm_mode = dvfs_priv_info.bgm_mode;
	mutex_unlock(&dvfs_lock);
}

void set_dvfs_vdd_arm_level(int level)
{
	if (level >= 1 && level <= DVFS_MAX_LEVEL) {
		mutex_lock(&dvfs_lock);
		dvfs_priv_info.vdd_arm = level;
		mutex_unlock(&dvfs_lock);

		pr_info("%s- level: %d.\n", __func__, level);
	}
}

void set_dvfs_bgm_mode(int mode)
{
	if (mode == DVFS_BGM_UNSET || mode == DVFS_BGM_SET) {
		mutex_lock(&dvfs_lock);
		dvfs_priv_info.bgm_mode = mode;
		mutex_unlock(&dvfs_lock);

		pr_info("%s- set: %d.\n", __func__, mode);
	}
}

unsigned int get_cpuspeed_by_level(int level)
{
	return s3c6410_freq_table[DVFS_MAX_LEVEL - level].frequency;
}

EXPORT_SYMBOL(set_dvfs_perf_level);
EXPORT_SYMBOL(get_dvfs_info);
EXPORT_SYMBOL(set_dvfs_vdd_arm_level);
EXPORT_SYMBOL(set_dvfs_bgm_mode);
EXPORT_SYMBOL(get_cpuspeed_by_level);

/* .. */
int s3c6410_verify_speed(struct cpufreq_policy *policy)
{
#ifndef USE_FREQ_TABLE
	struct clk *mpu_clk;
#endif	/* USE_FREQ_TABLE */

	if (policy->cpu)
		return -EINVAL;

#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_verify(policy, s3c6410_freq_table);
#else	/* USE_FREQ_TABLE */

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	policy->min = clk_round_rate(mpu_clk, policy->min * KHZ_T) / KHZ_T;
	policy->max = clk_round_rate(mpu_clk, policy->max * KHZ_T) / KHZ_T;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);

	clk_put(mpu_clk);

	return 0;
#endif
}

unsigned int s3c6410_getspeed(unsigned int cpu)
{
	struct clk * mpu_clk;
	unsigned long rate;

	/* single cpu */
	if (cpu)
		return 0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;
	clk_put(mpu_clk);

	return rate;
}

#ifdef USE_DVS
int pmic_scale_voltage(int freq_index)
{
	int vcc_arm, vcc_int;
	int arm_voltage, int_voltage;

	const unsigned int *p_vol_table;

	if (get_pmic_voltage(VCC_ARM, &vcc_arm) < 0)
		return -1;
	if (get_pmic_voltage(VCC_INT, &vcc_int) < 0)
		return -1;

	p_vol_table = VOLTAGE_TABLE_SEL(bgm_sync_indicator, freq_index);

	arm_voltage = p_vol_table[2];
	int_voltage = p_vol_table[3];

	if (vcc_arm != arm_voltage)
		set_pmic_voltage(VCC_ARM, arm_voltage);

	if (vcc_int != int_voltage)
		set_pmic_voltage(VCC_INT, int_voltage);

	udelay(50);

	return 0;
}
#endif	/* USE_DVS */

static int s3c6410_target(struct cpufreq_policy *policy,
		unsigned int target_freq, unsigned int relation)
{
	static int prev_index = -1;

	struct clk * mpu_clk;
	struct cpufreq_freqs freqs;

	unsigned long arm_clk;
	unsigned int index;

	const unsigned int *p_vol_table;
	int bgm_policy_changed = 0;
	int ret = 0;

	/* indicator sync. */
	mutex_lock(&dvfs_lock);
	if (bgm_sync_indicator != dvfs_priv_info.bgm_mode)
		bgm_policy_changed = 1;

	bgm_sync_indicator = dvfs_priv_info.bgm_mode;
	mutex_unlock(&dvfs_lock);

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	freqs.old = s3c6410_getspeed(0);
	if (freqs.old == s3c6410_freq_table[0].frequency)
		prev_index = 0;

	/* find index */
	if (cpufreq_frequency_table_target(policy, s3c6410_freq_table,
				target_freq, relation, &index))
		return -EINVAL;

	if (prev_index == index && !bgm_policy_changed)
		return 0;

	arm_clk = s3c6410_freq_table[index].frequency;
	freqs.new = arm_clk;
	freqs.cpu = 0;

	target_freq = arm_clk;
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

#ifdef USE_DVS
	if (freqs.new < freqs.old) {
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if (ret != 0) {
			printk("s3c6410_target: frequency scaling error!\n");
			return -EINVAL;
		}

		pmic_scale_voltage(index);
	} else {
		pmic_scale_voltage(index);

		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if (ret != 0) {
			printk("s3c6410_target: frequency scaling error!\n");
			return -EINVAL;
		}
	}
#else
	ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);

	if (ret != 0) {
		return -EINVAL;
	}
#endif	/* USE_DVS */

	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);

	prev_index = index;
	clk_put(mpu_clk);

	/* information message. */
	p_vol_table = VOLTAGE_TABLE_SEL(bgm_sync_indicator, index);
	dvfs_dev_dbg("%3dMHz / %dmV / %dmV\n", p_vol_table[1]/KHZ_T,
			p_vol_table[2],
			p_vol_table[3]);

	return ret;
}

static int __init s3c6410_cpu_init(struct cpufreq_policy *policy)
{
	struct clk * mpu_clk;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = policy->min = policy->max = s3c6410_getspeed(0);

#ifdef USE_FREQ_TABLE
	cpufreq_frequency_table_get_attr(s3c6410_freq_table, policy->cpu);
#else
	policy->cpuinfo.min_freq = clk_round_rate(mpu_clk, 0) / KHZ_T;
	policy->cpuinfo.max_freq = clk_round_rate(mpu_clk, VERY_HI_RATE) / KHZ_T;
#endif

	policy->cpuinfo.transition_latency = 40000;	//1us
	clk_put(mpu_clk);

#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_cpuinfo(policy, s3c6410_freq_table);
#else
	return 0;
#endif
}

static struct cpufreq_driver s3c6410_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s3c6410_verify_speed,
	.target		= s3c6410_target,
	.get		= s3c6410_getspeed,
	.init		= s3c6410_cpu_init,
	.name		= "s3c6410",
};

static int __init s3c6410_cpufreq_init(void)
{
	return cpufreq_register_driver(&s3c6410_driver);
}

arch_initcall(s3c6410_cpufreq_init);

