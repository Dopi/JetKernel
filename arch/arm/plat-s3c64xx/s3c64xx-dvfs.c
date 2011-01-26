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
#include <linux/i2c/pmic.h>

#include <asm/system.h>
#include <plat/s3c64xx-dvfs.h>

unsigned int S3C64XX_MAXFREQLEVEL = 3;
static unsigned int s3c64xx_cpufreq_level = 3;
unsigned int s3c64xx_cpufreq_index = 0;
static spinlock_t dvfs_lock;

#define CLIP_LEVEL(a, b) (a > b ? b : a)

static struct cpufreq_frequency_table freq_table_532MHz[] = {
	{0, 532*KHZ_T},
	{1, 266*KHZ_T},
	{2, 133*KHZ_T},
#ifdef USE_DVFS_AL1_LEVEL
	{3, 133*KHZ_T},
	{4, 66*KHZ_T},
	{5, CPUFREQ_TABLE_END},		
#else
	{3, 66*KHZ_T},
	{4, CPUFREQ_TABLE_END},		
#endif /* USE_DVFS_AL1_LEVEL */
};

static struct cpufreq_frequency_table freq_table_800MHz[] = {
	{0, 800*KHZ_T},
	{1, 400*KHZ_T},
	{2, 266*KHZ_T},	
	{3, 133*KHZ_T},
#ifdef USE_DVFS_AL1_LEVEL
	{4, 133*KHZ_T},
	{5, (66)*KHZ_T},
	{6, CPUFREQ_TABLE_END},
#else
	{4, (66)*KHZ_T},
	{5, CPUFREQ_TABLE_END},
#endif /* USE_DVFS_AL1_LEVEL */
};

static unsigned char transition_state_800MHz[][2] = {
	{1, 0},
	{2, 0},
	{3, 1},
	{4, 2},
#ifdef USE_DVFS_AL1_LEVEL
	{5, 3},
	{5, 4},
#else
	{4, 3},
#endif /* USE_DVFS_AL1_LEVEL */
};

static unsigned char transition_state_532MHz[][2] = {
	{1, 0},
	{2, 0},
	{3, 1},
#ifdef USE_DVFS_AL1_LEVEL
	{4, 2},
	{4, 3},		
#else
	{3, 2},
#endif /* USE_DVFS_AL1_LEVEL */
};

/* frequency voltage matching table */
static const unsigned int frequency_match_532MHz[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
	{532000, 1100, 1250, 0},
	{266000, 1100, 1250, 1},
	{133000, 1000, 1250, 2},
#ifdef USE_DVFS_AL1_LEVEL
	{133000, 1050, 1050, 3},
	{66000, 1050, 1050, 4},
#else
	{66000, 1050, 1050, 3},
#endif /* USE_DVFS_AL1_LEVEL */
};

/* frequency voltage matching table */
static const unsigned int frequency_match_800MHz[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
	{800000, 1350, 1250, 0},
	{400000, 1150, 1250, 1},
	{266000, 1100, 1250, 2},
	{133000, 1050, 1250, 3},
#ifdef USE_DVFS_AL1_LEVEL
	{133000, 1050, 1050, 4},
	{66000, 1050, 1050, 5},
#else
	{66000, 1050, 1050, 4},
#endif /* USE_DVFS_AL1_LEVEL */
};

extern int is_pmic_initialized(void);
static const unsigned int (*frequency_match[2])[4] = {
	frequency_match_532MHz,
	frequency_match_800MHz,
};

static unsigned char (*transition_state[2])[2] = {
	transition_state_532MHz,
	transition_state_800MHz,
};

static struct cpufreq_frequency_table *s3c6410_freq_table[] = {
	freq_table_532MHz,
	freq_table_800MHz,
};

int set_max_freq_flag = 0;
int dvfs_change_quick = 0;
void set_dvfs_perf_level(void)
{
	unsigned long flag;
	if(spin_trylock_irqsave(&dvfs_lock, flag)){

		/* if some user event (keypad, touchscreen) occur, freq will be raised to 532MHz */
		/* maximum frequency :532MHz(0), 266MHz(1) */
		s3c64xx_cpufreq_index = 0;
	dvfs_change_quick = 1;
		spin_unlock_irqrestore(&dvfs_lock,flag);
	}
}
EXPORT_SYMBOL(set_dvfs_perf_level);

static int dvfs_level_count = 0;
void set_dvfs_level(int flag)
{
//to do
	unsigned long irq_flags;
#if 1
	if(set_max_freq_flag){
	  dvfs_level_count = (flag == 0)?(dvfs_level_count + 1):(dvfs_level_count - 1);
	  return;
	}	
	if(spin_trylock_irqsave(&dvfs_lock,irq_flags)){	
	if(flag == 0) {
		if (dvfs_level_count > 0) {
			dvfs_level_count++;	
			spin_unlock_irqrestore(&dvfs_lock,irq_flags);
			return;
		}
#ifdef USE_DVFS_AL1_LEVEL
		s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL - 2;
#else
		s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL - 1;
#endif /* USE_DVFS_AL1_LEVEL */
		dvfs_level_count++;
	}
	else {
		if (dvfs_level_count > 1) {
			dvfs_level_count--;
				spin_unlock_irqrestore(&dvfs_lock,irq_flags);
			return;
		}
			s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL;
		dvfs_level_count--;
	}
		spin_unlock_irqrestore(&dvfs_lock,irq_flags);
	}
#endif
}
EXPORT_SYMBOL(set_dvfs_level);

#ifdef USE_DVS
int get_voltage(pmic_pm_type pm_type)
{
	int volatge = 0;
	if((pm_type == VCC_ARM) || (pm_type == VCC_INT))
		get_pmic(pm_type, &volatge);

	return volatge;
}

int set_voltage(unsigned int freq_index)
{
	static int index = 0;
	unsigned int arm_voltage, int_voltage;
	unsigned int vcc_arm, vcc_int;
	unsigned int arm_delay, int_delay, delay;
	
	if(index == freq_index)
		return 0;
		
	index = freq_index;
	
	vcc_arm = get_voltage(VCC_ARM);
	vcc_int = get_voltage(VCC_INT);
	
	arm_voltage = frequency_match[S3C64XX_FREQ_TAB][index][1];
	int_voltage = frequency_match[S3C64XX_FREQ_TAB][index][2];
	
	arm_delay = ((abs(vcc_arm - arm_voltage) / 50) * 5) + 10;
	int_delay = ((abs(vcc_int - int_voltage) / 50) * 5) + 10;
	
	delay = arm_delay > int_delay ? arm_delay : int_delay;

	if(arm_voltage != vcc_arm) {
		set_pmic(VCC_ARM, arm_voltage);
	}
	if(int_voltage != vcc_int) {
		set_pmic(VCC_INT, int_voltage);
	}

	udelay(delay);

	return 0;
}
#endif	/* USE_DVS */

unsigned int s3c64xx_target_frq(unsigned int pred_freq, 
				int flag)
{
	int index; 
	unsigned int freq;
	struct cpufreq_frequency_table *freq_tab = s3c6410_freq_table[S3C64XX_FREQ_TAB];

	spin_lock(&dvfs_lock);

	if(freq_tab[0].frequency < pred_freq) {
	   index = 0;	
	   goto s3c64xx_target_frq_end;
	}

	if((flag != 1)&&(flag != -1)) {
		printk(KERN_ERR "s3c64xx_target_frq: flag error!!!!!!!!!!!!!");
	}

	index = s3c64xx_cpufreq_index;
	
	if(freq_tab[index].frequency == pred_freq) {	
		if(flag == 1)
			index = transition_state[S3C64XX_FREQ_TAB][index][1];
		else
			index = transition_state[S3C64XX_FREQ_TAB][index][0];
	}
	else if(flag == -1) {
		index = 1;
	}
	else {
		index = 0; 
	}
s3c64xx_target_frq_end:
	index = CLIP_LEVEL(index, s3c64xx_cpufreq_level);
	s3c64xx_cpufreq_index = index;
	
	freq = freq_tab[index].frequency;
	spin_unlock(&dvfs_lock);
	return freq;
}

int s3c64xx_target_freq_index(unsigned int freq)
{
	int index = 0;
	
	struct cpufreq_frequency_table *freq_tab = s3c6410_freq_table[S3C64XX_FREQ_TAB];

	if(freq >= freq_tab[index].frequency) {
		goto s3c64xx_target_freq_index_end;
	}

	/*Index might have been calculated before calling this function.
	check and early return if it is already calculated*/
	if(freq_tab[s3c64xx_cpufreq_index].frequency == freq) {		
		return s3c64xx_cpufreq_index;
	}

	while((freq < freq_tab[index].frequency) &&
			(freq_tab[index].frequency != CPUFREQ_TABLE_END)) {
		index++;
	}

	if(index > 0) {
		if(freq != freq_tab[index].frequency) {
			index--;
		}
	}

	if(freq_tab[index].frequency == CPUFREQ_TABLE_END) {
		index--;
	}

s3c64xx_target_freq_index_end:
	spin_lock(&dvfs_lock);	
	index = CLIP_LEVEL(index, s3c64xx_cpufreq_level);
	spin_unlock(&dvfs_lock);
	s3c64xx_cpufreq_index = index;
	
	return index; 
} 

int s3c6410_verify_speed(struct cpufreq_policy *policy)
{
	if(policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s3c6410_freq_table[S3C64XX_FREQ_TAB]);
}

extern unsigned long s3c_fclk_get_rate(void);
unsigned int s3c6410_getspeed(unsigned int cpu)
{
	struct clk * mpu_clk;
	unsigned long rate;

	if(cpu)
		return 0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return 0;

	rate = s3c_fclk_get_rate() / KHZ_T;
	clk_put(mpu_clk);

	return rate;
}

static int s3c6410_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct clk * mpu_clk;
	struct cpufreq_freqs freqs;
	static int prevIndex = 0;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index;

	if(!is_pmic_initialized())
		return ret;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if(IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	freqs.old = s3c6410_getspeed(0);

	if(freqs.old == s3c6410_freq_table[S3C64XX_FREQ_TAB][0].frequency) {
		prevIndex = 0;
	}
	
	index = s3c64xx_target_freq_index(target_freq);
	if(index == INDX_ERROR) {
		printk(KERN_ERR "s3c6410_target: INDX_ERROR \n");
		return -EINVAL;
	}
	
	if(prevIndex == index)
		return ret;

	arm_clk = s3c6410_freq_table[S3C64XX_FREQ_TAB][index].frequency;
	freqs.new = arm_clk;
	freqs.cpu = 0;
	freqs.new_hclk = 133000;
  
	if(index > S3C64XX_MAXFREQLEVEL) {
		freqs.new_hclk = 66000;         
	} 

	target_freq = arm_clk;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

#ifdef USE_DVS
	if(prevIndex < index) {
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk(KERN_ERR "frequency scaling error\n");
			ret = -EINVAL;
			goto s3c6410_target_end;
		}
		/* voltage scaling */
		set_voltage(index);
	}
	else {
		/* voltage scaling */
		set_voltage(index);
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk(KERN_ERR "frequency scaling error\n");
			ret = -EINVAL;
			goto s3c6410_target_end;
		}
	}
#else
	ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
	if(ret != 0) {
		printk(KERN_ERR "frequency scaling error\n");
		ret = -EINVAL;
		goto s3c6410_target_end;
	}
#endif	/* USE_DVS */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	prevIndex = index;
	clk_put(mpu_clk);
s3c6410_target_end:
	return ret;
}

void dvfs_set_max_freq_lock(void)
{
	//Interrupts must be enabled when this function is called.
   //there may be race..but no problem
	//don't use locks...locks might cause soft lockup here
   struct cpufreq_frequency_table *freq_tab = s3c6410_freq_table[S3C64XX_FREQ_TAB];
	set_max_freq_flag = 1;
	s3c64xx_cpufreq_level = 0;
	s3c6410_target(NULL, freq_tab[0].frequency, 1);
	dvfs_change_quick = 1;   //better to have this flag because we are not using locks. 
	return; 
	
}

void dvfs_set_max_freq_unlock(void)
{
        set_max_freq_flag = 0;
	if (dvfs_level_count > 0) {
#ifdef USE_DVFS_AL1_LEVEL
		s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL - 2;
#else
		s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL - 1;
#endif /* USE_DVFS_AL1_LEVEL */
	}
	else {
		s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL;
	}
	return;

}

unsigned int get_min_cpufreq(void)
{
	return (s3c6410_freq_table[S3C64XX_FREQ_TAB][S3C64XX_MAXFREQLEVEL].frequency);
}

static int __init s3c6410_cpu_init(struct cpufreq_policy *policy)
{
	struct clk * mpu_clk;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if(IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if(policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s3c6410_getspeed(0);

	if(policy->max == MAXIMUM_FREQ) {
		S3C64XX_FREQ_TAB = 1;
#ifdef USE_DVFS_AL1_LEVEL
		S3C64XX_MAXFREQLEVEL = 5;
#else
		S3C64XX_MAXFREQLEVEL = 4;
#endif /* USE_DVFS_AL1_LEVEL */
	}
	else {
		S3C64XX_FREQ_TAB = 0;
#ifdef USE_DVFS_AL1_LEVEL
		S3C64XX_MAXFREQLEVEL = 4;
#else
		S3C64XX_MAXFREQLEVEL = 3;
#endif /* USE_DVFS_AL1_LEVEL */
	}
	s3c64xx_cpufreq_level = S3C64XX_MAXFREQLEVEL;

	cpufreq_frequency_table_get_attr(s3c6410_freq_table[S3C64XX_FREQ_TAB], policy->cpu);

	policy->cpuinfo.transition_latency = 10000;

	clk_put(mpu_clk);

	spin_lock_init(&dvfs_lock);

	return cpufreq_frequency_table_cpuinfo(policy, s3c6410_freq_table[S3C64XX_FREQ_TAB]);
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
