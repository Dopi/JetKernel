/* linux/drivers/media/video/samsung/tv20/s5pc100/tv_clock_s5pc100.c
 *
 * clock raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <plat/map-base.h>
#include <plat/regs-clock.h>

#include "tv_out_s5pc110.h"
#include "regs/regs-clock_extra.h"

#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_TVOUT_CLK_DEBUG 1
#endif

//#define S5P_TVOUT_CLK_DEBUG 1

#ifdef S5P_TVOUT_CLK_DEBUG
#define TVCLKPRINTK(fmt, args...) \
	printk("\t\t[TVCLK] %s: " fmt, __FUNCTION__ , ## args)
#else
#define TVCLKPRINTK(fmt, args...)
#endif


void __s5p_tv_clk_init_hpll(bool vsel,unsigned int lock_time,
			unsigned int mdiv,
			unsigned int pdiv,
			unsigned int sdiv)
{
	u32 temp;
	
	TVCLKPRINTK("%d,%d,%d,%d\n\r", lock_time, mdiv, pdiv, sdiv);

	#ifdef VPLL_USES_HDMI27M
	writel((readl(S5P_CLK_SRC1) | VPLLSRC_SEL_HDMI27M),S5P_CLK_SRC1);
	#endif

	temp = readl(S5P_VPLL_CON);

	temp &= ~VPLL_ENABLE;

	writel(temp, S5P_VPLL_CON);

	temp = 0;

	if(vsel)
		temp |= VCO_FREQ_SEL;

	temp |= VPLL_ENABLE;
	temp |= MDIV(mdiv) | PDIV(pdiv) | SDIV(sdiv);
		
	writel(VPLL_LOCKTIME(lock_time), S5P_VPLL_LOCK);
	writel(temp, S5P_VPLL_CON);

	while(!VPLL_LOCKED(readl(S5P_VPLL_CON)));	
	TVCLKPRINTK("++++++(0x%08x)\n\r", readl(S5P_CLK_SRC1));
	TVCLKPRINTK("0x%08x,0x%08x\n\r", readl(S5P_VPLL_LOCK), readl(S5P_VPLL_CON));
}

void __s5p_tv_clk_hpll_onoff(bool en)
{
	TVCLKPRINTK("%d\n\r", en);
#if 0 
	if (en) {
		writel(readl(S5P_VPLL_CON) | VPLL_ENABLE, S5P_VPLL_CON) ;

		while (!VPLL_LOCKED(readl(S5P_VPLL_CON))) {
			msleep(1);
		}
	} else {
		writel(readl(S5P_VPLL_CON) & ~VPLL_ENABLE, S5P_VPLL_CON);
	}

	TVCLKPRINTK("0x%08x,0x%08x\n\r", readl(S5P_VPLL_LOCK), readl(S5P_VPLL_CON));
#endif
}

s5p_tv_clk_err __s5p_tv_clk_init_href(s5p_tv_clk_hpll_ref hpll_ref)
{
	TVCLKPRINTK("(%d)\n\r", hpll_ref);

#if 0 //mkh
	switch (hpll_ref) {

	case S5P_TV_CLK_HPLL_REF_27M:
		writel(readl(S5P_CLK_SRC0) & HREF_SEL_MASK, S5P_CLK_SRC0);
		break;

	case S5P_TV_CLK_HPLL_REF_SRCLK:
		writel(readl(S5P_CLK_SRC0) | HREF_SEL_SRCLK, S5P_CLK_SRC0);
		break;

	default:
		TVCLKPRINTK("invalid hpll_ref parameter = %d\n\r", hpll_ref);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_SRC0));
#endif

	return S5P_TV_CLK_ERR_NO_ERROR;
}

s5p_tv_clk_err __s5p_tv_clk_init_mout_hpll(s5p_tv_clk_mout_hpll mout_hpll)
{
	TVCLKPRINTK("(%d)\n\r", mout_hpll);

	writel((readl(S5P_CLK_SRC1) & HDMI_SEL_MASK) | HDMI_SEL_HDMIPHY, S5P_CLK_SRC1);
//	writel(readl(S5P_CLK_SRC1) & HDMI_SEL_MASK, S5P_CLK_SRC1);

#if 0 
	switch (mout_hpll) {

	case S5P_TV_CLK_MOUT_HPLL_27M:
		writel(readl(S5P_CLK_SRC0) & VPLL_SEL_MASK, S5P_CLK_SRC0);
		break;

	case S5P_TV_CLK_MOUT_HPLL_FOUT_HPLL:
		writel(readl(S5P_CLK_SRC0) | VPLL_SEL_FOUT_HPLL, S5P_CLK_SRC0);
		break;

	default:
		TVCLKPRINTK(" invalid mout_hpll parameter = %d\n\r", mout_hpll);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_SRC0));
#endif
	return S5P_TV_CLK_ERR_NO_ERROR;
}

s5p_tv_clk_err __s5p_tv_clk_init_video_mixer(s5p_tv_clk_vmiexr_srcclk src_clk)
{
	TVCLKPRINTK("(%d)\n\r", src_clk);

	switch (src_clk) {

	case TVOUT_CLK_VMIXER_SRCCLK_CLK27M:
		writel(((readl(S5P_CLK_SRC1) &VMIXER_SEL_MASK) | VMIXER_SEL_CLK27M), 
			S5P_CLK_SRC1);
		break;

	case TVOUT_CLK_VMIXER_SRCCLK_VCLK_54:
		writel(((readl(S5P_CLK_SRC1) &VMIXER_SEL_MASK) /*| VMIXER_SEL_VCLK_54*/), //mkh
			S5P_CLK_SRC1);
	#ifdef TVENC_USES_HDMIPHY
		writel((readl(S5P_CLK_SRC1) | DAC_SEL_HDMI_PHY),S5P_CLK_SRC1);
	#endif
		break;

	case TVOUT_CLK_VMIXER_SRCCLK_MOUT_HPLL:
// SPMOON_C110:		writel(((readl(S5P_CLK_SRC1) &VMIXER_SEL_MASK) | VMIXER_SEL_MOUT_HPLL),
		writel(readl(S5P_CLK_SRC1) | VMIXER_SEL_MOUT_VPLL, S5P_CLK_SRC1);
			
		break;

	default:
		TVCLKPRINTK("invalid src_clk parameter = %d\n\r", src_clk);
		return S5P_TV_CLK_ERR_INVALID_PARAM;
		break;
	}

	//TVCLKPRINTK

	TVCLKPRINTK(" CLK_SRC1(0x%08x)\n\r", readl(S5P_CLK_SRC1));
	return S5P_TV_CLK_ERR_NO_ERROR;
}

void __s5p_tv_clk_init_hdmi_ratio(unsigned int clk_div)
{
	TVCLKPRINTK("(%d)\n\r", clk_div);

	writel((readl(S5P_CLK_DIV1)& HDMI_DIV_RATIO_MASK) | HDMI_DIV_RATIO(clk_div), S5P_CLK_DIV1);

	TVCLKPRINTK("(0x%08x)\n\r", readl(S5P_CLK_DIV3));
}

/*
 * hclk gating
 */
 
/* VP */
void __s5p_tv_clk_set_vp_clk_onoff(bool clk_on)
{
	TVCLKPRINTK("VP hclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(S5P_CLKGATE_IP1_VP, S5P_CLKGATE_IP1);
	} else {
		bit_del_l(S5P_CLKGATE_IP1_VP, S5P_CLKGATE_IP1);	
	}

	TVCLKPRINTK("S5P_CLKGATE_MAIN1 :0x%08x\n\r", readl(S5P_CLKGATE_MAIN1));
}

/* MIXER */
void __s5p_tv_clk_set_vmixer_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("MIXER hclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(S5P_CLKGATE_IP1_MIXER, S5P_CLKGATE_IP1);
	} else {
		bit_del_l(S5P_CLKGATE_IP1_MIXER, S5P_CLKGATE_IP1);
	}

	TVCLKPRINTK("S5P_CLKGATE_MAIN1 :0x%08x\n\r", readl(S5P_CLKGATE_MAIN1));
}

/* TVENC */
void __s5p_tv_clk_set_sdout_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("TVENC hclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(S5P_CLKGATE_IP1_TVENC, S5P_CLKGATE_IP1);
		//bit_add_l(VMIXER_OUT_SEL_SDOUT, S5P_MIXER_OUT_SEL);
	} else {
		bit_del_l(S5P_CLKGATE_IP1_TVENC, S5P_CLKGATE_IP1);

	}

}

/* HDMI */
void __s5p_tv_clk_set_hdmi_hclk_onoff(bool clk_on)
{
	TVCLKPRINTK("HDMI hclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(S5P_CLKGATE_IP1_HDMI, S5P_CLKGATE_IP1);
		bit_add_l(VMIXER_OUT_SEL_HDMI, S5P_MIXER_OUT_SEL);
	} else {
		bit_del_l(S5P_CLKGATE_IP1_HDMI, S5P_CLKGATE_IP1) ;
	}

	TVCLKPRINTK("S5P_CLKGATE_PERI1 :0x%08x\n\r", readl(S5P_CLKGATE_PERI1));
	TVCLKPRINTK("clk output is %s\n\r", readl(S5P_MIXER_OUT_SEL) ? "HDMI":"SDOUT");
}

/* 
 * sclk gating 
 */
 
/* MIXER */
void __s5p_tv_clk_set_vmixer_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("MIXER sclk : %s\n\r", clk_on ? "on":"off");
	
	if (clk_on) {
		bit_add_l(CLK_SCLK_VMIXER_PASS, S5P_SCLKGATE0);
	} else {
		bit_del_l(CLK_SCLK_VMIXER_PASS, S5P_SCLKGATE0);
	}

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

/* TVENC */
void __s5p_tv_clk_set_sdout_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("TVENC sclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(CLK_SCLK_TV54_PASS | CLK_SCLK_VDAC54_PASS, S5P_SCLKGATE0);
	} else {
		bit_del_l(CLK_SCLK_TV54_PASS | CLK_SCLK_VDAC54_PASS, S5P_SCLKGATE0);
	}

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

/* HDMI */
void __s5p_tv_clk_set_hdmi_sclk_onoff(bool clk_on)
{
#if 0
	TVCLKPRINTK("HDMI sclk : %s\n\r", clk_on ? "on":"off");

	if (clk_on) {
		bit_add_l(CLK_SCLK_HDMI_PASS, S5P_SCLKGATE0);
	} else {
		bit_del_l(CLK_SCLK_HDMI_PASS, S5P_SCLKGATE0);
	}

	TVCLKPRINTK("S5P_SCLKGATE0 :0x%08x\n\r", readl(S5P_SCLKGATE0));
#endif
}

void __s5p_tv_clk_start(bool vp, bool sdout, bool hdmi)
{
	__s5p_tv_clk_set_vp_clk_onoff(vp);
	__s5p_tv_clk_set_sdout_hclk_onoff(sdout);
	__s5p_tv_clk_set_sdout_sclk_onoff(sdout);
	__s5p_tv_clk_set_hdmi_hclk_onoff(hdmi);
	__s5p_tv_clk_set_vmixer_hclk_onoff(true);
	__s5p_tv_clk_set_vmixer_sclk_onoff(true);	

	if (hdmi) {
		__s5p_tv_clk_hpll_onoff(true);
	}
}


void __s5p_tv_clk_stop(void)
{
	__s5p_tv_clk_set_sdout_sclk_onoff(false);
	__s5p_tv_clk_set_sdout_hclk_onoff(false);
	__s5p_tv_clk_set_hdmi_sclk_onoff(false);
	__s5p_tv_clk_set_hdmi_hclk_onoff(false);
	__s5p_tv_clk_set_vp_clk_onoff(false);
	__s5p_tv_clk_set_vmixer_sclk_onoff(false);
	__s5p_tv_clk_set_vmixer_hclk_onoff(false);
	__s5p_tv_clk_hpll_onoff(false);
}

int __init __s5p_tvclk_probe(struct platform_device *pdev, u32 res_num)
{
	return 0;
}

int __init __s5p_tvclk_release(struct platform_device *pdev)
{
	return 0;
}
