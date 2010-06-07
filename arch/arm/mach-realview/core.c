/*
 *  linux/arch/arm/mach-realview/core.c
 *
 *  Copyright (C) 1999 - 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/io.h>
#include <linux/smc911x.h>

#include <asm/clkdev.h>
#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/icst307.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/mmc.h>

#include <asm/hardware/gic.h>

#include "core.h"
#include "clock.h"

#define REALVIEW_REFCOUNTER	(__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_24MHz_OFFSET)

/* used by entry-macro.S and platsmp.c */
void __iomem *gic_cpu_base_addr;

/*
 * This is the RealView sched_clock implementation.  This has
 * a resolution of 41.7ns, and a maximum value of about 179s.
 */
unsigned long long sched_clock(void)
{
	unsigned long long v;

	v = (unsigned long long)readl(REALVIEW_REFCOUNTER) * 125;
	do_div(v, 3);

	return v;
}


#define REALVIEW_FLASHCTRL    (__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_FLASH_OFFSET)

static int realview_flash_init(void)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);

	return 0;
}

static void realview_flash_exit(void)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);
}

static void realview_flash_set_vpp(int on)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	if (on)
		val |= REALVIEW_FLASHPROG_FLVPPEN;
	else
		val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);
}

static struct flash_platform_data realview_flash_data = {
	.map_name		= "cfi_probe",
	.width			= 4,
	.init			= realview_flash_init,
	.exit			= realview_flash_exit,
	.set_vpp		= realview_flash_set_vpp,
};

struct platform_device realview_flash_device = {
	.name			= "armflash",
	.id			= 0,
	.dev			= {
		.platform_data	= &realview_flash_data,
	},
};

int realview_flash_register(struct resource *res, u32 num)
{
	realview_flash_device.resource = res;
	realview_flash_device.num_resources = num;
	return platform_device_register(&realview_flash_device);
}

static struct smc911x_platdata realview_smc911x_platdata = {
	.flags		= SMC911X_USE_32BIT,
	.irq_flags	= IRQF_SHARED,
	.irq_polarity	= 1,
};

static struct platform_device realview_eth_device = {
	.name		= "smc911x",
	.id		= 0,
	.num_resources	= 2,
};

int realview_eth_register(const char *name, struct resource *res)
{
	if (name)
		realview_eth_device.name = name;
	realview_eth_device.resource = res;
	if (strcmp(realview_eth_device.name, "smc911x") == 0)
		realview_eth_device.dev.platform_data = &realview_smc911x_platdata;

	return platform_device_register(&realview_eth_device);
}

static struct resource realview_i2c_resource = {
	.start		= REALVIEW_I2C_BASE,
	.end		= REALVIEW_I2C_BASE + SZ_4K - 1,
	.flags		= IORESOURCE_MEM,
};

struct platform_device realview_i2c_device = {
	.name		= "versatile-i2c",
	.id		= -1,
	.num_resources	= 1,
	.resource	= &realview_i2c_resource,
};

#define REALVIEW_SYSMCI	(__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_MCI_OFFSET)

static unsigned int realview_mmc_status(struct device *dev)
{
	struct amba_device *adev = container_of(dev, struct amba_device, dev);
	u32 mask;

	if (adev->res.start == REALVIEW_MMCI0_BASE)
		mask = 1;
	else
		mask = 2;

	return readl(REALVIEW_SYSMCI) & mask;
}

struct mmc_platform_data realview_mmc0_plat_data = {
	.ocr_mask	= MMC_VDD_32_33|MMC_VDD_33_34,
	.status		= realview_mmc_status,
};

struct mmc_platform_data realview_mmc1_plat_data = {
	.ocr_mask	= MMC_VDD_32_33|MMC_VDD_33_34,
	.status		= realview_mmc_status,
};

/*
 * Clock handling
 */
static const struct icst307_params realview_oscvco_params = {
	.ref		= 24000,
	.vco_max	= 200000,
	.vd_min		= 4 + 8,
	.vd_max		= 511 + 8,
	.rd_min		= 1 + 2,
	.rd_max		= 127 + 2,
};

static void realview_oscvco_set(struct clk *clk, struct icst307_vco vco)
{
	void __iomem *sys_lock = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_LOCK_OFFSET;
	void __iomem *sys_osc;
	u32 val;

	if (machine_is_realview_pb1176())
		sys_osc = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_OSC0_OFFSET;
	else
		sys_osc = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_OSC4_OFFSET;

	val = readl(sys_osc) & ~0x7ffff;
	val |= vco.v | (vco.r << 9) | (vco.s << 16);

	writel(0xa05f, sys_lock);
	writel(val, sys_osc);
	writel(0, sys_lock);
}

static struct clk oscvco_clk = {
	.params	= &realview_oscvco_params,
	.setvco = realview_oscvco_set,
};

/*
 * These are fixed clocks.
 */
static struct clk ref24_clk = {
	.rate	= 24000000,
};

static struct clk_lookup lookups[] = {
	{	/* UART0 */
		.dev_id		= "dev:f1",
		.clk		= &ref24_clk,
	}, {	/* UART1 */
		.dev_id		= "dev:f2",
		.clk		= &ref24_clk,
	}, {	/* UART2 */
		.dev_id		= "dev:f3",
		.clk		= &ref24_clk,
	}, {	/* UART3 */
		.dev_id		= "fpga:09",
		.clk		= &ref24_clk,
	}, {	/* KMI0 */
		.dev_id		= "fpga:06",
		.clk		= &ref24_clk,
	}, {	/* KMI1 */
		.dev_id		= "fpga:07",
		.clk		= &ref24_clk,
	}, {	/* MMC0 */
		.dev_id		= "fpga:05",
		.clk		= &ref24_clk,
	}, {	/* EB:CLCD */
		.dev_id		= "dev:20",
		.clk		= &oscvco_clk,
	}, {	/* PB:CLCD */
		.dev_id		= "issp:20",
		.clk		= &oscvco_clk,
	}
};

static int __init clk_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(lookups); i++)
		clkdev_add(&lookups[i]);
	return 0;
}
arch_initcall(clk_init);

/*
 * CLCD support.
 */
#define SYS_CLCD_NLCDIOON	(1 << 2)
#define SYS_CLCD_VDDPOSSWITCH	(1 << 3)
#define SYS_CLCD_PWR3V5SWITCH	(1 << 4)
#define SYS_CLCD_ID_MASK	(0x1f << 8)
#define SYS_CLCD_ID_SANYO_3_8	(0x00 << 8)
#define SYS_CLCD_ID_UNKNOWN_8_4	(0x01 << 8)
#define SYS_CLCD_ID_EPSON_2_2	(0x02 << 8)
#define SYS_CLCD_ID_SANYO_2_5	(0x07 << 8)
#define SYS_CLCD_ID_VGA		(0x1f << 8)

static struct clcd_panel vga = {
	.mode		= {
		.name		= "VGA",
		.refresh	= 60,
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 39721,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 32,
		.lower_margin	= 11,
		.hsync_len	= 96,
		.vsync_len	= 2,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_BGR | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel xvga = {
	.mode		= {
		.name		= "XVGA",
		.refresh	= 60,
		.xres		= 1024,
		.yres		= 768,
		.pixclock	= 15748,
		.left_margin	= 152,
		.right_margin	= 48,
		.upper_margin	= 23,
		.lower_margin	= 3,
		.hsync_len	= 104,
		.vsync_len	= 4,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_BGR | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel sanyo_3_8_in = {
	.mode		= {
		.name		= "Sanyo QVGA",
		.refresh	= 116,
		.xres		= 320,
		.yres		= 240,
		.pixclock	= 100000,
		.left_margin	= 6,
		.right_margin	= 6,
		.upper_margin	= 5,
		.lower_margin	= 5,
		.hsync_len	= 6,
		.vsync_len	= 6,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD,
	.cntl		= CNTL_LCDTFT | CNTL_BGR | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel sanyo_2_5_in = {
	.mode		= {
		.name		= "Sanyo QVGA Portrait",
		.refresh	= 116,
		.xres		= 240,
		.yres		= 320,
		.pixclock	= 100000,
		.left_margin	= 20,
		.right_margin	= 10,
		.upper_margin	= 2,
		.lower_margin	= 2,
		.hsync_len	= 10,
		.vsync_len	= 2,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_IVS | TIM2_IHS | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_BGR | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel epson_2_2_in = {
	.mode		= {
		.name		= "Epson QCIF",
		.refresh	= 390,
		.xres		= 176,
		.yres		= 220,
		.pixclock	= 62500,
		.left_margin	= 3,
		.right_margin	= 2,
		.upper_margin	= 1,
		.lower_margin	= 0,
		.hsync_len	= 3,
		.vsync_len	= 2,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_BGR | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

/*
 * Detect which LCD panel is connected, and return the appropriate
 * clcd_panel structure.  Note: we do not have any information on
 * the required timings for the 8.4in panel, so we presently assume
 * VGA timings.
 */
static struct clcd_panel *realview_clcd_panel(void)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	struct clcd_panel *vga_panel;
	struct clcd_panel *panel;
	u32 val;

	if (machine_is_realview_eb())
		vga_panel = &vga;
	else
		vga_panel = &xvga;

	val = readl(sys_clcd) & SYS_CLCD_ID_MASK;
	if (val == SYS_CLCD_ID_SANYO_3_8)
		panel = &sanyo_3_8_in;
	else if (val == SYS_CLCD_ID_SANYO_2_5)
		panel = &sanyo_2_5_in;
	else if (val == SYS_CLCD_ID_EPSON_2_2)
		panel = &epson_2_2_in;
	else if (val == SYS_CLCD_ID_VGA)
		panel = vga_panel;
	else {
		printk(KERN_ERR "CLCD: unknown LCD panel ID 0x%08x, using VGA\n",
			val);
		panel = vga_panel;
	}

	return panel;
}

/*
 * Disable all display connectors on the interface module.
 */
static void realview_clcd_disable(struct clcd_fb *fb)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	u32 val;

	val = readl(sys_clcd);
	val &= ~SYS_CLCD_NLCDIOON | SYS_CLCD_PWR3V5SWITCH;
	writel(val, sys_clcd);
}

/*
 * Enable the relevant connector on the interface module.
 */
static void realview_clcd_enable(struct clcd_fb *fb)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	u32 val;

	/*
	 * Enable the PSUs
	 */
	val = readl(sys_clcd);
	val |= SYS_CLCD_NLCDIOON | SYS_CLCD_PWR3V5SWITCH;
	writel(val, sys_clcd);
}

static int realview_clcd_setup(struct clcd_fb *fb)
{
	unsigned long framesize;
	dma_addr_t dma;

	if (machine_is_realview_eb())
		/* VGA, 16bpp */
		framesize = 640 * 480 * 2;
	else
		/* XVGA, 16bpp */
		framesize = 1024 * 768 * 2;

	fb->panel		= realview_clcd_panel();

	fb->fb.screen_base = dma_alloc_writecombine(&fb->dev->dev, framesize,
						    &dma, GFP_KERNEL);
	if (!fb->fb.screen_base) {
		printk(KERN_ERR "CLCD: unable to map framebuffer\n");
		return -ENOMEM;
	}

	fb->fb.fix.smem_start	= dma;
	fb->fb.fix.smem_len	= framesize;

	return 0;
}

static int realview_clcd_mmap(struct clcd_fb *fb, struct vm_area_struct *vma)
{
	return dma_mmap_writecombine(&fb->dev->dev, vma,
				     fb->fb.screen_base,
				     fb->fb.fix.smem_start,
				     fb->fb.fix.smem_len);
}

static void realview_clcd_remove(struct clcd_fb *fb)
{
	dma_free_writecombine(&fb->dev->dev, fb->fb.fix.smem_len,
			      fb->fb.screen_base, fb->fb.fix.smem_start);
}

struct clcd_board clcd_plat_data = {
	.name		= "RealView",
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.disable	= realview_clcd_disable,
	.enable		= realview_clcd_enable,
	.setup		= realview_clcd_setup,
	.mmap		= realview_clcd_mmap,
	.remove		= realview_clcd_remove,
};

#ifdef CONFIG_LEDS
#define VA_LEDS_BASE (__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_LED_OFFSET)

void realview_leds_event(led_event_t ledevt)
{
	unsigned long flags;
	u32 val;

	local_irq_save(flags);
	val = readl(VA_LEDS_BASE);

	switch (ledevt) {
	case led_idle_start:
		val = val & ~REALVIEW_SYS_LED0;
		break;

	case led_idle_end:
		val = val | REALVIEW_SYS_LED0;
		break;

	case led_timer:
		val = val ^ REALVIEW_SYS_LED1;
		break;

	case led_halted:
		val = 0;
		break;

	default:
		break;
	}

	writel(val, VA_LEDS_BASE);
	local_irq_restore(flags);
}
#endif	/* CONFIG_LEDS */

/*
 * Where is the timer (VA)?
 */
void __iomem *timer0_va_base;
void __iomem *timer1_va_base;
void __iomem *timer2_va_base;
void __iomem *timer3_va_base;

/*
 * How long is the timer interval?
 */
#define TIMER_INTERVAL	(TICKS_PER_uSEC * mSEC_10)
#if TIMER_INTERVAL >= 0x100000
#define TIMER_RELOAD	(TIMER_INTERVAL >> 8)
#define TIMER_DIVISOR	(TIMER_CTRL_DIV256)
#define TICKS2USECS(x)	(256 * (x) / TICKS_PER_uSEC)
#elif TIMER_INTERVAL >= 0x10000
#define TIMER_RELOAD	(TIMER_INTERVAL >> 4)		/* Divide by 16 */
#define TIMER_DIVISOR	(TIMER_CTRL_DIV16)
#define TICKS2USECS(x)	(16 * (x) / TICKS_PER_uSEC)
#else
#define TIMER_RELOAD	(TIMER_INTERVAL)
#define TIMER_DIVISOR	(TIMER_CTRL_DIV1)
#define TICKS2USECS(x)	((x) / TICKS_PER_uSEC)
#endif

static void timer_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *clk)
{
	unsigned long ctrl;

	switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		writel(TIMER_RELOAD, timer0_va_base + TIMER_LOAD);

		ctrl = TIMER_CTRL_PERIODIC;
		ctrl |= TIMER_CTRL_32BIT | TIMER_CTRL_IE | TIMER_CTRL_ENABLE;
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ctrl = TIMER_CTRL_ONESHOT;
		ctrl |= TIMER_CTRL_32BIT | TIMER_CTRL_IE;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		ctrl = 0;
	}

	writel(ctrl, timer0_va_base + TIMER_CTRL);
}

static int timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	unsigned long ctrl = readl(timer0_va_base + TIMER_CTRL);

	writel(evt, timer0_va_base + TIMER_LOAD);
	writel(ctrl | TIMER_CTRL_ENABLE, timer0_va_base + TIMER_CTRL);

	return 0;
}

static struct clock_event_device timer0_clockevent =	 {
	.name		= "timer0",
	.shift		= 32,
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= timer_set_mode,
	.set_next_event	= timer_set_next_event,
	.rating		= 300,
	.cpumask	= cpu_all_mask,
};

static void __init realview_clockevents_init(unsigned int timer_irq)
{
	timer0_clockevent.irq = timer_irq;
	timer0_clockevent.mult =
		div_sc(1000000, NSEC_PER_SEC, timer0_clockevent.shift);
	timer0_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &timer0_clockevent);
	timer0_clockevent.min_delta_ns =
		clockevent_delta2ns(0xf, &timer0_clockevent);

	clockevents_register_device(&timer0_clockevent);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t realview_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &timer0_clockevent;

	/* clear the interrupt */
	writel(1, timer0_va_base + TIMER_INTCLR);

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction realview_timer_irq = {
	.name		= "RealView Timer Tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= realview_timer_interrupt,
};

static cycle_t realview_get_cycles(void)
{
	return ~readl(timer3_va_base + TIMER_VALUE);
}

static struct clocksource clocksource_realview = {
	.name	= "timer3",
	.rating	= 200,
	.read	= realview_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 20,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init realview_clocksource_init(void)
{
	/* setup timer 0 as free-running clocksource */
	writel(0, timer3_va_base + TIMER_CTRL);
	writel(0xffffffff, timer3_va_base + TIMER_LOAD);
	writel(0xffffffff, timer3_va_base + TIMER_VALUE);
	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC,
		timer3_va_base + TIMER_CTRL);

	clocksource_realview.mult =
		clocksource_khz2mult(1000, clocksource_realview.shift);
	clocksource_register(&clocksource_realview);
}

/*
 * Set up the clock source and clock events devices
 */
void __init realview_timer_init(unsigned int timer_irq)
{
	u32 val;

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
	/*
	 * The dummy clock device has to be registered before the main device
	 * so that the latter will broadcast the clock events
	 */
	local_timer_setup();
#endif

	/* 
	 * set clock frequency: 
	 *	REALVIEW_REFCLK is 32KHz
	 *	REALVIEW_TIMCLK is 1MHz
	 */
	val = readl(__io_address(REALVIEW_SCTL_BASE));
	writel((REALVIEW_TIMCLK << REALVIEW_TIMER1_EnSel) |
	       (REALVIEW_TIMCLK << REALVIEW_TIMER2_EnSel) | 
	       (REALVIEW_TIMCLK << REALVIEW_TIMER3_EnSel) |
	       (REALVIEW_TIMCLK << REALVIEW_TIMER4_EnSel) | val,
	       __io_address(REALVIEW_SCTL_BASE));

	/*
	 * Initialise to a known state (all timers off)
	 */
	writel(0, timer0_va_base + TIMER_CTRL);
	writel(0, timer1_va_base + TIMER_CTRL);
	writel(0, timer2_va_base + TIMER_CTRL);
	writel(0, timer3_va_base + TIMER_CTRL);

	/* 
	 * Make irqs happen for the system timer
	 */
	setup_irq(timer_irq, &realview_timer_irq);

	realview_clocksource_init();
	realview_clockevents_init(timer_irq);
}
