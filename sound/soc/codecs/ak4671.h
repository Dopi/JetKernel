/*
 * ak4535.h  --  AK4535 Soc Audio driver
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <richard@openedhand.com>
 *
 * Based on wm8753.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <sound/soc.h>

#ifndef _AK4671_H
#define _AK4671_H

/* AK4671 Path */
#define MM_AUDIO_OUT_RCV 			0x01
#define MM_AUDIO_OUT_SPK 			0x02
#define MM_AUDIO_OUT_HP 			0x03
#define MM_AUDIO_OUT_BT 			0x04
#define MM_AUDIO_OUT_SPK_HP			0x05
#define MM_AUDIO_OUT_RING_SPK_HP	0x06
#define MM_AUDIO_PLAYBACK			0x00
#define MM_AUDIO_PLAYBACK_RCV 		0x01
#define MM_AUDIO_PLAYBACK_SPK 		0x02
#define MM_AUDIO_PLAYBACK_HP 		0x03
#define MM_AUDIO_PLAYBACK_BT 		0x04
#define MM_AUDIO_PLAYBACK_SPK_HP	0x05
#define MM_AUDIO_PLAYBACK_RING_SPK_HP	0x06
#define MM_AUDIO_VOICECALL	 		0x10
#define MM_AUDIO_VOICECALL_RCV 		0x11
#define MM_AUDIO_VOICECALL_SPK 		0x12
#define MM_AUDIO_VOICECALL_HP 		0x13
#define MM_AUDIO_VOICECALL_BT 		0x14
#define MM_AUDIO_VOICECALL_SPK_LOOP 	0x15
#define MM_AUDIO_VOICEMEMO	 		0x20
#define MM_AUDIO_VOICEMEMO_MAIN		0x21
#define MM_AUDIO_VOICEMEMO_SUB 		0x22
#define MM_AUDIO_VOICEMEMO_EAR 		0x23
#define MM_AUDIO_VOICEMEMO_BT 		0x24
#define MM_AUDIO_FMRADIO	 		0x30
#define MM_AUDIO_FMRADIO_RCV		0x31
#define MM_AUDIO_FMRADIO_SPK 		0x32
#define MM_AUDIO_FMRADIO_HP 		0x33
#define MM_AUDIO_FMRADIO_BT 		0x34
#define MM_AUDIO_FMRADIO_SPK_HP      0x35

#define AK4671_PATH_CHANGE	 		0xf1

#define IDLE_POWER_DOWN_MODE_OFF		0
#define IDLE_POWER_DOWN_MODE_ON		1

/* AK4671 register space */
#define AK4671_PM					0x00
#define AK4671_PLL_MODE0			0x01
#define AK4671_PLL_MODE1			0x02
#define AK4671_FORMAT				0x03
#define AK4671_MIC_SELECT			0x04
#define AK4671_MIC_GAIN				0x05
#define AK4671_MIXING_PM0			0x06
#define AK4671_MIXING_PM1			0x07
#define AK4671_OUTPUT_VOL			0x08
#define AK4671_LOUT1_SELECT			0x09
#define AK4671_ROUT1_SELECT			0x0A
#define AK4671_LOUT2_SELECT			0x0B
#define AK4671_ROUT2_SELECT			0x0C
#define AK4671_LOUT3_SELECT			0x0D
#define AK4671_ROUT3_SELECT			0x0E
#define AK4671_LOUT1_PM				0x0F
#define AK4671_LOUT2_PM				0x10
#define AK4671_LOUT3_PM				0x11
#define AK4671_L_INPUT_VOL			0x12
#define AK4671_R_INPUT_VOL			0x13
#define AK4671_ALC_REF_SELECT		0x14
#define AK4671_DIGITAL_MIXING		0x15
#define AK4671_ALC_TIMER_SELECT		0x16
#define AK4671_ALC_MODE_CONTROL		0x17
#define AK4671_MODE_CONTROL1		0x18
#define AK4671_MODE_CONTROL2		0x19
#define AK4671_L_OUTPUT_VOL			0x1A
#define AK4671_R_OUTPUT_VOL			0x1B
#define AK4671_SIDE_TONE_A			0x1C
#define AK4671_DIGITAL_FILTER		0x1D
#define AK4671_FIL3_0				0x1E
#define AK4671_FIL3_1				0x1F
#define AK4671_FIL3_2				0x20
#define AK4671_FIL3_3				0x21
#define AK4671_EQ_0					0x22
#define AK4671_EQ_1					0x23
#define AK4671_EQ_2					0x24
#define AK4671_EQ_3					0x25
#define AK4671_EQ_4					0x26
#define AK4671_EQ_5					0x27
#define AK4671_FIL1_0				0x28
#define AK4671_FIL1_1				0x29
#define AK4671_FIL1_2				0x2A
#define AK4671_FIL1_3				0x2B
#define AK4671_FIL2_0				0x2C
#define AK4671_FIL2_1				0x2D
#define AK4671_FIL2_2				0x2E
#define AK4671_FIL2_3				0x2F
#define AK4671_DIGITAL_FILTER2		0x30
#define AK4671_E1_0					0x32
#define AK4671_E1_1					0x33
#define AK4671_E1_2					0x34
#define AK4671_E1_3					0x35
#define AK4671_E1_4					0x36
#define AK4671_E1_5					0x37
#define AK4671_E2_0					0x38
#define AK4671_E2_1					0x39
#define AK4671_E2_2					0x3A
#define AK4671_E2_3					0x3B
#define AK4671_E2_4					0x3C
#define AK4671_E2_5					0x3D
#define AK4671_E3_0					0x3E
#define AK4671_E3_1					0x3F
#define AK4671_E3_2					0x40
#define AK4671_E3_3					0x41
#define AK4671_E3_4					0x42
#define AK4671_E3_5					0x43
#define AK4671_E4_0					0x44
#define AK4671_E4_1					0x45
#define AK4671_E4_2					0x46
#define AK4671_E4_3					0x47
#define AK4671_E4_4					0x48
#define AK4671_E4_5					0x49
#define AK4671_E5_0					0x4A
#define AK4671_E5_1					0x4B
#define AK4671_E5_2					0x4C
#define AK4671_E5_3					0x4D
#define AK4671_E5_4					0x4E
#define AK4671_E5_5					0x4F
#define AK4671_EQ_CONTROL1			0x50
#define AK4671_EQ_CONTROL2			0x51
#define AK4671_EQ_CONTROL3			0x52
#define AK4671_PCM_CONTROL0			0x53
#define AK4671_PCM_CONTROL1			0x54
#define AK4671_PCM_CONTROL2			0x55
#define AK4671_DIGITAL_VOL_B		0x56
#define AK4671_DIGITAL_VOL_C		0x57
#define AK4671_SIDE_TONE_VOL		0x58
#define AK4671_DIGITAL_MIXING2		0x59
#define AK4671_SAR_ADC				0x5A

#define AK4671_CACHEREGNUM 	0x5B

#define AK4535_PM1		0x0
#define AK4535_PM2		0x1
#define AK4535_SIG1		0x2
#define AK4535_SIG2		0x3
#define AK4535_MODE1		0x4
#define AK4535_MODE2		0x5
#define AK4535_DAC		0x6
#define AK4535_MIC		0x7
#define AK4535_TIMER		0x8
#define AK4535_ALC1		0x9
#define AK4535_ALC2		0xa
#define AK4535_PGA		0xb
#define AK4535_LATT		0xc
#define AK4535_RATT		0xd
#define AK4535_VOL		0xe
#define AK4535_STATUS		0xf

#define AK4535_CACHEREGNUM 	0x10


/* 1 : Speaker
 * 2 : Earjack */
#define AK4671_AMP_PATH_SPK				1
#define AK4671_AMP_PATH_HP				2
#define AK4671_AMP_PATH_SPK_HP			3

/* 0 : MAIN
 * 1 : SUB */
#define AK4671_MIC_PATH_MAIN			0
#define AK4671_MIC_PATH_SUB				1

struct ak4671_setup_data {
	unsigned short i2c_address;
};

/* codec private data */
struct ak4671_priv {
	unsigned int sysclk;
	unsigned int bitrate;
};

extern struct snd_soc_dai ak4671_dai;
extern struct snd_soc_codec_device soc_codec_dev_ak4671;

/* Board specific function */
extern int voice_call_auto_response_disable(struct snd_soc_codec *, int);
extern int voice_call_auto_response_enable(struct snd_soc_codec *, int);
extern int audio_init(void);
extern int audio_power(int);
extern int amp_init(void);
extern int amp_enable(int);
extern int amp_set_path(int);
extern int amp_get_path(int);
extern int amp_register_string(char *);
extern int amp_set_register(unsigned char, unsigned char);
extern int mic_enable(int);
extern int mic_set_path(int);
extern int path_enable(struct snd_soc_codec *, int);
extern int path_disable(struct snd_soc_codec *, int);
extern int set_sample_rate(struct snd_soc_codec *, int);
extern int idle_mode_enable(struct snd_soc_codec *, int);
extern int idle_mode_disable(struct snd_soc_codec *, int);
extern int voice_call_rec_enable(struct snd_soc_codec *, int);
extern int voice_call_rec_disable(struct snd_soc_codec *, int);


#endif
