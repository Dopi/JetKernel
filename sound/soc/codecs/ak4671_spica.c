/*
 * ak4671_spica.c  --  AK4671 Spica Board specific code
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
#include "max9877_def.h"

//#define AUDIO_SPECIFIC_DEBUG	1
//#define MAX9877_SPECIFIC_DEBUG  1
#define SUBJECT "ak4671_spica.c"

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

#define AK4671_PLL 			0x08   // 19.2MHz
#define MAX9877_ADDRESS 	0x9A

static unsigned short reg_pll_mode = 0xf0 | AK4671_PLL; // Default setting : 44.1kHz, 19.2MHz
static unsigned int sub_mic_path;
static unsigned int cur_amp_path;

static unsigned short max9877_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max9877_ignore[] = { I2C_CLIENT_END };
static unsigned short max9877_i2c_probe[] = { 5, MAX9877_ADDRESS >> 1, I2C_CLIENT_END };

static struct i2c_driver max9877_i2c_driver;
static struct i2c_client max9877_i2c_client;

static struct i2c_client_address_data max9877_addr_data = {
	.normal_i2c = max9877_normal_i2c,
	.ignore     = max9877_ignore,
	.probe      = max9877_i2c_probe,
};


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

	return 0;
}

int audio_power(int en)
{
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
		printk(KERN_ERR "can't add i2c driver");

//	FO
	return ret;
}

int amp_enable(int en)
{
        u8 pData = 0;

        P("AMP EN : %d", en);
        max9877_read(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, &pData);

        if (en){
                max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 0);
                max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0);
                max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0);

                max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, pData | SHDN ); /* Low Power shutdown mode : 1 */
                mdelay(30);

                if (cur_amp_path == AK4671_AMP_PATH_SPK_HP)
                {/*when ringtone earphone level under 100db for ear : HW require  */
                     max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 31);
                     max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0x11);
                     max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0x11);
                }
                else if (cur_amp_path == AK4671_AMP_PATH_HP)
                {
                     max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 0);
                     max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,31);
                     max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,31);
                }
                else if (cur_amp_path == AK4671_AMP_PATH_SPK)
                {
                     max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 31);
                     max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0);
                     max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0);
                }

        }else{
                max9877_write(&max9877_i2c_client, MAX9877_SPEAKER_VOLUME, 0);
                max9877_write(&max9877_i2c_client, MAX9877_LEFT_HEADPHONE_VOLUME,0);
                max9877_write(&max9877_i2c_client, MAX9877_RIGHT_HEADPHONE_VOLUME,0);

                mdelay(30);
                max9877_write(&max9877_i2c_client, MAX9877_OUTPUT_MODE_CONTROL, pData & ~SHDN ); /* Low Power shutdown mode : 0 */

        }
        return 0;
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
	u8 pData;

	sprintf(buf, "%s MAX9877\r\n", buf);
    for(i = 0; i <= MAX9877_OUTPUT_MODE_CONTROL; i++) {
		max9877_read(&max9877_i2c_client, i, &pData);
		sprintf(buf, "%s[0x%02x] = 0x%02x\r\n", buf, i + 0xe0, pData);
	}

	return 0;
}

int amp_set_register(unsigned char reg, unsigned char val)
{
	P("AMP Register (Write) reg:0x%02x, val:0x%02x\n", reg, val);
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
			P("[%s] not support bit rate (%d).\n", __func__, bitRate);
			reg_pll_mode = 0x70 | AK4671_PLL;
			break;
	}
	printk("[AK4671] Set bitRate : 0x%02x (%d) \n", reg_pll_mode, bitRate);
	codec->write(codec, 0x01, reg_pll_mode); 	

	return 0;
}

/* Rx only recorded in spica */
int voice_call_rec_enable(struct snd_soc_codec *codec, int mode)
{
	P1("Rec Enable (mode : 0x%x)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("enable REC (MM_AUDIO_VOICECALL_RCV)");
			/* MIC-AMP Rch mixing A/P Rch -> RCP/RCN OFF */
			codec->write(codec, 0x19, 0x04); 	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0A, 0x00); 	//Swith-off all output
			codec->write(codec, 0x0D, 0x00);	// Rout3 mute
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up

			codec->write(codec, 0x00, 0x2D); 	// ADC-Lch power-down, Rch power up
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("enable REC (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Rch mixing A/P Rch -> Lout2/Rout OFF */
			codec->write(codec, 0x19, 0x04); 	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0B, 0x00); 	// Swith-off all output
			codec->write(codec, 0x0C, 0x00); 	// Swith-off all output
			codec->write(codec, 0x0D, 0x00);	// Rout3 mute
			codec->write(codec, 0x00, 0x0D); 	// DAC-Rch power-up

			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x00, 0x2D); 	// ADC-Lch power-down, Rch power up
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("enable REC (MM_AUDIO_VOICECALL_BT)");
			/* mixing ADC Rch and A/P Rch -> SRC-A -> PCM-A */
			codec->write(codec, 0x15, 0x14); 	// 5-band-EQ-Lch: from SRC-B, Rch: from SVOLA Rch

			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			//codec->write(codec, 0x59, 0x10); 	// SDTO-Lch: from SRC-B
			codec->write(codec, 0x59, 0x00); 	// SDTO-Lch: from ADM (lch is muted), SDTO-Rch : from ADM (modem)
			codec->write(codec, 0x53, 0x14);   // Swith-off all output
			codec->write(codec, 0x0D, 0x00);	// Rout3 mute
			break;
		default :
				printk("[%s] Invalid mode\n", __func__);
	}

	codec->write(codec, 0x18, 0x06); 	// IVOLC, ADM Mono

	return 0;
}

int voice_call_rec_disable(struct snd_soc_codec *codec, int mode) 
{
	P1("Rec Disable (mode : 0x%x)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("disable REC (MM_AUDIO_VOICECALL_RCV)");
			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x0A, 0x21); 	// only MIC-AMP-Rch to RCP/RCN	
			codec->write(codec, 0x0D, 0x20);	
			codec->write(codec, 0x00, 0x8D); 	// DAC-Rch power-up
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("disable REC (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x0B, 0x01); 	// Swith-off DAC-Lch
			codec->write(codec, 0x0C, 0x21); 	// Swith-off DAC-Rch
			codec->write(codec, 0x0D, 0x20);	

			codec->write(codec, 0x00, 0xCD); 	// DAC-Rch power-up
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("disable REC (MM_AUDIO_VOICECALL_BT)");
			/* mixing ADC Rch and A/P Rch -> SRC-A -> PCM-A */
			codec->write(codec, 0x15, 0x18); 	// 5-band-EQ-Lch: from SRC-B, Rch: from SVOLA Rch
			
			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			codec->write(codec, 0x59, 0x00); 	// default
			codec->write(codec, 0x53, 0x17); 		// PLLBT1,PMSRA/B, PMPCM power up
			codec->write(codec, 0x0D, 0x01);
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
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO )
	{
		if ((mode & 0x0f) == MM_AUDIO_OUT_RCV || (mode & 0x0f) == MM_AUDIO_OUT_SPK
			|| (mode & 0x0f) == MM_AUDIO_OUT_HP ) 
			mic_enable(1);
	}
	else
	{
		if(!get_headset_status())
			mic_enable(0);
	}

	/* Set AMP BIAS */
	/* SPK, EARJACK, VOICEMEMO */
	if ((mode & 0x0f) == MM_AUDIO_OUT_SPK || 
		(mode & 0x0f) == MM_AUDIO_OUT_HP || 
		(mode & 0x0f) == MM_AUDIO_OUT_SPK_HP || 
		(mode & 0xf0) == MM_AUDIO_VOICEMEMO ) 
	{
		if (mode != MM_AUDIO_VOICECALL_BT)
			amp_enable(1);	
	} 
	else
	{
			amp_enable(0);	
	}
}
static void set_input_path_gain(struct snd_soc_codec *codec, int mode)
{
	P("set INPUT path gain : 0x%x\n", mode);
	switch(mode)
	{
  		case MM_AUDIO_VOICEMEMO_MAIN:		
			codec->write(codec, 0x05, 0x5B); 	//MIC-AMP 18dB 2009.07.10 -> fix by sircid
			codec->write(codec, 0x12, 0xD9); 
			codec->write(codec, 0x13, 0xD9); 
			break;		
  		case MM_AUDIO_VOICEMEMO_SUB:		
			codec->write(codec, 0x05, 0x5B); 	//MIC-AMP 18dB 2009.07.10 -> fix by sircid
			codec->write(codec, 0x12, 0xD9);
			codec->write(codec, 0x13, 0xD9);
			break;
  		case MM_AUDIO_VOICEMEMO_EAR:		
			codec->write(codec, 0x05, 0x5B); 	// => MIC-AMP Gain=0dB (default)
			codec->write(codec, 0x12, 0xC5);
			codec->write(codec, 0x13, 0xC5);
			break;
		default :
			//printk("[%s] Invalid input gain path\n", __func__);
			break;
	}
}

static void set_path_gain(struct snd_soc_codec *codec, int mode)
{

	set_input_path_gain(codec, mode);

	/* VOICEMEMO Path : only SPK */
    if (mode == MM_AUDIO_VOICEMEMO_MAIN ||
		mode == MM_AUDIO_VOICEMEMO_SUB)
            mode = MM_AUDIO_VOICECALL_RCV; // fix by sircid
    else if(mode  == MM_AUDIO_VOICEMEMO_EAR)
            mode = MM_AUDIO_PLAYBACK_HP;


	P("SET Path gain : 0x%x\n", mode);

	/* Set output tunning value */
	switch (mode) 
	{
		case MM_AUDIO_PLAYBACK_RCV :
			break;
		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_SPK_HP :
			codec->write(codec, 0x08, 0xC5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0] [increase vol]
			codec->write(codec, 0x1A, 0x18); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x18); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_PLAYBACK_HP :
			codec->write(codec, 0x08, 0xB5); 	// Output Volume Control : OUT2[7:4]/OUT1[2:0]
			codec->write(codec, 0x1A, 0x18); 	// Lch Output Digital Vol
			codec->write(codec, 0x1B, 0x18); 	// Rch Output Digital Vol
			break;
		case MM_AUDIO_VOICECALL_RCV:		
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring this register set in routing sequence also
			codec->write(codec, 0x05, 0x55);
			//codec->write(codec, 0x11, 0x10);	//warring this register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xb4);
			break;
	 	case MM_AUDIO_VOICECALL_HP:	
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring 0x0D register set in routing sequence also
			codec->write(codec, 0x05, 0x55);
			//codec->write(codec, 0x11, 0x10);	//warring 0x11 register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xb5);
			break;
	 	case MM_AUDIO_VOICECALL_SPK:		
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring 0x0D register set in routing sequence also
			codec->write(codec, 0x05, 0x5B);
			//codec->write(codec, 0x11, 0x10);	//warring 0x11 register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xa4);
			break;
		case MM_AUDIO_VOICECALL_SPK_LOOP:
			//RX
			//codec->write(codec, 0x0D, 0x20);	//warring 0x0D register set in routing sequence also
			codec->write(codec, 0x05, 0x5B);
			//codec->write(codec, 0x11, 0x10);	//warring 0x11 register set in routing sequence also
			//TX
			codec->write(codec, 0x08, 0xa4);
			break;
	 	case MM_AUDIO_VOICECALL_BT:	
			//RX
			codec->write(codec, 0x05, 0x55);
			codec->write(codec, 0x11, 0x10);
			//TX
			codec->write(codec, 0x08, 0xa5);
			codec->write(codec, 0x56, 0x55);
			break;
		default :
			printk("[%s] Invalid output gain path\n", __func__);
	}
}


int path_enable(struct snd_soc_codec *codec, int mode)
{
	P("Enable PATH : 0x%x\n", mode);

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
			//P("set MM_AUDIO_PLAYBACK_RCV");
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
			//P("set MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP");
			codec->write(codec, 0x0B, 0x01); 	// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x01); 	// D/A Rch -> Rout2
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 				// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);				// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			break;

		case MM_AUDIO_PLAYBACK_BT :
			//P("set MM_AUDIO_PLAYBACK_BT");
			codec->write(codec, 0x15, 0x00); 	// 5-band EQ Rch=STDI Rch
			codec->write(codec, 0x15, 0x41); 	// SRC-A = MIX Rch
			mdelay(1);				// wait 100ns
			codec->write(codec, 0x00, 0x01); 	// PMPCM, PMSRA='1', PCM ref = BICKA
			mdelay(40);
			
			break;

		case MM_AUDIO_VOICECALL_RCV :
			//P("set MM_AUDIO_VOICECALL_RCV");
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
		case MM_AUDIO_VOICECALL_SPK_LOOP:
			//P("set MM_AUDIO_VOICECALL_SPK,HP");
			codec->write(codec, 0x0C, 0x20); 	// MIC-AMP-Lch to Lout2
			codec->write(codec, 0x10, 0x08); 	// MIC-AMP-Lch to Rout2
			codec->write(codec, 0x0D, 0x20); 	// MIC-AMP Lch -> LOP/LON
			codec->write(codec, 0x11, 0xA0); 	// LOP/LON gain=0dB
			if (mode == MM_AUDIO_VOICECALL_SPK || mode == MM_AUDIO_VOICECALL_SPK_LOOP)
				codec->write(codec, 0x04, 0x8D); 	// MIC-AMP Lch= LIN2, Rch= IN4+/-
				//codec->write(codec, 0x04, 0x0D); 	// MIC-AMP Lch= LIN2, Rch= RIN4
			else
				codec->write(codec, 0x04, 0xCE); 	// MIC-AMP Lch=IN3+/-, Rch=IN4+/-
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
			codec->write(codec, 0x0B, 0x01);	// (MIC-AMP-Rch mixing DAC-Rch) to Lout2
			codec->write(codec, 0x0C, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to Rout2
			
			break;

		case MM_AUDIO_VOICECALL_BT :
			//P("set MM_AUDIO_VOICECALL_BT");
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
								
			mdelay(40);					//PLLBT lock time: max 40ms (base on BICKA)
			codec->write(codec, 0x11, 0xA4); 		// LOPS3=1, use for pop noise cancel
			mdelay(1); 		// Wait more than 100ns
			codec->write(codec, 0x11, 0xA7); 		// PMLO3, PMRO3 power-up
			mdelay(100);					// Wait more than 100ms
									// (Output capacitor=1uF, AVDD=3.3V)
			codec->write(codec, 0x11, 0xA3); 		// LOPS3=0
			
			/* Mixing ADC Rch and A/P Rch */
			codec->write(codec, 0x15, 0x18); 		// Lch: from SRC-B;
													//Rch: from (SVOLA Rch + SDTI Rch)
			break;
		
		case MM_AUDIO_VOICEMEMO_MAIN :
		case MM_AUDIO_VOICEMEMO_SUB :
			//P("set MM_AUDIO_VOICEMEMO_MAIN");
			mic_set_path(AK4671_MIC_PATH_MAIN);
			codec->write(codec, 0x0B, 0x00); 		// D/A Lch -> Lout2
			codec->write(codec, 0x0C, 0x00); 		// D/A Rch -> Rout2
			codec->write(codec, 0x10, 0x00); 		// D/A Lch -> Lout2


			codec->write(codec, 0x0F, 0x20);	// RCP/RCN mode
			codec->write(codec, 0x11, 0xA0);	// LOP/LON, gain=0dB
			codec->write(codec, 0x0A, 0x20);	// MIC-AMP Rch -> RCP/RCN
			codec->write(codec, 0x0D, 0x20);	// MIC-AMP Lch -> LOP/LON
			codec->write(codec, 0x04, 0x9C);	// MIC-AMP Lch=IN1+/-, IN4+/- Differential Input
			codec->write(codec, 0x00, 0x01); 	// => VCOM power-up
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
			codec->write(codec, 0x00, 0xD5);	// DAC-Rch power-up
			mdelay(2);
			codec->write(codec, 0x0A, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN
			break;

		case MM_AUDIO_VOICEMEMO_EAR :
			//P("set MM_AUDIO_VOICEMEMO_EAR");
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
			//P("set MM_AUDIO_VOICEMEMO_BT");
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


	set_bias(codec, mode);
	return 0;
}

int path_disable(struct snd_soc_codec *codec, int mode)
{
	P("Diasble PATH : 0x%x\n", mode);

	amp_enable(0);//for noise reduce

	switch(mode) {
		case 0:
			P("Path : Off");
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			//P("MM_AUDIO_PLAYBACK_RCV Off");
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
			//P("MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP Off");
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
		case MM_AUDIO_VOICEMEMO_MAIN : // fix by sircid
		case MM_AUDIO_VOICEMEMO_SUB :
			//P("set MM_AUDIO_VOICECALL_RCV Off");
			/* MIC-AMP Rch + A/P Rch => RCP/RCN Off */
			codec->write(codec, 0x19, 0x04);	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0A, 0x20);	// only MIC-AMP-Rch to RCP/RCN
			codec->write(codec, 0x00, 0x0D);	// DAC-Rch power-up
			codec->write(codec, 0x04, 0x00);   // Mic Amp input select default
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
		case MM_AUDIO_VOICECALL_SPK_LOOP:
			//P("set MM_AUDIO_VOICECALL_SPK,HP Off");

			/* MIC-AMP Rch + A/P Rch => Lout2/Rout2 Off */
			#if 0
			codec->write(codec, 0x19, 0x04); 	// use soft mute to shut-down signal
			mdelay(24);
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
			//P("set MM_AUDIO_VOICECALL_BT Off");

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

		case MM_AUDIO_VOICEMEMO_EAR :
			//P("set MM_AUDIO_VOICEMEMO_EAR Off");
			codec->write(codec, 0x00, 0x01);
			codec->write(codec, 0x00, 0x00);
			break;

		case MM_AUDIO_VOICEMEMO_BT :
			//P("set MM_AUDIO_VOICEMEMO_BT Off");
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
	P("Enable Idle Mode : 0x%x\n", mode);

	switch(mode) {
		case 0:
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			//P("set MM_AUDIO_PLAYBACK_RCV");
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
			//P("set MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP");
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			mdelay(1); 		// wait 100ns
			codec->write(codec, 0x00, 0xC1); 	// D/A power-up
			codec->write(codec, 0x10, 0x63); 	// PMLO2,PMRO2,PMLO2S,PMRO2s='1'
			mdelay(1);		// wait 100ns
			codec->write(codec, 0x10, 0x67); 	// MUTEN='1'
			break;

		case MM_AUDIO_PLAYBACK_BT :
			//P("set MM_AUDIO_PLAYBACK_BT");
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
	P("Diasble PATH : 0x%x\n", mode);


	if(!get_headset_status())
		mic_enable(0);

	amp_enable(0);

	switch(mode) {
		case 0:
			P("Path : Off");
			break;

		case MM_AUDIO_PLAYBACK_RCV :
			//P("MM_AUDIO_PLAYBACK_RCV Off");
			codec->write(codec, 0x0F, 0x04); 	// RCP/RCN power-down
			mdelay(2); 	// wait more than 1ms
			codec->write(codec, 0x00, 0x01); 	// VCOM power up
			codec->write(codec, 0x00, 0x00); 	// VCOM power down
			break;

		case MM_AUDIO_PLAYBACK_SPK :
		case MM_AUDIO_PLAYBACK_HP :
		case MM_AUDIO_PLAYBACK_SPK_HP :
		case MM_AUDIO_PLAYBACK_RING_SPK_HP :
			//P("MM_AUDIO_PLAYBACK_SPK, HP, SPK_HP Off");
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

int voice_call_auto_response_enable(struct snd_soc_codec *codec, int mode)
{
	P1("auto response Enable (mode : 0x%x)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("enable Play (MM_AUDIO_VOICECALL_RCV)");
			codec->write(codec, 0x19, 0x01); 	// use soft mute to cut signal
			mdelay(24);
			codec->write(codec, 0x0D, 0x01);	//DAC to LON,LOP 
			codec->write(codec, 0x0E, 0x01);
			codec->write(codec, 0x0A, 0x00);	//RCP/RON DAC Off

			codec->write(codec, 0x00, 0xC1);	//DAC-L/R power up
			
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("enable Play (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Rch mixing A/P Rch -> Lout2/Rout OFF */
			codec->write(codec, 0x19, 0x01); 	// use soft mute to cut signal
			mdelay(24);

			codec->write(codec, 0x0D, 0x01);	//DAC to LON,LOP 
			codec->write(codec, 0x0E, 0x01);
			codec->write(codec, 0x0B, 0x00);	//LOUT2 DAC Off
			codec->write(codec, 0x0C, 0x00);	//OUT2 DAC Off
			
			codec->write(codec, 0x00, 0xC1); 	// DAC-Rch power-up

			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
//			codec->write(codec, 0x00, 0x3D); 	// ADC-Lch power-up
			mdelay(24);
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("enable REC (MM_AUDIO_VOICECALL_BT)");
			/* mixing ADC Rch and A/P Rch -> SRC-A -> PCM-A */
			codec->write(codec, 0x15, 0x24); 	// PFMXL -> SDIM Lch, PFMXR -> SVOLA, SRMXL -> PFMXL + SRC-B, SRMXR -> PFMXR

			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			codec->write(codec, 0x59, 0x10); 	// SDTO-Lch: from SRC-B
			codec->write(codec, 0x53, 0x14);   // Swith-off all output
			break;
		default :
				printk("[%s] Invalid mode\n", __func__);
	}

	codec->write(codec, 0x18, 0x06); 	// IVOLC, ADM Mono

	return 0;
}

int voice_call_auto_response_disable(struct snd_soc_codec *codec, int mode) 
{
	P1("auto response Disable (mode : 0x%x)\n", mode);

	switch(mode) {
		case MM_AUDIO_VOICECALL_RCV :
			P("disable auto reponse (MM_AUDIO_VOICECALL_RCV)");

			codec->write(codec, 0x0A, 0x20);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN		
			codec->write(codec, 0x0D, 0x20);	// MIC-AMP Lch -> LOP/LON			
			codec->write(codec, 0x0E, 0x00);
			codec->write(codec, 0x00, 0x8D);	// DAC-Rch power-up
			mdelay(2);						
			codec->write(codec, 0x0A, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to RCP/RCN			
			break;

		case MM_AUDIO_VOICECALL_SPK :
		case MM_AUDIO_VOICECALL_HP :
			P("disable auto response (MM_AUDIO_VOICECALL_HP_SPK)");
			/* MIC-AMP Lch->LOP/LON and ADC Lch->AP Lch */
			codec->write(codec, 0x0D, 0x20); 	// MIC-AMP Lch -> LOP/LON
			codec->write(codec, 0x0E, 0x00);
			codec->write(codec, 0x00, 0xCD);	// DAC-Rch power-up
			mdelay(2);	
			codec->write(codec, 0x0B, 0x01);	// (MIC-AMP-Rch mixing DAC-Rch) to Lout2
			codec->write(codec, 0x0C, 0x21);	// (MIC-AMP-Rch mixing DAC-Rch) to Rout2
	
			break;

		case MM_AUDIO_VOICECALL_BT :
			P("disable auto response (MM_AUDIO_VOICECALL_BT)");
			codec->write(codec, 0x15, 0x18); 	// 5-band-EQ-Lch: from SRC-B, Rch: from SVOLA Rch
			/* SRC-B -> DAC Lch -> LOP/LON -> and -> A/P Lch */
			codec->write(codec, 0x59, 0x00); 	// default
			codec->write(codec, 0x53, 0x17); 		// PLLBT1,PMSRA/B, PMPCM power up
			break;
		
		default :
			printk("[%s] Invalid mode\n", __func__);
	}

	codec->write(codec, 0x18, 0x02); 	// IVOLC

	return 0;
}
	

