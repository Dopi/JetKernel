/*
 * ak4671_saturn.c  --  AK4671 Saturn Board specific code
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
#include <linux/gpio.h>

#include <plat/egpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/hardware.h>

#include <sound/soc.h>

#include "ak4671.h"
#include "max9877_def.h"

#define AUDIO_SPECIFIC_DEBUG	0
#define MAX9877_SPECIFIC_DEBUG 0

#if AUDIO_SPECIFIC_DEBUG
#define SUBJECT "ak4671_saturn.c"
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

static unsigned short reg_pll_mode = 0xf0 | AK4671_PLL; // Default setting : 44.1kHz, 19.2MHz
static unsigned int sub_mic_path;
static unsigned int cur_amp_path;

static unsigned short max9877_normal_i2c[] = {I2C_CLIENT_END };
static unsigned short max9877_ignore[] = { I2C_CLIENT_END };
static unsigned short max9877_i2c_probe[] = { 5, MAX9877_ADDRESS >> 1, I2C_CLIENT_END };

static struct i2c_driver max9877_i2c_driver;
static struct i2c_client max9877_i2c_client;

static struct i2c_client_address_data max9877_addr_data = {
	.normal_i2c = max9877_normal_i2c,
	.ignore     = max9877_ignore,
	.probe      = max9877_i2c_probe,
};

extern int shutdown_flag;

short int get_headset_status();//import from arc/arm/s3c6410/sec_headset.c

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

static int max9877_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	int ret;
#if MAX9877_SPECIFIC_DEBUG
	u8 pData = 0, i;
#endif

//	FI
	max9877_i2c_client.adapter = adap;
	max9877_i2c_client.addr = addr;

	ret = i2c_attach_client(&max9877_i2c_client);
	if (ret < 0) {
		printk("failed to attach max9877 at addr %x\n", addr);
		return -1;
	}

	max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, KMAX9877_Output_Gain_0ToMinus7(0)); 
	max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME, KMAX9877_Output_Gain_0ToMinus7(0));
	max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME, KMAX9877_Output_Gain_0ToMinus7(0));
	max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, 
		SHDN |  /* Operational mode  */
		(BYPASS & (~BYPASS)) | /* Bypass mode disabled */
		(OSC0 & (~OSC0))	 | /* Class D : 1176, CHARGE-PUMP : 588  */
		(OUTMODE0|OUTMODE3)); 	/* SPK:INA1+INA2+INB1+INB2, LHP:INA1+INB2, RHP:INA2+INB2 */


#if MAX9877_SPECIFIC_DEBUG
	/* real all */
    for(i = 0; i <= MAX9877_OUTPUT_MODE_CONTROL; i++) {
		max9877_read(&max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif

	return 0;
}

static int max9877_i2c_attach(struct i2c_adapter *adap)
{
//   	FI
	return i2c_probe(adap, &max9877_addr_data, max9877_codec_probe);
}

static int max9877_i2c_detach(struct i2c_client *client)
{
 //	FI
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver max9877_i2c_driver = {
	.driver = {
		.name = "MAX9877 I2C (AMP)",
		.owner = THIS_MODULE,
	},
	.id =             1,
	.attach_adapter = max9877_i2c_attach,
	.detach_client =  max9877_i2c_detach,
	.command =        NULL,
};

static struct i2c_client max9877_i2c_client = {
	.name =   "max9877",
	.driver = &max9877_i2c_driver,
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

	/* MIC_SEL */
	if (gpio_is_valid(GPIO_MIC_SEL)) {
		if (gpio_request(GPIO_MIC_SEL, S3C_GPIO_LAVEL(GPIO_MIC_SEL))) 
			printk(KERN_ERR "Failed to request GPIO_MIC_SEL! \n");
		gpio_direction_output(GPIO_MIC_SEL, 0);
	}
	s3c_gpio_setpull(GPIO_MIC_SEL, S3C_GPIO_PULL_NONE);

	return 0;
}

int audio_power(int en)
{
	if(shutdown_flag)
	{
		gpio_set_value(GPIO_AUDIO_EN, 0);
		return 0;
	}

	P("AUDIO POWER : %d", en);
	if (en)
		gpio_set_value(GPIO_AUDIO_EN, 1);
	else
		gpio_set_value(GPIO_AUDIO_EN, 0);

	return 0;
}

int amp_init(void)
{
	int ret;

//	FI

	ret = i2c_add_driver(&max9877_i2c_driver);
	if (ret != 0)
		printk(KERN_ERR "can't add i2c driver\n");

//	FO
	return ret;
}

int amp_enable(int en)
{
	u8 pData = 0;

	if(shutdown_flag && en)
		return 0;
	
	P("AMP EN : %d", en);
	max9877_read(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, &pData);
	
	if (en){ 
		max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, pData | SHDN ); /* Low Power shutdown mode : 1 */
		mdelay(10);

//		if (cur_amp_path == AK4671_AMP_PATH_SPK_HP) {/*when ringtone earphone level under 100db for ear : HW require  */
//			max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 31);
//			max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0x14); 
//			max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0x14); 
//		}else {
//		max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 31);
//		max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,31); 
//		max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,31); 
//		}

	}else{
		max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 0);
		max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0); 
		max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0); 
		
		mdelay(10);
		max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, pData & ~SHDN ); /* Low Power shutdown mode : 0 */
			
	}
	return 0;
}

static void set_amp_gain(int mode)
{
	u8 spk_vol, left_ear_vol, right_ear_vol;
    
	spk_vol = 0x1F;
	left_ear_vol = 0x1F;
	right_ear_vol = 0x1F;
  
	P("SET AMP gain : 0x%x", mode);

	/* Set output amp gain */
            if( (0xf0 & mode) == MM_AUDIO_VOICECALL )
            {
                P("MM_AUDIO_VOICECALL amp gain setting", mode);
                if( (0x0f & mode ) == MM_AUDIO_OUT_HP )
                {
                    P("Amp Gain Setting for MM_AUDIO_VOICECALL -> MM_AUDIO_OUT_HP");
                    spk_vol = 0x00;
                    left_ear_vol = 0x18; // 0x15 -> 0x18
                    right_ear_vol = 0x18; // 0x15 -> 0x18
 
                }
                else if ( (0x0f & mode) == MM_AUDIO_OUT_SPK)
                {
                    P("Amp Gain Setting for MM_AUDIO_VOICECALL -> MM_AUDIO_OUT_SPK");
                    spk_vol = 0x17; // 0x16 -> 0x17
                    left_ear_vol = 0x00;
                    right_ear_vol = 0x00; 
                }              
            }
            else if ( ( 0xf0 & mode ) == MM_AUDIO_PLAYBACK )
            {
                P("MM_AUDIO_PLAYBACK amp gain setting", mode);
                if ( (0x0f & mode ) == MM_AUDIO_OUT_SPK_HP 
                    || (0x0f & mode ) == MM_AUDIO_OUT_RING_SPK_HP) {/*when ringtone earphone level under 100db for ear : HW require  */
                    P("Amp Gain Setting for MM_AUDIO_PLAYBACK -> MM_AUDIO_OUT_SPK_HP");
                    spk_vol = 0x1F;
                    left_ear_vol = 0x11; // 0x14 -> 0x11
                    right_ear_vol = 0x11; // 0x14 -> 0x11
                }else  if( (0x0f & mode ) == MM_AUDIO_OUT_HP )
                {
                    P("Amp Gain Setting for MM_AUDIO_PLAYBACK -> MM_AUDIO_OUT_HP");
                    spk_vol = 0x00;
                    left_ear_vol = 0x1F;
                    right_ear_vol = 0x1F; 
                }
                else if ( (0x0f & mode) == MM_AUDIO_OUT_SPK)
                {
                    P("Amp Gain Setting for MM_AUDIO_PLAYBACK -> MM_AUDIO_OUT_SPK");
                    spk_vol = 0x1F;
                    left_ear_vol = 0x00;
                    right_ear_vol = 0x00; 
                }       
            }
            else if ( ( 0xf0 & mode ) == MM_AUDIO_FMRADIO )
            {
                P("MM_AUDIO_FMRADIO amp gain setting", mode);
                if( (0x0f & mode ) == MM_AUDIO_OUT_HP )
                {
                    P("Amp Gain Setting for MM_AUDIO_FMRADIO -> MM_AUDIO_OUT_HP");
                    spk_vol = 0x00;
                    left_ear_vol = 0x1F;
                    right_ear_vol = 0x1F; 
                }
                else if ( (0x0f & mode) == MM_AUDIO_OUT_SPK)
                {
                    P("Amp Gain Setting for MM_AUDIO_FMRADIO -> MM_AUDIO_OUT_SPK");
                    spk_vol = 0x1F;
                    left_ear_vol = 0x00;
                    right_ear_vol = 0x00; 
                }
                else if ( (0x0f & mode ) == MM_AUDIO_OUT_SPK_HP )
                {
                    P("Amp Gain Setting for MM_AUDIO_FMRADIO -> MM_AUDIO_OUT_SPK_HP");
                    spk_vol = 0x1F;
                    left_ear_vol = 0x14;
                    right_ear_vol = 0x14; 
                }
            }
            else if ( (0xf0 & mode ) == MM_AUDIO_VOICEMEMO )
            {
                P("MM_AUDIO_VOICEMEMO amp gain setting", mode);
                if( (0x0f & mode ) == MM_AUDIO_OUT_HP )
                {
                    P("Amp Gain Setting for MM_AUDIO_VOICEMEMO -> MM_AUDIO_OUT_HP");
                    spk_vol = 0x00;
                    left_ear_vol = 0x1F;
                    right_ear_vol = 0x1F; 
                }
                else if ( (0x0f & mode) == MM_AUDIO_OUT_SPK)
                {
                    P("Amp Gain Setting for MM_AUDIO_VOICEMEMO -> MM_AUDIO_OUT_SPK");
                    spk_vol = 0x1F;
                    left_ear_vol = 0x00;
                    right_ear_vol = 0x00; 
                } 
            }
            else
            {
                P("unknown mode for amp gain setting = 0x%x",mode);
            }
            
            P("Amp Gain Setting for spk_vol = 0x%02x, left_ear_vol = 0x%02x, right_ear_vol = 0x%02x",spk_vol,left_ear_vol,right_ear_vol);
            max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, spk_vol);
            max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,left_ear_vol); 
            max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,right_ear_vol); 
}

int amp_set_path(int path)
{
	int i; 
#if MAX9877_SPECIFIC_DEBUG
	u8 pData;
	/* real all */
    for(i = 0; i <= 0x4; i++) {
		max9877_read(&max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif

	if (path == AK4671_AMP_PATH_SPK) {
		P("AMP Path : SPK");
		max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, 
			//SHDN |  /* Operational mode  */
			(BYPASS & (~BYPASS)) |  /* Bypass mode disabled */
			(OSC0 & (~OSC0))	 | /* Class D : 1176, CHARGE-PUMP : 588  */
			(OUTMODE0) );  	/* SPK:INA1INB1 */
	} else if (path == AK4671_AMP_PATH_HP) {
		P("AMP Path : HP");
			max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, 
			//SHDN |  /* Operational mode  */
			(BYPASS & (~BYPASS)) |  /* Bypass mode disabled */
			(OSC0 & (~OSC0))	 | /* Class D : 1176, CHARGE-PUMP : 588  */
			(OUTMODE0 | OUTMODE2) );  	/* LHP:INB1, RHP:INB2 */
	} else if (path == AK4671_AMP_PATH_SPK_HP) {
		P("AMP Path : SPK & HP");
			max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, 
			//SHDN |  /* Operational mode  */
			(BYPASS & (~BYPASS)) |  /* Bypass mode disabled */
			(OSC0 & (~OSC0))	 | /* Class D : 1176, CHARGE-PUMP : 588  */
			(OUTMODE1|OUTMODE2) );  	/* SPK:INB1+INB2   LHP:INB1, RHP:INB2 */
	}

	cur_amp_path =  path ;

#if MAX9877_SPECIFIC_DEBUG
	/* real all */
    for(i = 0; i <= MAX9877_OUTPUT_MODE_CONTROL; i++) {
		max9877_read(&max9877_i2c_client, i, &pData);
		P("MAX9877 REG - 0x%02x : 0x%02x", i, pData);
	}
#endif
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
    for(i = 0; i <= MAX9877_OUTPUT_MODE_CONTROL; i++) {
		max9877_read(&max9877_i2c_client, i, &pData);
		sprintf(buf, "%s[0x%02x] = 0x%02x\r\n", buf, i + 0xe0, pData);
	}

	return 0;
}

int amp_set_register(unsigned char reg, unsigned char val)
{
	P("AMP Register (Write) reg:0x%02x, val:0x%02x", reg, val);
	max9877_write(&max9877_i2c_client, reg, val);
	return 0;
}

int mic_enable(int en)
{
	P("MIC EN : %d", en);
	if (en) {
		gpio_set_value(GPIO_MICBIAS_EN, 1);
	} else {
		gpio_set_value(GPIO_MICBIAS_EN, 0);
	}
	return 0;
}

int mic_set_path(int path)
{
	if (path == AK4671_MIC_PATH_MAIN) { // MAIN
		P("MIC PATH : MAIN");
		sub_mic_path = 0x9C;		// MIC-AMP Lch=IN1+/-, IN4+/- Differential Input
	} else { // SUB
		P("MIC PATH : SUB");
		sub_mic_path = 0x8D; 		// MIC-AMP Lch= LIN2, Rch= IN4+/-
	}

	return 0;
}

int set_sample_rate(struct snd_soc_codec *codec, int bitRate)
{
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
			P("[%s] not support bit rate (%d).", __func__, bitRate);
			reg_pll_mode = 0x70 | AK4671_PLL;
			break;
	}
	printk("[AK4671] Set bitRate : 0x%02x (%d) \n", reg_pll_mode, bitRate);
	codec->write(codec, 0x01, reg_pll_mode); 	

	return 0;
}

int voice_call_rec_enable(struct snd_soc_codec *codec, int mode)
{
	P("Rec Enable (mode : 0x%x)", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("enable REC (MM_AUDIO_VOICECALL_RCV)");
			/* MIC-AMP Rch mixing A/P Rch -> RCP/RCN OFF */
			codec->write(codec, 0x19, 0x04); 	// use soft mute to cut signal
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

int voice_call_rec_disable(struct snd_soc_codec *codec, int mode) 
{
	P("Rec Disable (mode : 0x%x)", mode);

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

static void set_bias (struct snd_soc_codec *codec, int mode)
{

	/* Set MIC BIAS */
	/* VOICECALL, VOICEMEMO, PLAYBACK_HP */
	if ((mode & 0xf0) == MM_AUDIO_VOICECALL || 
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO ||
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_HP ||  /* for earjack send/end key interrupt */
		(mode & 0x0f) == MM_AUDIO_PLAYBACK_SPK_HP )
	{
		if( mode== MM_AUDIO_VOICECALL_RCV || mode == MM_AUDIO_VOICEMEMO_MAIN || mode == MM_AUDIO_VOICEMEMO_SUB)
		{
                   P("Main Mic");
			gpio_set_value(GPIO_MIC_SEL, 1); // 1 : Main-Mic  0 : Sub-Mic
	        }
		else if ( mode== MM_AUDIO_VOICECALL_SPK )
		{
                   P("Sub Mic");
			gpio_set_value(GPIO_MIC_SEL, 0); 
		}

                P("MIC_SEL = %d",gpio_get_value(GPIO_MIC_SEL));
             
		if( (mode & 0xf0) == MM_AUDIO_VOICECALL 
                || (mode & 0xf0) == MM_AUDIO_VOICEMEMO 
                )
		{
			P("mic bias enable ");
			if ((mode & 0x0f) == MM_AUDIO_OUT_HP) 
				mic_enable(1);
			else if ((mode & 0x0f) == MM_AUDIO_OUT_RCV || (mode & 0x0f) == MM_AUDIO_OUT_SPK)
				mic_enable(1);
		}
	}
	else
	{
		if(!get_headset_status()){
			P("mic bias disable ");
			mic_enable(0);
		}
	}

      /*SET AMP Gain before Amp enable/disable */
      set_amp_gain(mode);

      mdelay(40); // amp pop-up noise

	/* Set AMP BIAS */
	/* SPK, EARJACK, VOICEMEMO */
	if ((mode & 0x0f) == MM_AUDIO_OUT_SPK || 
		(mode & 0x0f) == MM_AUDIO_OUT_HP || 
		(mode & 0x0f) == MM_AUDIO_OUT_SPK_HP || 
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO ) 
	{
		P("amp bias enable ");
		if (mode != MM_AUDIO_VOICECALL_BT)
			amp_enable(1);	
	} 
	else
	{
		P("amp bias disable ");
		amp_enable(0);	
	}
}

static void set_input_path_gain(struct snd_soc_codec *codec, int mode)
{
	P("set INPUT path gain : 0x%x", mode);
	switch(mode)
	{
  		case MM_AUDIO_VOICEMEMO_MAIN:		
			codec->write(codec, 0x05, 0x5B); 	//MIC-AMP 18dB 2009.07.10
			codec->write(codec, 0x12, 0xD6);   // 27dB
			codec->write(codec, 0x13, 0xD6);
			break;		
  		case MM_AUDIO_VOICEMEMO_SUB:		
			codec->write(codec, 0x05, 0x5B); 	//MIC-AMP 18dB 2009.07.10
			codec->write(codec, 0x12, 0xD6);   // 27dB
			codec->write(codec, 0x13, 0xD6);
			break;
  		case MM_AUDIO_VOICEMEMO_EAR:		
			codec->write(codec, 0x05, 0x5B); 	// MIC-AMP Gain 0dB -> 18dB (default)
			codec->write(codec, 0x12, 0xC5);   // 19.5
			codec->write(codec, 0x13, 0xC5);
			break;

  		case MM_AUDIO_FMRADIO_SPK:		
			codec->write(codec, 0x05, 0x66); 	// => MIC-AMP Gain=0dB (default)
			break;

  		case MM_AUDIO_FMRADIO_HP:		
			codec->write(codec, 0x05, 0x66); 	// => MIC-AMP Gain=0dB (default)
			break;
		default :
			printk("[%s] Invalid input gain path\n", __func__);
			break;
	}
}

static void set_path_gain(struct snd_soc_codec *codec, int mode)
{

	set_input_path_gain(codec, mode);

	/* VOICEMEMO Path : only SPK */
	if ((mode & 0xf0) == MM_AUDIO_VOICEMEMO)
		mode = MM_AUDIO_PLAYBACK_SPK;

	P("SET Path gain : 0x%x", mode);

	/* Set output tunning value */
	switch (mode) 
	{
		case MM_AUDIO_PLAYBACK_RCV :
			break;
		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_SPK_HP:
			codec->write(codec, 0x08, 0x95); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x0E); 	// 0x18 -> 0x0E Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x0E); 	// 0x18 -> 0x0E Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_HP :
			codec->write(codec, 0x08, 0xB5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x18); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x18); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_VOICECALL_RCV:		
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring this register set in routing sequence also
			codec->write(codec, 0x05, 0x45); // 0x35 -> 0x45
			//codec->write(codec, 0x11, 0x10);	//warring this register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xB4); // A5 -> 0xB4
			break;
	 	case MM_AUDIO_VOICECALL_HP:	
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring 0x0D register set in routing sequence also
			codec->write(codec, 0x05, 0x45); // 0x55 -> 0x45
			//codec->write(codec, 0x11, 0x10);	//warring 0x11 register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xB5); // 0xA5 -> 0xB5
			break;
	 	case MM_AUDIO_VOICECALL_SPK:		
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring 0x0D register set in routing sequence also
			codec->write(codec, 0x05, 0x55); // 0x55 -> 0x56 -> 0x55
			//codec->write(codec, 0x11, 0x10);	//warring 0x11 register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xD5); // 0xC5 ->0xD5
			break;
	 	case MM_AUDIO_VOICECALL_BT:	
			//RX
			codec->write(codec, 0x05, 0x25); // 0x55 -> 0x25
			codec->write(codec, 0x11, 0x10);
			codec->write(codec, 0x1B, 0x18); // Rch Output Volume			
			//TX
			codec->write(codec, 0x08, 0xa5);
			codec->write(codec, 0x56, 0x18); // -> 0x18 (0dB)
			codec->write(codec, 0x1A, 0x1E); // 0x18 -> 0x1E Lch Output Volume
			break;

  		case MM_AUDIO_FMRADIO_SPK:			
			codec->write(codec, 0x08, 0xB5); 	// Output Volume Control : OUT2[7:4] = 0dB /OUT1[2:0] = 0dB
			codec->write(codec, 0x1A, 0x18); 	// Lch Output Digital Vol : 0dB
			codec->write(codec, 0x1B, 0x18); 	// Rch Output Digital Vol : 0dB
			break;

  		case MM_AUDIO_FMRADIO_HP:
			codec->write(codec, 0x08, 0xD5); 	// 0xB5(0dB) -> 0xD5(6dB) Output Volume Control : OUT2[7:4] = 0dB /OUT1[2:0] = 0dB
			codec->write(codec, 0x1A, 0x18); 	// Lch Output Digital Vol : 0dB
			codec->write(codec, 0x1B, 0x18); 	// Rch Output Digital Vol : 0dB
			break;
                
		default :
			printk("[%s] Invalid output gain path\n", __func__);
	}
}


int path_enable(struct snd_soc_codec *codec, int mode)
{
	P("Enable PATH : 0x%x", mode);

	/* general init register */
	if(mode) {
		//codec->write(codec, 0x02, 0x01); 	// PLL Mode and Power up
		codec->write(codec, 0x01, reg_pll_mode); 	
		codec->write(codec, 0x02, 0x03); 	// PLL Mode and Power up, Master Mode
		codec->write(codec, 0x03, 0x03); 	// I2S mode
	}

	/* Set gain value */
	set_path_gain(codec, mode);	

	/* Set path register sequence */
	switch(mode) {
		case 0:
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			P("set MM_AUDIO_PLAYBACK_RCV");
			codec->write(codec, 0x09, 0x01); 	// D/A Lch -> Lout1
			codec->write(codec, 0x0A, 0x01); 	// D/A Rch -> Rout1
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 				// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x0F, 0x04); 	// LOPS1="1",use for pop noise cancel 
			mdelay(1);				// wait 100ns
			codec->write(codec, 0x0F, 0x07); 	// PML01, PMRO1 power-up
			mdelay(30); 				// wait more than 30ms
			codec->write(codec, 0x0F, 0x23);	// LOPS1='0', RCV mono
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
			P("set MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP");
			codec->write(codec, 0x0B, 0x01); 	// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 	// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 				// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);				// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			if( mode != MM_AUDIO_PLAYBACK_SPK)
				mdelay(130);
			break;

		case MM_AUDIO_PLAYBACK_BT :
			P("set MM_AUDIO_PLAYBACK_BT");
			codec->write(codec, 0x15, 0x00); 	// 5-band EQ Rch=STDI Rch
			codec->write(codec, 0x15, 0x41); 	// SRC-A = MIX Rch
			mdelay(1);				// wait 100ns
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
			codec->write(codec, 0x11, 0xA4);	// LOPS3=1, use for pop noise cancel
			codec->write(codec, 0x11, 0xA7);	// PMLO3, PMRO3 power-up
			mdelay(30); 				// Wait more than 30ms(Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0xA3);	// LOPS3=0

			/* MIC-AMP Rch + A/P Rch => RCP/RCN */
			codec->write(codec, 0x00, 0x8D);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0A, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN

			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("set MM_AUDIO_VOICECALL_SPK,HP");
			codec->write(codec, 0x0C, 0x20); 	// MIC-AMP-Rch to Rout2
			codec->write(codec, 0x10, 0x08); 	// MONO DAC to Lout2/Rout2
			codec->write(codec, 0x0D, 0x20); 	// MIC-AMP Lch -> LOP/LON
			codec->write(codec, 0x11, 0xA0); 	// LOP/LON gain=0dB
			if (mode == MM_AUDIO_VOICECALL_SPK){
				codec->write(codec, 0x04, 0x9C);	// MIC-AMP Lch=IN1+/-, IN4+/- Differential Input
				//codec->write(codec, 0x04, 0x8D); 	// MIC-AMP Lch= LIN2, Rch= IN4+/-
			}
			else
			{
				codec->write(codec, 0x04, 0xCE); 	// MIC-AMP Lch=IN3+/-, Rch=IN4+/-
			}
			/* Other setting if needed except for power setting */
			codec->write(codec, 0x00, 0x01); 	// VCOM power-up
			codec->write(codec, 0x06, 0x03); 	// PMLOOPL, PMLOOPR power-up
			codec->write(codec, 0x00, 0x0D); 	// MIC-AMP power-up
			codec->write(codec, 0x10, 0x73); 	// PMLO2, PMRO2, PMLO2S,PMRO2s=1
			codec->write(codec, 0x10, 0x77); 	// MUTEN=1
			codec->write(codec, 0x11, 0xA4); 	// LOPS3=1, use for pop noise cancel
			codec->write(codec, 0x11, 0xA7); 	// PMLO3, PMRO3 power-up
			mdelay(30); 				// Wait more than 30ms(Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0xA3); 	// LOPS3=0

			/* MIC-AMP Rch + A/P Rch => Lout2/Rout2 */
			codec->write(codec, 0x00, 0xCD);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0B, 0x01);	// (MIC-AMP-Rch mixing DAC-Lch) to Lout2
			codec->write(codec, 0x0C, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to Rout2

			break;

		case MM_AUDIO_VOICECALL_BT :
			P("set MM_AUDIO_VOICECALL_BT");
			codec->write(codec, 0x01, 0xF8); 		// fs=44.1kHz, MCKI=19.2MHz input

			codec->write(codec, 0x11, 0xA0); 		// LOP/LON, gain=0dB
			codec->write(codec, 0x0D, 0x01); 		// D/A Lch -> LOP/LON
			codec->write(codec, 0x04, 0xCE); 		// MIC-AMP Rch= IN4+/- be selected
			codec->write(codec, 0x15, 0x14); 		// 5-band EQ Lch=SRC-B, Rch=SVOLA Rch
			codec->write(codec, 0x19, 0x40); 		// OVOLC, SRC-A : Rch, OVOLC = independnet 
			codec->write(codec, 0x00, 0x01); 		// VCOM power-up
			mdelay(40);
			codec->write(codec, 0x00, 0x69); 		// PMVCM, PMMICR, PMADR, PMDAL, PMDAR power up
			codec->write(codec, 0x53, 0x17); 		// PLLBT1,PMSRA/B, PMPCM power up
								
			mdelay(40);					//PLLBT lock time: max 40ms (base on BICKA)
			codec->write(codec, 0x11, 0x14); // 0xA4 -> 0x14(-6dB) 		// LOPS3=1, use for pop noise cancel
			mdelay(1); 		// Wait more than 100ns
			codec->write(codec, 0x11, 0x17); //0xA7 -> 0x17(-6dB) 		// PMLO3, PMRO3 power-up
			mdelay(100);					// Wait more than 100ms
									// (Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0x13); //0xA3 -> 0x13(-6dB) 		// LOPS3=0
			
			/* Mixing ADC Rch and A/P Rch */
			codec->write(codec, 0x15, 0x18); 		// Lch: from SRC-B;
													//Rch: from (SVOLA Rch + SDTI Rch)
			break;
		
		case MM_AUDIO_VOICEMEMO_MAIN :
			P("set MM_AUDIO_VOICEMEMO_MAIN");
			mic_set_path(AK4671_MIC_PATH_MAIN);
			codec->write(codec, 0x04, 0x04); 		// MIC-AMP Lch=IN1+/-=> MIC-AMP Lch=IN1
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
			mic_set_path(AK4671_MIC_PATH_SUB);
			codec->write(codec, 0x04, 0x04); 		// MIC-AMP Lch=IN1+/-=> MIC-AMP Lch=IN1
			codec->write(codec, 0x04, 0x04); 		// => MIC-AMP Lch=LIN2
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

		case MM_AUDIO_FMRADIO_RCV :
			P("set MM_AUDIO_FMRADIO_RCV");
			codec->write(codec, 0x09, 0x04);	// => LOUT1= LIN2
			codec->write(codec, 0x0A, 0x04);	// => ROUT1= RIN2
			codec->write(codec, 0x08, 0xB5);	// => gain=0dB(default)
			codec->write(codec, 0x00, 0x01);	// => VCOM power-up
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x07, 0x0C);	// => PMAINL2, PMAINR2 power-up
			codec->write(codec, 0x0F, 0x04);	// => LOPS1=1
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x0F, 0x07);	// => PMLO1, PMRO1 power-up
			mdelay(310);						// Wait more than 300ms 
												// (Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x0F, 0x03);	// => LOPS1=0
			codec->write(codec, 0x0F, 0x23);	// LOPS1='0', RCV mono

                   /* RIN2(FM_ROUT) Rch + DAC(A/P) Rch => RCP/RCN */
			codec->write(codec, 0x00, 0x81);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0A, 0x05);	// (FM_ROUT mixing DAC-Rch) to RCP/RCN
			break;

		case MM_AUDIO_FMRADIO_SPK :
		case MM_AUDIO_FMRADIO_HP :
		case MM_AUDIO_FMRADIO_SPK_HP : 		            
			P("set MM_AUDIO_FMRADIO_SPK, HP, SPK_HP");

#if 0 // for FM radio recording
			/* FM Radio -> LOUT2/ROUT2 */
			codec->write(codec, 0x0B, 0x04);	// => LOUT2= LIN2
			codec->write(codec, 0x0C, 0x04);	// => ROUT2= RIN2
			// codec->write(codec, 0x08, 0xD5);	// => L1OUT/R1OUT = 0dB, L2/R2 gain=0dB -> 6dB
			codec->write(codec, 0x07, 0x0C);	// => PMAINL2, PMAINR2 = 1
			codec->write(codec, 0x00, 0x01);	// => VCOM power-up
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x10, 0x63);	// => PMLO2, PMRO2, PMLO2S, PMRO2S = 1
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x10, 0x67);	// => MUTEN=1

			mdelay(2);							// Wait more than 100ns
			/* MIC-AMP-Lch -> ADC Lch/Rch -> AP Lch/Rch */
			codec->write(codec, 0x02, 0x26);	// MCKO out, BCKO=64fs, PLL master mode
			codec->write(codec, 0x04, 0x05); 	// MIC-AMP Lch:LIN2, Rch:RIN2
			codec->write(codec, 0x01, 0xF8);	// fs=44.1kHz, MCKI=19.2MHz input
			codec->write(codec, 0x02, 0x27);	// PLL power-up
			mdelay(40);
			codec->write(codec, 0x00, 0x3D);	// ADC power-up
			mdelay(24);

                    	/* FM Radio + DAC => Lout2/Rout2 */
			codec->write(codec, 0x00, 0xC1);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0B, 0x05);	// (FM_LOUT mixing DAC-Lch) to Lout2
			codec->write(codec, 0x0C, 0x05);	// (FM_ROUT mixing DAC-Rch) to Rout2
#else
			/* FM Radio Path Setting */
			codec->write(codec, 0x0B, 0x20);	// MIC-AMP Lch => LOUT2
			codec->write(codec, 0x0C, 0x20);	// MIC-AMP Rch => ROUT2

		  	codec->write(codec, 0x04, 0x05); 	// MDIF2[5] = 0 , [3:0] = 0x5 
			// codec->write(codec, 0x08, 0xB5);	// => gain=0dB(default)
			codec->write(codec, 0x07, 0x0C);	// => PMAINL2, PMAINR2 = 1
			codec->write(codec, 0x00, 0x01);	// => VCOM power-up
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x00, 0x0D);	// => MIC-AMP
			codec->write(codec, 0x06, 0x03);	// => PMLOOPL,PMLOOPR power-up
			codec->write(codec, 0x10, 0x63);	// => PMLO2, PMRO2, PMLO2S, PMRO2S = 1
			mdelay(2);							// Wait more than 100ns
			codec->write(codec, 0x10, 0x67);	// => MUTEN=1

			/* FM Radio Recording */
			codec->write(codec, 0x01, 0xF8);	// Fs = 44.1kHz, MCKI=19.2MHz input
			// codec->write(codec, 0x05, 0x55);	// MIC-AMP Lch/Rch Gain = 0dB
			// ALC Setting
			codec->write(codec, 0x16, 0x05);	// WTM[2:0] = 1(5.8ms) , [ZTM[1:0] = 1(3.8ms)
			codec->write(codec, 0x14, 0xE1);	// +30dB
			codec->write(codec, 0x17, 0x01);	// LMTH[1:0] = 1 
			codec->write(codec, 0x18, 0x03);	// ALC Enable
			codec->write(codec, 0x00, 0x3D);	// => MIC-AMP + A/D power-up
		
			/* AP Path Setting */	
			/* FM Radio + DAC => LOUT2/ROUT2  */
			codec->write(codec, 0x00, 0xFD); 	// => MIC-AMP + ADC Lch/Rch power-up + DAC Rch/Lch power-up
			codec->write(codec, 0x0B, 0x21);	// => MIC_AMP Lch + DAC Lch => LOUT2
			codec->write(codec, 0x0C, 0x21); 	// => MIC_AMP Rch + DAC Rch => ROUT2
#endif

			break;

		case MM_AUDIO_FMRADIO_BT :
			P("set MM_AUDIO_FMRADIO_BT");
			codec->write(codec, 0x04, 0x24);	// => MIC-AMP Rch=IN2+/-
			codec->write(codec, 0x19, 0x41);	// => SRC-A= MIX Rch
			codec->write(codec, 0x15, 0x04);	// => 5-band EQ Rch=SVOLA Rch
			codec->write(codec, 0x00, 0x01);	// => VCOM power-up
			mdelay(2); 							// Wait more than 100ns
			codec->write(codec, 0x00, 0x39);	// => A/D, MIC-AMP Rch power-up
			codec->write(codec, 0x53, 0x00);	// => PMPCM, PMSRA, PMSRB= ¡°1¡±,
												// PCM reference= BICKA(if VCOCBT=10kohm & 4.7nF)
			mdelay(40); 						// Lock time= 40ms
			break;

		default :
			P("[SOUND MODE] invalid mode!!! ");
	}

	/* disable soft mute */
	if (mode ==  MM_AUDIO_VOICECALL_BT)
		codec->write(codec, 0x19, 0x40); 		// OVOLC, SRC-A : Rch
	else if ( mode == MM_AUDIO_FMRADIO_BT )
		codec->write(codec, 0x19, 0x41); 
	else
		codec->write(codec, 0x19, 0x01); 	// disable soft mute


	set_bias(codec, mode);

	return 0;
}

int path_disable(struct snd_soc_codec *codec, int mode)
{
	P("Diasble PATH : 0x%x", mode);

	amp_enable(0);//for noise reduce

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
			P("MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP Off");
			codec->write(codec, 0x10, 0x73); 	// MUTEN='0'
			mdelay(30); 	// wait more than 30ms
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
			//codec->write(codec, 0x19, 0x04);	// use soft mute to cut signal
			//mdelay(24);
			codec->write(codec, 0x0A, 0x20);	// only MIC-AMP-Rch to RCP/RCN
			codec->write(codec, 0x00, 0x0D);	// DAC-Rch power-up
			codec->write(codec, 0x04, 0x00);	// Mic Amp input select default
			codec->write(codec, 0x0A, 0x00);	//Mic Amp Rch -> RcP/Rcn select default
			codec->write(codec, 0x0D, 0x00);	//Mic Amp Lch -> Lop/Lon select default
			codec->write(codec, 0x06, 0x00);	//pmloopl, pmloopr power down

			/* Normal Off */
			codec->write(codec, 0x11, 0xA7);	// LOPS3=1
			codec->write(codec, 0x0F, 0x27);	// LOPS1=1
			codec->write(codec, 0x0F, 0x24);	// RCP/RCN power-down
			codec->write(codec, 0x11, 0xA4);	// LOP/LON power-down
			mdelay(30);
			codec->write(codec, 0x11, 0x10);	// default
			codec->write(codec, 0x0F, 0x00);	// default
			//codec->write(codec, 0x01, 0x00); 	// 
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("set MM_AUDIO_VOICECALL_SPK,HP Off");

			/* MIC-AMP Rch + A/P Rch => Lout2/Rout2 Off */
			#if 0
//			codec->write(codec, 0x19, 0x04); 	// use soft mute to shut-down signal
//			mdelay(24);
			codec->write(codec, 0x0B, 0x00); 	// Swith-off DAC-Lch
			codec->write(codec, 0x0C, 0x20); 	// Swith-off DAC-Rch 
			codec->write(codec, 0x00, 0x0D); 	// DAC power-up
			codec->write(codec, 0x02, 0x00); 	// PLL power-down
			#endif
			
			/* Normal Off */
			codec->write(codec, 0x11, 0xA7); 	// LOPS3=1
			codec->write(codec, 0x11, 0xA4); 	// LOP/LON power-down
			codec->write(codec, 0x10, 0x6B); 	// MUTEN=0
			mdelay(30);
			codec->write(codec, 0x11, 0x10); 	// default
			codec->write(codec, 0x10, 0x00); 	// default
			//codec->write(codec, 0x01, 0x00); 	//
			codec->write(codec, 0x00, 0x00); 	//
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("set MM_AUDIO_VOICECALL_BT Off");

			/* Mixing ADC Rch and A/P Rch Off */
			//codec->write(codec, 0x15, 0x14); 	// 5-band-EQ-Lch: from SRC-B;
			codec->write(codec, 0x15, 0x00); 	// 5-band-EQ-Lch: from SRC-B;
												// Rch: from SVOLA Rch
													
			/* Normal Off */
			codec->write(codec, 0x11, 0xA7); 		// LOPS3=1
			codec->write(codec, 0x11, 0xA4); 	// LOP/LON power-down
			mdelay(30); 		// Wait more than 30ms
			codec->write(codec, 0x11, 0x10);	// default
			codec->write(codec, 0x53, 0x00);	// PMSRA/B, PMPCM power-down
			//codec->write(codec, 0x02, 0x00); 	// PLL power-down
			//codec->write(codec, 0x01, 0x00);
			codec->write(codec, 0x00, 0x00);	// VCOM and others power-down

			break;

		case MM_AUDIO_VOICEMEMO_MAIN :
			P("set MM_AUDIO_VOICEMEMO_MAIN Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_SUB :
			P("set MM_AUDIO_VOICEMEMO_SUB Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_EAR :
			P("set MM_AUDIO_VOICEMEMO_EAR Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_BT :
			P("set MM_AUDIO_VOICEMEMO_BT Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_FMRADIO_RCV :
			P("set MM_AUDIO_FMRADIO_RCV Off");
			codec->write(codec, 0x0F, 0x27);	// => LOPS1=1
			mdelay(1);						 	// Wait more than 1ms
			codec->write(codec, 0x0F, 0x04); 	// => LOUT1/ROUT1 power-down
			mdelay(1);		 					// Wait more than 1ms
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_FMRADIO_SPK :
		case MM_AUDIO_FMRADIO_HP :
			P("set MM_AUDIO_FMRADIO_HP/SPK Off");
			codec->write(codec, 0x10, 0x73);	// => MUTEN=0
			mdelay(500);						// Wait more than 500ms
			codec->write(codec, 0x10, 0x00); 	// => LOUT2/ROUT2 power-down
			codec->write(codec, 0x00, 0x01);

			/* Recording ALC Disable */
			codec->write(codec, 0x18, 0x02); 	// ALC Disable

			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_FMRADIO_BT :
			P("set MM_AUDIO_FMRADIO_BT Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		default:
			P("[SOUND MODE] invalid mode!!! ");
	}

	return 0;
}

int idle_mode_enable(struct snd_soc_codec *codec, int mode)
{
	P("Enable Idle Mode : 0x%x", mode);

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
			mdelay(310); 	// wait more than 300ms
			codec->write(codec, 0x0F, 0x23);	// LOPS1='0', RCV mono
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
			P("set MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP");
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			if( mode != MM_AUDIO_PLAYBACK_SPK)
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

	set_bias(codec, mode);

	return 0;
}

int idle_mode_disable(struct snd_soc_codec *codec, int mode)
{
	P("Disable Idle mode : PATH : 0x%x", mode);


	if(!get_headset_status())
		mic_enable(0);

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
			P("MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP Off");
			codec->write(codec, 0x10, 0x73); 	// MUTEN='0'
			mdelay(30); 	// wait more than 30ms
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

void set_mic_mute(struct snd_soc_codec *codec, int mode, int mute)
{

	if(mute)
	{
		P("mute input digital gain");
		codec->write(codec, AK4671_L_INPUT_VOL, 0x0000); // input digital gain : Mute
		codec->write(codec, AK4671_L_INPUT_VOL, 0x0000); // input digital gain : Mute
	}
	else
	{
		P("restore input digital gain");
		set_input_path_gain(codec, mode);
	}
}
