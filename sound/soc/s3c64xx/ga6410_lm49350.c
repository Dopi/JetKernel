/*
 * smdk6400_wm8753.c  --  SoC audio for Neo1973
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
#include <sound/driver.h>
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

#include "../codecs/lm49350.h"
#include "s3c-pcm.h"
#include "s3c-i2s.h"

/* define the scenarios */
#define SMDK6400_AUDIO_OFF		0
#define SMDK6400_CAPTURE_MIC1		3
#define SMDK6400_STEREO_TO_HEADPHONES	2
#define SMDK6400_CAPTURE_LINE_IN	1


#define SUBJECT "ga6410_lm49350"
#define P(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define FI \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - IN" "\n", __func__, __LINE__, __func__);
#define FO \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - OUT" "\n", __func__, __LINE__, __func__);


#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) P(x)
#else
#define s3cdbg(x...)
#endif

#define S3C_VA_IIS          S3C_ADDR(0x03e40000) /*IIS*/
#define S3C24XX_VA_IIS     S3C_VA_IIS
#define S3C_IIS0REG(x)      ((x) + S3C24XX_VA_IIS)
#define S3C_IIS0MOD     S3C_IIS0REG(0x04)
#define S3C_IIS0PSR     S3C_IIS0REG(0x0C)  
#define S3C_IIS0MOD_256FS       (0x0<<3)

static int smdk6400_hifi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	unsigned int pll_out = 0, bclk = 0;
	int ret = 0;
	unsigned long iispsr, iismod,iiscon;
	unsigned long epll_con0=0;
	unsigned int prescaler = 4;
	unsigned int m,p,s,k = 0;
	unsigned long val;

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
	case 16000:
	case 32000:
	case 64000:
		//(49.152 / divider)
		m = 32; p = 2; s = 3; k= 50332;
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
		pll_out = 49152000;
		prescaler = 24; 
		break;
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

	iismod |= S3C_IIS0MOD_256FS;
	writel(iismod , (regs + S3C64XX_IIS0MOD));
	
	/* set codec DAI configuration */		//lm49350_mode1h_set_dai_fmt
	ret = codec_dai->dai_ops.set_fmt(codec_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS ); 
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */		//s3c_i2s_set_fmt
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai,
		SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
		SND_SOC_DAIFMT_CBS_CFS ); 
	if (ret < 0)
		return ret;

	/* set the codec system clock for DAC and ADC */		//	lm49350_set_dai_sysclk
	ret = codec_dai->dai_ops.set_sysclk(codec_dai, WM8753_MCLK, pll_out,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	/* set MCLK division for sample rate */		//s3c_i2s_set_clkdiv
//	ret = cpu_dai->dai_ops.set_clkdiv(cpu_dai, S3C24XX_DIV_MCLK,
//		S3C2410_IISMOD_32FS );
//	if (ret < 0)
//		return ret;

	/* set codec BCLK division for sample rate */	//lm49350_set_dai_clkdiv
	ret = codec_dai->dai_ops.set_clkdiv(codec_dai, WM8753_BCLKDIV, bclk);
	if (ret < 0)
		return ret;

	/* set prescaler division for sample rate */		// s3c_i2s_set_clkdiv
	//ret = cpu_dai->dai_ops.set_clkdiv(cpu_dai, S3C24XX_DIV_PRESCALER,prescaler);
	ret = cpu_dai->dai_ops.set_clkdiv(cpu_dai, 2,prescaler);
	if (ret < 0)
		return ret;

#if 0
	s3cdbg("after :  IISCON: %x IISMOD: %x,IISFIC: %x,IISPSR: %x\n",
			 readl(S3C64XX_IIS0REG(S3C64XX_IIS0CON)), readl(S3C64XX_IIS0REG(S3C64XX_IIS0MOD)), 
			readl(S3C64XX_IIS0REG(S3C64XX_IIS0FIC)), readl(S3C64XX_IIS0REG(S3C64XX_IIS0PSR)));
	s3cdbg("	: EPLL_CON0: 0x%x EPLLCON1:0x%x CLK_SRC:0x%x CLK_DIV2:0x%x\n",\
		readl(S3C_EPLL_CON0),readl(S3C_EPLL_CON1),readl(S3C_CLK_SRC), readl(S3C_CLK_DIV2));
#endif

	return 0;
}

static int smdk6400_hifi_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;

	/* disable the PLL */
	return codec_dai->dai_ops.set_pll(codec_dai, WM8753_PLL1, 0, 0);
}

/*
 * Neo1973 WM8753 HiFi DAI opserations.
 */
static struct snd_soc_ops smdk6400_hifi_ops = {
	.hw_params = smdk6400_hifi_hw_params,
	.hw_free = smdk6400_hifi_hw_free,
};

static int smdk6400_scenario = 0;

static int smdk6400_get_scenario(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = smdk6400_scenario;
	return 0;
}

static int set_scenario_endpoints(struct snd_soc_codec *codec, int scenario)
{
	s3cdbg("Entered %s, scenario = %d\n", __FUNCTION__, scenario);
	
	switch(smdk6400_scenario) {
	case SMDK6400_AUDIO_OFF:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack",    0);
		snd_soc_dapm_set_endpoint(codec, "Mic1 Jack",  0);
		snd_soc_dapm_set_endpoint(codec, "Line In Jack",  0);
		break;
	case SMDK6400_STEREO_TO_HEADPHONES:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack",    1);
		snd_soc_dapm_set_endpoint(codec, "Mic1 Jack",  0);
		snd_soc_dapm_set_endpoint(codec, "Line In Jack",  0);
		break;
	case SMDK6400_CAPTURE_MIC1:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack",    0);
		snd_soc_dapm_set_endpoint(codec, "Mic1 Jack",  1);
		snd_soc_dapm_set_endpoint(codec, "Line In Jack",  0);
		break;
	case SMDK6400_CAPTURE_LINE_IN:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack",    0);
		snd_soc_dapm_set_endpoint(codec, "Mic1 Jack",  0);
		snd_soc_dapm_set_endpoint(codec, "Line In Jack",  1);
		break;
	default:
		snd_soc_dapm_set_endpoint(codec, "Headphone Jack",    1);
		snd_soc_dapm_set_endpoint(codec, "Mic1 Jack",  1);
		snd_soc_dapm_set_endpoint(codec, "Line In Jack",  1);
		break;
	}

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

static int smdk6400_set_scenario(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	s3cdbg("Entered %s \n", __FUNCTION__);

	if (smdk6400_scenario == ucontrol->value.integer.value[0])
		return 0;

	smdk6400_scenario = ucontrol->value.integer.value[0];
	set_scenario_endpoints(codec, smdk6400_scenario);
	return 1;
}

static const struct snd_soc_dapm_widget wm8753_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_MIC("Mic1 Jack", NULL),
	SND_SOC_DAPM_LINE("Line In Jack", NULL),
};


/* example machine audio_mapnections */
static const char* audio_map[][3] = {

	{"Headphone Jack", NULL, "LOUT1"},
	{"Headphone Jack", NULL, "ROUT1"},

	/* mic is connected to mic1 - with bias */
	/* mic is connected to mic1 - with bias */
	{"MIC1", NULL, "Mic1 Jack"},

	{"LINE1", NULL, "Line In Jack"},
	{"LINE2", NULL, "Line In Jack"},

	/* Connect the ALC pins */
	{"ACIN", NULL, "ACOP"},
		
	{NULL, NULL, NULL},
};

static const char *smdk_scenarios[] = {
	"Off",
	"Capture Line In",
	"Headphones",
	"Capture Mic1",
};

static const struct soc_enum smdk_scenario_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(smdk_scenarios),smdk_scenarios),
};

static const struct snd_kcontrol_new wm8753_smdk6400_controls[] = {
	SOC_ENUM_EXT("SMDK Mode", smdk_scenario_enum[0],
		smdk6400_get_scenario, smdk6400_set_scenario),
};



/*
 * This is an example machine initialisation for a wm8753 connected to a
 * smdk6400. It is missing logic to detect hp/mic insertions and logic
 * to re-route the audio in such an event.
 */
static int smdk6400_wm8753_init(struct snd_soc_codec *codec)
{
	int i, err;

    s3cdbg("[smdk_wm8753] smdk6400_wm8753_init\n");
    
	/* set endpoints to default mode */
	set_scenario_endpoints(codec, SMDK6400_AUDIO_OFF);

	/* Add smdk6400 specific widgets */
	for (i = 0; i < ARRAY_SIZE(wm8753_dapm_widgets); i++)
		snd_soc_dapm_new_control(codec, &wm8753_dapm_widgets[i]);

	/* add smdk6400 specific controls */
	for (i = 0; i < ARRAY_SIZE(wm8753_smdk6400_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&wm8753_smdk6400_controls[i],
				codec, NULL));
		if (err < 0)
			return err;
	}

	/* set up smdk6400 specific audio path audio_mapnects */
	for (i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(codec, audio_map[i][0],
			audio_map[i][1], audio_map[i][2]);
	}

	/* always connected */
	snd_soc_dapm_set_endpoint(codec, "Mic1 Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Line In Jack", 1);

	snd_soc_dapm_sync_endpoints(codec);
	return 0;
}

static struct snd_soc_dai_link smdk6400_dai[] = {
	{ /* Hifi Playback - for similatious use with voice below */
		.name = "LM49350",
		.stream_name = "LM49350 HiFi",
		//.cpu_dai = &s3c_i2s_dai,
		.cpu_dai = &s3c_i2s_v40_dai,
		.codec_dai = &lm49350_dai[WM8753_DAI_HIFI],
		.init = smdk6400_wm8753_init,
		.ops = &smdk6400_hifi_ops,
	},
};

static struct snd_soc_machine smdk6400 = {
	.name = "smdk6400",
	.dai_link = smdk6400_dai,
	.num_links = ARRAY_SIZE(smdk6400_dai),
};

static struct wm8753_setup_data smdk6400_lm49350_setup = {
	.i2c_address = 0x1a,
};

static struct snd_soc_device smdk6400_snd_devdata = {
	.machine = &smdk6400,
	.platform = &s3c24xx_soc_platform,
	.codec_dev = &soc_codec_dev_lm49350,
	.codec_data = &smdk6400_lm49350_setup,
};
static struct platform_device *smdk6400_snd_device;

static int __init smdk6400_init(void)
{
	int ret;
	
	s3cdbg("[smdk_wm8753] smdk6400_init ++\n");

	smdk6400_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdk6400_snd_device)
		return -ENOMEM;

    s3cdbg("[smdk_wm8753] platform_set_drvdata\n");
	platform_set_drvdata(smdk6400_snd_device, &smdk6400_snd_devdata);
	smdk6400_snd_devdata.dev = &smdk6400_snd_device->dev;
	
	s3cdbg("[smdk_wm8753] platform_device_add\n");
	ret = platform_device_add(smdk6400_snd_device);

	if (ret)
		platform_device_put(smdk6400_snd_device);
	
	s3cdbg("[smdk_wm8753] smdk6400_init --\n");
	return ret;
}

static void __exit smdk6400_exit(void)
{
	platform_device_unregister(smdk6400_snd_device);
}

module_init(smdk6400_init);
module_exit(smdk6400_exit);

/* Module information */
MODULE_AUTHOR("Ryu Euiyoul");
MODULE_DESCRIPTION("ALSA SoC WM8753 Neo1973");
MODULE_LICENSE("GPL");
