/*
 * linux/arch/arm/mach-omap2/memory.c
 *
 * Memory timing related functions for OMAP24XX
 *
 * Copyright (C) 2005 Texas Instruments Inc.
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * Copyright (C) 2005 Nokia Corporation
 * Tony Lindgren <tony@atomide.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/common.h>
#include <mach/clock.h>
#include <mach/sram.h>

#include "prm.h"

#include "memory.h"
#include "sdrc.h"

void __iomem *omap2_sdrc_base;
void __iomem *omap2_sms_base;

static struct memory_timings mem_timings;
static u32 curr_perf_level = CORE_CLK_SRC_DPLL_X2;

u32 omap2_memory_get_slow_dll_ctrl(void)
{
	return mem_timings.slow_dll_ctrl;
}

u32 omap2_memory_get_fast_dll_ctrl(void)
{
	return mem_timings.fast_dll_ctrl;
}

u32 omap2_memory_get_type(void)
{
	return mem_timings.m_type;
}

/*
 * Check the DLL lock state, and return tue if running in unlock mode.
 * This is needed to compensate for the shifted DLL value in unlock mode.
 */
u32 omap2_dll_force_needed(void)
{
	/* dlla and dllb are a set */
	u32 dll_state = sdrc_read_reg(SDRC_DLLA_CTRL);

	if ((dll_state & (1 << 2)) == (1 << 2))
		return 1;
	else
		return 0;
}

/*
 * 'level' is the value to store to CM_CLKSEL2_PLL.CORE_CLK_SRC.
 * Practical values are CORE_CLK_SRC_DPLL (for CORE_CLK = DPLL_CLK) or
 * CORE_CLK_SRC_DPLL_X2 (for CORE_CLK = * DPLL_CLK * 2)
 */
u32 omap2_reprogram_sdrc(u32 level, u32 force)
{
	u32 dll_ctrl, m_type;
	u32 prev = curr_perf_level;
	unsigned long flags;

	if ((curr_perf_level == level) && !force)
		return prev;

	if (level == CORE_CLK_SRC_DPLL) {
		dll_ctrl = omap2_memory_get_slow_dll_ctrl();
	} else if (level == CORE_CLK_SRC_DPLL_X2) {
		dll_ctrl = omap2_memory_get_fast_dll_ctrl();
	} else {
		return prev;
	}

	m_type = omap2_memory_get_type();

	local_irq_save(flags);
	__raw_writel(0xffff, OMAP24XX_PRCM_VOLTSETUP);
	omap2_sram_reprogram_sdrc(level, dll_ctrl, m_type);
	curr_perf_level = level;
	local_irq_restore(flags);

	return prev;
}

#if !defined(CONFIG_ARCH_OMAP2)
void omap2_sram_ddr_init(u32 *slow_dll_ctrl, u32 fast_dll_ctrl,
				u32 base_cs, u32 force_unlock)
{
}
void omap2_sram_reprogram_sdrc(u32 perf_level, u32 dll_val,
				      u32 mem_type)
{
}
#endif

void omap2_init_memory_params(u32 force_lock_to_unlock_mode)
{
	unsigned long dll_cnt;
	u32 fast_dll = 0;

	mem_timings.m_type = !((sdrc_read_reg(SDRC_MR_0) & 0x3) == 0x1); /* DDR = 1, SDR = 0 */

	/* 2422 es2.05 and beyond has a single SIP DDR instead of 2 like others.
	 * In the case of 2422, its ok to use CS1 instead of CS0.
	 */
	if (cpu_is_omap2422())
		mem_timings.base_cs = 1;
	else
		mem_timings.base_cs = 0;

	if (mem_timings.m_type != M_DDR)
		return;

	/* With DDR we need to determine the low frequency DLL value */
	if (((mem_timings.fast_dll_ctrl & (1 << 2)) == M_LOCK_CTRL))
		mem_timings.dll_mode = M_UNLOCK;
	else
		mem_timings.dll_mode = M_LOCK;

	if (mem_timings.base_cs == 0) {
		fast_dll = sdrc_read_reg(SDRC_DLLA_CTRL);
		dll_cnt = sdrc_read_reg(SDRC_DLLA_STATUS) & 0xff00;
	} else {
		fast_dll = sdrc_read_reg(SDRC_DLLB_CTRL);
		dll_cnt = sdrc_read_reg(SDRC_DLLB_STATUS) & 0xff00;
	}
	if (force_lock_to_unlock_mode) {
		fast_dll &= ~0xff00;
		fast_dll |= dll_cnt;		/* Current lock mode */
	}
	/* set fast timings with DLL filter disabled */
	mem_timings.fast_dll_ctrl = (fast_dll | (3 << 8));

	/* No disruptions, DDR will be offline & C-ABI not followed */
	omap2_sram_ddr_init(&mem_timings.slow_dll_ctrl,
			    mem_timings.fast_dll_ctrl,
			    mem_timings.base_cs,
			    force_lock_to_unlock_mode);
	mem_timings.slow_dll_ctrl &= 0xff00;	/* Keep lock value */

	/* Turn status into unlock ctrl */
	mem_timings.slow_dll_ctrl |=
		((mem_timings.fast_dll_ctrl & 0xF) | (1 << 2));

	/* 90 degree phase for anything below 133Mhz + disable DLL filter */
	mem_timings.slow_dll_ctrl |= ((1 << 1) | (3 << 8));
}

void __init omap2_set_globals_memory(struct omap_globals *omap2_globals)
{
	omap2_sdrc_base = omap2_globals->sdrc;
	omap2_sms_base = omap2_globals->sms;
}

/* turn on smart idle modes for SDRAM scheduler and controller */
void __init omap2_init_memory(void)
{
	u32 l;

	if (!cpu_is_omap2420())
		return;

	l = sms_read_reg(SMS_SYSCONFIG);
	l &= ~(0x3 << 3);
	l |= (0x2 << 3);
	sms_write_reg(l, SMS_SYSCONFIG);

	l = sdrc_read_reg(SDRC_SYSCONFIG);
	l &= ~(0x3 << 3);
	l |= (0x2 << 3);
	sdrc_write_reg(l, SDRC_SYSCONFIG);
}
