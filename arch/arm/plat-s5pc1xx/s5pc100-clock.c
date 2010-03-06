/* linux/arch/arm/plat-s5pc1xx/s5pc100-clock.c
 *
 * Copyright 2008 Samsung Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C6400 based common clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>

#include <plat/regs-clock.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>

/* fin_apll, fin_mpll and fin_epll are all the same clock, which we call
 * ext_xtal_mux for want of an actual name from the manual.
*/

struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
};

#define clk_fin_apll	clk_ext_xtal_mux
#define clk_fin_mpll	clk_ext_xtal_mux
#define clk_fin_epll	clk_ext_xtal_mux

#define clk_fout_mpll	clk_mpll

struct clk_sources {
	unsigned int	nr_sources;
	struct clk	**sources;
};

struct clksrc_clk {
	struct clk		clk;
	unsigned int		mask;
	unsigned int		shift;

	struct clk_sources	*sources;

	unsigned int		divider_shift;
	void __iomem		*reg_divider;
};

struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
};

static struct clk *clk_src_apll_list[] = {
	[0] = &clk_fin_apll,
	[1] = &clk_fout_apll,
};

static struct clk_sources clk_src_apll = {
	.sources	= clk_src_apll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_apll_list),
};

struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_APLL_SHIFT,
	.mask		= S5P_CLKSRC0_APLL_MASK,
	.sources	= &clk_src_apll,
};

struct clk clk_fout_epll = {
	.name		= "fout_epll",
	.id		= -1,
};

static struct clk *clk_src_epll_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_fout_epll,
};

static struct clk_sources clk_src_epll = {
	.sources	= clk_src_epll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_epll_list),
};

struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
};

static struct clk *clk_src_mpll_list[] = {
	[0] = &clk_fin_mpll,
	[1] = &clk_fout_mpll,
};

static struct clk_sources clk_src_mpll = {
	.sources	= clk_src_mpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_mpll_list),
};

struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MPLL_SHIFT,
	.mask		= S5P_CLKSRC0_MPLL_MASK,
	.sources	= &clk_src_mpll,
};

static unsigned long s5p_clk_doutmpll_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);

	printk(KERN_DEBUG "%s: parent is %ld\n", __func__, rate);
#if 0
	if (__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK)
		rate /= 2;
#endif

	return rate;
}

struct clk clk_dout_mpll = {
	.name		= "dout_mpll",
	.id		= -1,
	.parent		= &clk_mout_mpll.clk,
	.get_rate	= s5p_clk_doutmpll_get_rate,
};

#if 0
static struct clk *clkset_spi_mmc_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_fin_epll,
	&clk_27m,
};

static struct clk_sources clkset_spi_mmc = {
	.sources	= clkset_spi_mmc_list,
	.nr_sources	= ARRAY_SIZE(clkset_spi_mmc_list),
};

static struct clk *clkset_irda_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	NULL,
	&clk_27m,
};

static struct clk_sources clkset_irda = {
	.sources	= clkset_irda_list,
	.nr_sources	= ARRAY_SIZE(clkset_irda_list),
};

static struct clk *clkset_uart_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	NULL,
	NULL
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_uhost_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll,
	&clk_fin_epll,
	&clk_48m,
};

static struct clk_sources clkset_uhost = {
	.sources	= clkset_uhost_list,
	.nr_sources	= ARRAY_SIZE(clkset_uhost_list),
};
#endif


/* The peripheral clocks are all controlled via clocksource followed
 * by an optional divider and gate stage. We currently roll this into
 * one clock which hides the intermediate clock from the mux.
 *
 * Note, the JPEG clock can only be an even divider...
 *
 * The scaler and LCD clocks depend on the S3C64XX version, and also
 * have a common parent divisor so are not included here.
 */

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

#if 0
static unsigned long s5p_getrate_clksrc(struct clk *clk)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv = __raw_readl(sclk->reg_divider);

	clkdiv >>= sclk->divider_shift;
	clkdiv &= 0xf;
	clkdiv++;

	rate /= clkdiv;
	return rate;
}

static int s5p_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	div = clk_get_rate(clk->parent) / rate;

	val = __raw_readl(reg);
	val &= ~sclk->mask;
	val |= (rate - 1) << sclk->shift;
	__raw_writel(val, reg);

	return 0;
}

static int s5p_setparent_clksrc(struct clk *clk, struct clk *parent)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;
	u32 clksrc = __raw_readl(S5P_CLK_SRC0);
	int src_nr = -1;
	int ptr;

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, S5P_CLK_SRC0);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s3c64xx_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate > parent_rate)
		rate = parent_rate;
	else {
		div = rate / parent_rate;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}
#endif

#if 0
static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit        = S3C_CLKCON_SCLK_MMC0,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_MMC0_SHIFT,
	.mask		= S3C6400_CLKSRC_MMC0_MASK,
	.sources	= &clkset_spi_mmc,
	.divider_shift	= S3C6400_CLKDIV1_MMC0_SHIFT,
	.reg_divider	= S3C_CLK_DIV1,
};

static struct clksrc_clk clk_mmc1 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit        = S3C_CLKCON_SCLK_MMC1,
		.enable		= s3c64xx_sclk_ctrl,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.set_parent	= s5p_setparent_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_MMC1_SHIFT,
	.mask		= S3C6400_CLKSRC_MMC1_MASK,
	.sources	= &clkset_spi_mmc,
	.divider_shift	= S3C6400_CLKDIV1_MMC1_SHIFT,
	.reg_divider	= S3C_CLK_DIV1,
};

static struct clksrc_clk clk_mmc2 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit        = S3C_CLKCON_SCLK_MMC2,
		.enable		= s3c64xx_sclk_ctrl,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.set_parent	= s5p_setparent_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_MMC2_SHIFT,
	.mask		= S3C6400_CLKSRC_MMC2_MASK,
	.sources	= &clkset_spi_mmc,
	.divider_shift	= S3C6400_CLKDIV1_MMC2_SHIFT,
	.reg_divider	= S3C_CLK_DIV1,
};

static struct clksrc_clk clk_usbhost = {
	.clk	= {
		.name		= "usb-host-bus",
		.id		= -1,
		.ctrlbit        = S3C_CLKCON_SCLK_UHOST,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_UHOST_SHIFT,
	.mask		= S3C6400_CLKSRC_UHOST_MASK,
	.sources	= &clkset_uhost,
	.divider_shift	= S3C6400_CLKDIV1_UHOST_SHIFT,
	.reg_divider	= S3C_CLK_DIV1,
};

static struct clksrc_clk clk_uart_uclk1 = {
	.clk	= {
		.name		= "uclk1",
		.id		= -1,
		.ctrlbit        = S3C_CLKCON_SCLK_UART,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_UART_SHIFT,
	.mask		= S3C6400_CLKSRC_UART_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S3C6400_CLKDIV2_UART_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};

/* Where does UCLK0 come from? */

static struct clksrc_clk clk_spi0 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 0,
		.ctrlbit        = S3C_CLKCON_SCLK_SPI0,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_SPI0_SHIFT,
	.mask		= S3C6400_CLKSRC_SPI0_MASK,
	.sources	= &clkset_spi_mmc,
	.divider_shift	= S3C6400_CLKDIV2_SPI0_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};

static struct clksrc_clk clk_spi1 = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 1,
		.ctrlbit        = S3C_CLKCON_SCLK_SPI1,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_SPI1_SHIFT,
	.mask		= S3C6400_CLKSRC_SPI1_MASK,
	.sources	= &clkset_spi_mmc,
	.divider_shift	= S3C6400_CLKDIV2_SPI1_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};

static struct clk clk_iis_cd0 = {
	.name		= "iis_cdclk0",
	.id		= -1,
};

static struct clk clk_iis_cd1 = {
	.name		= "iis_cdclk1",
	.id		= -1,
};

static struct clk clk_pcm_cd = {
	.name		= "pcm_cdclk",
	.id		= -1,
};

static struct clk *clkset_audio0_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_dout_mpll,
	[2] = &clk_fin_epll,
	[3] = &clk_iis_cd0,
	[4] = &clk_pcm_cd,
};

static struct clk_sources clkset_audio0 = {
	.sources	= clkset_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio0 = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 0,
		.ctrlbit        = S3C_CLKCON_SCLK_AUDIO0,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_AUDIO0_SHIFT,
	.mask		= S3C6400_CLKSRC_AUDIO0_MASK,
	.sources	= &clkset_audio0,
	.divider_shift	= S3C6400_CLKDIV2_AUDIO0_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};

static struct clk *clkset_audio1_list[] = {
	[0] = &clk_mout_epll.clk,
	[1] = &clk_dout_mpll,
	[2] = &clk_fin_epll,
	[3] = &clk_iis_cd1,
	[4] = &clk_pcm_cd,
};

static struct clk_sources clkset_audio1 = {
	.sources	= clkset_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio1_list),
};

static struct clksrc_clk clk_audio1 = {
	.clk	= {
		.name		= "audio-bus",
		.id		= 1,
		.ctrlbit        = S3C_CLKCON_SCLK_AUDIO1,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_AUDIO1_SHIFT,
	.mask		= S3C6400_CLKSRC_AUDIO1_MASK,
	.sources	= &clkset_audio1,
	.divider_shift	= S3C6400_CLKDIV2_AUDIO1_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};

static struct clksrc_clk clk_irda = {
	.clk	= {
		.name		= "irda-bus",
		.id		= 0,
		.ctrlbit        = S3C_CLKCON_SCLK_IRDA,
		.enable		= s3c64xx_sclk_ctrl,
		.set_parent	= s5p_setparent_clksrc,
		.get_rate	= s5p_getrate_clksrc,
		.set_rate	= s5p_setrate_clksrc,
		.round_rate	= s3c64xx_roundrate_clksrc,
	},
	.shift		= S3C6400_CLKSRC_IRDA_SHIFT,
	.mask		= S3C6400_CLKSRC_IRDA_MASK,
	.sources	= &clkset_irda,
	.divider_shift	= S3C6400_CLKDIV2_IRDA_SHIFT,
	.reg_divider	= S3C_CLK_DIV2,
};
#endif

/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
#if 0
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
	&clk_usbhost,
	&clk_uart_uclk1,
	&clk_spi0,
	&clk_spi1,
	&clk_audio0,
	&clk_audio1,
	&clk_irda,
#endif
};

static void __init_or_cpufreq s5p_set_clksrc(struct clksrc_clk *clk)
{
	struct clk_sources *srcs = clk->sources;
	u32 clksrc = __raw_readl(S5P_CLK_SRC0);

	clksrc &= clk->mask;
	clksrc >>= clk->shift;

	if (clksrc > srcs->nr_sources || !srcs->sources[clksrc]) {
		printk(KERN_ERR "%s: bad source %d\n",
		       clk->clk.name, clksrc);
		return;
	}

	clk->clk.parent = srcs->sources[clksrc];

	printk(KERN_INFO "%s: source is %s (%d), rate is %ld\n",
	       clk->clk.name, clk->clk.parent->name, clksrc,
	       clk_get_rate(&clk->clk));
}

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5pc100_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long armclk;
	unsigned long hclkd0;
	unsigned long hclk;
	unsigned long pclkd0;
	unsigned long pclk;
	unsigned long apll;
	unsigned long mpll;
	unsigned long hpll;
	unsigned long epll;
	unsigned int ptr;
	u32 clkdiv0, clkdiv1;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	clkdiv1 = __raw_readl(S5P_CLK_DIV1);

	printk(KERN_DEBUG "%s: clkdiv0 = %08x, clkdiv1 = %08x\n", __func__, clkdiv0, clkdiv1);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5p_get_pll(xtal, __raw_readl(S5P_APLL_CON));
	mpll = s5p_get_pll(xtal, __raw_readl(S5P_MPLL_CON));
	hpll = s5p_get_pll(xtal, __raw_readl(S5P_HPLL_CON));
	epll = s5p_get_pll(xtal, __raw_readl(S5P_EPLL_CON));

	printk(KERN_INFO "S5PC100: PLL settings, A=%ld, M=%ld, H=%ld, E=%ld\n",
	       apll, mpll, hpll, epll);

	armclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);
	armclk = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_ARM);
	hclkd0 = armclk / GET_DIV(clkdiv0, S5P_CLKDIV0_D0);
	pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_PCLKD0);

	hclk = mpll / GET_DIV(clkdiv1, S5P_CLKDIV1_D1);
	pclk = hclk / GET_DIV(clkdiv1, S5P_CLKDIV1_PCLKD1);

	printk(KERN_INFO "S5PC100: ARMCLK=%ld, HCLKD0=%ld, PCLKD0=%ld, HCLK=%ld, PCLK=%ld\n",
	       armclk, hclkd0, pclkd0, hclk, pclk);

	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;

	clk_f.rate = armclk;
	clk_hd0.rate = hclkd0;
	clk_pd0.rate = pclkd0;
	clk_h.rate = hclk;
	clk_p.rate = pclk;

	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5p_set_clksrc(init_parents[ptr]);
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
#if 0
	&clk_iis_cd0,
	&clk_iis_cd1,
	&clk_pcm_cd,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_dout_mpll,
	&clk_mmc0.clk,
	&clk_mmc1.clk,
	&clk_mmc2.clk,
	&clk_usbhost.clk,
	&clk_uart_uclk1.clk,
	&clk_spi0.clk,
	&clk_spi1.clk,
	&clk_audio0.clk,
	&clk_audio1.clk,
	&clk_irda.clk,
#endif
};

void __init s5pc100_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	clk_mpll.parent = &clk_mout_mpll.clk;
	clk_epll.parent = &clk_mout_epll.clk;
}
