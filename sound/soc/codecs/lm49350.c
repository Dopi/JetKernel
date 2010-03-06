
/* LM49350 modified */

/*
 * wm8753.c  --  WM8753 ALSA Soc Audio driver
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
 * Notes:
 *  The WM8753 is a lopw power, high quality stereo codec with integrated PCM
 *  codec designed for portable digital telephony applications.
 *
 * Dual DAI:-
 *
 * This driver support 2 DAI PCM's. This makes the default PCM available for
 * HiFi audio (e.g. MP3, ogg) playback/capture and the other PCM available for
 * voice.
 *
 * Please note that the voice PCM can be connected directly to a Bluetooth
 * codec or GSM modem and thus cannot be read or written to, although it is
 * available to be configured with snd_hw_params(), etc and kcontrols in the
 * normal alsa manner.
 *
 * Fast DAI switching:-
 *
 * The driver can now fast switch between the DAI configurations via a
 * an alsa kcontrol. This allows the PCM to remain open.
 *
 */
 

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <asm/div64.h>

#include "lm49350.h"
#include "i2c-emul.h"
#include <mach/map.h>
#include <asm/io.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-iic.h>
#include <mach/hardware.h>

#define AUDIO_NAME "lm49350"



/*
 * Debug
 */
#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

#define	LM_MASTER_TEST	0

#define	LM_PLAY_TEST		0

#if LM_PLAY_TEST
#define	LM_REC_TEST		0
#else 
#define	LM_REC_TEST		1
#endif

#define	EMUL_TEST			0

#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)


#define S3C_IICREG(x) ((x) + S3C24XX_VA_IIC)
#define M_IDLE          0x00    // Disable Rx/Tx
#define M_ACTIVE        0x10    // Enable  Rx/Tx
#define MTX_START       0xF0    // Master Tx Start
#define MTX_STOP        0xD0    // Master Tx Stop
#define MRX_START       0xB0    // Master Rx Start
#define MRX_STOP        0x90    // Master Rx Stop
#define RESUME_ACK      0xA4    // clear interrupt pending bit, ACK enabled, div=16, IICCLK=PCLK/16
#define RESUME_NO_ACK   0x24 


static int caps_charge = 2000;
module_param(caps_charge, int, 0);
MODULE_PARM_DESC(caps_charge, "WM8753 cap charge time (msecs)");


/* codec private data */
struct lm49350_priv {
	unsigned int sysclk;
	unsigned int pcmclk;
};
/*
 * lm49350 register cache (0x00 ~ 0xF0)
 */
static const u8 lm49350_reg[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,	// 0x00 ~ 0x0F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,	0xFF, 0xFF,	// 0x10 ~ 0x1F
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 0x20 ~ 0x2F
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 	// 0x30 ~ 0x3F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 	// 0x40 ~ 0x4F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 	// 0x50 ~ 0x5F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 	// 0x60 ~ 0x6F
	0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  	// 0x70 ~ 0x7F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  	// 0x80 ~ 0x8F
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 	// 0x90 ~ 0x9F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  	// 0xA0 ~ 0xAF
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,  	// 0xB0 ~ 0xBF
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  	// 0xC0 ~ 0xCF
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  	// 0xD0 ~ 0xDF
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 	// 0xE0 ~ 0xEF
	0x00
};


/*
 * read lm49350 register cache
 */
static inline unsigned int lm49350_read_reg_cache(struct snd_soc_codec *codec,
	unsigned int reg)
{
	u8 *cache = codec->reg_cache;
	if (reg < 0 || reg > (ARRAY_SIZE(lm49350_reg) + 1))
		return -1;

	s3cdbg("[lm49350] lm49350_read_reg_cache : reg 0x%x, val : 0x%x\n",reg,cache[reg]);
	
	return cache[reg];
}

/*
 * write lm49350 register cache
 */
static inline void lm49350_write_reg_cache(struct snd_soc_codec *codec,
	unsigned char reg, unsigned char value)
{
	u8 *cache = codec->reg_cache;
	if (reg < 0 || reg > 0xF0)
		return;
	cache[reg] = value;
}

/*
 * write to the WM8753 register space
 */
static int lm49350_write(struct snd_soc_codec *codec, unsigned char reg,
	unsigned int value)
{
	u8 data[2];

	s3cdbg("######### %s : reg = 0x%x, value = 0x%x\n",__FUNCTION__,reg,value);

	/* data is
	 *   D15..D9 WM8753 register offset
	 *   D8...D0 register data
	 */
	//data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	//data[1] = value & 0x00ff;
	data[0] = reg;
	data[1] = value;

	lm49350_write_reg_cache (codec, reg, value);
	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}

static int lm49350_read(struct snd_soc_codec *codec, unsigned char* pReg,
	unsigned char *pBuf, int count)
{
#if 1
	int ret, i;
	unsigned char addr = 0x1a;

	/*
	GPB2 - CPU_SCL
	GPB3 - CPU_SDA
	*/
	//printk("   before : GPBCON = 0x%x, GPBDAT = 0x%x, GPBPU = 0x%x\n",__raw_readl(S3C_GPBCON), __raw_readl(S3C_GPBDAT),__raw_readl(S3C_GPBPU));
	//printk("   set GPC6,7 output port \n");
	
#if 1
	if (gpio_is_valid(GPIO_I2C1_SCL)) {
		if (gpio_request(GPIO_I2C1_SCL, S3C_GPIO_LAVEL(GPIO_I2C1_SCL))) 
			printk(KERN_ERR "Failed to request GPIO_I2C1_SCL!\n");
		gpio_direction_output(GPIO_I2C1_SCL, 1);
		gpio_set_value(GPIO_I2C1_SCL, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)

	if (gpio_is_valid(GPIO_I2C1_SDA)) {
		if (gpio_request(GPIO_I2C1_SDA, S3C_GPIO_LAVEL(GPIO_I2C1_SDA))) 
			printk(KERN_ERR "Failed to request GPIO_I2C1_SDA!\n");
		gpio_direction_output(GPIO_I2C1_SDA, 1);
		gpio_set_value(GPIO_I2C1_SDA, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
#else
	s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_OUTP);
	s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_OUTP);
	s3c_gpio_pullup(S3C_GPB2, 2);
	s3c_gpio_pullup(S3C_GPB3, 2);
#endif
	//printk("   after : GPBCON = 0x%x, GPBDAT = 0x%x, GPBPU = 0x%x\n",__raw_readl(S3C_GPBCON), __raw_readl(S3C_GPBDAT),__raw_readl(S3C_GPBPU));


	mdelay(10);
	for(i = 0; i < count ; i++)
	{
		if((ret = i2c_emul_read(addr,pReg[i],&pBuf[i])) < 0)
		{
			break;
		}
	}


//	writel((readl(S3C_PCLK_GATE)|(0x1 << 27) ), S3C_PCLK_GATE);	// pass IIC1
	
    // setup GPIO for I2C
#if 1
	gpio_free(GPIO_I2C1_SCL);
	gpio_free(GPIO_I2C1_SDA);

	s3c_gpio_cfgpin(GPIO_I2C1_SCL, S3C_GPIO_SFN(GPIO_I2C1_SCL_AF));	// IIC SCL (GPB5)
	s3c_gpio_cfgpin(GPIO_I2C1_SDA, S3C_GPIO_SFN(GPIO_I2C1_SDA_AF));	// IIC SDA (GPB6)
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)
#else
	s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_I2C_SCL);	// IIC SCL (GPB5)
	s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_I2C_SDA);	// IIC SDA (GPB6)
	s3c_gpio_pullup(S3C_GPB2, 0x2); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_pullup(S3C_GPB3, 0x2); // IIC SCL Pull-Up enable (GPB6)
#endif

    // config controller
//	writel(M_IDLE, S3C_IICREG1(S3C2410_IICSTAT));
//	writel(RESUME_ACK, S3C_IICREG1(S3C2410_IICCON));
//	writel(0x10, S3C_IICREG1(S3C2410_IICADD));
	
	return i;
#else
	return 0;
#endif
}

static void print_lm49350regs(struct snd_soc_codec *codec)
{
	unsigned char reg = 0,val = 0;
	//int i=0;

	for(reg = 0; reg<0xF0 ; reg++)
	{
		if(lm49350_reg[reg] != 0xFF)
			lm49350_read(codec, &reg, &val,1);
			//lm49350_read_reg_cache(codec,reg);
		//printk(" 0x%x :  0x%x \r\n", reg, val);
	}
}

#if 0
/* Not Used */
static int lm49350_get_dai(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec =  snd_kcontrol_chip(kcontrol);
	int mode = lm49350_read_reg_cache(codec, WM8753_IOCTL);

	ucontrol->value.integer.value[0] = (mode & 0xc) >> 2;
	return 0;
}

static int lm49350_set_dai(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec =  snd_kcontrol_chip(kcontrol);
	int mode = lm49350_read_reg_cache(codec, WM8753_IOCTL);

    s3cdbg("%s\n",__FUNCTION__);

	if (((mode &0xc) >> 2) == ucontrol->value.integer.value[0])
		return 0;

	mode &= 0xfff3;
	mode |= (ucontrol->value.integer.value[0] << 2);

	return 1;
}
#endif

static const struct snd_kcontrol_new lm49350_snd_controls[] = {
SOC_SINGLE("MicL Volume", LM49350_MICL_LVL, 0, 15, 0),
SOC_SINGLE("MicR Volume", LM49350_MICR_LVL, 0, 15, 0),
SOC_DOUBLE_R("ADC Capture Volume", LM49350_ADCL_LVL, LM49350_ADCR_LVL, 0, 63, 0),
SOC_DOUBLE_R("DAC Playback Volume", LM49350_DACL_LVL, LM49350_DACR_LVL, 0, 63, 0),

{
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Audio Play",
	.info = snd_soc_info_audio_play_path,
	.put = snd_soc_put_audio_play_path,
	.get = snd_soc_get_audio_play_path,
},

{
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Audio Phone",
	.info = snd_soc_info_audio_phone_path,
	.put = snd_soc_put_audio_phone_path,
	.get = snd_soc_get_audio_phone_path,
},

{
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Audio Rec",
	.info = snd_soc_info_audio_rec_path,
	.put = snd_soc_put_audio_rec_path,
	.get = snd_soc_get_audio_rec_path,
},

{
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.name = "Audio Mic",
	.info = snd_soc_info_audio_mic_path,
	.put = snd_soc_put_audio_mic_path,
	.get = snd_soc_get_audio_mic_path,
}
	
};

/* add non dapm controls */
static int lm49350_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);

	for (i = 0; i < ARRAY_SIZE(lm49350_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				snd_soc_cnew(&lm49350_snd_controls[i],codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

/* 
 * _DAPM_ Controls for LM49350
 */
#if 0
/* Not Used */
static const struct snd_kcontrol_new lm49350_hp_left_mixer_controls[] ={
};

static const struct snd_kcontrol_new lm49350_hp_right_mixer_controls[] ={
};

static const struct snd_kcontrol_new lm49350_aux_mixer_controls[] ={
};

static const char *lm49350_audio_map[][3] = {
};

static int lm49350_add_widgets(struct snd_soc_codec *codec)
{
	int i;

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
	
	for (i = 0; i < ARRAY_SIZE(lm49350_dapm_widgets); i++)
		snd_soc_dapm_new_control(codec, &lm49350_dapm_widgets[i]);

	/* set up the LM49350 audio map */
	for (i = 0; lm49350_audio_map[i][0] != NULL; i++) {
		snd_soc_dapm_connect_input(codec, lm49350_audio_map[i][0],
			lm49350_audio_map[i][1], lm49350_audio_map[i][2]);
	}

	snd_soc_dapm_new_widgets(codec);

	s3cdbg("	snd_soc_dapm_new_widgets ++++\n");
	return 0;
}
#endif

/* PLL divisors */
struct _pll_div {
	u32 div2:1;
	u32 n:4;
	u32 k:24;
};

/* The size in bits of the pll divide multiplied by 10
 * to allow rounding later */
#if 0
/* Not Used */
#define FIXED_PLL_SIZE ((1 << 22) * 10)

static void pll_factors(struct _pll_div *pll_div, unsigned int target,
	unsigned int source)
{
	u64 Kpart;
	unsigned int K, Ndiv, Nmod;

    s3cdbg("[lm49350] pll_factors \n");

	Ndiv = target / source;
	if (Ndiv < 6) {
		source >>= 1;
		pll_div->div2 = 1;
		Ndiv = target / source;
	} else
		pll_div->div2 = 0;

	if ((Ndiv < 6) || (Ndiv > 12))
		printk(KERN_WARNING
			"WM8753 N value outwith recommended range! N = %d\n",Ndiv);

	pll_div->n = Ndiv;
	Nmod = target % source;
	Kpart = FIXED_PLL_SIZE * (long long)Nmod;

	do_div(Kpart, source);

	K = Kpart & 0xFFFFFFFF;

	/* Check if we need to round */
	if ((K % 10) >= 5)
		K += 5;

	/* Move down to proper range now rounding is done */
	K /= 10;

	pll_div->k = K;
}
#endif

static int lm49350_set_dai_pll(struct snd_soc_dai *codec_dai,
		int pll_id, unsigned int freq_in, unsigned int freq_out)
{
	s3cdbg("[lm49350] %s, pll_id = %d, freq_in = %d, freq_out = %d \n",__FUNCTION__, pll_id, freq_in, freq_out);

	// Off lm49350 PLLs
	return 0;
}

struct _coeff_div {
	u32 mclk;
	u32 rate;
	u8 sr:5;
	u8 usb:1;
};

/*
 * Clock after PLL and dividers
 */
static int lm49350_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
	s3cdbg("%s ++++\n",__FUNCTION__);
	return 0;
}

/*
 * Set's ADC and Voice DAC format.
 */
/*
static int lm49350_vdac_adc_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{ ... }
*/

/*
 * Set PCM DAI bit size and sample rate.
 */
 static int lm49350_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	s3cdbg("[lm49350] %s ++++\n",__FUNCTION__);
	return 0;
}

/*
 * Set's PCM dai fmt and BCLK.
 */
 static int lm49350_pcm_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	return 0;
}

static int lm49350_set_dai_clkdiv(struct snd_soc_dai *codec_dai,
		int div_id, int div)
{
	s3cdbg("%s ++++\n",__FUNCTION__);
	return 0;
}

/*
 * Set's HiFi DAC format.
 */
/*
static int lm49350_hdac_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{ ... }
*/

/*
 * Set's I2S DAI format.
 */
/*
static int lm49350_i2s_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{ ... }
*/

/*
 * Set PCM DAI bit size and sample rate.
 */
static int lm49350_i2s_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
	
	return 0;
}

static int lm49350_set_mode(struct snd_soc_dai *codec_dai,struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_codec * codec = codec_dai->codec;
	unsigned char reg_val = 0xFF;
	static int	last_mode = -1;
	
	s3cdbg("[lm49350] %s ++++,cmd = %d\n",__FUNCTION__, cmd);

	if(substream->stream == last_mode)
		return 0;
	
	if(substream->stream == SNDRV_PCM_STREAM_CAPTURE)	// settings for REC
	{
		s3cdbg("----------- set RECORD mode ------------\n");

		last_mode = 1;	//capture == 1
		
		lm49350_write(codec,0x01,0x00);
		lm49350_write(codec,0x02,0x00);
		lm49350_write(codec,0x03,0x00);
		lm49350_write(codec,0x04,0x04);
		lm49350_write(codec,0x05,0x20);
		lm49350_write(codec,0x07,0x00);
		lm49350_write(codec,0x08,0x95);
		lm49350_write(codec,0x15,0x04);
		lm49350_write(codec,0x17,0x04);
		lm49350_write(codec,0x20,0x30);
		lm49350_write(codec,0x43,0x24);
		lm49350_write(codec,0x44,0x00);
		lm49350_write(codec,0x45,0x00);
		lm49350_write(codec,0x60,0x05);
		lm49350_write(codec,0x64,0x00);
		lm49350_write(codec,0x65,0x00);
		lm49350_write(codec,0x66,0x02);
		lm49350_write(codec,0x80,0x04);
		lm49350_write(codec,0x81,0x05);
		lm49350_write(codec,0xa0,0x05);
		lm49350_write(codec,0x00,0x03);	// chip enable, PLL 1 enable
	}
	else		// settings for PLAY
	{
		s3cdbg("-----------set PLAY mode  ------------\n");

		last_mode = 0; 	// play == 0
		
		lm49350_write(codec,0x01,0x00);	// select  MCLK 
		lm49350_write(codec,0x02,0x00);	// PMC_CLK_DIV = 1
		lm49350_write(codec,0x03,0x02);	// PLL1_CLK = PORT2_CLK
		lm49350_write(codec,0x05,0x7D);	// PLL1 DIVIDER = 7D
		lm49350_write(codec,0x07,0x1F);
	//	lm49350_write(codec,0x10,0x03);	// DAC -> LS
	//	lm49350_write(codec,0x30,0x30);
		
		lm49350_write(codec,0x44,0x12);	// PORT2 RX -> DAC
		lm49350_write(codec,0x45,0x10);	// ADC input = none, Mix clk = MCLK
		lm49350_write(codec,0x60,0x01);	// stereo mode, RX enable

		lm49350_write(codec,0x64,0x1B);	// PORT2 - 16bit	
		lm49350_write(codec,0x65,0x02);	// Rx :  MSB, IIS mode
		lm49350_write(codec,0x66,0x02);	// Tx : MSB, IIS mode

		lm49350_write(codec,0x01,0x0);
		
		lm49350_write(codec,0x10,0x3);	 //DAC -> LS
		
		lm49350_write(codec,0x30,0x31);	// oversamling = 128, DAC clk = PLL1 clk
		
		reg_val = lm49350_read_reg_cache(codec,0x60);
		reg_val |= 0x2;	
		lm49350_write(codec,0x60,reg_val);	// RX enable
		
		lm49350_write(codec,0xA0,0x05);	// DAC ALC timer 48KHz
		lm49350_write(codec,0xA8,0x33);	// DAC level 0dB
		lm49350_write(codec,0xA9,0x33);
		
	// ADC
	//	lm49350_write(codec,0x80,0x04);	// ADC HPF = 32KHz
	//	lm49350_write(codec,0x81,0x05);	// ADC ALC timer 48KHz
	//	lm49350_write(codec,0x89,0x33);	// ADC level 0dB
	//	lm49350_write(codec,0x8A,0x33);

	// DAC
	//	lm49350_write(codec,0xA0,0x05);	// DAC ALC timer 48KHz
	//	lm49350_write(codec,0xA8,0x33);	// DAC level 0dB
	//	lm49350_write(codec,0xA9,0x33);
		lm49350_write(codec,0x00,0x03);	// chip enable, PLL 1 enable
	}
	
	return 0;
}
static int lm49350_mode1v_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec * codec = codec_dai->codec;
	unsigned char reg_val = 0xFF;
	
	
	s3cdbg("[lm49350] %s ++++,fmt = %d\n",__FUNCTION__, fmt);

	reg_val = lm49350_read_reg_cache(codec, 0x60);
	reg_val |= 0x4;	
	lm49350_write(codec,0x60,reg_val);	// Tx enable
	
	lm49350_write(codec,0x80,0x04);	// ADC HPF = 32KHz
	lm49350_write(codec,0x81,0x05);	// ADC ALC timer 48KHz
	lm49350_write(codec,0x89,0x33);	// ADC level 0dB
	lm49350_write(codec,0x8A,0x33);
	return 0;
}

static int lm49350_mode1h_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
#if 0
	struct snd_soc_codec * codec = codec_dai->codec;
	unsigned char reg_val = 0xFF;

	s3cdbg("\n[lm49350] %s ++++, fmt = %d\n",__FUNCTION__,fmt);
#if 0
	// set path : PORT2 -> SPK
	lm49350_write(codec,0x01,0x0);
	lm49350_write(codec,0x10,0x3);
	lm49350_write(codec,0x44,0x12);
	lm49350_write(codec,0x60,0x1B);
	
	lm49350_write(codec,0x61,0x05);
	lm49350_write(codec,0x62,0x0D);
	lm49350_write(codec,0x63,0x07);
	lm49350_write(codec,0x64,0x1B);
	lm49350_write(codec,0x65,0x02);
	lm49350_write(codec,0x31,0x02);
	
	lm49350_write(codec,0xa8,0x30);
	lm49350_write(codec,0xa9,0x30);
	lm49350_write(codec,0x00,0x1);
#endif
#if !LM_REC_TEST		// play test
// play
	lm49350_write(codec,0x01,0x0);
	lm49350_write(codec,0x10,0x3);	 //DAC -> LS

	lm49350_write(codec,0x30,0x31);	// oversamling = 128, DAC clk = PLL1 clk
	
	reg_val = lm49350_read_reg_cache(codec,0x60);
	reg_val |= 0x2;	
	lm49350_write(codec,0x60,reg_val);	// RX enable
	
	lm49350_write(codec,0xA0,0x05);	// DAC ALC timer 48KHz
	lm49350_write(codec,0xA8,0x33);	// DAC level 0dB
	lm49350_write(codec,0xA9,0x33);

// record	
	lm49350_write(codec,0x15,0x0C);	// MicR -> ADC R, MicL -> ADC L
	lm49350_write(codec,0x20,0x30);	// stereo mode, ADC clk = PLL1
	lm49350_write(codec,0x43,0x24);	// ADC R -> Port 2 Right Tx
	
	reg_val = lm49350_read_reg_cache(codec, 0x60);
	reg_val |= 0x4;	
	lm49350_write(codec,0x60,reg_val);	// Tx enable
	
	lm49350_write(codec,0x80,0x04);	// ADC HPF = 32KHz
	lm49350_write(codec,0x81,0x05);	// ADC ALC timer 48KHz
	lm49350_write(codec,0x89,0x33);	// ADC level 0dB
	lm49350_write(codec,0x8A,0x33);

// PLL enable
	lm49350_write(codec,0x00,0x3);

#else	// rec test
#endif
#endif
	//print_lm49350regs(codec);
	
	s3cdbg("\n[lm49350] %s ++++, fmt = %d\n",__FUNCTION__,fmt);

	return 0;
}

static int lm49350_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int lm49350_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{

	switch (level) {
	case SND_SOC_BIAS_ON:
		/* set vmid to 50k and unmute dac */
		break;
	case SND_SOC_BIAS_PREPARE:
		/* set vmid to 5k for quick power up */
		break;
	case SND_SOC_BIAS_STANDBY:
		/* mute dac and set vmid to 500k, enable VREF */
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->bias_level = level;
	return 0;
}


int lm49350_set_play_path(struct snd_soc_dai *codec_dai, int path,int val)
{
#if EMUL_TEST
	struct snd_soc_codec *codec = codec_dai->codec;
#endif
	
	s3cdbg("%s ++++ (path = %d, val = %d)\n", __FUNCTION__, path,val);

#if EMUL_TEST
		//write register (path : reg addr)
	lm49350_write(codec,path,val);
#endif

	return 0;
}
int lm49350_set_phone_path(struct snd_soc_dai *codec_dai, int path, int val)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	
	s3cdbg("%s ++++ (path = %d,val = %d)\n", __FUNCTION__, path,val);

#if EMUL_TEST
	if(path == 0)
	{
		s3cdbg(" PHONE PATH : GPB 2,3  GPIO TEST \n");
		gpio_test();		
	}
	else if(path ==1)
	{
		s3cdbg(" PHONE PATH : EMULATION TEST \n");
		i2c_emul_test();
	}
	else if(path == 2)
	{
		s3cdbg(" PHONE PATH : READ LM49350 Registers \n");
		//print_lm49350regs(codec);
	}
	else if(path == 3)
	{
		s3cdbg(" PHONE PATH : SET GPB2,3 as I2C PORT \n");
		s3c_gpio_cfgpin(GPIO_I2C1_SCL, S3C_GPIO_SFN(GPIO_I2C1_SCL_AF));	// IIC SCL (GPB5)
		s3c_gpio_cfgpin(GPIO_I2C1_SDA, S3C_GPIO_SFN(GPIO_I2C1_SDA_AF));	// IIC SDA (GPB6)
		s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB5)
		s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_UP); // IIC SCL Pull-Up enable (GPB6)
	}
	else if(path == 4)
	{
		if(val = 1)
		{
			s3cdbg("AUDIO_BT_SEL_SET\n");
			AUDIO_BT_SEL_SET;
		}
		else
		{
			s3cdbg("AUDIO_BT_SEL_CLR\n");
			AUDIO_BT_SEL_CLR;
		}
	}
#if 0
	else if(path ==5)
	{
		if(val = 1)
		{
			s3cdbg("RCV_SEL_SET\n");
			RCV_SEL_SET;
		}
		else
		{
			s3cdbg("RCV_SEL_CLR\n");
			RCV_SEL_CLR;
		}
	}
#endif
#endif

	if(path == 0)
	{
		lm49350_write(codec,0x13,0x20);
		lm49350_write(codec,0x18,0x00);	 //
		lm49350_write(codec,0x19,0x00);

//		RCV_SEL_CLR;		
		AUDIO_BT_SEL_CLR;
	}
	else
	{
		CODEC_CLK_EN_SET;

		// MSM -> AUXL,R is difference signal
		// so use AUX L only
		lm49350_write(codec,0x00,0x01);
		lm49350_write(codec,0x01,0x00);
		lm49350_write(codec,0x02,0x01);
		lm49350_write(codec,0x18,0x27);	 //
		lm49350_write(codec,0x19,0x80);
		lm49350_write(codec,0xfe,0x20);
		
		if(path & OUT_SPK)
		{
			s3cdbg(" PHONE PATH : OUT_SPK\n");	
			lm49350_write(codec,0x10,0x20);
			
		}
		if(path & OUT_EAR)
		{	
			s3cdbg(" PHONE PATH : OUT_EAR\n");	
			lm49350_write(codec,0x11,0x20);
			lm49350_write(codec,0x12,0x20);
		}
		if(path & OUT_RCV)	// OUT_SPK
		{
			s3cdbg(" PHONE PATH : OUT_RCV\n");

			// MSM -> AUXL,R is difference signal
			// so use AUX L only
			//lm49350_write(codec,0x00,0x01);
			//lm49350_write(codec,0x01,0x00);
			//lm49350_write(codec,0x02,0x01);
			lm49350_write(codec,0x13,0x20);
			//lm49350_write(codec,0x18,0x27);	 //
			//lm49350_write(codec,0x19,0x80);
			//lm49350_write(codec,0xfe,0x20);

			// AUDIO_BT_SEL : High
//	       	RCV_SEL_SET;
			AUDIO_BT_SEL_SET;		

			//print_lm49350regs(codec);
		
		}
		else if(path & OUT_BT)
		{
			s3cdbg(" PHONE PATH : OUT_BT\n");	
		}
	}
	return 0;
}

int lm49350_set_rec_path(struct snd_soc_dai *codec_dai, int path)
{
	s3cdbg("%s ++++ (path = %d\n", __FUNCTION__, path);
	return 0;
}

int lm49350_set_mic_path(struct snd_soc_dai *codec_dai, int path)
{
	s3cdbg("%s ++++ (path = %d\n", __FUNCTION__, path);
	if(path == 0)
	{
	}
	else
	{
		if (path & MIC_MAIN)
		{
			s3cdbg(" MIC PATH : MIC_MAIN\n");
			// key mic select
			/*  mic bias enable */
			VBIAS_EN_SET;

			/*select  key_mic */
			MIC_SEL_SET;
		}
		else if (path & MIC_EAR)
		{
			s3cdbg(" MIC PATH : MIC_EAR\n");
			// ear mic select
			/*  mic bias enable */
			VBIAS_EN_SET;

			/*select  key_mic */
			MIC_SEL_CLR;
		}
		else if (path & MIC_CAM)
		{
			s3cdbg(" MIC PATH : MIC_CAM\n");
		}
		else if (path & MIC_BT)
		{
			s3cdbg(" MIC PATH : MIC_BT\n");
		}
	}
	return 0;
}

int lm49350_ioctl(struct snd_soc_dai *codec_dai, int cmd, int arg1, int arg2)
{
	printk("%s ++++ (cmd = %d, arg1 = %d) \n",__FUNCTION__, cmd, arg1);

	/* cmd
		0 - play path
		1 - phone path
		2 - rec path
		3 - mic path
	*/
	switch(cmd)
	{
		case AUDIO_IOCTL_PLAY_PATH:
			lm49350_set_play_path(codec_dai,arg1,arg2);
			break;

		case AUDIO_IOCTL_PHONE_PATH:
			lm49350_set_phone_path(codec_dai,arg1,arg2);
			break;
			
		case AUDIO_IOCTL_REC_PATH:
			lm49350_set_rec_path(codec_dai,arg1);
			break;
			
		case AUDIO_IOCTL_MIC_PATH:
			lm49350_set_mic_path(codec_dai,arg1);
			break;

		default:
			s3cdbg("[lm49350] unused IOCTL command!! \n");
		
	}

	return 0;
}


#define LM49350_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

#define LM49350_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

static const struct snd_soc_dai lm49350_all_dai[] = {
/* DAI HiFi mode 1 */
{	.name = "LM49350 HiFi",
	.id = 1,
	.playback = {
		.stream_name = "HiFi Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = LM49350_RATES,
		.formats = LM49350_FORMATS,},
	.capture = { /* dummy for fast DAI switching */
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = LM49350_RATES,
		.formats = LM49350_FORMATS,},
	.ops = {
		.hw_params = lm49350_i2s_hw_params,
	},
	.dai_ops = {
		.digital_mute = lm49350_mute,
		.set_fmt = lm49350_mode1h_set_dai_fmt,
		.set_clkdiv = lm49350_set_dai_clkdiv,
		.set_pll = lm49350_set_dai_pll,
		.set_sysclk = lm49350_set_dai_sysclk,
		.set_mode = lm49350_set_mode,
		.ioctl = lm49350_ioctl,
	},
},
/* DAI Voice mode 1 */
{	.name = "LM49350 Voice",
	.id = 1,
	.playback = {
		.stream_name = "Voice Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = LM49350_RATES,
		.formats = LM49350_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = LM49350_RATES,
		.formats = LM49350_FORMATS,},
	.ops = {
		.hw_params = lm49350_pcm_hw_params,},
	.dai_ops = {
		.digital_mute = lm49350_mute,
		.set_fmt = lm49350_mode1v_set_dai_fmt,
		.set_clkdiv = lm49350_set_dai_clkdiv,
		.set_pll = lm49350_set_dai_pll,
		.set_sysclk = lm49350_set_dai_sysclk,
		.set_mode = lm49350_set_mode,
	},
}
};

struct snd_soc_dai lm49350_dai[2];
EXPORT_SYMBOL_GPL(lm49350_dai);

static void lm49350_set_dai_mode(struct snd_soc_codec *codec, unsigned int mode)
{
	s3cdbg("\n[lm49350] %s ++++, mode = %d  ##################\n",__FUNCTION__,mode);
	
	if (mode < 4) {
		int playback_active, capture_active, codec_active, pop_wait;
		void *private_data;

		playback_active = lm49350_dai[0].playback.active;
		capture_active = lm49350_dai[0].capture.active;
		codec_active = lm49350_dai[0].active;
		private_data = lm49350_dai[0].private_data;
		pop_wait = lm49350_dai[0].pop_wait;
		lm49350_dai[0] = lm49350_all_dai[mode << 1];
		lm49350_dai[0].playback.active = playback_active;
		lm49350_dai[0].capture.active = capture_active;
		lm49350_dai[0].active = codec_active;
		lm49350_dai[0].private_data = private_data;
		lm49350_dai[0].pop_wait = pop_wait;

		playback_active = lm49350_dai[1].playback.active;
		capture_active = lm49350_dai[1].capture.active;
		codec_active = lm49350_dai[1].active;
		private_data = lm49350_dai[1].private_data;
		pop_wait = lm49350_dai[1].pop_wait;
		lm49350_dai[1] = lm49350_all_dai[(mode << 1) + 1];
		lm49350_dai[1].playback.active = playback_active;
		lm49350_dai[1].capture.active = capture_active;
		lm49350_dai[1].active = codec_active;
		lm49350_dai[1].private_data = private_data;
		lm49350_dai[1].pop_wait = pop_wait;
	}
	lm49350_dai[0].codec = codec;
	lm49350_dai[1].codec = codec;
}

static void lm49350_work(struct work_struct *work)
{
	struct snd_soc_codec *codec =
		container_of(work, struct snd_soc_codec, delayed_work.work);

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
	lm49350_set_bias_level(codec, SND_SOC_BIAS_OFF);
}
static int lm49350_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);

	/* we only need to suspend if we are a valid card */
	if(!codec->card)
		return 0;
		
	lm49350_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int lm49350_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	int i;
	u8 data[2];
	u16 *cache = codec->reg_cache;

    s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
	/* we only need to resume if we are a valid card */
	if(!codec->card)
		return 0;

	/* Sync reg_cache with the hardware */
	for (i = 0; i < ARRAY_SIZE(lm49350_reg); i++) {
		if (i + 1 == WM8753_RESET)
			continue;
		data[0] = ((i + 1) << 1) | ((cache[i] >> 8) & 0x0001);
		data[1] = cache[i] & 0x00ff;
		codec->hw_write(codec->control_data, data, 2);
	}

	lm49350_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* charge lm49350 caps */
	if (codec->suspend_dapm_state == SNDRV_CTL_POWER_D0) {
		lm49350_set_bias_level(codec, SND_SOC_BIAS_PREPARE);
		codec->dapm_state = SNDRV_CTL_POWER_D0;
		schedule_delayed_work(&codec->delayed_work,
			msecs_to_jiffies(caps_charge));
	}

	return 0;
}

/*
 * initialise the WM8753 driver
 * register the mixer and dsp interfaces with the kernel
 */
#if 0


unsigned int i2c_raw_write_snd(
    unsigned int   SlaveAddr,   // slave address
    unsigned char  WordAddr,    // starting slave word address
    unsigned char*  pData,       // pdata
    unsigned int   DataCount        // bytes to write
    )
{
	int i;
	
	__raw_writel(M_ACTIVE, S3C_IICREG(S3C2410_IICSTAT));

	// write slave address
	__raw_writel(SlaveAddr, S3C_IICREG(S3C2410_IICDS));
	__raw_writel(MTX_START, S3C_IICREG(S3C2410_IICSTAT));

	mdelay(1);	// need delay between signals

	// write i2c
	__raw_writel(WordAddr, S3C_IICREG(S3C2410_IICDS));
	__raw_writel(RESUME_ACK, S3C_IICREG(S3C2410_IICCON));

#if 0	

	mdelay(1);	// need delay between signals
	__raw_writel(*pData, S3C_IICREG(S3C2410_IICDS));
	__raw_writel(RESUME_ACK, S3C_IICREG(S3C2410_IICCON));
	
#else
	for (i = 0; i < DataCount; i++) {
		mdelay(1);	// need delay between signals
		__raw_writel(pData[i], S3C_IICREG(S3C2410_IICDS));
		__raw_writel(RESUME_ACK, S3C_IICREG(S3C2410_IICCON));
		//printk(KERN_INFO "pData[%d]= 0x%x\n", i, pData[i]);
	}
#endif

	mdelay(1);	// need delay between signals

	__raw_writel(MTX_STOP, S3C_IICREG(S3C2410_IICSTAT));
	__raw_writel(RESUME_ACK, S3C_IICREG(S3C2410_IICCON));
	__raw_writel(M_IDLE, S3C_IICREG(S3C2410_IICSTAT));

	mdelay(1);
	
	return 0;
}

unsigned int i2c_raw_read_snd(
    unsigned int  SlaveAddr,   // slave address
    unsigned char  WordAddr,    // starting word address
    unsigned char*  pData,       // pdata
    unsigned int  DataCount        // bytes to read
    )
{
	unsigned char ret;
	//unsigned long tmp;
	
	//tmp = __raw_readl(S3C2410_IICCON);
	//writel(tmp | S3C2410_IICCON_ACKEN, i2c->regs + S3C2410_IICCON);
	//printk(KERN_INFO "+I2C_Read: 0x%X, 0x%X, 0x%X, %u\n",
	//	SlaveAddr, WordAddr, *pData, DataCount);
	//__raw_writel(SlaveAddr, S3C_IICREG(S3C2410_IICADD));


	// write slave address
	__raw_writel(MTX_START, S3C_IICREG(S3C2410_IICSTAT));
	__raw_writel(SlaveAddr, S3C_IICREG(S3C2410_IICDS));	

	mdelay(1);	// need delay between signals

	// write word address
	// For setup time of SDA before SCL rising edge, rIICDS must be written
	// before clearing the interrupt pending bit.
	__raw_writel(WordAddr, S3C_IICREG(S3C2410_IICDS));
	// clear interrupt pending bit (resume)
	__raw_writel(RESUME_NO_ACK, S3C_IICREG(S3C2410_IICCON));

	//printk(KERN_INFO "SET_READ_ADDR  0x%X\n", WordAddr);
	mdelay(1);	// need delay between signals

	__raw_writel(MTX_STOP, S3C_IICREG(S3C2410_IICSTAT));
	__raw_writel(RESUME_NO_ACK, S3C_IICREG(S3C2410_IICCON));


	// get read data

	mdelay(1);	// need delay between signals

	__raw_writel((SlaveAddr+1), S3C_IICREG(S3C2410_IICDS));
	__raw_writel(MRX_START, S3C_IICREG(S3C2410_IICSTAT));

	mdelay(10);	// this is critical !!

	__raw_writel(RESUME_NO_ACK, S3C_IICREG(S3C2410_IICCON));

	mdelay(1);

	ret = __raw_readl(S3C_IICREG(S3C2410_IICDS));
	//printk(KERN_INFO "Read IIC Data(0x%X)\n", ret);

	//The pending bit will not be set after issuing stop condition.
	__raw_writel(MRX_STOP, S3C_IICREG(S3C2410_IICSTAT));
	__raw_writel(RESUME_NO_ACK, S3C_IICREG(S3C2410_IICCON));

	__raw_writel(M_IDLE, S3C_IICREG(S3C2410_IICSTAT));

	mdelay(1);	// this is critial to consecutive read operation


	return (unsigned int)ret;
}
#endif
#if 0
unsigned char sensor_read(struct i2c_client *client, unsigned char subaddr)
{
	int ret;
	unsigned char buf[1];
	struct i2c_msg msg = { client->addr, 0, 1, buf };
	buf[0] = subaddr;

	//printk("sensor_read \n");

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) {
		printk(" I2C write Error \n");
		return -EIO;
	}

	msg.flags = I2C_M_RD;
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;

	return buf[0];
}
#endif

static void lm49350_reset(struct snd_soc_codec *codec)
{
	//struct snd_soc_codec *codec = socdev->codec;
#if 0
	int ret = -1;
	unsigned char buf[5] ;
	unsigned char regs[] = { 0x00, 0x01, 0x10,0x18,0x19 };
#endif

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);

	CODEC_CLK_EN_SET;

#if 0

	/*  mic bias enable */
	VBIAS_EN_SET;

	// for key_mic test
	MIC_SEL_SET;
	
		

#if LM_MASTER_TEST
	lm49350_write(codec,0x01,0x00);
	lm49350_write(codec,0x02,0x00);
	lm49350_write(codec,0x10,0x03);
	lm49350_write(codec,0x18,0x00);
	lm49350_write(codec,0x19,0x00);
	lm49350_write(codec,0x31,0x01);
	lm49350_write(codec,0x44,0x12);	// path: PORT2 LR -> DAC LR

//	lm49350_write(codec,0x54,0x00);	// default 24bit
//	lm49350_write(codec,0x54,0x1B);	// PORT1 - 16bit
	lm49350_write(codec,0x60,0x1B);	// Rx enable, clock enable
//	lm49350_write(codec,0x60,0x1F);	// Rx,Tx enable, clock disable
	
	
	lm49350_write(codec,0x61,0x07);
	lm49350_write(codec,0x62,0x00);
	lm49350_write(codec,0x63,0x07);
//	lm49350_write(codec,0x64,0x00);	// 24bit	
	lm49350_write(codec,0x64,0x1B);	// PORT2 - 16bit
//	lm49350_write(codec,0x64,0x3F);	// 8bit
	lm49350_write(codec,0x65,0x02);	// Rx -  IIS/PCM short  mode
	lm49350_write(codec,0x66,0x02);	// Tx -  IIS/PCM short  mode

	
	lm49350_write(codec,0xa8,0x33);	// volume
	lm49350_write(codec,0xa9,0x33);
/*	
	lm49350_write(codec,0x30,0x20);	// for slave test
	lm49350_write(codec,0x60,0x1F);	//Rx,Tx enable -> for slave test
	lm49350_write(codec,0x01,0x02);
*/	
	lm49350_write(codec,0x00,0x1);
#else	

#if !LM_REC_TEST		// play test
	printk("----------- PLAY TEST ------------\n");
	lm49350_write(codec,0x01,0x00);	// select  MCLK 
	lm49350_write(codec,0x02,0x00);	// PMC_CLK_DIV = 1
	lm49350_write(codec,0x03,0x02);	// PLL1_CLK = PORT2_CLK
	lm49350_write(codec,0x05,0x7D);	// PLL1 DIVIDER = 7D
	lm49350_write(codec,0x07,0x1F);
//	lm49350_write(codec,0x10,0x03);	// DAC -> LS
//	lm49350_write(codec,0x30,0x30);
	
	lm49350_write(codec,0x44,0x12);	// PORT2 RX -> DAC
	lm49350_write(codec,0x45,0x10);	// ADC input = none, Mix clk = MCLK
	lm49350_write(codec,0x60,0x01);	// stereo mode, RX enable

	lm49350_write(codec,0x64,0x1B);	// PORT2 - 16bit	
	lm49350_write(codec,0x65,0x02);	// Rx :  MSB, IIS mode
	lm49350_write(codec,0x66,0x02);	// Tx : MSB, IIS mode

// ADC
//	lm49350_write(codec,0x80,0x04);	// ADC HPF = 32KHz
//	lm49350_write(codec,0x81,0x05);	// ADC ALC timer 48KHz
//	lm49350_write(codec,0x89,0x33);	// ADC level 0dB
//	lm49350_write(codec,0x8A,0x33);

// DAC
//	lm49350_write(codec,0xA0,0x05);	// DAC ALC timer 48KHz
//	lm49350_write(codec,0xA8,0x33);	// DAC level 0dB
//	lm49350_write(codec,0xA9,0x33);
	lm49350_write(codec,0x00,0x03);	// chip enable, PLL 1 enable

#else	//rec test
	printk("----------- RECORD TEST ------------\n");
	lm49350_write(codec,0x01,0x00);
	lm49350_write(codec,0x02,0x00);
	lm49350_write(codec,0x03,0x00);
	lm49350_write(codec,0x04,0x04);
	lm49350_write(codec,0x05,0x20);
	lm49350_write(codec,0x07,0x00);
	lm49350_write(codec,0x08,0x95);
	lm49350_write(codec,0x15,0x04);
	lm49350_write(codec,0x17,0x04);
	lm49350_write(codec,0x20,0x30);
	lm49350_write(codec,0x43,0x24);
	lm49350_write(codec,0x44,0x00);
	lm49350_write(codec,0x45,0x00);
	lm49350_write(codec,0x60,0x05);
	lm49350_write(codec,0x64,0x00);
	lm49350_write(codec,0x65,0x00);
	lm49350_write(codec,0x66,0x02);
	lm49350_write(codec,0x80,0x04);
	lm49350_write(codec,0x81,0x05);
	lm49350_write(codec,0xa0,0x05);
	lm49350_write(codec,0x00,0x03);	// chip enable, PLL 1 enable
	
#endif

#endif

#endif	//end of #if 0

/*
	s3cdbg("============ read start !! ==============\n");

	if((ret = lm49350_read(codec, &regs[0], &buf[0], 5)) < 0)
	{
		printk("[lm49350] %s ---- with ERROR ######### \n",__FUNCTION__);
	}

	print_lm49350regs(codec);
*/
	//mdelay(10000);

#if 0
/* test code */
	printk("============ raw read/write test !! ==============\n");
	mdelay(2000);
	__raw_writel(M_IDLE, S3C_IICREG(S3C2410_IICSTAT));
	__raw_writel(RESUME_ACK, S3C_IICREG(S3C2410_IICCON));
	__raw_writel(0x10, S3C_IICREG(S3C2410_IICADD));
		
	i2c_raw_write_snd((0x1a<<1),0x10,&buf[0],1);
	printk("[lm49350] %s ---- (ret = %d, rcvbuf[0] = %d) \n",__FUNCTION__,ret,buf[0]);
	
	
	//msg.addr = client->addr;
	
	ret = i2c_raw_read_snd((0x1a<<1),0x10,&buf[0],1);
	//printk("	p = %d\n",p);
	printk("[lm49350] %s ---- (ret = %d, rcvbuf[0] = %d) \n",__FUNCTION__,ret,buf[0]);
#endif
	
}
 static int lm49350_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	//int reg, ret = 0;
	int ret = 0;

	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);

	codec->name = "LM49350";
	codec->owner = THIS_MODULE;
	codec->read = lm49350_read_reg_cache;
	codec->write = lm49350_write;
	codec->set_bias_level = lm49350_set_bias_level;
	codec->dai = lm49350_dai;
	codec->num_dai = 2;
	codec->reg_cache_size = sizeof(lm49350_reg);
	codec->reg_cache = kmemdup(lm49350_reg, sizeof(lm49350_reg), GFP_KERNEL);

	lm49350_set_dai_mode(codec, 0);
	
	lm49350_reset(codec);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "lm49350: failed to create pcms\n");
		goto pcm_err;
	}
	
	/* charge output caps */
	codec->dapm_state = SNDRV_CTL_POWER_D3hot;
	schedule_delayed_work(&codec->delayed_work,
		msecs_to_jiffies(caps_charge));

	if((ret  = lm49350_add_controls(codec)) < 0)
	{
		printk("add_controls returns error \n");
	}

	/* skip widget registration 
	if ((ret = lm49350_add_widgets(codec)) < 0)
	{
		printk("add_widgets returns error \n");
	}
	*/
	
	ret = snd_soc_register_card(socdev);
	if (ret < 0) {
      	printk(KERN_ERR "lm49350: failed to register card\n");
		goto card_err;
    	}

	s3cdbg("[lm49350] %s ---- \n",__FUNCTION__);
	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;

}

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */
static struct snd_soc_device *lm49350_socdev;

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

static unsigned short normal_i2c[] = { 0, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

#define LM49350_ID		0x34
static unsigned short lm49350_normal_i2c[] = { (LM49350_ID >> 1), I2C_CLIENT_END };
static unsigned short lm49350_ignore[] = { 0, LM49350_ID >> 1, I2C_CLIENT_END };
static unsigned short lm49350_i2c_probe[] = { I2C_CLIENT_END };

static struct i2c_client_address_data lm49350_addr_data = {
	.normal_i2c = lm49350_normal_i2c,
	.ignore     = lm49350_ignore,
	.probe      = lm49350_i2c_probe,
};

static struct i2c_client client_template;
static struct i2c_driver lm49350_i2c_driver;

static int lm49350_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = lm49350_socdev;
	struct lm49350_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;
	
	s3cdbg("[lm49350] %s  ++++ \n",__FUNCTION__);

	if (addr != setup->i2c_address)
		return -ENODEV;

	client_template.adapter = adap;
	client_template.addr = addr;

	i2c = kmemdup(&client_template, sizeof(client_template), GFP_KERNEL);
	if (i2c == NULL){
		kfree(codec);
		return -ENOMEM;
	}
	s3cdbg("    start i2c_set_clientdata...\n");
	
	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

    s3cdbg("    start i2c_attach_client...\n");
	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		err("failed to attach codec at addr %x\n", addr);
		goto err;
	}

	ret = lm49350_init(socdev);
	if (ret < 0) {
		err("failed to initialise WM8753\n");
		goto err;
	}
	s3cdbg("[lm49350] lm49350_codec_probe -- \n");

	return ret;

	s3cdbg("[lm49350] %s  ---- \n",__FUNCTION__);
err:
	kfree(codec);
	kfree(i2c);
	s3cdbg("[lm49350] lm49350_codec_probe with error -- \n");
	return ret;
}

static int lm49350_i2c_detach(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	
	s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
	
	i2c_detach_client(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}

static int lm49350_i2c_attach(struct i2c_adapter *adap)
{
    s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);
    
	return i2c_probe(adap, &lm49350_addr_data, lm49350_codec_probe);
}

/* corgi i2c codec control layer */
static struct i2c_driver lm49350_i2c_driver = {
	.driver = {
		.name = "lm49350 I2C Codec",
		.owner = THIS_MODULE,
	},
	.id =             I2C_DRIVERID_WM8753,
	.attach_adapter = lm49350_i2c_attach,
	.detach_client =  lm49350_i2c_detach,
	.command =        NULL,
};

static struct i2c_client client_template = {
	.name =   "lm49350",
	.driver = &lm49350_i2c_driver,
};
#endif


static int lm49350_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct lm49350_setup_data *setup;
	struct snd_soc_codec *codec;
	struct lm49350_priv *lm49350;
	int ret = 0;

    s3cdbg("[lm49350] lm49350_probe ++++ \n");
    
	info("lm49350 Audio Codec");

#if 0
	CODEC_CLK_EN_SET
	
	s3c_gpio_cfgpin(S3C_GPB2, S3C_GPB2_I2C_SCL);	// IIC SCL (GPB5)
	s3c_gpio_cfgpin(S3C_GPB3, S3C_GPB3_I2C_SDA);	// IIC SDA (GPB6)
	s3c_gpio_pullup(S3C_GPB2, 0x2); // IIC SCL Pull-Up enable (GPB5)
	s3c_gpio_pullup(S3C_GPB3, 0x2); // IIC SCL Pull-Up enable (GPB6)
#endif

	//printk("EGPIO_REG0 address : 0x%08x\n", EGPIO_REG0);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	lm49350 = kzalloc(sizeof(struct lm49350_priv), GFP_KERNEL);
	if (lm49350 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = lm49350;
	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	lm49350_socdev = socdev;
	INIT_DELAYED_WORK(&codec->delayed_work, lm49350_work);

    //ret = i2c_emul_init();
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		normal_i2c[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t)i2c_master_send;
		s3cdbg("[lm49350] i2c_add_driver start , i2c_address = (0x%x)\n",setup->i2c_address);
		ret = i2c_add_driver(&lm49350_i2c_driver);
		if (ret != 0)
			//printk(KERN_ERR "can't add i2c driver");
			printk("[lm49350] can't add i2c driver");
	}
#else
		/* Add other interfaces here */
#endif
	s3cdbg("[lm49350] lm49350_probe ---- \n");

	return ret;
}

/*
 * This function forces any delayed work to be queued and run.
 */
static int run_delayed_work(struct delayed_work *dwork)
{
	int ret;

	/* cancel any work waiting to be queued. */
	ret = cancel_delayed_work(dwork);

	/* if there was any work waiting then we run it now and
	 * wait for it's completion */
	if (ret) {
		schedule_delayed_work(dwork, 0);
		flush_scheduled_work();
	}
	return ret;
}

/* power down chip */
static int lm49350_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

    s3cdbg("[lm49350] %s ++++ \n",__FUNCTION__);

	if (codec->control_data)
		lm49350_set_bias_level(codec, SND_SOC_BIAS_OFF);

	run_delayed_work(&codec->delayed_work);
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&lm49350_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_lm49350 = {
	.probe = 	lm49350_probe,
	.remove = 	lm49350_remove,
	.suspend = 	lm49350_suspend,
	.resume =	lm49350_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_lm49350);

MODULE_DESCRIPTION("ASoC LM49350 driver");
MODULE_AUTHOR("Liam Girdwood");
MODULE_LICENSE("GPL");
