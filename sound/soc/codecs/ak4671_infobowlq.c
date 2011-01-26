/*
 * ak4671_instinctq.c  --  AK4671 Instinctq Board specific code
 *
 * Copyright (C) 2008 Samsung Electronics, Seung-Bum Kang
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
#include <linux/i2c.h>
#include <linux/switch.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>

#include <plat/egpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/hardware.h>

#include <sound/soc.h>

#include "ak4671.h"

#define AUDIO_SPECIFIC_DEBUG	0

#if AUDIO_SPECIFIC_DEBUG
#define SUBJECT "ak4671_infobowlq.c"
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

#define AK4671_PLL 			0x08   // 19.2MHz
#define MAX9877_ADDRESS 	0x9A

/* ak4671.c : spk <-> hp path value */

extern int ak4671_amp_path;
extern unsigned int tty_mode;
extern unsigned int loopback_mode;

short int get_headset_status(void);

static unsigned short reg_pll_mode = 0xf0 | AK4671_PLL; // Default setting : 44.1kHz, 19.2MHz

static struct i2c_driver max9877_i2c_driver;
static struct i2c_client* max9877_i2c_client;
static unsigned int sample_rate = 0;

static int max9877_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int max9877_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

static int max9877_i2c_probe(struct i2c_client *client,
			                 const struct i2c_device_id *id)
{
	u8 pData = 0, i;

	FI
	max9877_i2c_client = client;
	i2c_set_clientdata(client, max9877_i2c_client);

	max9877_write(max9877_i2c_client, 0x01, 0x1F); 
	max9877_write(max9877_i2c_client, 0x02, 0x1F);
	max9877_write(max9877_i2c_client, 0x03, 0x1F);
	max9877_write(max9877_i2c_client, 0x04, 
		(1 << 7) |  /* Operational mode  */
		(0 << 6) |  /* Bypass mode disabled */
		(0 << 4) |  /* Class D : 1176, CHARGE-PUMP : 588  */
		(0x9) );  	/* SPK:INA1+INA2+INB1+INB2, LHP:INA1+INB2, RHP:INA2+INB2 */

#if AUDIO_SPECIFIC_DEBUG
	/* real all */
    for(i = 0; i <= 0x4; i++) {
		max9877_read(max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif

	return 0;
}

static int max9877_i2c_remove(struct i2c_client *client)
{
   	FI
	max9877_i2c_client = i2c_get_clientdata(client);
	kfree(max9877_i2c_client);

	return 0;
}

static const struct i2c_device_id max9877_i2c_id[] = {
	{ "MAX9877 I2C (AMP)", 0}, 
	{ }
};

static struct i2c_driver max9877_i2c_driver = {
	.driver = {
		.name = "MAX9877 I2C (AMP)",
		.owner = THIS_MODULE,
	},
	.probe			= max9877_i2c_probe,
	.remove			= max9877_i2c_remove,
	.id_table		= max9877_i2c_id,
};

int audio_init(void)
{
	/* AUDIO_EN */
	if (gpio_is_valid(GPIO_AUDIO_EN)) {
		if (gpio_request(GPIO_AUDIO_EN, S3C_GPIO_LAVEL(GPIO_AUDIO_EN))) 
			printk(KERN_ERR "Failed to request GPIO_AUDIO_EN! \n");
		gpio_direction_output(GPIO_AUDIO_EN, 0);
	}
	s3c_gpio_setpull(GPIO_AUDIO_EN, S3C_GPIO_PULL_NONE);

	/* MICBIAS_EN */
	if (gpio_is_valid(GPIO_MICBIAS_EN)) {
		if (gpio_request(GPIO_MICBIAS_EN, S3C_GPIO_LAVEL(GPIO_MICBIAS_EN))) 
			printk(KERN_ERR "Failed to request GPIO_MICBIAS_EN! \n");
		gpio_direction_output(GPIO_MICBIAS_EN, 0);
	}
	s3c_gpio_setpull(GPIO_MICBIAS_EN, S3C_GPIO_PULL_NONE);

	//HYH_20100526 if (system_rev >= 0x40) 
	{
		if (gpio_is_valid(GPIO_MIC_SEL_EN_REV04)) {
			if (gpio_request(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_LAVEL(GPIO_MIC_SEL_EN_REV04))) 
				printk(KERN_ERR "Failed to request GPIO_MIC_SEL_EN_REV04! \n");
			gpio_direction_output(GPIO_MIC_SEL_EN_REV04, 0);
		}
		s3c_gpio_setpull(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_PULL_NONE);
	} 
	#if 0  //HYH_20100526
	else {
	if (gpio_is_valid(GPIO_MIC_SEL_EN)) {
		if (gpio_request(GPIO_MIC_SEL_EN, S3C_GPIO_LAVEL(GPIO_MIC_SEL_EN))) 
			printk(KERN_ERR "Failed to request GPIO_MIC_SEL_EN! \n");
		gpio_direction_output(GPIO_MIC_SEL_EN, 0);
	}
	s3c_gpio_setpull(GPIO_MIC_SEL_EN, S3C_GPIO_PULL_NONE);
	}
	#endif
	
       //[[HYH_20100512
       if(system_rev>=0x80) {
		// we control ear mic bias in sec_headset.c
       }
       else
       {
       	if (gpio_is_valid(GPIO_EAR_MIC_BIAS)) {
       		if (gpio_request(GPIO_EAR_MIC_BIAS, S3C_GPIO_LAVEL(GPIO_EAR_MIC_BIAS))) 
       			printk(KERN_ERR "Failed to request GPIO_EAR_MIC_BIAS! \n");
       		gpio_direction_output(GPIO_EAR_MIC_BIAS, 0);
       	}
       	s3c_gpio_setpull(GPIO_EAR_MIC_BIAS, S3C_GPIO_PULL_NONE);
	}
       //]]HYH_20100512

	return 0;
}

int audio_power(int en)
{
	P("AUDIO POWER : %d", en);
	if (en)
		gpio_set_value(GPIO_AUDIO_EN, 1);
	else
		gpio_set_value(GPIO_AUDIO_EN, 0);

	mic_enable(1); 

	return 0;
}

int amp_init(void)
{
	int ret;

	FI

	ret = i2c_add_driver(&max9877_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "can't add i2c driver");

	FO
	return ret;
}

int amp_enable(int en)
{
	u8 pData = 0;

	P("AMP EN : %d", en);
	max9877_read(max9877_i2c_client, 0x04, &pData);

	if (en)  {
		max9877_write(max9877_i2c_client, 0x04, 
			pData | 1 << 7 ); /* Low Power shutdown mode : 1 */
		mdelay(10);
	} else {
		max9877_write(max9877_i2c_client, 0x01, 0x00); 
		max9877_write(max9877_i2c_client, 0x02, 0x00); 
		max9877_write(max9877_i2c_client, 0x03, 0x00); 

		mdelay(10);
		max9877_write(max9877_i2c_client, 0x04, 
			pData & ~(1 << 7) ); /* Low Power shutdown mode : 0 */
	}
	
	return 0;
}

int amp_set_path(int path)
{
#if AUDIO_SPECIFIC_DEBUG
	int i; 
	u8 pData;

	/* real all */
    for(i = 0; i <= 0x4; i++) {
		max9877_read(max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif

	if (path == AK4671_AMP_PATH_SPK) {
		P("AMP Path : SPK");
		max9877_write(max9877_i2c_client, 0x04, 
			(1 << 7) |  /* Operational mode  */
			(0 << 6) |  /* Bypass mode disabled */
			(0 << 4) |  /* Class D : 1176, CHARGE-PUMP : 588  */
			(0x1) );  	/* SPK:INA1INB1 */
	} else if (path == AK4671_AMP_PATH_HP) {
		P("AMP Path : HP");
		max9877_write(max9877_i2c_client, 0x04, 
			(1 << 7) |  /* Operational mode  */
			(0 << 6) |  /* Bypass mode disabled */
			(0 << 4) |  /* Class D : 1176, CHARGE-PUMP : 588  */
			(0x5) );  	/* LHP:INB1, RHP:INB2 */
	} else if (path == AK4671_AMP_PATH_SPK_HP) {
		P("AMP Path : SPK & HP");
		max9877_write(max9877_i2c_client, 0x04, 
			(1 << 7) |  /* Operational mode  */
			(0 << 6) |  /* Bypass mode disabled */
			(0 << 4) |  /* Class D : 1176, CHARGE-PUMP : 588  */
			(0x6) );  	/* SPK:INB1+INB2   LHP:INB1, RHP:INB2 */
			// (0x3) );  	/* SPK:INA1+INA2   LHP:INA1, RHP:INA2 */
	}

#if AUDIO_SPECIFIC_DEBUG
	/* real all */
    for(i = 0; i <= 0x4; i++) {
		max9877_read(max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif

	ak4671_amp_path = path;

	return 0;
}

int amp_get_path(int path)
{
	/* TODO */
	P("AMP SET PATH : ?");

	return 0;
}

int amp_register_string(char *buf)
{
	int i; 
	u8 pData = 0;

	sprintf(buf, "%s MAX9877\r\n", buf);
    for(i = 0; i <= 0x4; i++) {
		max9877_read(max9877_i2c_client, i, &pData);
		sprintf(buf, "%s[0x%02x] = 0x%02x\r\n", buf, i + 0xe0, pData);
	}

	return 0;
}

int amp_set_register(unsigned char reg, unsigned char val)
{
	P("AMP Register (Write) reg:0x%02x, val:0x%02x\n", reg, val);
	max9877_write(max9877_i2c_client, reg, val);
	return 0;
}

int mic_enable(int en)
{
	P("MIC EN : %d", en);
	if (en)
		gpio_set_value(GPIO_MICBIAS_EN, 1);
	else
		gpio_set_value(GPIO_MICBIAS_EN, 0);

	return 0;
}

int mic_ear_enable(int en)
{
	P("EAR MIC EN : %d", en);

      //[[HYH_20100512
      #if 0
	if(system_rev>=0x80)
	{
	    //Do Noting...
	}
	else
	{
       	if (en)
       		gpio_set_value(GPIO_EAR_MIC_BIAS, 0);
       	else
       		gpio_set_value(GPIO_EAR_MIC_BIAS, 1);
      }
      #endif
	//]]HYH_20100512

	return 0;
}
//HYH_20100507 EXPORT_SYMBOL(mic_ear_enable);

int mic_set_path(int path)
{
	/* NLAS3158 - H : MAIN, L : SUB */
	//HYH_20100526 if (system_rev >= 0x40) 
	{
		if (path == AK4671_MIC_PATH_MAIN) {
			P("MIC PATH : MAIN(1)");
			gpio_set_value(GPIO_MIC_SEL_EN_REV04, 1);
			s3c_gpio_slp_cfgpin(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_SLP_OUT1);
			s3c_gpio_slp_setpull_updown(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_PULL_NONE);
		} else {
			P("MIC PATH : SUB(0)");
			gpio_set_value(GPIO_MIC_SEL_EN_REV04, 0);
			s3c_gpio_slp_cfgpin(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_MIC_SEL_EN_REV04, S3C_GPIO_PULL_NONE);
		}
	}
	#if 0   //HYH_20100526
	else {
	if (path == AK4671_MIC_PATH_MAIN) {
		P("MIC PATH : MAIN(1)");
		gpio_set_value(GPIO_MIC_SEL_EN, 1);
		s3c_gpio_slp_cfgpin(GPIO_MIC_SEL_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_MIC_SEL_EN, S3C_GPIO_PULL_NONE);
	} else {
		P("MIC PATH : SUB(0)");
		gpio_set_value(GPIO_MIC_SEL_EN, 0);
		s3c_gpio_slp_cfgpin(GPIO_MIC_SEL_EN, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_MIC_SEL_EN, S3C_GPIO_PULL_NONE);
	}
	}
       #endif
	return 0;
}

int set_sample_rate(struct snd_soc_codec *codec, int bitRate)
{
	sample_rate = bitRate;
	switch(bitRate) {
		case 8000: // Sampling 8kHz
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_REC_DOWNSAMPLING // sangsu fix : recording down scaling
			reg_pll_mode = 0xf0 | AK4671_PLL;
			break;
#else // sangsu fix
			reg_pll_mode = 0x00 | AK4671_PLL;
			codec->write(codec, 0x01, 0x00 | AK4671_PLL); 	
			break;
#endif // sangsu fix
		case 16000: // Sampling 16kHz
			reg_pll_mode = 0x20 | AK4671_PLL;
			break;
		case 11025: // Sampling 11.025kHz
			reg_pll_mode = 0x50 | AK4671_PLL;
			break;
		case 22050: // Sampling 22.05kHz
			reg_pll_mode = 0x70 | AK4671_PLL;
			break;
		case 32000: // Sampling 32kHz
			reg_pll_mode = 0xa0 | AK4671_PLL;
			break;
		case 44100: // Sampling 44.1kHz
			reg_pll_mode = 0xf0 | AK4671_PLL;
			break;
		case 48000: // Sampling 48kHz
			reg_pll_mode = 0xb0 | AK4671_PLL;
			break;
		default:
			P("[%s] not support bit rate (%d).\n", __func__, bitRate);
			reg_pll_mode = 0x70 | AK4671_PLL;
			break;
	}
	printk("[AK4671] Set bitRate : 0x%02x (%d) \n", reg_pll_mode, bitRate);
	codec->write(codec, 0x01, reg_pll_mode); 	

	return 0;
}

static void set_bias (struct snd_soc_codec *codec, int mode)
{
	P("BIAS Control (%d)", mode);

	/* Set MIC BIAS */
	/* VOICECALL, VOICEMEMO, PLAYBACK_HP */
	if ((mode & 0xf0) == MM_AUDIO_VOICECALL || 
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO ||
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_HP ||
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_RING_SPK_HP ||
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_SPK_HP) /* for earjack send/end key interrupt */
	{
		if (((mode & 0x0f) == MM_AUDIO_OUT_HP) ||
			((mode & 0x0f) == MM_AUDIO_PLAYBACK_RING_SPK_HP) ||
			((mode & 0x0f) == MM_AUDIO_PLAYBACK_SPK_HP) ) 
		{
			mic_ear_enable(1);
		} 
	}
	else
	{
		if(!get_headset_status())
			mic_ear_enable(0);
	}

	/* Set AMP BIAS */
	/* SPK, EARJACK, VOICEMEMO */
	if ((mode & 0x0f) == MM_AUDIO_OUT_SPK || 
		(mode & 0x0f) == MM_AUDIO_OUT_HP || 
		(mode & 0x0f) == MM_AUDIO_OUT_SPK_HP || 
		(mode & 0x0f) == MM_AUDIO_OUT_RING_SPK_HP || 
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO ) 
	{
		if (mode != MM_AUDIO_VOICECALL_BT)
			amp_enable(1);	
	} else
		amp_enable(0);	
}

static void set_amp_gain(struct snd_soc_codec *codec, int mode)
{
	P("SET AMP gain : %d\n", mode);

	/* VOICEMEMO Path : only SPK */
	if ((mode & 0xf0) == MM_AUDIO_VOICEMEMO)
		mode = MM_AUDIO_PLAYBACK_SPK;

	/* Set AMP tunning value */
	switch (mode) 
	{
		case MM_AUDIO_PLAYBACK_SPK_HP :
			/* Earjack Gain : 0dB */
			P("Set Default SPK+HP AMP gain");
            max9877_write(max9877_i2c_client, 0x01, 0x1F); 
            max9877_write(max9877_i2c_client, 0x02, 0x1F); 
            max9877_write(max9877_i2c_client, 0x03, 0x1F); 
			break;
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			/* Earjack Gain : -23dB */
			P("Set Default SPK+HP AMP gain");
            max9877_write(max9877_i2c_client, 0x01, 0x1F); 
            max9877_write(max9877_i2c_client, 0x02, 0x0E); 
            max9877_write(max9877_i2c_client, 0x03, 0x0E); 
			break;
		default :
			/* Earjack Gain : 0dB */
			P("Set Default AMP gain");
			mdelay(10);
			max9877_write(max9877_i2c_client, 0x01, 0x1F); 
			max9877_write(max9877_i2c_client, 0x02, 0x1F); 
			max9877_write(max9877_i2c_client, 0x03, 0x1F); 
			break;
	}
}

static void set_codec_gain(struct snd_soc_codec *codec, int mode)
{
	P("SET Path gain : %d\n", mode);
	struct ak4671_priv *ak4671 = codec->private_data;

	/* Set output tunning value */
	switch (mode) 
	{
		case MM_AUDIO_PLAYBACK_RCV :
			break;
		case MM_AUDIO_PLAYBACK_SPK :
			codec->write(codec, 0x08, 0xB6); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x15); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x15); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_HP :
			codec->write(codec, 0x08, 0xB5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x20); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x20); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_SPK_HP :
			codec->write(codec, 0x08, 0xB6); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x17); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x17); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			codec->write(codec, 0x08, 0xC6); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x20); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x20); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_BT :
			break;

		case MM_AUDIO_VOICECALL_RCV :
			codec->write(codec, 0x08, 0xB5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x20); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x20); 	// Rch Output Digital Vol
			codec->write(codec, 0x05, 0x55); 	// MIC-AMP Gain=0dB
			codec->write(codec, 0x12, 0x91); 	// Lch Input Volume : 0dB
			codec->write(codec, 0x13, 0x91); 	// Rch Input Volume : 0dB 
			break;
		case MM_AUDIO_VOICECALL_SPK :
			codec->write(codec, 0x08, 0xA5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x17); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x17); 	// Rch Output Digital Vol
			codec->write(codec, 0x05, 0x55); 	// MIC-AMP Gain=0dB
			codec->write(codec, 0x12, 0x91); 	// Lch Input Volume : 0dB
			codec->write(codec, 0x13, 0x91); 	// Rch Input Volume : 0dB 
			break;
		case MM_AUDIO_VOICECALL_HP:
            if(loopback_mode == LOOPBACK_MODE_OFF)
            {
			codec->write(codec, 0x08, 0x65); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x20); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x20); 	// Rch Output Digital Vol
			codec->write(codec, 0x05, 0x55); 	// MIC-AMP Gain=0dB
			codec->write(codec, 0x12, 0x91); 	// Lch Input Volume : 0dB
			codec->write(codec, 0x13, 0x91); 	// Rch Input Volume : 0dB 
            }
            else
            {
                codec->write(codec, 0x08, 0xB5);  // Output Volume Control : OUT2[7:4]/OUT1[2:0]
                codec->write(codec, 0x1A, 0x18);  // Lch Output Digital Vol
                codec->write(codec, 0x1B, 0x18);  // Rch Output Digital Vol
                codec->write(codec, 0x05, 0x11);  // MIC-AMP Gain=0dB
                codec->write(codec, 0x12, 0x91);  // Lch Input Volume : 0dB
                codec->write(codec, 0x13, 0x91);  // Rch Input Volume : 0dB
            }
			break;
		case MM_AUDIO_VOICECALL_BT:
			codec->write(codec, 0x05, 0x55); 	// MIC-AMP Gain=0dB
			codec->write(codec, 0x12, 0x91); 	// Lch Input Volume : 0dB
			codec->write(codec, 0x13, 0x91); 	// Rch Input Volume : 0dB 
			codec->write(codec, 0x56, 0x54); 	// Digital Vol-B Control : -29.5dB
			break;

		case MM_AUDIO_VOICEMEMO_MAIN:
			if(sample_rate == 8000)
			{
				codec->write(codec, 0x05, 0xCC); 		// MIC-AMP Gain=+27dB ->15dB
				codec->write(codec, 0x12, 0xB9); 		// Lch Input Volume : 6dB -> 17.25dB
				codec->write(codec, 0x13, 0xB9); 		// Rch Input Volume : 6dB -> 17.25dB
			}
			else
			{
				codec->write(codec, 0x05, 0xBB); 		// MIC-AMP Gain = +27dB->21dB Request from Nuance CI12
				codec->write(codec, 0x12, 0x91); 		// Lch Input Volume : 0dB
				codec->write(codec, 0x13, 0x91); 		// Rch Input Volume : 0dB 
			}
			break;
		case MM_AUDIO_VOICEMEMO_SUB:
			if(sample_rate == 8000)
			{
				codec->write(codec, 0x05, 0xBB); 		// MIC-AMP Gain=+27dB ->15dB
				codec->write(codec, 0x12, 0xB9); 		// Lch Input Volume : 6dB -> 17.25dB
				codec->write(codec, 0x13, 0xB9); 		// Rch Input Volume : 6dB -> 17.25dB
			}
			else
			{
			      if(ak4671->recognition_active == REC_ON)  //HYH_20100709
				{
					codec->write(codec, 0x05, 0xCC); 		// MIC-AMP Gain = +27dB->21dB Request from Nuance CI12
					codec->write(codec, 0x12, 0x93); 		// Lch Input Volume : 0dB For ASR +5dB 0701    9B->93
					codec->write(codec, 0x13, 0x93); 		// Rch Input Volume : 0dB					   9B->93
				}
				else
				{
					codec->write(codec, 0x05, 0xCC); 		// MIC-AMP Gain = +27dB->21dB Request from Nuance CI12
       				codec->write(codec, 0x12, 0x91); 		// Lch Input Volume : 0dB
       				codec->write(codec, 0x13, 0x91); 		// Rch Input Volume : 0dB 
				}
			}
			break;
		case MM_AUDIO_VOICEMEMO_EAR:
			if(sample_rate == 8000)
			{
				codec->write(codec, 0x05, 0xEE); 		// => MIC-AMP Gain = +27dB 
				codec->write(codec, 0x12, 0xD1); 		// Lch Input Volume : +18dB
				codec->write(codec, 0x13, 0xD1); 		// Rch Input Volume : +18dB 
			}
			else
			{
				codec->write(codec, 0x05, 0xEE); 		// => MIC-AMP Gain = +27dB 
				codec->write(codec, 0x12, 0x91); 		// Lch Input Volume : 0dB
				codec->write(codec, 0x13, 0x91); 		// Rch Input Volume : 0dB 		
			}
			break;
		case MM_AUDIO_VOICEMEMO_BT :
			break;

		default :
			printk("[%s] Invalid gain path\n", __func__);
	}
}

int rec_enable(struct snd_soc_codec *codec, int mode)
{
	P("Rec Enable (mode : %d)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("enable REC (MM_AUDIO_VOICECALL_RCV)");
			/* MIC-AMP Rch mixing A/P Rch -> RCP/RCN OFF */
			codec->write(codec, 0x19, 0x05); 	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0A, 0x20); 	// only MIC-AMP-Rch to RCP/RCN
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up

			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x00, 0x3D); 	// ADC-Lch power-up
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("enable REC (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Rch mixing A/P Rch -> Lout2/Rout OFF */
			codec->write(codec, 0x19, 0x04); 	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0B, 0x00); 	// Swith-off DAC-Lch
			codec->write(codec, 0x0C, 0x20); 	// Swith-off DAC-Rch
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up

			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x00, 0x3D); 	// ADC-Lch power-up
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("enable REC (MM_AUDIO_VOICECALL_BT)");
			/* mixing ADC Rch and A/P Rch -> SRC-A -> PCM-A */
			codec->write(codec, 0x15, 0x14); 	// 5-band-EQ-Lch: from SRC-B, Rch: from SVOLA Rch

			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			codec->write(codec, 0x59, 0x10); 	// SDTO-Lch: from SRC-B
			break;


		default :
				printk("[%s] Invalid mode\n", __func__);
	}

	codec->write(codec, 0x18, 0x06); 	// IVOLC, ADM Mono

	return 0;
}

int rec_disable(struct snd_soc_codec *codec, int mode) 
{
	P("Rec Disable (mode : %d)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("disable REC (MM_AUDIO_VOICECALL_RCV)");
			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("disable REC (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("disable REC (MM_AUDIO_VOICECALL_BT)");
			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			codec->write(codec, 0x59, 0x00); 	// default
			break;
		
		default :
			printk("[%s] Invalid mode\n", __func__);
	}

	codec->write(codec, 0x18, 0x02); 	// IVOLC

	return 0;
}

int path_change(struct snd_soc_codec *codec, int to_mode, int from_mode)
{
	int ret = 1;

	switch (to_mode) {
		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
			switch (from_mode) {
				case MM_AUDIO_VOICEMEMO_MAIN :
				case MM_AUDIO_VOICEMEMO_SUB :
				case MM_AUDIO_VOICEMEMO_EAR :
					P("VOICEMEMO->PLAYBACK");
					set_codec_gain(codec, to_mode);
					if (to_mode == MM_AUDIO_PLAYBACK_SPK) 
						amp_set_path(AK4671_AMP_PATH_SPK);
					else
						amp_set_path(AK4671_AMP_PATH_HP);
				
					codec->write(codec, 0x00, 0xC1);        // DAC Enable, ADC Disable, MicAMP Disable
					if (to_mode != MM_AUDIO_PLAYBACK_HP)
						mic_ear_enable(0);
					break;
				default :
					ret = -1;
					break;
			}
			break;

		case MM_AUDIO_PLAYBACK_RCV :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
		case MM_AUDIO_PLAYBACK_BT :
		case MM_AUDIO_VOICECALL_RCV :
		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
		case MM_AUDIO_VOICECALL_BT :
			ret = -1;
			break;

		case MM_AUDIO_VOICEMEMO_MAIN :
			switch (from_mode) {
				case MM_AUDIO_PLAYBACK_SPK :
					P("PLAYBACK_SPK->VOICEMEMO_MAIN");
					mic_set_path(AK4671_MIC_PATH_MAIN);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
					codec->write(codec, 0x00, 0xD5); 		// D/A power-up
					break;
				case MM_AUDIO_VOICEMEMO_EAR :
					P("VOICEMEMO_EAR->VOICEMEMO_MAIN");
					mic_set_path(AK4671_MIC_PATH_MAIN);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
					mic_ear_enable(0);
					amp_set_path(AK4671_AMP_PATH_SPK);
					break;

				default :
					ret = -1;
					break;
			}
			break;

		case MM_AUDIO_VOICEMEMO_SUB :
			switch (from_mode) {
				case MM_AUDIO_PLAYBACK_SPK :
					P("PLAYBACK_SPK->VOICEMEMO_SUB");
					mic_set_path(AK4671_MIC_PATH_MAIN);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
					codec->write(codec, 0x00, 0xD5); 		// D/A power-up
					break;
				case MM_AUDIO_VOICEMEMO_EAR :
					P("VOICEMEMO_EAR->VOICEMEMO_SUB");
					mic_set_path(AK4671_MIC_PATH_MAIN);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
					mic_ear_enable(0);
					amp_set_path(AK4671_AMP_PATH_SPK);
					break;

				default :
					ret = -1;
					break;
			}
			break;

		case MM_AUDIO_VOICEMEMO_EAR :
			switch (from_mode) {
				case MM_AUDIO_PLAYBACK_HP :
					P("PLAYBACK_HP->VOICEMEMO_EAR");
					mic_ear_enable(1);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x42); 		// => MIC-AMP Lch=IN3+/-
					codec->write(codec, 0x00, 0xD5); 		// D/A power-up
					break;
				case MM_AUDIO_VOICEMEMO_MAIN :
				case MM_AUDIO_VOICEMEMO_SUB :
					P("VOICEMEMO_MAIN&SUB->VOICEMEMO_EAR");
					mic_ear_enable(1);
					set_codec_gain(codec, to_mode);
					codec->write(codec, 0x04, 0x42); 		// => MIC-AMP Lch=IN3+/-
					amp_set_path(AK4671_AMP_PATH_HP);
					break;

				default :
					ret = -1;
					break;
			}
			break;

		default :
			ret = -1;
			break;
	}

	return ret;
}

int path_enable(struct snd_soc_codec *codec, int mode)
{
	P("Enable PATH : 0x%02x\n", mode);

	/* general init register */
	if(mode) {
		//codec->write(codec, 0x02, 0x01); 	// PLL Mode and Power up
		codec->write(codec, 0x01, reg_pll_mode); 	
		codec->write(codec, 0x02, 0x03); 	// PLL Mode and Power up, Master Mode
		codec->write(codec, 0x03, 0x03); 	// I2S mode
	}

	/* Set gain value */
	set_codec_gain(codec, mode);	

	/* Set Bias */
	set_bias(codec, mode);

	/* Set path register sequence */
	switch(mode) {
		case 0:
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			P("set MM_AUDIO_PLAYBACK_RCV");
			codec->write(codec, 0x09, 0x01); 	// D/A Lch -> Lout1
			codec->write(codec, 0x0A, 0x01); 	// D/A Rch -> Rout1
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x0F, 0x04); 	// LOPS1="1",use for pop noise cancel 
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x0F, 0x07); 	// PML01, PMRO1 power-up
			mdelay(30); 	// wait more than 300ms
			codec->write(codec, 0x0F, 0x23);	// LOPS1='0', RCV mono
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			P("set MM_AUDIO_PLAYBACK_SPK");
			codec->write(codec, 0x0B, 0x01); 	// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 	// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			if (mode != MM_AUDIO_PLAYBACK_SPK)
				mdelay(130);
			break;

		case MM_AUDIO_PLAYBACK_BT :
			P("set MM_AUDIO_PLAYBACK_BT");
			codec->write(codec, 0x15, 0x00); 	// 5-band EQ Rch=STDI Rch
			codec->write(codec, 0x15, 0x41); 	// SRC-A = MIX Rch
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x00, 0x01); 	// PMPCM, PMSRA='1', PCM ref = BICKA
			mdelay(40);
			
			break;

		case MM_AUDIO_VOICECALL_RCV :
			P("set MM_AUDIO_VOICECALL_RCV");
			codec->write(codec, 0x0F, 0x20);	// RCP/RCN mode
			codec->write(codec, 0x11, 0xA0);	// LOP/LON, gain=0dB
			codec->write(codec, 0x0A, 0x20);	// MIC-AMP Rch -> RCP/RCN
			codec->write(codec, 0x0D, 0x20);	// MIC-AMP Lch -> LOP/LON
			codec->write(codec, 0x04, 0x9C);	// MIC-AMP Lch=IN1+/-, IN4+/- Differential Input
			codec->write(codec, 0x00, 0x01);	// VCOM power-up
			codec->write(codec, 0x00, 0x0D);	// MIC-AMP, A/D power-up
			codec->write(codec, 0x06, 0x03);	// PMLOOPL, PMLOOPR power-up
			codec->write(codec, 0x0F, 0x27);	// PMLO1, PMRO1 power-up
			mdelay(1);
			codec->write(codec, 0x0F, 0x23);	// LOPS1=0
			codec->write(codec, 0x11, 0x24);	// LOPS3=1, use for pop noise cancel
			codec->write(codec, 0x11, 0x27);	// PMLO3, PMRO3 power-up
			mdelay(30); // Wait more than 300ms(Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0x23);	// LOPS3=0

			/* MIC-AMP Rch + A/P Rch => RCP/RCN */
			codec->write(codec, 0x00, 0x8D);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0A, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN

			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("set MM_AUDIO_VOICECALL_SPK,HP");
			if(tty_mode == 2)
			{
				codec->write(codec, 0x0F, 0x20);	// RCP/RCN mode
				codec->write(codec, 0x0A, 0x20);	// MIC-AMP Rch -> RCP/RCN
			}
			else
			{
				codec->write(codec, 0x0C, 0x20); 	// MIC-AMP-Lch to Lout2
				codec->write(codec, 0x10, 0x08); 	// MIC-AMP-Lch to Rout2
			}			
			codec->write(codec, 0x0D, 0x20); 	// MIC-AMP Lch -> LOP/LON
			if (mode == MM_AUDIO_VOICECALL_SPK)
				codec->write(codec, 0x11, 0x20); 	// LOP/LON gain=-6dB
			else // HP
				codec->write(codec, 0x11, 0xA0); 	// LOP/LON gain=0dB
			if ((mode == MM_AUDIO_VOICECALL_SPK)|(tty_mode == 3))
				codec->write(codec, 0x04, 0x9C); 	// MIC-AMP Lch= IN3+/-, Rch= IN4+/-
			else
				codec->write(codec, 0x04, 0xCE); 	// MIC-AMP Lch=IN3+/-, Rch=IN4+/-

			/* Other setting if needed except for power setting */
			codec->write(codec, 0x00, 0x01); 	// VCOM power-up
			codec->write(codec, 0x06, 0x03); 	// PMLOOPL, PMLOOPR power-up
			codec->write(codec, 0x00, 0x0D); 	// MIC-AMP power-up
			if(tty_mode == 2)
			{
				codec->write(codec, 0x0F, 0x27);	// PMLO1, PMRO1 power-up
				mdelay(1);
				codec->write(codec, 0x0F, 0x23);	// LOPS1=0
			}
			else
			{
				codec->write(codec, 0x10, 0x73); 	// PMLO2, PMRO2, PMLO2S,PMRO2s=1
				codec->write(codec, 0x10, 0x77); 	// MUTEN=1
			}
			if ((mode == MM_AUDIO_VOICECALL_SPK)|(tty_mode == 2))
				codec->write(codec, 0x11, 0x24); 	// LOPS3=1, use for pop noise cancel
			else // HP
				codec->write(codec, 0x11, 0xA4); 	// LOPS3=1, use for pop noise cancel
			if ((mode == MM_AUDIO_VOICECALL_SPK)|(tty_mode == 2))
				codec->write(codec, 0x11, 0x27); 	// PMLO3, PMRO3 power-up
			else // HP
				codec->write(codec, 0x11, 0xA7); 	// PMLO3, PMRO3 power-up

			mdelay(30); 		// Wait more than 300ms(Output capacitor=1uF, AVDD=3.3V)

			if ((mode == MM_AUDIO_VOICECALL_SPK)|(tty_mode == 2))
				codec->write(codec, 0x11, 0x23); 	// LOPS3=0 , gain = -6dB
			else // HP
				codec->write(codec, 0x11, 0xA3); 	// LOPS3=0 , gain = 0dB

			/* MIC-AMP Rch + A/P Rch => Lout2/Rout2 */
			if(tty_mode == 2)
			{
				codec->write(codec, 0x00, 0x8D);	// DAC-Rch power-up
				mdelay(2);
				codec->write(codec, 0x0A, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN
			}
			else
			{
				codec->write(codec, 0x00, 0xCD);	// DAC-Rch power-up
				mdelay(2);
				codec->write(codec, 0x0B, 0x01);	// (MIC-AMP-Rch mixing DAC-Rch) to Lout2
				codec->write(codec, 0x0C, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to Rout2
			}
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("set MM_AUDIO_VOICECALL_BT");
			codec->write(codec, 0x59, 0x10); 		// => SDTO Lch=SRC-B
			codec->write(codec, 0x01, 0xF8); 		// fs=44.1kHz, MCKI=19.2MHz input

			codec->write(codec, 0x11, 0xA0); 		// LOP/LON, gain=0dB
			codec->write(codec, 0x0D, 0x01); 		// D/A Lch -> LOP/LON
			codec->write(codec, 0x04, 0xCE); 		// MIC-AMP Rch= IN4+/- be selected
			codec->write(codec, 0x15, 0x14); 		// 5-band EQ Lch=SRC-B, Rch=SVOLA Rch
			codec->write(codec, 0x19, 0x41); 		// OVOLC, SRC-A : Rch
			codec->write(codec, 0x00, 0x01); 		// VCOM power-up
			mdelay(40);
			codec->write(codec, 0x00, 0x69); 		// PMVCM, PMMICR, PMADR, PMDAL, PMDAR power up
			codec->write(codec, 0x53, 0x17); 		// PLLBT1,PMSRA/B, PMPCM power up
													//PLLBT lock time: max 40ms (base on BICKA)
			mdelay(40);
			codec->write(codec, 0x11, 0x24); 		// LOPS3=1, use for pop noise cancel
			mdelay(1); 		// Wait more than 100ns
			codec->write(codec, 0x11, 0x27); 		// PMLO3, PMRO3 power-up
			mdelay(100);							// Wait more than 300ms
													// (Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0x23); 		// LOPS3=0
			
			/* Mixing ADC Rch and A/P Rch */
			codec->write(codec, 0x15, 0x18); 		// Lch: from SRC-B;
													//Rch: from (SVOLA Rch + SDTI Rch)
			break;
		
		case MM_AUDIO_VOICEMEMO_MAIN :
			P("set MM_AUDIO_VOICEMEMO_MAIN");
			mic_set_path(AK4671_MIC_PATH_MAIN);
			codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
			codec->write(codec, 0x0B, 0x01); 		// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 		// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 		// => VCOM power-up
			mdelay(2); 		// Wait more than 100ns
			codec->write(codec, 0x00, 0xD5); 		// D/A power-up
			codec->write(codec, 0x10, 0x63); 		// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 		// MUTEN='1'

			break;

		case MM_AUDIO_VOICEMEMO_SUB :
			P("set MM_AUDIO_VOICEMEMO_SUB");
			mic_set_path(AK4671_MIC_PATH_MAIN);
			codec->write(codec, 0x04, 0x14); 		// => MIC-AMP Lch=IN1+/-
			codec->write(codec, 0x0B, 0x01); 		// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 		// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 		// => VCOM power-up
			mdelay(2); 		// Wait more than 100ns
			codec->write(codec, 0x00, 0xD5); 		// D/A power-up
			codec->write(codec, 0x10, 0x63); 		// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 		// MUTEN='1'
			
			break;

		case MM_AUDIO_VOICEMEMO_EAR :
			P("set MM_AUDIO_VOICEMEMO_EAR");
			codec->write(codec, 0x04, 0x42); 		// => MIC-AMP Lch=IN3+/-
			codec->write(codec, 0x0B, 0x01); 		// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 		// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 		// => VCOM power-up
			mdelay(2); 		// Wait more than 100ns
			codec->write(codec, 0x00, 0xD5); 		// D/A power-up
			codec->write(codec, 0x10, 0x63); 		// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 		// MUTEN='1'
			mdelay(130);
			break;

		case MM_AUDIO_VOICEMEMO_BT :
			P("set MM_AUDIO_VOICEMEMO_BT");
			codec->write(codec, 0x59, 0x10); 		// => SDTO Lch=SRC-B
			codec->write(codec, 0x00, 0x01); 		// => VCOM power-up
			mdelay(2); 		//Wait more than 100ns
			codec->write(codec, 0x53, 0x0F); 		// => PMPCM,PMSRA, PMSRB=1
													// PCM reference= BICKA(VCOCBT=10kohm&4.7nF)
			mdelay(40); 		// Lock time= 40ms
			break;

		default :
			printk("[SOUND MODE] invalid mode!!! \n");
	}

	/* disable soft mute */
	if (mode ==  MM_AUDIO_VOICECALL_BT)
		codec->write(codec, 0x19, 0x41); 		// OVOLC, SRC-A : Rch
	else
	codec->write(codec, 0x19, 0x01); 	// disable soft mute 

	set_amp_gain(codec, mode);

	return 0;
}

int path_disable(struct snd_soc_codec *codec, int mode)
{
	P("Diasble PATH : %d\n", mode);

	/* for Noise */
	amp_enable(0);

	/* use soft mute to shut-down signal */
	codec->write(codec, 0x19, 0x05); 	

	/* Set path disable sequence */
	switch(mode) {
		case 0:
			P("Path : Off");
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			P("MM_AUDIO_PLAYBACK_RCV Off");
			codec->write(codec, 0x0F, 0x27); 	// LOPS='1'
			mdelay(2); 	// wait more than 1ms
			codec->write(codec, 0x0F, 0x04); 	// RCP/RCN power-donw
			mdelay(2); 	// wait more than 1ms
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			P("MM_AUDIO_PLAYBACK_SPK Off");
			codec->write(codec, 0x10, 0x73); 	// MUTEN='0'
			mdelay(30); 	// wait more than 500ms
			codec->write(codec, 0x10, 0x00); 	// LOUT2/ROUT2 power-down
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_PLAYBACK_BT :
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;
			
		case MM_AUDIO_VOICECALL_RCV :
			P("set MM_AUDIO_VOICECALL_RCV Off");
			/* MIC-AMP Rch + A/P Rch => RCP/RCN Off */
			codec->write(codec, 0x19, 0x05);	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0A, 0x20);	// only MIC-AMP-Rch to RCP/RCN
			codec->write(codec, 0x00, 0x0D);	// DAC-Rch power-up
			codec->write(codec, 0x02, 0x00);	// PLL power-down

			/* Normal Off */
			codec->write(codec, 0x11, 0xA7);	// LOPS3=1
			codec->write(codec, 0x0F, 0x27);	// LOPS1=1
			codec->write(codec, 0x0F, 0x24);	// RCP/RCN power-down
			codec->write(codec, 0x11, 0xA4);	// LOP/LON power-down
			mdelay(30);
			codec->write(codec, 0x11, 0x10);	// default
			codec->write(codec, 0x0F, 0x00);	// default
			codec->write(codec, 0x01, 0x00); 	// 
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("set MM_AUDIO_VOICECALL_SPK,HP Off");

			/* MIC-AMP Rch + A/P Rch => Lout2/Rout2 Off */
			codec->write(codec, 0x19, 0x05); 	// use soft mute to shut-down signal
			mdelay(24);
			if(tty_mode == 2)
			{
				codec->write(codec, 0x0A, 0x20);	// only MIC-AMP-Rch to RCP/RCN
			}
			else
			{
				codec->write(codec, 0x0B, 0x00); 	// Swith-off DAC-Lch
				codec->write(codec, 0x0C, 0x20); 	// Swith-off DAC-Rch
			}		
			codec->write(codec, 0x00, 0x0D); 	// DAC power-up
			codec->write(codec, 0x02, 0x00); 	// PLL power-down
			
			/* Normal Off */
			codec->write(codec, 0x11, 0xA7); 	// LOPS3=1
			if(tty_mode == 2)
			{
				codec->write(codec, 0x0F, 0x27);	// LOPS1=1
				codec->write(codec, 0x0F, 0x24);	// RCP/RCN power-down
			}
			else
			{
				codec->write(codec, 0x10, 0x6B); 	// MUTEN=0
			}
			codec->write(codec, 0x11, 0xA4); 	// LOP/LON power-down
			mdelay(30);
			codec->write(codec, 0x11, 0x10); 	// default
			codec->write(codec, 0x10, 0x00); 	// default
			codec->write(codec, 0x01, 0x00); 	//
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("set MM_AUDIO_VOICECALL_BT Off");
			codec->write(codec, 0x59, 0x00); 
			/* Mixing ADC Rch and A/P Rch Off */
			codec->write(codec, 0x15, 0x14); 	// 5-band-EQ-Lch: from SRC-B;
												// Rch: from SVOLA Rch
			codec->write(codec, 0x15, 0x00); 	// Default value
													
			/* Normal Off */
			codec->write(codec, 0x11, 0xA7); 		// LOPS3=1
			codec->write(codec, 0x11, 0xA4); 	// LOP/LON power-down
			mdelay(30); 		// Wait more than 300ms
			codec->write(codec, 0x11, 0x10);	// default
			codec->write(codec, 0x53, 0x00);	// PMSRA/B, PMPCM power-down
			codec->write(codec, 0x02, 0x00); 	// PLL power-down
			codec->write(codec, 0x01, 0x00);
			codec->write(codec, 0x00, 0x00);	// VCOM and others power-down

			break;

		case MM_AUDIO_VOICEMEMO_MAIN :
			P("set MM_AUDIO_VOICEMEMO_MAIN Off");
			codec->write(codec, 0x05, 0x55); 		// => MIC-AMP Gain=0dB (default)
			codec->write(codec, 0x12, 0x91); 		// Lch Input Volume : +0dB
			codec->write(codec, 0x13, 0x91); 		// Rch Input Volume : +0dB
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_SUB :
			P("set MM_AUDIO_VOICEMEMO_SUB Off");
			codec->write(codec, 0x05, 0x55); 		// => MIC-AMP Gain=0dB (default)
			codec->write(codec, 0x12, 0x91); 		// Lch Input Volume : +0dB (default)
			codec->write(codec, 0x13, 0x91); 		// Rch Input Volume : +0dB (default)
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_EAR :
			P("set MM_AUDIO_VOICEMEMO_EAR Off");
			codec->write(codec, 0x05, 0x55); 		// => MIC-AMP Gain=0dB (default)
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_BT :
			P("set MM_AUDIO_VOICEMEMO_BT Off");
			codec->write(codec, 0x59, 0x00); 	// default
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		default:
			printk("[SOUND MODE] invalid mode!!! \n");
	}

	return 0;
}

int idle_mode_enable(struct snd_soc_codec *codec, int mode)
{
	P("Enable Idle Mode : %d\n", mode);

	set_bias(codec, mode);

	switch(mode) {
		case 0:
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			P("set MM_AUDIO_PLAYBACK_RCV");
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x0F, 0x04); 	// LOPS1="1",use for pop noise cancel 
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x0F, 0x07); 	// PML01, PMRO1 power-up
			mdelay(30); 	// wait more than 300ms
			codec->write(codec, 0x0F, 0x23);	// LOPS1='0', RCV mono
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			P("set MM_AUDIO_PLAYBACK_SPK");
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			if (mode != MM_AUDIO_PLAYBACK_SPK)
				mdelay(130);
			break;

		case MM_AUDIO_PLAYBACK_BT :
			P("set MM_AUDIO_PLAYBACK_BT");
			codec->write(codec, 0x15, 0x00); 	// 5-band EQ Rch=STDI Rch
			codec->write(codec, 0x15, 0x41); 	// SRC-A = MIX Rch
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x00, 0x01); 	// PMPCM, PMSRA='1', PCM ref = BICKA
			mdelay(40);
			
			break;

		default :
			printk("[SOUND MODE] invalid IDLE mode!!! \n");
	}

	set_amp_gain(codec, mode);

	return 0;
}

int idle_mode_disable(struct snd_soc_codec *codec, int mode)
{
	P("Diasble PATH : %d\n", mode);

	/* PLAYBACK_HP */
	if ( (mode & 0x0f) != MM_AUDIO_PLAYBACK_HP &&  /* for earjack send/end key interrupt */
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_SPK_HP)
	{
		if(!get_headset_status())
			mic_ear_enable(0);
	}

	amp_enable(0);

	switch(mode) {
		case 0:
			P("Path : Off");
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			P("MM_AUDIO_PLAYBACK_RCV Off");
			codec->write(codec, 0x0F, 0x04); 	// RCP/RCN power-down
			mdelay(2); 	// wait more than 1ms
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			P("MM_AUDIO_PLAYBACK_SPK Off");
			codec->write(codec, 0x10, 0x73); 	// MUTEN='0'
			mdelay(30); 	// wait more than 500ms
			codec->write(codec, 0x10, 0x00); 	// LOUT2/ROUT2 power-down
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_PLAYBACK_BT :
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;
			
		default:
			printk("[SOUND MODE] invalid IDLE mode!!! \n");
	}

	return 0;
}
