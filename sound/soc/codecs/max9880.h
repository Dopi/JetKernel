/*
 * max9880.h -- MAX9880 ALSA SoC Audio driver
 *
 * Copyright 2009 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MAX9880_H
#define _MAX9880_H

/*
 *
 * MAX9880 register space
 *
 */
#define MAX9880_STATUS              0x00
#define MAX9880_JACK_STATUS         0x01
#define MAX9880_AUX_HIGH            0x02
#define MAX9880_AUX_LOW             0x03
#define MAX9880_INT_EN              0x04
#define MAX9880_SYS_CLK             0x05
#define MAX9880_DAI1_CLK_CTL_HIGH   0x06
#define MAX9880_DAI1_CLK_CTL_LOW    0x07
#define MAX9880_DAI1_MODE_A         0x08
#define MAX9880_DAI1_MODE_B         0x09
#define MAX9880_DAI1_TDM            0x0a
#define MAX9880_DAI2_CLK_CTL_HIGH   0x0b
#define MAX9880_DAI2_CLK_CTL_LOW    0x0c
#define MAX9880_DAI2_MODE_A         0x0d
#define MAX9880_DAI2_MODE_B         0x0e
#define MAX9880_DAI2_TDM            0x0f
#define MAX9880_DAC_MIXER           0x10
#define MAX9880_CODEC_FILTER        0x11
#define MAX9880_DSD_CONFIG          0x12
#define MAX9880_DSD_INPUT           0x13
#define MAX9880_SIDETONE            0x15
#define MAX9880_STEREO_DAC_LVL      0x16
#define MAX9880_VOICE_DAC_LVL       0x17
#define MAX9880_LEFT_ADC_LVL        0x18
#define MAX9880_RIGHT_ADC_LVL       0x19
#define MAX9880_LEFT_LINE_IN_LVL    0x1a
#define MAX9880_RIGHT_LINE_IN_LVL   0x1b
#define MAX9880_LEFT_VOL_LVL        0x1c
#define MAX9880_RIGHT_VOL_LVL       0x1d
#define MAX9880_LEFT_LINE_OUT_LVL   0x1e
#define MAX9880_RIGHT_LINE_OUT_LVL  0x1f
#define MAX9880_LEFT_MIC_GAIN       0x20
#define MAX9880_RIGHT_MIC_GAIN      0x21
#define MAX9880_INPUT_CONFIG        0x22
#define MAX9880_MIC_CONFIG          0x23
#define MAX9880_MODE_CONFIG         0x24
#define MAX9880_JACK_DETECT         0x25
#define MAX9880_PM_ENABLE           0x26
#define MAX9880_PM_SHUTDOWN         0x27
#define MAX9880_REVISION_ID         0xff

struct max9880_setup_data {
	unsigned short i2c_address;
};

extern struct snd_soc_dai max9880_dai;
extern struct snd_soc_codec_device soc_codec_dev_max9880;

#endif
