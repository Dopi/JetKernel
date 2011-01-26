/*
 * 
 *
 * Copyright 2007 Wolfson Microelectronics PLC.
 * Author: Graeme Gregory
 *         graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    20th Jan 2007   Initial version.
 *    05th Feb 2007   Rename all to Neo1973
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <asm/hardware/scoop.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/audio.h>
#include <mach/map.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-iic.h>
#include <plat/regs-clock.h>
#include <plat/regs-iis.h>

#include "../codecs/ak4671.h"
#include "s3c-pcm.h"
#include "s3c6410-i2s.h"

#define SUBJECT "capella_ak4671"
#define P(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define FI \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - IN" "\n", __func__, __LINE__, __func__);
#define FO \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - OUT" "\n", __func__, __LINE__, __func__);

//#define CONFIG_SND_DEBUG

#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) P(x)
#else
#define s3cdbg(x...)
#endif
static int android_hifi_hw_params(struct snd_pcm_substream *, struct snd_pcm_hw_params *);
static int android_hifi_hw_free(struct snd_pcm_substream *);

static struct snd_soc_ops android_hifi_ops = {
	.hw_params = android_hifi_hw_params,
	.hw_free = android_hifi_hw_free,
};

static struct snd_soc_dai_link android_dai[] = {
	{ /* Hifi Playback - for similatious use with voice below */
		.name = "AK4671",
		.stream_name = "AK4671 Codec",
		.cpu_dai = &s3c_i2s_dai,
		.codec_dai = &ak4671_dai,
		.ops = &android_hifi_ops,
	},
};

static struct snd_soc_card android = {
	.name = "android",
	.platform = &s3c24xx_soc_platform,
	.dai_link = android_dai,
	.num_links = ARRAY_SIZE(android_dai),
};

static struct ak4671_setup_data android_ak4671_setup = {
	.i2c_address = (0x24 >> 1),
};

static struct snd_soc_device android_snd_devdata = {
	.card = &android,
	.codec_dev = &soc_codec_dev_ak4671,
	.codec_data = &android_ak4671_setup,
};
static struct platform_device *android_snd_device;



static int android_hifi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int pll_out = 0, bclk = 0;
	int ret = 0;
	unsigned long iispsr, iismod;
	unsigned long epll_con0=0;
	unsigned int prescaler = 4;
	unsigned int m,p,s,k = 0;

    u32*    regs;

	regs = ioremap(S3C64XX_PA_IIS, 0x100);
	if (regs == NULL)
		return -ENXIO;

	s3cdbg("Entered %s, rate = %d\n", __FUNCTION__, params_rate(params));
	s3cdbg("before :   IISMOD: %lx,IISPSR: %lx\n",
			 readl((regs + S3C64XX_IIS0MOD)), 
			readl((regs + S3C64XX_IIS0PSR)));
	s3cdbg("	GPDCON = 0x%x \n",readl(S3C64XX_GPDCON));
	
	/*PCLK & SCLK gating enable*/
	writel(readl(S3C_PCLK_GATE)|S3C_CLKCON_PCLK_IIS0, S3C_PCLK_GATE);
	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_AUDIO0, S3C_SCLK_GATE);

	iismod = readl((regs + S3C64XX_IIS0MOD)); 
	iismod &=~(S3C64XX_IIS0MOD_FS_MASK);		// 256fs
	iismod &= ~(S3C64XX_IIS0MOD_BLC_MASK);		// 16bit

	/*Clear I2S prescaler value [13:8] and disable prescaler*/
	iispsr = readl((regs + S3C64XX_IIS0PSR));	
	//printk("iispsr = 0x%lx\n",iispsr);
	iispsr &=~((0x3f<<8)|(1<<15)); 
	writel(iispsr, (regs + S3C64XX_IIS0PSR));

	//(1<<31) |  (m<<16) | (p<<8) | (s<<0)
	switch (params_rate(params)) {
	case 8000:
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_REC_DOWNSAMPLING // sangsu fix : recording down scaling
		// (45.158 / divider)
		m = 30; p = 1; s = 3; k= 6903;
		//writel(10398, S3C_EPLL_CON1);
		//writel((1<<31)|(45<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);
		break;
#endif // sangsu fix
	case 16000:
	case 32000:
	case 64000:
		//(49.152 / divider)
		m = 32; p = 1; s = 3; k= 50332;
		//writel(50332, S3C_EPLL_CON1);
		//writel((1<<31)|(32<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		// (45.158 / divider)
		m = 30; p = 1; s = 3; k= 6903;
		//writel(10398, S3C_EPLL_CON1);
		//writel((1<<31)|(45<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);
		break;
	case 48000:
	case 96000:
		m = 49; p = 1; s = 3; k= 9961;
		//writel(9961, S3C_EPLL_CON1);
		//writel((1<<31)|(49<<16)|(1<<8)|(3<<0) ,S3C_EPLL_CON0);
		break;
	default:
		m = 128; p = 25; s = 0; k= 0;
		//writel(0, S3C_EPLL_CON1);
		//writel((1<<31)|(128<<16)|(25<<8)|(0<<0) ,S3C_EPLL_CON0);
		break;
	}
	writel(k, S3C_EPLL_CON1);
	writel((1<<31)|(m<<16)|(p<<8)|(s<<0) ,S3C_EPLL_CON0);
	s3cdbg("m = %d, EPLL_CON0 : 0x%x, epll_con0:0x%x\n",m,readl(S3C_EPLL_CON0),epll_con0);
	
	while(!(__raw_readl(S3C_EPLL_CON0)&(1<<30)));

	s3cdbg(" !!!! EPLL set - m:%d, p:%d, s:%d, k:%d  => EPLL_CON0 : 0x%x, EPLL_CON1: 0x%x !!!!\n",\
		m,p,s,k,readl(S3C_EPLL_CON0),readl(S3C_EPLL_CON1) );

	
	/* MUXepll : FOUTepll */
	writel(readl(S3C_CLK_SRC)|S3C_CLKSRC_EPLL_CLKSEL, S3C_CLK_SRC);
	/* AUDIO0 sel : FOUTepll */
	writel((readl(S3C_CLK_SRC)&~(0x7<<7))|(0<<7), S3C_CLK_SRC);

	/* CLK_DIV2 setting */
	writel(readl(S3C_CLK_DIV2)&~(0xf<<8),S3C_CLK_DIV2);
	
	switch (params_rate(params)) {
	case 8000:
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_REC_DOWNSAMPLING  // sangsu fix : recording down scaling
		pll_out = 45158000;
		prescaler = 4;  
		break;
#else // sangsu fix
		pll_out = 49152000;
		prescaler = 24; 
		break;
#endif // sangsu fix
	case 11025:
		pll_out = 45158000;
		prescaler = 16; 
		break;
	case 16000:
		pll_out = 49152000;
		prescaler = 12; 
		break;
	case 22050:
		pll_out = 45158000;
		prescaler = 8; 
		break;
	case 32000:
		pll_out = 49152000;
		prescaler = 6; 
		break;
	case 44100:
		pll_out = 45158000;
		prescaler = 4;  
		break;
	case 48000:
		pll_out = 49152000;
		prescaler = 4; 
		break;
	case 64000:
		pll_out = 49152000;
		prescaler = 3;  
		break;
	case 88200:
		pll_out = 45158000;
		prescaler = 2; 
		break;
	case 96000:
		pll_out = 49152000;
		prescaler = 2; 
		break;
	default:		// same as 44.1Khz
		pll_out = 45158000;
		prescaler = 4; 
		break;
	}

	iismod |= S3C_IISMOD_256FS;
	writel(iismod , (regs + S3C64XX_IIS0MOD));
	
	/* set codec DAI configuration */		//lm49350_mode1h_set_dai_fmt
	ret = codec_dai->ops->set_fmt(codec_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBM_CFM ); 
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */		//s3c_i2s_set_fmt
	ret = cpu_dai->ops->set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBM_CFM ); 
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */		// for codec sample rate
	ret = codec_dai->ops->set_sysclk(codec_dai, 0, params_rate(params), SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_I2SEXT, 0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#if 0
	/* set MCLK division for sample rate */		//s3c_i2s_set_clkdiv
	ret = cpu_dai->dai_ops.set_clkdiv(cpu_dai, S3C24XX_DIV_MCLK,
		S3C2410_IISMOD_32FS );
	if (ret < 0)
		return ret;

	/* set codec BCLK division for sample rate */	//lm49350_set_dai_clkdiv
	//ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8753_BCLKDIV, bclk);
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, 1, bclk);
	if (ret < 0)
		return ret;
#endif
	/* set prescaler division for sample rate */		// s3c_i2s_set_clkdiv
	ret = cpu_dai->ops->set_clkdiv(cpu_dai, S3C_DIV_PRESCALER,prescaler);
	if (ret < 0)
		return ret;

	s3cdbg("after :  IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
			 readl(regs + S3C64XX_IIS0CON), readl(regs + S3C64XX_IIS0MOD), 
			readl(regs + S3C64XX_IIS0FIC), readl(regs + S3C64XX_IIS0PSR));
	s3cdbg("	: EPLL_CON0: 0x%x EPLLCON1:0x%x CLK_SRC:0x%x CLK_DIV2:0x%x\n",\
		readl(S3C_EPLL_CON0),readl(S3C_EPLL_CON1),readl(S3C_CLK_SRC), readl(S3C_CLK_DIV2));

	return 0;
}

static int android_hifi_hw_free(struct snd_pcm_substream *substream)
{
#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;

	/* disable the PLL */
	return codec_dai->dai_ops.set_pll(codec_dai, WM8753_PLL1, 0, 0);
#endif
	return 0;
}
static int __init android_init(void)
{
	int ret;
	
	s3cdbg("[android_ak4671] android_init ++\n");

	android_snd_device = platform_device_alloc("soc-audio", -1);
	if (!android_snd_device)
	{	
		printk("[android_ak4671] soc-audio create fail \n");
		return -ENOMEM;
	}

    	s3cdbg("[android_ak4671] platform_set_drvdata\n");
	platform_set_drvdata(android_snd_device, &android_snd_devdata);
	android_snd_devdata.dev = &android_snd_device->dev;
	
	s3cdbg("[android_ak4671] platform_device_add\n");
	ret = platform_device_add(android_snd_device);

	if (ret)
		platform_device_put(android_snd_device);
	else
		printk("[android_ak4671] android_snd_device add fail \n");
	
	s3cdbg("[android_ak4671] android_init --\n");
	return ret;
}

static void __exit android_exit(void)
{
	platform_device_unregister(android_snd_device);
}

module_init(android_init);
module_exit(android_exit);

/* Module information */
MODULE_AUTHOR("SB Kang");
MODULE_DESCRIPTION("ALSA SoC AK4671");
MODULE_LICENSE("GPL");
