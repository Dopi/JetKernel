/*
 * ak4671.c  --  AK4671 ALSA Soc Audio driver
 *
 * Copyright (C) 2008 Samsung Electronics, Seung-Bum Kang
 *
 * Based on ak4535.c by Richard Purdie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/i2c/maximi2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include <plat/egpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/hardware.h>

#include "ak4671.h"

#define AUDIO_NAME "ak4671"
#define AK4671_VERSION "0.2"

#define SUBJECT "ak4671.c"

//#define AUDIO_SPECIFIC_DEBUG	1

#define P1(format,...)\
	printk ("["SUBJECT "(%d)] " format "\n", __LINE__, ## __VA_ARGS__);

#if AUDIO_SPECIFIC_DEBUG
#define P(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define FI \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - IN" "\n", __func__, __LINE__, __func__);
#define FO \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - OUT" "\n", __func__, __LINE__, __func__);
#else
#define P(format,...)
#define FI 
#define FO 
#endif

#define DIGITAL_FILTER_CONTROL		0

struct snd_soc_codec_device soc_codec_dev_ak4671;
static int set_registers(struct snd_soc_codec *, int);

static const char *audio_path[] = { "Playback Path", 
									"Voice Call Path", 
									"Voice Memo Path", 
									"FM Radio Path", 
									"MIC Path", 
									NULL};

/* first 8bit : audio_path id
 * last  8bit : path id */
int ak4671_path = 0;

/* spk <-> hp path value */
int ak4671_amp_path = 0;

/* main <-> sub mic path value */
int ak4671_mic_path = AK4671_MIC_PATH_MAIN; 

/* for idle current */
int ak4671_idle_mode = IDLE_POWER_DOWN_MODE_OFF; 

/* Recording mode */
int ak4671_voice_call_rec_mode = 0; 
/* voicecall response mode */
int ak4671_voice_call_auto_response_mode = 0;

/* ak4671 AUDIO_EN control */
static int ak4671_power = 0;


/*
 * ak4671 register cache
 */
static const u16 ak4671_reg[AK4671_CACHEREGNUM] = {
    0x0000, 0x00F6, 0x0000, 0x0002, 0x0000, 0x0055, 0x0000, 0x0000, /* 0x00 ~ 0x07 */
    0x00B5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x08 ~ 0x0F */

    0x0000, 0x0080, 0x0091, 0x0091, 0x00E1, 0x0000, 0x0000, 0x0000, /* 0x10 ~ 0x17 */
    0x0002, 0x0001, 0x0018, 0x0018, 0x0000, 0x0002, 0x0000, 0x0000, /* 0x18 ~ 0x1F */

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x20 ~ 0x27 */
    0x00A9, 0x001F, 0x0020, 0x00AD, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x28 ~ 0x2F */

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x30 ~ 0x37 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x38 ~ 0x3F */

    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x40 ~ 0x47 */
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, /* 0x48 ~ 0x4F */

    0x0088, 0x0088, 0x0008, 0x0000, 0x0000, 0x0000, 0x0018, 0x0018, /* 0x50 ~ 0x57 */
    0x0000, 0x0000, 0x0000					 						/* 0x58 ~ 0x5A */
};
static u16 ak4671_reg_default[AK4671_CACHEREGNUM];

/*
 * read ak4671 register cache
 */
static inline unsigned int ak4671_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u16 *cache = codec->reg_cache;

	if (reg >= AK4671_CACHEREGNUM)
		return -1;
	return cache[reg];
}

/*
 * read to the AK4671 register space
 */
static inline unsigned int ak4671_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u8 data;
	data = reg;

	if (codec->hw_write(codec->control_data, &data, 1) != 1)
		return -EIO;

	if (codec->hw_read(codec->control_data, &data, 1) != 1)
		return -EIO;

	return data;
};

/*
 * write ak4671 register cache
 */
static inline void ak4671_write_reg_cache(struct snd_soc_codec *codec,
	u16 reg, unsigned int value)
{
	u16 *cache = codec->reg_cache;
	if (reg >= AK4671_CACHEREGNUM)
		return;
	cache[reg] = value;
}

/*
 * write to the AK4671 register space
 */
static int ak4671_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[2];

	/* data is
	 *   D15..D8 AK4671 register offset
	 *   D7...D0 register data
	 */
	data[0] = reg & 0xff;
	data[1] = value & 0xff;

	//P("--- ak4671 i2c --- write : reg - 0x%02x, val - 0x%02x", reg, value);
	ak4671_write_reg_cache(codec, reg, value);
	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}

static int ak4671_sync(struct snd_soc_codec *codec)
{
	u16 *cache = codec->reg_cache;

	memcpy(cache, ak4671_reg_default, sizeof(ak4671_reg_default));
	
	return 0;
};

static int ak4671_get_idle_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4671_idle_mode;
	return 0;
}

static int ak4671_set_idle_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	P1(" idle_mode_value : %d", (int)ucontrol->value.integer.value[0]);

	if(ak4671_power == 0 && ak4671_idle_mode == IDLE_POWER_DOWN_MODE_ON)
	{
		P1("audio power up");
		set_registers(codec, ak4671_path);
		return 1;
	}

	if ( (ak4671_path & 0xf0) == MM_AUDIO_PLAYBACK)
	{
		if (ucontrol->value.integer.value[0] == 0 && ak4671_idle_mode == IDLE_POWER_DOWN_MODE_ON) { // Off
			idle_mode_enable(codec, ak4671_path);
		} else if (ucontrol->value.integer.value[0] == 1 && ak4671_idle_mode == IDLE_POWER_DOWN_MODE_OFF) { // On
			idle_mode_disable(codec, ak4671_path);
		} else {
			P1("invalid idle mode value");
			return -1;
		}

		ak4671_idle_mode = ucontrol->value.integer.value[0];
	} else 
		P1("only Playback mode!");

	return 1;
}

static int get_external_amp_power(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	//ucontrol->value.integer.value[0] = ak4671_idle_mode;
	return 0;
}

static int set_external_amp_power(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	P("amp power : %d", (int)ucontrol->value.integer.value[0]);

	if (ucontrol->value.integer.value[0] == 0) { // Off
		amp_enable(0);
	} else if (ucontrol->value.integer.value[0] == 1) { // On
		amp_enable(1);
	} else {
		P("invalid value");
		return -1;
	}

	return 1;
}

static int ak4671_get_voice_call_auto_response_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4671_voice_call_rec_mode;
	return 0;
}

static int ak4671_set_voice_call_auto_response_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	P1("Auto Response Mode On/Off :%d, Voicecall Mode :%d", ucontrol->value.integer.value[0], ak4671_voice_call_auto_response_mode);
	
	if ( (ak4671_path & 0xf0) == MM_AUDIO_VOICECALL ) 
	{
		if (ucontrol->value.integer.value[0] == 0 && ak4671_voice_call_auto_response_mode == 1) { // Off
			voice_call_auto_response_disable(codec, ak4671_path);
		} else if (ucontrol->value.integer.value[0] == 1 && ak4671_voice_call_auto_response_mode == 0) { // On
			voice_call_auto_response_enable(codec, ak4671_path);
		} else {
			P("invalid voicecall auto response mode value");
			return -1;
		}
		ak4671_voice_call_auto_response_mode = ucontrol->value.integer.value[0];
	} else
		printk("invalid voicecall auto response mode value");

	return 1;
}

static int ak4671_get_voice_call_rec_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4671_voice_call_rec_mode;
	return 0;
}

static int ak4671_set_voice_call_rec_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	P1("Voiceccall Rec Mode On/Off:%d, Voicecall Mode :%d", ucontrol->value.integer.value[0], ak4671_voice_call_rec_mode);

	if ( (ak4671_path & 0xf0) == MM_AUDIO_VOICECALL ) 
	{
		if (ucontrol->value.integer.value[0] == 0 && ak4671_voice_call_rec_mode == 1) { // Off
			voice_call_rec_disable(codec, ak4671_path);
		} else if (ucontrol->value.integer.value[0] == 1 && ak4671_voice_call_rec_mode == 0) { // On
			voice_call_rec_enable(codec, ak4671_path);
		} else {
			P("invalid recording mode value");
			return -1;
		}
		ak4671_voice_call_rec_mode = ucontrol->value.integer.value[0];
	} else
		printk("invalid recording mode value");

	return 1;
}

static int ak4671_get_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = ak4671_mic_path;
	return 0;
}

static int ak4671_set_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	P("");
	if (ucontrol->value.integer.value[0] == 0) { // MAIN MIC
		mic_set_path(AK4671_MIC_PATH_MAIN);
	} else if (ucontrol->value.integer.value[0] == 1) { // SUB MIC
		mic_set_path(AK4671_MIC_PATH_SUB);
	} else {
		P("invalid Mic value");
		return -1;
	}
	ak4671_mic_path = ucontrol->value.integer.value[0];

	return 1;
}

static int ak4671_get_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int i = 0;
	while(audio_path[i] != NULL) {
		if(!strcmp(audio_path[i], kcontrol->id.name) && ((ak4671_path >> 4) == i)) {
			ucontrol->value.integer.value[0] = ak4671_path & 0xf;
			break;
		}
		i++;
	}
	return 0;
}

static int ak4671_set_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int i = 0, new_path;

	P("path_state : 0x%02x, input path = 0x%02x", ak4671_path
			, (unsigned int)((i << 4) | ucontrol->value.integer.value[0]) );
	while(audio_path[i] != NULL) {
		new_path = (i << 4) | ucontrol->value.integer.value[0];
		if(!strcmp(audio_path[i], kcontrol->id.name) && ak4671_path != new_path)  {

			set_registers(codec, new_path);

			break;
		}
		i++;
	}
	return 1;
}

static const char *playback_path[] = { "Off", "RCV", "SPK", "HP", "BT", "SPK_HP", "R_SPK_HP", };
static const char *voicecall_path[] = { "Off", "RCV", "SPK", "HP", "BT", "SPK_LOOP"};
static const char *voicememo_path[] = { "Off", "MAIN", "SUB", "EAR", "BT", };
static const char *fmradio_path[] = { "Off", "RCV", "SPK", "HP", "BT", };
static const char *mic_path[] = { "Main Mic", "Sub Mic", };
static const char *idle_mode[] = { "Off", "ON" };
static const char *voicecall_rec_mode[] = { "Off", "ON" };
static const char *external_amp_control[] = { "Off", "ON" };
static const char *voicecall_auto_response_control[] = { "Off", "ON" };

static const struct soc_enum path_control_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(playback_path),playback_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_path),voicecall_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicememo_path),voicememo_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fmradio_path),fmradio_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_path),mic_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(idle_mode),idle_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_rec_mode),voicecall_rec_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(external_amp_control),external_amp_control),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_auto_response_control),voicecall_auto_response_control),
};

static const struct snd_kcontrol_new ak4671_snd_controls[] = {
	/* Volume Control */
	SOC_SINGLE("Output Volume L", AK4671_L_OUTPUT_VOL, 0, 0x30, 0),
	SOC_SINGLE("Output Volume R", AK4671_R_OUTPUT_VOL, 0, 0x30, 0),
	SOC_SINGLE("Output Volume - RCV", AK4671_OUTPUT_VOL, 0, 0x7, 0),
	SOC_SINGLE("Output Volume - SPK/EAR", AK4671_OUTPUT_VOL, 4, 0xf, 0),
	SOC_SINGLE("Output Volume - MUTE", AK4671_MODE_CONTROL2, 2, 1, 0),

	/* Path Control */
	SOC_ENUM_EXT("Playback Path", path_control_enum[0],
		ak4671_get_path, ak4671_set_path),
	SOC_ENUM_EXT("Voice Call Path", path_control_enum[1],
		ak4671_get_path, ak4671_set_path),
	SOC_ENUM_EXT("Voice Memo Path", path_control_enum[2],
		ak4671_get_path, ak4671_set_path),
	SOC_ENUM_EXT("MIC Path", path_control_enum[4],
		ak4671_get_mic_path, ak4671_set_mic_path),

	/* MIC Gain */
	SOC_DOUBLE("MIC Gain", 							AK4671_MIC_GAIN, 0, 4, 0xf, 0),

	SOC_ENUM_EXT("FM Radio Path", path_control_enum[3],
		ak4671_get_path, ak4671_set_path),

	SOC_ENUM_EXT("Idle Mode", path_control_enum[5],
		ak4671_get_idle_mode, ak4671_set_idle_mode),

	SOC_ENUM_EXT("Voice Call Rec Mode", path_control_enum[6],
		ak4671_get_voice_call_rec_mode, ak4671_set_voice_call_rec_mode),

	SOC_ENUM_EXT("External Amp Power", path_control_enum[7],
		get_external_amp_power, set_external_amp_power),

	SOC_ENUM_EXT("Voice Call Auto Response Mode", path_control_enum[8],
		ak4671_get_voice_call_auto_response_mode, ak4671_set_voice_call_auto_response_mode),
		
#if DIGITAL_FILTER_CONTROL
	/* ALC Control */
	SOC_SINGLE("ALC Enable", 						AK4671_MODE_CONTROL1, 0, 1, 0),
	SOC_SINGLE("ALC Limiter detection level / Recovery Counter Reset level",
													AK4671_ALC_MODE_CONTROL, 0, 3, 0),
	SOC_SINGLE("ALC Recovery GAIN Step",  			AK4671_ALC_MODE_CONTROL, 2, 3, 0),
	SOC_SINGLE("ALC ATT Step",			  			AK4671_ALC_MODE_CONTROL, 4, 3, 0),
	SOC_SINGLE("ALC Zero Crossing detection Enable",AK4671_ALC_MODE_CONTROL, 6, 1, 0),
	SOC_SINGLE("ALC Reference Select", 				AK4671_ALC_REF_SELECT, 0, 0xff, 0),
	SOC_SINGLE("ALC Timer Select", 					AK4671_ALC_TIMER_SELECT, 0, 0xff, 0),

	/* Digital Filter */
	SOC_SINGLE("Signal Select of Programmable Filter Block", 	AK4671_DIGITAL_FILTER, 0, 1, 0),
	SOC_SINGLE("Gain Select at GAIN Block", 	AK4671_DIGITAL_FILTER, 6, 3, 0),
	SOC_SINGLE("Digital Volume Transition Time Setting", 	AK4671_MODE_CONTROL2, 1, 1, 0),

	SOC_SINGLE("FIL3 Coefficient Setting Enable", 	AK4671_DIGITAL_FILTER, 2, 1, 0),
	SOC_SINGLE("FIL3 Co-efficient 0 (F3A7-F3A0)", 	AK4671_FIL3_0, 0, 0xff, 0),
	SOC_SINGLE("FIL3 Co-efficient 1 (F3A13-F3A8)", 	AK4671_FIL3_1, 0, 0x6f, 0),
	SOC_SINGLE("FIL3 Co-efficient 2 (F3B7-F3B0)", 	AK4671_FIL3_2, 0, 0xff, 0),
	SOC_SINGLE("FIL3 Co-efficient 3 (F3B13-F3B8)", 	AK4671_FIL3_3, 0, 0x6f, 0),

	SOC_SINGLE("EQ0 Coefficient Setting Enable", 	AK4671_DIGITAL_FILTER, 3, 1, 0),
	SOC_SINGLE("EQ0 Co-efficient 0 (E0A7-F3A0)", 	AK4671_EQ_0, 0, 0xff, 0),
	SOC_SINGLE("EQ0 Co-efficient 1 (E0A15-F3A8)", 	AK4671_EQ_1, 0, 0xff, 0),
	SOC_SINGLE("EQ0 Co-efficient 2 (E0B7-F3B0)", 	AK4671_EQ_2, 0, 0xff, 0),
	SOC_SINGLE("EQ0 Co-efficient 3 (E0B13-F3B8)", 	AK4671_EQ_3, 0, 0x6f, 0),
	SOC_SINGLE("EQ0 Co-efficient 4 (E0C7-F3C0)", 	AK4671_EQ_4, 0, 0xff, 0),
	SOC_SINGLE("EQ0 Co-efficient 5 (E0C15-F3C8)", 	AK4671_EQ_5, 0, 0xff, 0),

	SOC_SINGLE("HPF Coefficient Setting Enable", 	AK4671_DIGITAL_FILTER, 4, 1, 0),
	SOC_SINGLE("FIL1 Co-efficient 0 (F1A7-F1A0)", 	AK4671_FIL1_0, 0, 0xff, 0),
	SOC_SINGLE("FIL1 Co-efficient 1 (F1A13-F1A8)", 	AK4671_FIL1_1, 0, 0x6f, 0),
	SOC_SINGLE("FIL1 Co-efficient 2 (F1B7-F1B0)", 	AK4671_FIL1_2, 0, 0xff, 0),
	SOC_SINGLE("FIL1 Co-efficient 3 (F1B13-F1B8)", 	AK4671_FIL1_3, 0, 0x6f, 0),

	SOC_SINGLE("LPF Coefficient Setting Enable", 	AK4671_DIGITAL_FILTER, 5, 1, 0),
	SOC_SINGLE("FIL2 Co-efficient 0 (F2A7-F2A0)", 	AK4671_FIL2_0, 0, 0xff, 0),
	SOC_SINGLE("FIL2 Co-efficient 1 (F2A13-F2A8)", 	AK4671_FIL2_1, 0, 0x6f, 0),
	SOC_SINGLE("FIL2 Co-efficient 2 (F2B7-F2B0)", 	AK4671_FIL2_2, 0, 0xff, 0),
	SOC_SINGLE("FIL2 Co-efficient 3 (F2B13-F2B8)", 	AK4671_FIL2_3, 0, 0x6f, 0),

	/* Equalizer Coefficient */
	SOC_SINGLE("Select 5-Band Equalizer", 					AK4671_MODE_CONTROL1, 3, 1, 0),

	SOC_SINGLE("Equalizer 1 Enable", 	AK4671_DIGITAL_FILTER2, 0, 1, 0),
	SOC_SINGLE("Equalizer 2 Enable", 	AK4671_DIGITAL_FILTER2, 1, 1, 0),
	SOC_SINGLE("Equalizer 3 Enable", 	AK4671_DIGITAL_FILTER2, 2, 1, 0),
	SOC_SINGLE("Equalizer 4 Enable", 	AK4671_DIGITAL_FILTER2, 3, 1, 0),
	SOC_SINGLE("Equalizer 5 Enable", 	AK4671_DIGITAL_FILTER2, 4, 1, 0),

	SOC_SINGLE("E1 Co-efficient 0", 	AK4671_E1_0, 0, 0xff, 0),
	SOC_SINGLE("E1 Co-efficient 1", 	AK4671_E1_1, 0, 0xff, 0),
	SOC_SINGLE("E1 Co-efficient 2", 	AK4671_E1_2, 0, 0xff, 0),
	SOC_SINGLE("E1 Co-efficient 3", 	AK4671_E1_3, 0, 0xff, 0),
	SOC_SINGLE("E1 Co-efficient 4", 	AK4671_E1_4, 0, 0xff, 0),
	SOC_SINGLE("E1 Co-efficient 5", 	AK4671_E1_5, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 0", 	AK4671_E2_0, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 1", 	AK4671_E2_1, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 2", 	AK4671_E2_2, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 3", 	AK4671_E2_3, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 4", 	AK4671_E2_4, 0, 0xff, 0),
	SOC_SINGLE("E2 Co-efficient 5", 	AK4671_E2_5, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 0", 	AK4671_E3_0, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 1", 	AK4671_E3_1, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 2", 	AK4671_E3_2, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 3", 	AK4671_E3_3, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 4", 	AK4671_E3_4, 0, 0xff, 0),
	SOC_SINGLE("E3 Co-efficient 5", 	AK4671_E3_5, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 0", 	AK4671_E4_0, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 1", 	AK4671_E4_1, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 2", 	AK4671_E4_2, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 3", 	AK4671_E4_3, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 4", 	AK4671_E4_4, 0, 0xff, 0),
	SOC_SINGLE("E4 Co-efficient 5", 	AK4671_E4_5, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 0", 	AK4671_E5_0, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 1", 	AK4671_E5_1, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 2", 	AK4671_E5_2, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 3", 	AK4671_E5_3, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 4", 	AK4671_E5_4, 0, 0xff, 0),
	SOC_SINGLE("E5 Co-efficient 5", 	AK4671_E5_5, 0, 0xff, 0),
#endif
};

/* add non dapm controls */
static int ak4671_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(ak4671_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
			snd_soc_cnew(&ak4671_snd_controls[i], codec, NULL));
		if (err < 0)
			return err;
	}

	return 0;
}

static int ak4671_set_dai_sysclk(struct snd_soc_dai *codec_dai,
	int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct ak4671_priv *ak4671 = codec->private_data;

	if (ak4671->sysclk != freq) {
		set_sample_rate(codec, freq);
		ak4671->sysclk = freq;
	}

	return 0;
}

static int ak4671_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
#if 0
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct ak4671_priv *ak4671 = codec->private_data;
	u8 mode2 = ak4671_read_reg_cache(codec, AK4671_MODE2) & ~(0x3 << 5);
	int rate = params_rate(params), fs = 256;

	if (rate)
		fs = ak4671->sysclk / rate;

	/* set fs */
	switch (fs) {
	case 1024:
		mode2 |= (0x2 << 5);
		break;
	case 512:
		mode2 |= (0x1 << 5);
		break;
	case 256:
		break;
	}

	/* set rate */
	ak4671_write(codec, ak4671_mode2, mode2);
#endif
	return 0;
}

static int ak4671_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 mode1 = 0;

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		mode1 = 0x03;
		break;
	default:
		printk("!! invalid interface format !!\n");
		return -EINVAL;
	}

	ak4671_write(codec, AK4671_FORMAT, mode1);

	return 0;
}

static int ak4671_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	P("mute %d", mute);

	if(ak4671_path == MM_AUDIO_VOICECALL_BT)
		return 0;

	u16 mute_reg = ak4671_read_reg_cache(codec, AK4671_MODE_CONTROL2) & 0xfffb;
	if (!mute)
		ak4671_write(codec, AK4671_MODE_CONTROL2, mute_reg | 1);
	else
		ak4671_write(codec, AK4671_MODE_CONTROL2, mute_reg | 0x4 | 1);
	
	return 0;
}

static int ak4671_set_bias_level(struct snd_soc_codec *codec,
	enum snd_soc_bias_level level)
{

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->bias_level = level;
	return 0;
}

#define AK4671_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
		SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000)

struct snd_soc_dai ak4671_dai = {
	.name = "AK4671",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AK4671_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = AK4671_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = {
		.hw_params = ak4671_hw_params,
		.set_fmt = ak4671_set_dai_fmt,
		.digital_mute = ak4671_mute,
		.set_sysclk = ak4671_set_dai_sysclk,
	},
};
EXPORT_SYMBOL_GPL(ak4671_dai);

static int ak4671_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	P("");

	ak4671_set_bias_level(codec, SND_SOC_BIAS_OFF);

	if ( (ak4671_path & 0xf0) != MM_AUDIO_VOICECALL && (ak4671_path & 0xf0) != MM_AUDIO_VOICEMEMO ) 
	{
		path_disable(codec, ak4671_path);

		/* AUDIO_EN & MAX8906_AMP_EN Disable */
		amp_enable(0); /* Board Specific function */
		audio_power(0); /* Board Specific function */
		//mic_enable(0); /* Board Specific function */
		ak4671_power = 0;
		ak4671_idle_mode = IDLE_POWER_DOWN_MODE_ON;
	}

	return 0;
}

static int ak4671_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	P("");

	ak4671_sync(codec);
	ak4671_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	ak4671_set_bias_level(codec, codec->suspend_bias_level);

#if 0
	if ( (ak4671_path & 0xf0) != MM_AUDIO_VOICECALL && (ak4671_path & 0xf0) != MM_AUDIO_VOICEMEMO )
	{
		set_registers(codec, ak4671_path);
	}
#endif

	return 0;
}

/*
 * initialise the AK4671 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int ak4671_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;

	memcpy(ak4671_reg_default, ak4671_reg, sizeof(ak4671_reg)); // copy ak4671 default register

	codec->name = "AK4671";
	codec->owner = THIS_MODULE;
	codec->read = ak4671_read_reg_cache;
	codec->write = ak4671_write;
	codec->set_bias_level = ak4671_set_bias_level;
	codec->dai = &ak4671_dai;
	codec->num_dai = 1;
	codec->reg_cache_size = ARRAY_SIZE(ak4671_reg);
	codec->reg_cache = kmemdup(ak4671_reg, sizeof(ak4671_reg), GFP_KERNEL);

	if (codec->reg_cache == NULL)
		return -ENOMEM;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "ak4671: failed to create pcms\n");
		goto pcm_err;
	}

	/* power on device */
	ak4671_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	ak4671_add_controls(codec);
	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "ak4671: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);

	return ret;
}

static struct snd_soc_device *ak4671_socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

#define I2C_DRIVERID_AK4671 0xfefe /* liam -  need a proper id */

static unsigned short normal_i2c[] = { 0, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

static struct i2c_driver ak4671_i2c_driver;
static struct i2c_client ak4671_client;

static unsigned short ak4671_normal_i2c[] = { 0x24 >> 1, I2C_CLIENT_END };
static unsigned short ak4671_ignore[] = { 1, 0x24 >> 1, I2C_CLIENT_END };
static unsigned short ak4671_i2c_probe[] = { I2C_CLIENT_END };

static struct i2c_client_address_data ak4671_addr_data = {
	.normal_i2c = ak4671_normal_i2c,
	.ignore     = ak4671_ignore,
	.probe      = ak4671_i2c_probe,
};

static void amp_path_control(int mode)
{
	int amp_path;
	/* SPK <- HP */
	if((mode & 0xf) == MM_AUDIO_OUT_SPK)
	{
		amp_path = AK4671_AMP_PATH_SPK;
	}

	/* SPK -> HP */
	if((mode & 0xf) == MM_AUDIO_OUT_HP)
	{ 
		amp_path = AK4671_AMP_PATH_HP;
	}

	/* SPK & HP */
	if(((mode & 0xf) == MM_AUDIO_OUT_SPK_HP) || ((mode & 0xf) == MM_AUDIO_OUT_RING_SPK_HP))
	{ 
		amp_path = AK4671_AMP_PATH_SPK_HP;
	}

	if((mode & 0xf0) == MM_AUDIO_VOICEMEMO)
	{
		if((mode & 0xf) == MM_AUDIO_OUT_HP)
			amp_path = AK4671_AMP_PATH_HP;
		else if((mode & 0xf) != MM_AUDIO_OUT_RCV) // fix by sircid
			amp_path = AK4671_AMP_PATH_SPK;
	}

	if (ak4671_amp_path != amp_path && ak4671_path != mode) 
	{
		amp_set_path(amp_path);
		ak4671_amp_path = amp_path;
	} 
}

static int set_registers(struct snd_soc_codec *codec, int mode)
{
	P1("Set Audio PATH : 0x%02x\n", mode);
	path_disable(codec, ak4671_path);

	/* voice call rec MODE off */
	if (ak4671_voice_call_rec_mode != 0)
		ak4671_voice_call_rec_mode = 0; 

	amp_path_control(mode);

	if (ak4671_power == 0) {
		audio_power(1); /* Board Specific function */
		ak4671_power = 1;
	}

	path_enable(codec, mode);

	ak4671_idle_mode = IDLE_POWER_DOWN_MODE_OFF; // IDLE Mode reset (off)
	ak4671_path = mode;

	return 0;
}

/* sysfs control */
int hex2dec(u8 ch)
{
	if(ch >= '0' && ch <= '9')
		return ch - '0';
	else if(ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else if(ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

static ssize_t ak4671_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 i;
	struct snd_soc_device *socdev = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = socdev->codec;

	sprintf(buf, "\nMODE:%d\r\n", ak4671_path);
	for (i = 0; i < AK4671_CACHEREGNUM; i++) {
		if( i < 0x1E || i > 0x4f )
			sprintf(buf, "%s[0x%02x] = 0x%02x\r\n", buf, i, ak4671_read(codec, i));
		if( i == 0x30 )
			sprintf(buf, "%s[0x%02x] = 0x%02x\r\n", buf, i, ak4671_read(codec, i));
		//printk("ak4671_reg[%02x] = 0x%02x\n", i, ak4671_reg[i]);
	}
	sprintf(buf, "%sAMP Register : ", buf);
	amp_register_string(buf);

	return sprintf(buf, "%s[AK4671] register read done.\r\n", buf);
}

static ssize_t ak4671_control_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	u8 reg, value = 0;
	int ret = 0;
	struct snd_soc_device *socdev = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = socdev->codec;

	printk("echo [REGISTER NUMBER(HEX)][VALUE(HEX)] > ak4671_control\n");
	printk("ex) echo 030f > ak4671_control\n");

	P("buf = %s", buf);
	P("buf size = %d", sizeof(buf));
	P("buf size = %d", strlen(buf));

	if(sizeof(buf) != 4) {
		printk("input error\n");
		printk("store ex) echo 030f\n");
		return -1;
	}

	ret = hex2dec(buf[0]);
	if (ret == -1) {
		printk("store error.\n");
		return -1;
	}
	reg = ret << 4;

	ret = hex2dec(buf[1]);
	if (ret == -1) {
		printk("store error.\n");
		return -1;
	}
	reg |= (ret & 0xf);

	ret = hex2dec(buf[2]);
	if (ret == -1) {
		printk("store error.\n");
		return -1;
	}
	value = ret << 4;

	ret = hex2dec(buf[3]);
	if (ret == -1) {
		printk("store error.\n");
		return -1;
	}
	value |= (ret & 0xf);

	if (reg == 0xf1) { // path control
		set_registers(codec, value);
	} else if (reg >= 0xe0 && reg <= 0xe5)
		amp_set_register(reg - 0xe0, value);
	else
		ak4671_write(codec, reg, value);
	printk("Set  : reg = 0x%02x, value = 0x%02x\n", reg, value);
	printk("Read : reg = 0x%02x, value = 0x%02x\n", reg, ak4671_read(codec, reg));

	return size;
}
static DEVICE_ATTR(ak4671_control, S_IRUGO | S_IWUSR, ak4671_control_show, ak4671_control_store);


/* If the i2c layer weren't so broken, we could pass this kind of data
   around */
static int ak4671_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = ak4671_socdev;
	struct ak4671_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;

	if (addr != setup->i2c_address)
		return -ENODEV;
	ak4671_client.adapter = adap;
	ak4671_client.addr = addr;

	i2c = kmemdup(&ak4671_client, sizeof(ak4671_client), GFP_KERNEL);
	if (i2c == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		printk(KERN_ERR "failed to attach codec at addr %x\n", addr);
		goto err;
	}

	ret = ak4671_init(socdev);
	if (ret < 0) {
		printk(KERN_ERR "failed to initialise AK4671\n");
		goto err;
	}

	set_registers(codec, MM_AUDIO_PLAYBACK_SPK);

#if 0 // i2c test
	p("---> 0x01 = 0x%02x\n", ak4671_read(codec, 0x01)); 	
	ak4671_write(codec, 0x01, 0x78); 	
	P("---> 0x01 = 0x%02x\n", ak4671_read(codec, 0x01)); 	

	P("---> 0x02 = 0x%02x\n", ak4671_read(codec, 0x02)); 	
	ak4671_write(codec, 0x02, 0x01); 
	P("---> 0x02 = 0x%02x\n", ak4671_read(codec, 0x02)); 	

	P("---> 0x03 = 0x%02x\n", ak4671_read(codec, 0x03)); 	
	ak4671_write(codec, 0x03, 0x03);
	P("---> 0x03 = 0x%02x\n", ak4671_read(codec, 0x03)); 	
#endif

	return ret;

err:
	kfree(i2c);
	return ret;
}

static int ak4671_i2c_detach(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);

	i2c_detach_client(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}

static int ak4671_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &ak4671_addr_data, ak4671_codec_probe);
}

/* corgi i2c codec control layer */
static struct i2c_driver ak4671_i2c_driver = {
	.driver = {
		.name = "AK4671 I2C Codec",
		.owner = THIS_MODULE,
	},
	.id =             I2C_DRIVERID_AK4671,
	.attach_adapter = ak4671_i2c_attach,
	.detach_client =  ak4671_i2c_detach,
	.command =        NULL,
};

static struct i2c_client ak4671_client = {
	.name =   "AK4671",
	.driver = &ak4671_i2c_driver,
};
#endif

static int ak4671_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct ak4671_setup_data *setup;
	struct snd_soc_codec *codec;
	struct ak4671_priv *ak4671;
	int ret = 0;

	printk(KERN_INFO "AK4671 Audio Codec %s\n", AK4671_VERSION);

	/* Board Specific function */
	audio_init();
	if (amp_init() < 0)
		printk("amp init failed.\n");
	audio_power(1);
	ak4671_power = 1;
	amp_set_path(AK4671_AMP_PATH_SPK);
	mic_set_path(AK4671_MIC_PATH_MAIN);

	ret = device_create_file(&pdev->dev, &dev_attr_ak4671_control);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
	{
		printk("[sound]snd_soc_codec create fail\n");
		return -ENOMEM;
	}

	ak4671 = kzalloc(sizeof(struct ak4671_priv), GFP_KERNEL);
	if (ak4671 == NULL) {
		printk("[sound]ak4671_priv create fail\n");
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = ak4671;
	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ak4671_socdev = socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		normal_i2c[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t)i2c_master_send;
		codec->hw_read = (hw_read_t)i2c_master_recv;
		ret = i2c_add_driver(&ak4671_i2c_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add i2c driver");
	}
#else
	/* Add other interfaces here */
#endif

	if (ret != 0) {
		printk("[sound] sound device create file fail\n");
		kfree(codec->private_data);
		kfree(codec);
	}
	return ret;
}

/* power down chip */
static int ak4671_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	if (codec->control_data)
		ak4671_set_bias_level(codec, SND_SOC_BIAS_OFF);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_del_driver(&ak4671_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

/* power down chip */
static int ak4671_shutdown(struct platform_device *pdev)
{
	P1(" ");

	amp_enable(0);
	
	return 0;
}


struct snd_soc_codec_device soc_codec_dev_ak4671 = {
	.probe = 	ak4671_probe,
	.remove = 	ak4671_remove,
	.suspend = 	ak4671_suspend,
	.resume =	ak4671_resume,
	.shutdown =	ak4671_shutdown,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak4671);

static int __init ak4671_codec_init(void)
{
	return snd_soc_register_dai(&ak4671_dai);	
}
module_init(ak4671_codec_init);

static void __exit ak4671_codec_exit(void)
{
	snd_soc_unregister_dai(&ak4671_dai);
}
module_exit(ak4671_codec_exit);

MODULE_DESCRIPTION("Soc AK4671 driver");
MODULE_AUTHOR("Richard Purdie");
MODULE_LICENSE("GPL");
