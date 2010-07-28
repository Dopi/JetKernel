/*
 * gumstix.c  --  SoC audio for Gumstix
 *
 * Copyright 2009 Maxim Integrated Products
 *
 * Authors: Jesse Marroquin <jesse.marroquin@maxim-ic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    20 March 2009 - Initial revision forked from gumstix.c
 *
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <asm/arch/pxa-regs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/audio.h>
#include <asm/arch/gumstix.h>

#include "pxa2xx-pcm.h"
#include "pxa2xx-i2s.h"
#include "../codecs/max9880.h"

#define MA9867EVKIT_AUDIO_CLOCK 12288000

/*
 *
 *
 *
 */
static int gumstix_startup(struct snd_pcm_substream *substream)
{
	return 0;
}


/*
 *
 *
 *
 */
static void gumstix_shutdown(struct snd_pcm_substream *substream)
{
}


/*
 *
 *
 *
 */
static int gumstix_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	int ret = 0;

#if 0
	Right now the driver is using a default setup. This function can be added back
	at a later time if required.
	/* set codec DAI configuration */
	ret = codec_dai->dai_ops.set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
#endif

	/* set cpu DAI configuration */
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

#if 0
	/* set the codec system clock for DAC and ADC */
	ret = codec_dai->dai_ops.set_sysclk(codec_dai, MAX9888_SYSCLK, MA9867EVKIT_AUDIO_CLOCK,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;
#endif

	/* set the I2S system clock as input (unused) */
	ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, PXA2XX_I2S_SYSCLK, 0,
		SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}


static struct snd_soc_ops gumstix_ops = {
	.startup = gumstix_startup,
	.hw_params = gumstix_hw_params,
	.shutdown = gumstix_shutdown,
};


static const struct snd_soc_dapm_widget max9880_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line in", NULL),
};

static const char *audio_map[][3] = {
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},
	{"LMICIN",         NULL, "Mic Jack"},
	{"RMICIN",         NULL, "Mic Jack"},
	{"LLINEIN",        NULL, "Line in"},
	{"RLINEIN",        NULL, "Line in"},
	{NULL, NULL, NULL},
};


/*
 *
 * Logic for a MAX9888 as connected on a Gumstix SBC
 *
 */
static int gumstix_max9880_init(struct snd_soc_codec *codec)
{
	int i;

	/* Add widgets */
	for(i = 0; i < ARRAY_SIZE(max9880_dapm_widgets); i++) {
		snd_soc_dapm_new_control(codec, &max9880_dapm_widgets[i]);
	}

	/* Set audio path audio_map */
	for(i = 0; audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(codec, audio_map[i][0],
			audio_map[i][1], audio_map[i][2]);
	}

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

/* gumstix digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link gumstix_dai = {
	.name = "MAX9880",
	.stream_name = "MAX9880",
	.cpu_dai = &pxa_i2s_dai,
	.codec_dai = &max9880_dai,
	.init = gumstix_max9880_init,
	.ops = &gumstix_ops,
};

/* gumstix audio machine driver */
static struct snd_soc_machine snd_soc_machine_gumstix = {
	.name = "Gumstix",
	.dai_link = &gumstix_dai,
	.num_links = 1,
};

/* gumstix audio private data */
static struct max9880_setup_data gumstix_max9880_setup = {
	/*
		The I2C address of the MAX9888 is 0x20. To the I2C
		driver the address is a 7-bit number hence the right
		shift and the value 0x10.
	*/
	.i2c_address = 0x10,
};

/* gumstix audio subsystem */
static struct snd_soc_device gumstix_snd_devdata = {
	.machine = &snd_soc_machine_gumstix,
	.platform = &pxa2xx_soc_platform,
	.codec_dev = &soc_codec_dev_max9880,
	.codec_data = &gumstix_max9880_setup,
};

static struct platform_device *gumstix_snd_device;


/*
 *
 *
 *
 */
static int __init gumstix_init(void)
{
	int ret;

	/* Start off by making sure this is running on the Gumstix platform */
	if (!machine_is_gumstix())
		return -ENODEV;

	/*
		Allocate and initialize a platform device object. The platform device
		is not ALSA specific and falls under the more general platform device/driver
		model.
	*/
	gumstix_snd_device = platform_device_alloc("soc-audio", -1);
	if (!gumstix_snd_device)
		return -ENOMEM;

	/*
		Attach device/driver data to platform device object and attempt
		to add the device to the platform hierarchy.
	*/
	platform_set_drvdata(gumstix_snd_device, &gumstix_snd_devdata);
	gumstix_snd_devdata.dev = &gumstix_snd_device->dev;
	ret = platform_device_add(gumstix_snd_device);

	/* Free the platform device object if adding it to the hierarchy failed. */
	if (ret)
		platform_device_put(gumstix_snd_device);

	return ret;
}


/*
 *
 *
 *
 */
static void __exit gumstix_exit(void)
{
	platform_device_unregister(gumstix_snd_device);
}


module_init(gumstix_init);
module_exit(gumstix_exit);

MODULE_AUTHOR("Jesse Marroquin");
MODULE_DESCRIPTION("ALSA SoC MAX9880");
MODULE_LICENSE("GPL");

