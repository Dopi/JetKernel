/*
 * max9880.c -- MAX9880 ALSA SoC Audio driver
 *
 * Copyright 2009 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Revision history
 *    2010/11/11   vaclavpe@gmail.com - Support of 2.6.29 kernel for Samsung Jet - version 0.12
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
//#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

#include "max9880.h"

#define AUDIO_NAME "max9880"
#define MAX9880_VERSION "0.12" 

#define MAX9880_DEBUG

#ifdef MAX9880_DEBUG
	#define dbg(format, arg...) \
	printk(KERN_DEBUG AUDIO_NAME ": " format "\n" , ## arg)
#else
	#define dbg(format, arg...) do {} while (0)
#endif

#define err(format, arg...) \
	printk(KERN_ERR AUDIO_NAME ": " format "\n" , ## arg)
	
#define info(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
	
#define warn(format, arg...) \
	printk(KERN_WARNING AUDIO_NAME ": " format "\n" , ## arg)

#define MAX9880_TRACE

#ifdef MAX9880_TRACE
#define trace(format, arg...) \
	printk(KERN_INFO AUDIO_NAME ": " format "\n" , ## arg)
#else
	#define trace(format, arg...) do {} while (0)
#endif

/* Keep track of device revision ID */
unsigned char revision_id = 0x00;


/*******************************************************************************
 * Read from MAX9880 register space
 ******************************************************************************/
static unsigned int max9880_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct i2c_msg msg[2];
	struct i2c_client *client;
	u8 data[2];
	int ret;

	client = (struct i2c_client *)codec->control_data;
	data[0] = reg & 0xff;
	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].buf = &data[0];
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = &data[1];
	msg[1].len = 1;

	ret = i2c_transfer(client->adapter, &msg[0], 2);

#if 0
	trace("%s", __FUNCTION__);
	trace("   reg %#.4hx data %#.4hx", reg, data[1]);
#endif

	return (ret == 2) ? data[1] : -EIO;
}


/*******************************************************************************
 * Write to MAX9880 register space
 ******************************************************************************/
static int max9880_write(struct snd_soc_codec *codec, unsigned int reg,	unsigned int value)
{
	u8 data[2];

#if 0
	trace("%s", __FUNCTION__);
	trace("   reg %#.4hx data %#.4hx", reg, value);
#endif

	data[0] = reg & 0x00ff;
	data[1] = value & 0x00ff;

	if (codec->hw_write(codec->control_data, data, 2) == 2)
		return 0;
	else
		return -EIO;
}


/*******************************************************************************
 * Power managment
 ******************************************************************************/
static int max9880_dapm_event(struct snd_soc_codec *codec, int event)
{
	trace("%s %d\n", __FUNCTION__, event);

	switch (event) {
	case SNDRV_CTL_POWER_D0: /* full On */
	case SNDRV_CTL_POWER_D1: /* partial On */
	case SNDRV_CTL_POWER_D2: /* partial On */
#ifdef MAX9880_XTAL
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x88);
#else
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x84);
#endif
		break;

	case SNDRV_CTL_POWER_D3hot: /* Off, with power */
#ifdef MAX9880_XTAL
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x88);
#else
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x84);
#endif
		break;

	case SNDRV_CTL_POWER_D3cold: /* Off, without power */
#ifdef MAX9880_XTAL
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x08);
#else
		max9880_write(codec, MAX9880_PM_SHUTDOWN, 0x00);
#endif
		break;

	}

	codec->dapm_state = event;

	return 0;
}


/*******************************************************************************
 * Get current input mixer setting.
 *
 * Left channel is used to determine current setting.
 ******************************************************************************/
static int max9880_input_mixer_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	trace("%s\n", __FUNCTION__);

	ucontrol->value.integer.value[0] = max9880_read(codec, MAX9880_INPUT_CONFIG) >> 6;

	return 0;
}


/*******************************************************************************
 * Configure input mixer
 *
 * Input mixer mapping
 *  0x00=none
 *  0x01=analog mic
 *  0x02=line in
 *  0x03=mic+line
 *
 * Both channels are in the same register.
 * Left channel is shifted left 6 bits.
 * Right channel is shifted left 4 bits.
 *
 * Left channel is used to determine current setting.
 ******************************************************************************/
static int max9880_input_mixer_set(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 reg = max9880_read(codec, MAX9880_INPUT_CONFIG);

	trace("%s\n", __FUNCTION__);

	if ((reg >> 6) == ucontrol->value.integer.value[0])
	{
		return 0;
	}
	else
	{
		// Clear mux bits
		reg &= 0x0f;

		// VALUE index matches MAX9880_INPUT_MIXER enumeration
		switch (ucontrol->value.integer.value[0])
		{
			case 0: // None
				// Leave mux bits cleared
				break;
			case 1: // Analog Microphone
				reg |= 0x50;
				break;
			case 2: // Line In
				reg |= 0xa0;
				break;
			case 3: // Mic+Line
				reg |= 0xf0;
				break;
		}
	}

	max9880_write(codec, MAX9880_INPUT_CONFIG, reg);

	return 1;
}


static const char* max9880_filter_mode[] = {"IIR", "FIR"};
static const char* max9880_hp_filter[] = {"None", "1", "2", "3", "4", "5"};
static const char* max9880_sidetone_source[] = {"None", "Left ADC", "Right ADC", "L+R ADC"};

static const char *max9880_input_mixer[] = {"None", "Analog Mic", "Line In", "Mic+Line"};
static const struct soc_enum max9880_input_mixer_enum[] = { 
	SOC_ENUM_SINGLE_EXT(4, max9880_input_mixer),
};

static const struct soc_enum max9880_enum[] = {
	// Mask value for SOC_ENUM_SINGLE is the number of elements in the enumeration.
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 7, 2, max9880_filter_mode),
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 4, 5, max9880_hp_filter),
	SOC_ENUM_SINGLE(MAX9880_CODEC_FILTER, 0, 5, max9880_hp_filter),
	SOC_ENUM_SINGLE(MAX9880_SIDETONE, 6, 4, max9880_sidetone_source),
};

static const struct snd_kcontrol_new max9880_snd_controls[] = {
	// CODEC Filter MODE
	SOC_ENUM("Filter Mode", max9880_enum[0]),
	// ADC Filter
	SOC_ENUM("ADC Filter", max9880_enum[1]),
	// DAC Filter
	SOC_ENUM("DAC Filter", max9880_enum[2]),
	// Sidetone Source
	SOC_ENUM("Sidetone Source", max9880_enum[3]),
	// Sidetone level
	SOC_SINGLE("Sidetone Level", MAX9880_SIDETONE, 0, 31, 1),
	// SDACA Attenuation
	SOC_SINGLE("SDACA Attenuation", MAX9880_STEREO_DAC_LVL, 0, 15, 1),
	// VDACA Attenuation
	SOC_SINGLE("VDACA Attenuation", MAX9880_VOICE_DAC_LVL, 0, 15, 1),
	// DAC Gain
	SOC_SINGLE("DAC Gain", MAX9880_VOICE_DAC_LVL, 4, 3, 0),
	// ADC Gain
	SOC_DOUBLE_R("ADC Gain", MAX9880_LEFT_ADC_LVL, MAX9880_RIGHT_ADC_LVL, 4, 3, 0),
	// ADC Level
	SOC_DOUBLE_R("ADC Level", MAX9880_LEFT_ADC_LVL, MAX9880_RIGHT_ADC_LVL, 0, 15, 1),
	// Line Input Gain
	SOC_DOUBLE_R("Line Input Gain", MAX9880_LEFT_LINE_IN_LVL, MAX9880_RIGHT_LINE_IN_LVL, 0, 15, 1),
	// VOLL and VOLR
	SOC_DOUBLE_R("Master Playback Volume", MAX9880_LEFT_VOL_LVL, MAX9880_RIGHT_VOL_LVL, 0, 63, 1),
	// Line Output Gain
	SOC_DOUBLE_R("Line Output Gain", MAX9880_LEFT_LINE_OUT_LVL, MAX9880_RIGHT_LINE_OUT_LVL, 0, 15, 1),
	// Microphone Pre-Amp
	SOC_DOUBLE_R("Microphone Pre-Amp", MAX9880_LEFT_MIC_GAIN, MAX9880_RIGHT_MIC_GAIN, 5, 3, 1),
	// Microphone PGA
	SOC_DOUBLE_R("Microphone PGA", MAX9880_LEFT_MIC_GAIN, MAX9880_RIGHT_MIC_GAIN, 0, 31, 1),
	// Input Mixer
  SOC_ENUM_EXT("Input Mixer", max9880_input_mixer_enum , max9880_input_mixer_get, max9880_input_mixer_set),
};


/*******************************************************************************
 * Add non dapm controls 
 ******************************************************************************/
static int max9880_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	trace("%s\n", __FUNCTION__);

	for (i = 0; i < ARRAY_SIZE(max9880_snd_controls); i++) {
		if ((err = snd_ctl_add(codec->card,
				snd_soc_cnew(&max9880_snd_controls[i],codec, NULL))) < 0)
			return err;
	}

	return 0;
}


static const struct snd_soc_dapm_widget max9880_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("LMICIN"),  // Analog microphone input
	SND_SOC_DAPM_INPUT("RMICIN"),  // Analog microphone input
	SND_SOC_DAPM_INPUT("LLINEIN"), // Line input
	SND_SOC_DAPM_INPUT("RLINEIN"), // Line input

	SND_SOC_DAPM_OUTPUT("LOUTP"), // Speaker output
	SND_SOC_DAPM_OUTPUT("ROUTP"), // Speaker output
	SND_SOC_DAPM_OUTPUT("LOUTL"), // Line output
	SND_SOC_DAPM_OUTPUT("ROUTL"), // Line output

	SND_SOC_DAPM_ADC("LADC", "HiFi Capture", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("RADC", "HiFi Capture", SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_MIXER("Input Mixer", SND_SOC_NOPM, 0, 0, NULL, 0), 

	SND_SOC_DAPM_DAC("LDAC", "HiFi Playback", SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("RDAC", "HiFi Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route intercon[] = {
	/* Inputs */
	{"Mic Input", NULL, "LMICIN"},
	{"Mic Input", NULL, "RMICIN"},
	{"Line Input", NULL, "LLINEIN"},
	{"Line Input", NULL, "RLINEIN"},

	/* Outputs */
	{"LOUTP", NULL, "Output Mixer"}, // Speaker output
	{"ROUTP", NULL, "Output Mixer"}, // Speaker output
	{"LOUTL", NULL, "Output Mixer"}, // Line output
	{"ROUTL", NULL, "Output Mixer"}, // Line output

	/* Input mixer */
	{"Input Mixer", NULL, "Line Input"},
	{"Input Mixer", NULL, "Mic Input"},

	/* ADC */
	{"LDAC", NULL, "Input Mixer"},
	{"RDAC", NULL, "Input Mixer"},

	/* Output mixer */
	{"Output Mixer", NULL, "Line Input"},
	{"Output Mixer", NULL, "LDAC"},
	{"Output Mixer", NULL, "RDAC"},

	/* TERMINATOR */
	{NULL, NULL, NULL},
};


/*******************************************************************************
 * Add control widgets
 ******************************************************************************/
static int max9880_add_widgets(struct snd_soc_codec *codec)
{
	int i;

	trace("%s\n", __FUNCTION__);

	for(i = 0; i < ARRAY_SIZE(max9880_dapm_widgets); i++) {
		snd_soc_dapm_new_control(codec, &max9880_dapm_widgets[i]);
	}

	/* Setup audio path interconnects */
	snd_soc_dapm_add_routes(codec, intercon, ARRAY_SIZE(intercon));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}


#if 0
/*******************************************************************************
 * Not required - left in place for later reference
 ******************************************************************************/
static int max9880_pcm_prepare(struct snd_pcm_substream *substream)
{
	trace("%s %d\n", __FUNCTION__, substream->stream);

	return 0;
}


/*******************************************************************************
 * Not required - left in place for later reference
 ******************************************************************************/
static void max9880_shutdown(struct snd_pcm_substream *substream)
{
	trace("%s", __FUNCTION__);
}
#endif


/*******************************************************************************
 * Volume control mute
 ******************************************************************************/
static int max9880_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	u8 muteL = max9880_read(codec, MAX9880_LEFT_VOL_LVL) & 0x3f;
	u8 muteR = max9880_read(codec, MAX9880_RIGHT_VOL_LVL) & 0x3f;

	trace("%s %s\n", __FUNCTION__, (mute) ? "muted" : "unmuted");

	if (mute)
	{
		muteL |= 0x40;
		muteR |= 0x40;
	}

	max9880_write(codec, MAX9880_LEFT_VOL_LVL, muteL);
	max9880_write(codec, MAX9880_RIGHT_VOL_LVL, muteR);

	return 0;
}


#define MAX9880_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
                       SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
                       SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
                       SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
                       SNDRV_PCM_RATE_96000)

#define MAX9880_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)

struct snd_soc_dai max9880_dai = {
	.name = "MAX9880",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = MAX9880_RATES,
		.formats = MAX9880_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = MAX9880_RATES,
		.formats = MAX9880_FORMATS,},
	.ops = {
#if 0
		.prepare = max9880_pcm_prepare,
		.shutdown = max9880_shutdown,
#endif
		.digital_mute = max9880_mute,
	}
};
EXPORT_SYMBOL_GPL(max9880_dai);


/*******************************************************************************
 * Initialise device
 *
 * Register mixer and dsp interfaces with kernel
 ******************************************************************************/
static int max9880_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;

	trace("%s", __FUNCTION__);

	codec->name = "MAX9880";
	codec->owner = THIS_MODULE;
	codec->read = max9880_read;
	codec->write = max9880_write;
	//codec->dapm_event = max9880_dapm_event;
	codec->dai = &max9880_dai;
	codec->num_dai = 1;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "max9880: failed to create pcms\n");
		goto pcm_err;
	}

	/* Grab device revision ID */
	revision_id = max9880_read(codec, MAX9880_REVISION_ID);

	// Disable chip using DAPM event
	max9880_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

  // Enabling input and output
	max9880_write(codec, MAX9880_DAI1_MODE_A,
						(0 << 7) | // MAX
						(0 << 6) | // MCI
						(0 << 5) | // BCI
						(1 << 4) | // DLY - Opposite of Littleton platform?
						(0 << 3) | // HIZOFF1
						(0 << 2) | // TDM
						(0 << 1) | // FSW
						(0 << 0)); // reserved

	max9880_write(codec, MAX9880_DAI1_MODE_B, (revision_id == 0x41) ?
						(0 << 6) | // SEL
						(1 << 5) | // SDOEN
						(1 << 4) | // SDIEN
						(0 << 3) | // DMONO1
						(0 << 0)   // BSEL
						:
						(1 << 6) | // SEL
						(1 << 5) | // SDOEN
						(1 << 4) | // SDIEN
						(0 << 3) | // DMONO1
						(0 << 0)); // BSEL

	max9880_write(codec, MAX9880_DAI1_TDM,
						(0 << 6) | // STOTL
						(1 << 4) | // STOTR
						(0 << 0)); // STOTDLY

	// Select DAI 1 as source for DAC
	max9880_write(codec, MAX9880_DAC_MIXER,
						(1 << 7) | // DAI1 Left
						(1 << 2)); // DAI1 Right

#ifdef JACK_DETECT
	max9880_write(codec, MAX9880_INT_EN, 0x02);
#else
	max9880_write(codec, MAX9880_INT_EN, 0x00);
#endif

	// Setup clock for slave mode using anyclock feature.
	// Both the max9880 evkit and littleton platform provide a 13MHz clock
	max9880_write(codec, MAX9880_SYS_CLK, 0x10);

	// DAI1 uses PLL instead of NI divider
	max9880_write(codec, MAX9880_DAI1_CLK_CTL_HIGH, 0x80);
	max9880_write(codec, MAX9880_DAI1_CLK_CTL_LOW, 0x00);

	// voice/audio filter
	max9880_write(codec, MAX9880_CODEC_FILTER, 0x80);

	// DSD not used
	max9880_write(codec, MAX9880_DSD_CONFIG, 0x00);
	max9880_write(codec, MAX9880_DSD_INPUT, 0x00);

	// side tone off
	max9880_write(codec, MAX9880_SIDETONE, 0x00);

	// DAC levels
	max9880_write(codec, MAX9880_STEREO_DAC_LVL, 0x0f);
	max9880_write(codec, MAX9880_VOICE_DAC_LVL, 0x0f);

	// line in
	max9880_write(codec, MAX9880_LEFT_LINE_IN_LVL, 0x00);
	max9880_write(codec, MAX9880_RIGHT_LINE_IN_LVL, 0x00);

	// DAC volume
	max9880_write(codec, MAX9880_LEFT_VOL_LVL, 0x0d);
	max9880_write(codec, MAX9880_RIGHT_VOL_LVL, 0x0d);

	// line out
	max9880_write(codec, MAX9880_LEFT_LINE_OUT_LVL, 0x0f);
	max9880_write(codec, MAX9880_RIGHT_LINE_OUT_LVL, 0x0f);

	// ADC gain
	max9880_write(codec, MAX9880_LEFT_ADC_LVL, 0x03);
	max9880_write(codec, MAX9880_RIGHT_ADC_LVL, 0x03);

	// ADC mixer configuration
	max9880_write(codec, MAX9880_INPUT_CONFIG,
		(2 << 6) | // MXINL : 0=mute, 1=mic, 2=line, 3=mic+line
		(2 << 4) | // MXINR : 0=mute, 1=mic, 2=line, 3=mic+line
		(0 << 3) | // AUXCAP
		(0 << 2) | // AUXGAIN : 0=normal, 1=gain calibration mode
		(0 << 1) | // AUXCAL : 0=normal, 1=offset calibration mode
		(0 << 0)); // AUXEN : 0=JACKSNS/AUXIN

	// mode
	max9880_write(codec, MAX9880_MODE_CONFIG,
		(0 << 7) | // DSLEW : 0=10ms, 1=80ms
		(0 << 6) | // VSEN : 0=bypass intermediate, 1=step through
		(0 << 5) | // ZDEN : 0=change at zero cross, 1=change immediate
		(1 << 4) | // DZDEN : 0=change immediate, 1=near zero cross
		(4 << 0)); // SPMODE

#ifdef JACK_DETECT
	max9880_write(codec, MAX9880_JACK_DETECT, 0x80);
#else
	max9880_write(codec, MAX9880_JACK_DETECT, 0x00);
#endif

	// Enable circuitry based on application
	max9880_write(codec, MAX9880_PM_ENABLE,
		(1 << 7) | // LNLEN : Left-Line Input Enable
		(1 << 6) | // LNREN : Right-Line Input Enable
		(1 << 5) | // LOLEN : Left-Line Output Enable
		(1 << 4) | // LOREN : Right-Line Output Enable
		(1 << 3) | // DALEN : Left DAC Enable
		(1 << 2) | // DAREN : Right DAC Enable
		(1 << 1) | // ADLEN : Left ADC Enable
		(1 << 0)); // ADREN : Right ADC Enable

	// enable chip using DAPM event
	max9880_dapm_event(codec, SNDRV_CTL_POWER_D0);

	max9880_add_widgets(codec);
	max9880_add_controls(codec);

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "max9880: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:

	return ret;
}


static struct snd_soc_device *max9880_socdev;


#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)

static unsigned short ignore_i2c_addr[] = { I2C_CLIENT_END };
static unsigned short normal_i2c_addr[] = {5, I2C_CLIENT_END};
static unsigned short probe_i2c_addr[] = { I2C_CLIENT_END };

static struct i2c_client_address_data max9880_addr_data = {
   .normal_i2c     = normal_i2c_addr,
   .probe          = probe_i2c_addr,
   .ignore         = ignore_i2c_addr,
};

/* Required I2C client boiler plate */
/* I2C_CLIENT_INSMOD; */

static struct i2c_driver max9880_i2c_driver;
static struct i2c_client client_template;

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */

/*******************************************************************************
 * Probe for an attched MAX9880 and initialize it if found.
 ******************************************************************************/
static int max9880_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = max9880_socdev;
	struct max9880_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;

	printk("%s - max9880 found\n", __FUNCTION__);

	if (addr != setup->i2c_address)
		return -ENODEV;

	client_template.adapter = adap;
	client_template.addr = addr;

	i2c = kmemdup(&client_template, sizeof(client_template), GFP_KERNEL);
	if (i2c == NULL) {
		kfree(codec);
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		err("%s: failed to attach codec at addr %x\n", __FUNCTION__, addr);
		goto err;
	}

	ret = max9880_init(socdev);
	if (ret < 0) {
		err("%s: failed to initialise MAX9880\n", __FUNCTION__);
		goto err;
	}
	return ret;

err:
	kfree(codec);
	kfree(i2c);
	return ret;
}


/*******************************************************************************
 * Release any resources allocated during attach
 ******************************************************************************/
static int max9880_i2c_detach(struct i2c_client *client)
{
	struct snd_soc_codec* codec = i2c_get_clientdata(client);

	i2c_detach_client(client);
	kfree(codec->reg_cache);
	kfree(client);
	return 0;
}


/*******************************************************************************
 * Probe for an attached MAX9880
 ******************************************************************************/
static int max9880_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &max9880_addr_data, max9880_codec_probe);
}


/*******************************************************************************
 * I2C codec control layer
 ******************************************************************************/
static struct i2c_driver max9880_i2c_driver = {
	.driver = {
		.name = "MAX9880 Codec",
		.owner = THIS_MODULE,
	},
	.attach_adapter = max9880_i2c_attach,
	.detach_client =  max9880_i2c_detach,
};

static struct i2c_client client_template = {
	.name =   "MAX9880",
	.driver = &max9880_i2c_driver,
};
#endif


/*******************************************************************************
 * Power down codec.
 *
 * Current register values would need to be saved is power is diconnected.
 ******************************************************************************/
static int max9880_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	trace("%s\n", __FUNCTION__);

	/* power codec down */
	max9880_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	return 0;
}


/*******************************************************************************
 * Power up codec.
 *
 * Register values need to be restored if power was actully diconnected
 * from it's supply.
 ******************************************************************************/
static int max9880_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	trace("%s\n", __FUNCTION__);

	max9880_dapm_event(codec, SNDRV_CTL_POWER_D3hot);
	max9880_dapm_event(codec, codec->suspend_dapm_state);
	return 0;	
}


/*******************************************************************************
 * Make sure that a MAX9880 is attached to I2C bus.
 ******************************************************************************/
static int max9880_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct max9880_setup_data *setup;
	struct snd_soc_codec *codec;
	int ret = 0;

	printk("Maxim MAX9880 Audio CODEC %s\n", MAX9880_VERSION);

	setup = socdev->codec_data;

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	max9880_socdev = socdev;
#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
	  printk("%s assign i2c address 0x%02X", __FUNCTION__, setup->i2c_address);
		normal_i2c_addr[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t)i2c_master_send;
		codec->hw_read = (hw_read_t)i2c_master_recv;
		ret = i2c_add_driver(&max9880_i2c_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add i2c driver");
	}
#else
	/* Add other interfaces here */
	  printk("%s NO I2C ??? Something goes bad...\n", __FUNCTION__);
#endif

	return ret;
}


/*******************************************************************************
 * Driver is being unloaded, power down codec and free allocated resources.
 ******************************************************************************/
static int max9880_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	trace("%s", __FUNCTION__);

	if (codec->control_data)
		max9880_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

#if defined (CONFIG_I2C) || defined (CONFIG_I2C_MODULE)
	i2c_del_driver(&max9880_i2c_driver);
#endif

	kfree(codec);

	return 0;
}


struct snd_soc_codec_device soc_codec_dev_max9880 = {
	.probe = max9880_probe,
	.remove = max9880_remove,
	.suspend = max9880_suspend,
	.resume = max9880_resume,
};

EXPORT_SYMBOL_GPL(soc_codec_dev_max9880);

static int __init max9880_codec_init(void)
{
	return snd_soc_register_dai(&max9880_dai);	
}
module_init(max9880_codec_init);

static void __exit max9880_codec_exit(void)
{
	snd_soc_unregister_dai(&max9880_dai);
}
module_exit(max9880_codec_exit);

MODULE_DESCRIPTION("ASoC MAX9880 driver");
MODULE_AUTHOR("Maxim Integrated Products");
MODULE_LICENSE("GPL");
