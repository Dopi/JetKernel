/*
 * wm8753.h  --  audio driver for WM8753
 *
 * Copyright 2003 Wolfson Microelectronics PLC.
 * Author: Liam Girdwood
 *         liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef _LM49350_H
#define _LM49350_H

/* LM49350 register space */
#define	LM49350_PMCSET	0x00
#define	LM49350_PMCCLK	0x01
#define	LM49350_PMCCLKDIV		0x02
#define	LM49350_PLL1M			0x04
#define	LM49350_PLL1N			0x05
#define	LM49350_PLL1MOD		0x06
#define	LM49350_PLL1P1		0x07
#define	LM49350_PLL1P2		0x08
#define	LM49350_PLL2M		0x09
#define	LM49350_PLL2N		0x0A
#define	LM49350_PLL2MOD	0x0B
#define	LM49350_PLL2P		0x0C
#define	LM49350_CLASSD	0x10
#define	LM49350_HPSL		0x11
#define	LM49350_HPSR		0x12
#define	LM49350_AUXOUT	0x13
#define	LM49350_OUTPUT_OP	0x14
#define	LM49350_ADC_LVL		0x15
#define	LM49350_MICL_LVL		0x16
#define	LM49350_MICR_LVL		0x17
#define	LM49350_AUXL_LVL		0x18
#define	LM49350_AUXR_LVL		0x19
#define	LM49350_ADC_BASIC		0x20
#define	LM49350_ADC_CLK		0x21
#define	LM49350_ADC_DSP		0x22
#define	LM49350_DAC_BASIC		0x30
#define	LM49350_DAC_CLK		0x31
#define	LM49350_DAC_DSP		0x32
#define	LM49350_IP_LVL1		0x40
#define	LM49350_IP_LVL2		0x41
#define	LM49350_OPPORT1		0x42
#define	LM49350_OPPORT2		0x43
#define	LM49350_OPDAC		0x44
#define	LM49350_OPDECI	0x45
#define	LM49350_AUD1_BASIC		0x50
#define	LM49350_AUD1_CLKGEN1		0x51
#define	LM49350_AUD1_CLKGEN2		0x52
#define	LM49350_AUD1_SYNCGEN		0x53
#define	LM49350_AUD1_DATAWD		0x54
#define	LM49350_AUD1_RX_MODE	0x55
#define	LM49350_AUD1_TX_MODE	0x56
#define	LM49350_AUD2_BASIC		0x60
#define	LM49350_AUD2_CLKGEN1		0x61
#define	LM49350_AUD2_CLKGEN2		0x62
#define	LM49350_AUD2_SYNCGEN		0x63
#define	LM49350_AUD2_DATAWD		0x64
#define	LM49350_AUD2_RX_MODE	0x65
#define	LM49350_AUD2_TX_MODE	0x66
#define	LM49350_ADCFX			0x70
#define	LM49350_DACFX			0x71
#define	LM49350_HPF			0x80
#define	LM49350_ADC_ALC1		0x81
#define	LM49350_ADC_ALC2		0x82
#define	LM49350_ADC_ALC3		0x83
#define	LM49350_ADC_ALC4		0x84
#define	LM49350_ADC_ALC5		0x85
#define	LM49350_ADC_ALC6		0x86
#define	LM49350_ADC_ALC7		0x87
#define	LM49350_ADC_ALC8		0x88
#define	LM49350_ADCL_LVL		0x89
#define	LM49350_ADCR_LVL		0x8A
#define	LM49350_ADC_EQBAND1		0x8B
#define	LM49350_ADC_EQBAND2		0x8C
#define	LM49350_ADC_EQBAND3		0x8D
#define	LM49350_ADC_EQBAND4		0x8E
#define	LM49350_ADC_EQBAND5		0x8F
#define	LM49350_ADC_SOFTCLIP1		0x90
#define	LM49350_ADC_SOFTCLIP2		0x91
#define	LM49350_ADC_SOFTCLIP3		0x92
#define	LM49350_ADC_LVLMONL		0x98
#define	LM49350_ADC_LVLMONR		0x99
#define	LM49350_ADC_FXCLIP			0x9A
#define	LM49350_ADC_ALCMONL		0x9B
#define	LM49350_ADC_ALCMONR		0x9C
#define	LM49350_DAC_ALC1		0xA0
#define	LM49350_DAC_ALC2		0xA1
#define	LM49350_DAC_ALC3		0xA2
#define	LM49350_DAC_ALC4		0xA3
#define	LM49350_DAC_ALC5		0xA4
#define	LM49350_DAC_ALC6		0xA5
#define	LM49350_DAC_ALC7		0xA6
#define	LM49350_DAC_ALC8		0xA7
#define	LM49350_DACL_LVL		0xA8
#define	LM49350_DACR_LVL		0xA9
#define	LM49350_DAC_3D		0xAA
#define	LM49350_DAC_EQBAND1		0xAB
#define	LM49350_DAC_EQBAND2		0xAC
#define	LM49350_DAC_EQBAND3		0xAD
#define	LM49350_DAC_EQBAND4		0xAE
#define	LM49350_DAC_EQBAND5		0xAF
#define	LM49350_DAC_SOFTCLIP1		0xB0
#define	LM49350_DAC_SOFTCLIP2		0xB1
#define	LM49350_DAC_SOFTCLIP3		0xB2
#define	LM49350_DAC_LVLMONL		0xB8
#define	LM49350_DAC_LVLMONR		0xB9
#define	LM49350_DAC_FXCLIP			0xBA
#define	LM49350_DAC_ALCMONL		0xBB
#define	LM49350_DAC_ALCMONR		0xBC
#define	LM49350_GPIO		0xE0
#define	LM49350_DEBUG		0xF0



/* WM8753 register space */

#define WM8753_DAC		0x01
#define WM8753_ADC		0x02
#define WM8753_PCM		0x03
#define WM8753_HIFI		0x04
#define WM8753_IOCTL		0x05
#define WM8753_SRATE1		0x06
#define WM8753_SRATE2		0x07
#define WM8753_LDAC		0x08
#define WM8753_RDAC		0x09
#define WM8753_BASS		0x0a
#define WM8753_TREBLE		0x0b
#define WM8753_ALC1		0x0c
#define WM8753_ALC2		0x0d
#define WM8753_ALC3		0x0e
#define WM8753_NGATE		0x0f
#define WM8753_LADC		0x10
#define WM8753_RADC		0x11
#define WM8753_ADCTL1		0x12
#define WM8753_3D		0x13
#define WM8753_PWR1		0x14
#define WM8753_PWR2		0x15
#define WM8753_PWR3		0x16
#define WM8753_PWR4		0x17
#define WM8753_ID		0x18
#define WM8753_INTPOL		0x19
#define WM8753_INTEN		0x1a
#define WM8753_GPIO1		0x1b
#define WM8753_GPIO2		0x1c
#define WM8753_RESET		0x1f
#define WM8753_RECMIX1		0x20
#define WM8753_RECMIX2		0x21
#define WM8753_LOUTM1		0x22
#define WM8753_LOUTM2		0x23
#define WM8753_ROUTM1		0x24
#define WM8753_ROUTM2		0x25
#define WM8753_MOUTM1		0x26
#define WM8753_MOUTM2		0x27
#define WM8753_LOUT1V		0x28
#define WM8753_ROUT1V		0x29
#define WM8753_LOUT2V		0x2a
#define WM8753_ROUT2V		0x2b
#define WM8753_MOUTV		0x2c
#define WM8753_OUTCTL		0x2d
#define WM8753_ADCIN		0x2e
#define WM8753_INCTL1		0x2f
#define WM8753_INCTL2		0x30
#define WM8753_LINVOL		0x31
#define WM8753_RINVOL		0x32
#define WM8753_MICBIAS		0x33
#define WM8753_CLOCK		0x34
#define WM8753_PLL1CTL1		0x35
#define WM8753_PLL1CTL2		0x36
#define WM8753_PLL1CTL3		0x37
#define WM8753_PLL1CTL4		0x38
#define WM8753_PLL2CTL1		0x39
#define WM8753_PLL2CTL2		0x3a
#define WM8753_PLL2CTL3		0x3b
#define WM8753_PLL2CTL4		0x3c
#define WM8753_BIASCTL		0x3d
#define WM8753_ADCTL2		0x3f

struct wm8753_setup_data {
	unsigned short i2c_address;
};

#define WM8753_PLL1			0
#define WM8753_PLL2			1

/* clock inputs */
#define WM8753_MCLK		0
#define WM8753_PCMCLK		1

/* clock divider id's */
#define WM8753_PCMDIV		0
#define WM8753_BCLKDIV		1
#define WM8753_VXCLKDIV		2

/* PCM clock dividers */
#define WM8753_PCM_DIV_1	(0 << 6)
#define WM8753_PCM_DIV_3	(2 << 6)
#define WM8753_PCM_DIV_5_5	(3 << 6)
#define WM8753_PCM_DIV_2	(4 << 6)
#define WM8753_PCM_DIV_4	(5 << 6)
#define WM8753_PCM_DIV_6	(6 << 6)
#define WM8753_PCM_DIV_8	(7 << 6)

/* BCLK clock dividers */
#define WM8753_BCLK_DIV_1	(0 << 3)
#define WM8753_BCLK_DIV_2	(1 << 3)
#define WM8753_BCLK_DIV_4	(2 << 3)
#define WM8753_BCLK_DIV_8	(3 << 3)
#define WM8753_BCLK_DIV_16	(4 << 3)

/* VXCLK clock dividers */
#define WM8753_VXCLK_DIV_1	(0 << 6)
#define WM8753_VXCLK_DIV_2	(1 << 6)
#define WM8753_VXCLK_DIV_4	(2 << 6)
#define WM8753_VXCLK_DIV_8	(3 << 6)
#define WM8753_VXCLK_DIV_16	(4 << 6)

#define WM8753_DAI_HIFI		0
#define WM8753_DAI_VOICE		1

extern struct snd_soc_dai wm8753_dai[2];
extern struct snd_soc_device soc_codec_dev_wm8753;


struct lm49350_setup_data {
	unsigned short i2c_address;
};

#define lm49350_DAI_HIFI		0
#define lm49350_DAI_VOICE		1

#define	MODE_PLAYBACK		0
#define	MODE_CAPTURE		1
extern struct snd_soc_dai lm49350_dai[2];
extern struct snd_soc_codec_device soc_codec_dev_lm49350;


/* audio path */
#define AUDIO_OUT_SEL		(1 << 0)
#define OUT_SPK			(AUDIO_OUT_SEL << 1)
#define OUT_EAR			(AUDIO_OUT_SEL << 2)
#define OUT_RCV			(AUDIO_OUT_SEL << 3)
#define OUT_PHONE		(AUDIO_OUT_SEL << 4)
#define OUT_BT			(AUDIO_OUT_SEL << 5)

#define AUDIO_MIC_SEL		(1 << 8) 
#define MIC_SEL_MASK		(MIC_MAIN|MIC_EAR|MIC_CAM|MIC_BT)
#define MIC_MAIN		(AUDIO_MIC_SEL << 1)
#define MIC_EAR			(AUDIO_MIC_SEL << 2)
#define MIC_CAM			(AUDIO_MIC_SEL << 3)
#define MIC_BT			(AUDIO_MIC_SEL << 4)

#define AUDIO_REC_SEL		(1 << 16)
#define ADC_MIC			(AUDIO_REC_SEL << 1)
#define ADC_PHONE		(AUDIO_REC_SEL << 2)
#define ADC_MIC_PHONE		(AUDIO_REC_SEL << 3) 

#define AUDIO_PLAY		(1 << 22)
#define AUDIO_PHONE		(AUDIO_PLAY << 1)
#define AUDIO_REC		(AUDIO_PLAY << 2) 
 
#define AUDIO_PLAY_OFF		(AUDIO_PLAY << 3) 
#define AUDIO_PHONE_OFF		(AUDIO_PLAY << 4) 
#define AUDIO_REC_OFF		(AUDIO_PLAY << 5) 
 
#define AUDIO_AMP_ON		(AUDIO_PLAY << 6) 
#define AUDIO_AMP_OFF		(AUDIO_PLAY << 7)

#endif
