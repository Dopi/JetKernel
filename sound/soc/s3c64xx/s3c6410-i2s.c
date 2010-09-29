/*
 * s3c6410-i2s.c  --  ALSA Soc Audio Layer
 *
 * (c) 2009 Samsung Electronics   - Jaswinder Singh Brar <jassi.brar@samsung.com>
 *  Derived from Ben Dooks' driver for s3c24xx
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/map.h>

#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#ifdef CONFIG_S3C_DMA_PL080_SOL
#include <plat/s3c6410-dma.h>
#endif
#include "s3c-pcm.h"
#include "s3c6410-i2s.h"

//#define CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

#ifdef CONFIG_S3C_DMA_PL080	
static struct s3c2410_dma_client s3c_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct s3c24xx_pcm_dma_params s3c_i2s_pcm_stereo_out = {
	.client		= &s3c_dma_client_out,
	.channel	= S3C_DMACH_I2S_OUT,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISTXD,
	.dma_size	= 4,
};

static struct s3c24xx_pcm_dma_params s3c_i2s_pcm_stereo_in = {
	.client		= &s3c_dma_client_in,
	.channel	= S3C_DMACH_I2S_IN,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISRXD,
	.dma_size	= 4,
};
#endif

struct s3c_i2s_info {
	void __iomem	*regs;
	struct clk	*iis_clk;
	struct clk	*audio_bus;
	u32		iiscon;
	u32		iismod;
	u32		iisfic;
	u32		iispsr;
	u32		slave;
	u32		clk_rate;
};
static struct s3c_i2s_info s3c_i2s;

/*
static void dump_i2s(void)
{
	printk("S3C_IISCON : %x\n", readl(s3c_i2s.regs + S3C_IISCON));
	printk("S3C_IISMOD : %x\n", readl(s3c_i2s.regs + S3C_IISMOD));
	printk("S3C_IISFIC : %x\n", readl(s3c_i2s.regs + S3C_IISFIC));
	printk("S3C_IISPSR : %x\n", readl(s3c_i2s.regs + S3C_IISPSR));
}
*/

static void s3c_snd_txctrl(int on)
{
	u32 iiscon;

	debug_msg("%s\n", __FUNCTION__);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if (on) {
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_TXCHPAUSE;
		iiscon  &= ~S3C_IISCON_TXDMAPAUSE;
		iiscon  |= S3C_IISCON_TXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	} else {
	
		//if recording is running, do not disable IIS clock
		if ( (iiscon & S3C_IISCON_RXDMACTIVE) != S3C_IISCON_RXDMACTIVE)
			{
		iiscon &= ~S3C_IISCON_I2SACTIVE;
			debug_msg("I2S off\n");
			}
		iiscon  |= S3C_IISCON_TXCHPAUSE;
		iiscon  |= S3C_IISCON_TXDMAPAUSE;
		iiscon  &= ~S3C_IISCON_TXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}
}

static void s3c_snd_rxctrl(int on)
{
	u32 iiscon;

	debug_msg("%s\n", __FUNCTION__);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_RXCHPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMAPAUSE;
		iiscon  |= S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}else{
	
		//if playback is running, do not disable IIS clock
		if ( (iiscon & S3C_IISCON_TXDMACTIVE)  != S3C_IISCON_TXDMACTIVE)
			{
			iiscon &= ~S3C_IISCON_I2SACTIVE;
			debug_msg("I2S off\n");
			}
		
		iiscon  |= S3C_IISCON_RXCHPAUSE;
		iiscon  |= S3C_IISCON_RXDMAPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}

}

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s3c_snd_lrsync(void)
{
	u32 iiscon;
	int timeout = 50; /* 5ms */

	debug_msg("%s\n", __FUNCTION__);

	while (1) {
		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if (iiscon & S3C_IISCON_LRI)
			break;

		if (!timeout--)
			return -ETIMEDOUT;
		udelay(100);
	}

	return 0;
}

/*
 * Set s3c_ I2S DAI format
 */
static int s3c_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 iismod;

	debug_msg("%s\n", __FUNCTION__);

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~S3C_IISMOD_SDFMASK;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		//printk("AP Slave Mode\n");
		s3c_i2s.slave = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		//printk("AP Master Mode\n");
		s3c_i2s.slave = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iismod &= ~S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iismod |= S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iismod |= S3C_IISMOD_LSB;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iismod &= ~S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iismod |= S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	default:
		printk("Inv-combo(%d) not supported!\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

static int s3c_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 iismod;
	
	debug_msg("%s\n", __FUNCTION__);

#ifdef CONFIG_S3C_DMA_PL080_SOL
	rtd->dai->cpu_dai->dma_data = s3c_i2s_dai.dma_data;
#endif

#ifdef CONFIG_S3C_DMA_PL080	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_out;
	else
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_in;
#endif
	/* Working copies of register */
	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~S3C_IISMOD_BLCMASK;

#ifdef CONFIG_S3C_DMA_PL080	
	/* TODO */
	switch(params_channels(params)) {
	case 1:
		s3c_i2s_pcm_stereo_in.dma_size = 2;
		break;
	case 2:
		s3c_i2s_pcm_stereo_in.dma_size = 4;
		break;
	case 4:
		break;
	case 6:
		break;
	default:
		break;
	}
#endif

	/* RFS & BFS are set by dai_link(machine specific) code via set_clkdiv */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C_IISMOD_8BIT;
 		break;
	case SNDRV_PCM_FORMAT_S16_LE:
 		iismod |= S3C_IISMOD_16BIT;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
 		iismod |= S3C_IISMOD_24BIT;
		break;
	default:
		return -EINVAL;
	}


	writel(iismod, s3c_i2s.regs + S3C_IISMOD);


	debug_msg("s3c iis mode: 0x%08x\n", readl(s3c6410_i2s.regs + S3C64XX_IIS0MOD));
	debug_msg("s3c: params_channels %d\n", params_channels(params));
	debug_msg("s3c: params_format %d\n", params_format(params));
	debug_msg("s3c: params_subformat %d\n", params_subformat(params));
	debug_msg("s3c: params_period_size %d\n", params_period_size(params));
	debug_msg("s3c: params_period_bytes %d\n", params_period_bytes(params));
	debug_msg("s3c: params_periods %d\n", params_periods(params));
	debug_msg("s3c: params_buffer_size %d\n", params_buffer_size(params));
	debug_msg("s3c: params_buffer_bytes %d\n", params_buffer_bytes(params));
	debug_msg("hw_params: IISMOD: %x\n", iismod);

	return 0;
}

static int s3c_i2s_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	u32 iiscon, iisfic;
	int timeout;

	debug_msg("%s\n", __FUNCTION__);


	/*Empty Tx FIFO*/
	timeout = 50;
	if (substream->stream  == SNDRV_PCM_STREAM_PLAYBACK){
		iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
		iisfic |= S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC);

	do{
	   iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		   if (!timeout--)
		   	{
		   	printk("Warning: Tx FIFO is not empty\n");
		   	return 0;
		   	}
	}while((iiscon & S3C_IISCON_FTX0EMPT) != (S3C_IISCON_FTX0EMPT));
	}


	/*Empty Rx FIFO*/
	timeout = 50;
	if (substream->stream  == SNDRV_PCM_STREAM_CAPTURE){
		iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
		iisfic |= S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC);

	do{
	   iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		   if (!timeout--)
		   	{
		   	printk("Warning: Rx FIFO is not empty\n");
		   	return 0;
		   	}
	}while((iiscon & S3C_IISCON_FRXEMPT) != (S3C_IISCON_FRXEMPT));
	}
	iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
	iisfic &= ~(S3C_IISFIC_TFLUSH | S3C_IISFIC_RFLUSH);
	writel(iisfic, s3c_i2s.regs + S3C_IISFIC);

	return 0;
}

static int s3c_i2s_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	u32 iismod;
	
	debug_msg("%s\n", __FUNCTION__);

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_RX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}else{
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_TX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);


	return 0;
}


static int s3c_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *dai)
{
	int ret = 0;

	debug_msg("%s, stream:%d \n", __FUNCTION__, substream->stream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (s3c_i2s.slave) {
			ret = s3c_snd_lrsync();
			if (ret)
				goto exit_err;
		}

		debug_msg("Start\n");
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(1);
		else
			s3c_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		debug_msg("Stop\n");
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s3c_snd_rxctrl(0);
		else
			s3c_snd_txctrl(0);
		break;
	default:
		ret = -EINVAL;
		break;
	}


exit_err:
	return ret;
}

/*
 * Set s3c_ Clock source
 * Since, we set frequencies using PreScaler and BFS, RFS, we select input clock source to the IIS here.
 */
static int s3c_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
#if USE_CLKAUDIO
	struct clk *clk;
#endif
	u32 iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	debug_msg("%s\n", __FUNCTION__);

	switch (clk_id) {
	case S3C_CLKSRC_PCLK:
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);
		break;

#if USE_CLKAUDIO
	case S3C_CLKSRC_CLKAUDIO:
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
/*
8000 x 256 = 2048000
         49152000 mod 2048000 = 0
         32768000 mod 2048000 = 0 
         73728000 mod 2048000 = 0

11025 x 256 = 2822400
         67738000 mod 2822400 = 400

16000 x 256 = 4096000
         49152000 mod 4096000 = 0
         32768000 mod 4096000 = 0 
         73728000 mod 4096000 = 0

22050 x 256 = 5644800
         67738000 mod 5644800 = 400

32000 x 256 = 8192000
         49152000 mod 8192000 = 0
         32768000 mod 8192000 = 0
         73728000 mod 8192000 = 0

44100 x 256 = 11289600
         67738000 mod 11289600 = 400

48000 x 256 = 12288000
         49152000 mod 12288000 = 0
         73728000 mod 12288000 = 0

64000 x 256 = 16384000
         49152000 mod 16384000 = 0
         32768000 mod 16384000 = 0

88200 x 256 = 22579200
         67738000 mod 22579200 = 400

96000 x 256 = 24576000
         49152000 mod 24576000 = 0
         73728000 mod 24576000 = 0

	From the table above, we find that 49152000 gives least(0) residue 
	for most sample rates, followed by 67738000.
*/
		clk = clk_get(NULL, "fout_epll");
		if (IS_ERR(clk)) {
			printk("failed to get FOUTepll\n");
			return -EBUSY;
		}
		clk_disable(clk);
		switch (freq) {
		case 8000:
#ifdef CONFIG_SND_S3C64XX_SOC_I2S_REC_DOWNSAMPLING // sangsu fix
			clk_set_rate(clk, 67738000);
			break;
#endif // sangsu fix
		case 16000:
		case 32000:
		case 48000:
		case 64000:
		case 96000:
			clk_set_rate(clk, 49152000);
			break;
		case 11025:
		case 22050:
		case 44100:
		case 88200:
		default:
			clk_set_rate(clk, 67738000);
			break;
		}
		clk_enable(clk);
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
		//printk("Setting FOUTepll to %dHz", s3c_i2s.clk_rate);
		clk_put(clk);
		break;
#endif

	case S3C_CLKSRC_SLVPCLK:
	case S3C_CLKSRC_I2SEXT:
#if USE_AP_MASTER
		if(!s3c_i2s.slave)
			{
			printk("[warning]slave is not set\n");
			return -EINVAL;
			}
#endif		
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		break;

	/* Not sure about these two! */
	case S3C_CDCLKSRC_INT:
		iismod &= ~S3C_IISMOD_CDCLKCON;
		break;

	case S3C_CDCLKSRC_EXT:
		iismod |= S3C_IISMOD_CDCLKCON;
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

/*
 * Set s3c_ Clock dividers
 * NOTE: NOT all combinations of RFS, BFS and BCL are supported! XXX
 * Machine specific(dai-link) code must consider that while setting MCLK and BCLK in this function. XXX
 */
/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS_VAL(must be a multiple of BFS)                                 XXX */
/* XXX RFS_VAL & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int s3c_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	u32 reg;

	debug_msg("%s\n", __FUNCTION__);

	switch (div_id) {
	case S3C_DIV_MCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_RFSMASK;
		switch(div) {
		case 256: div = S3C_IISMOD_256FS; break;
		case 512: div = S3C_IISMOD_512FS; break;
		case 384: div = S3C_IISMOD_384FS; break;
		case 768: div = S3C_IISMOD_768FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_BCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_BFSMASK;
		switch(div) {
		case 16: div = S3C_IISMOD_16FS; break;
		case 24: div = S3C_IISMOD_24FS; break;
		case 32: div = S3C_IISMOD_32FS; break;
		case 48: div = S3C_IISMOD_48FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_PRESCALER:
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSRAEN;
		writel(reg, s3c_i2s.regs + S3C_IISPSR);
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSVALA;
		div &= 0x3f;
		writel(reg | (div<<8) | S3C_IISPSR_PSRAEN, s3c_i2s.regs + S3C_IISPSR);
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

/*
 * To avoid duplicating clock code, allow machine driver to
 * get the clockrate from here.
 */
u32 s3c_i2s_get_clockrate(void)
{
	debug_msg("%s\n", __FUNCTION__);

	return s3c_i2s.clk_rate;
}
EXPORT_SYMBOL_GPL(s3c_i2s_get_clockrate);

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon;
	
	debug_msg("%s\n", __FUNCTION__);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	if(S3C_IISCON_FTXURSTATUS & iiscon) {
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		printk("underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}

	return IRQ_HANDLED;
}

static int s3c_i2s_probe(struct platform_device *pdev,
	struct snd_soc_dai *dai)
{
	int ret = 0;
#if USE_CLKAUDIO
	struct clk *cm, *cf;
#endif

	debug_msg("%s\n", __FUNCTION__);

	s3c_i2s.regs = ioremap(S3C_IIS_PABASE, 0x100);

	/* Configure the I2S pins in correct mode */
	s3c_gpio_cfgpin(GPIO_I2S_LRCLK, S3C_GPIO_SFN(GPIO_I2S_LRCLK_AF)); 
	s3c_gpio_cfgpin(GPIO_I2S_CLK, S3C_GPIO_SFN(GPIO_I2S_CLK_AF)); 
	s3c_gpio_cfgpin(GPIO_I2S_DI, S3C_GPIO_SFN(GPIO_I2S_DI_AF));   
	s3c_gpio_cfgpin(GPIO_I2S_DO, S3C_GPIO_SFN(GPIO_I2S_DO_AF));

    /* pull-up-enable, pull-down-disable*/
	s3c_gpio_setpull(GPIO_I2S_CLK, S3C_GPIO_PULL_UP); 
	s3c_gpio_setpull(GPIO_I2S_LRCLK, S3C_GPIO_PULL_UP); 
	s3c_gpio_setpull(GPIO_I2S_DI, S3C_GPIO_PULL_UP); 
	s3c_gpio_setpull(GPIO_I2S_DO, S3C_GPIO_PULL_UP); 

	if (s3c_i2s.regs == NULL)
		return -ENXIO;

	ret = request_irq(S3C_IISIRQ, s3c_iis_irq, 0, "s3c-i2s", pdev);
	if (ret < 0) {
		printk("fail to claim i2s irq , ret = %d\n", ret);
		iounmap(s3c_i2s.regs);
		return -ENODEV;
	}

	s3c_i2s.iis_clk = clk_get(&pdev->dev, PCLKCLK);
	if (IS_ERR(s3c_i2s.iis_clk)) {
		printk("failed to get clk(%s)\n", PCLKCLK);
		goto lb5;
	}
	clk_enable(s3c_i2s.iis_clk);
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);

#if USE_CLKAUDIO
	s3c_i2s.audio_bus = clk_get(NULL, EXTCLK);
	if (IS_ERR(s3c_i2s.audio_bus)) {
		printk("failed to get clk(%s)\n", EXTCLK);
		goto lb4;
	}

	cm = clk_get(NULL, "mout_epll");
	if (IS_ERR(cm)) {
		printk("failed to get mout_epll\n");
		goto lb3;
	}
	if(clk_set_parent(s3c_i2s.audio_bus, cm)){
		printk("failed to set MOUTepll as parent of CLKAUDIO0\n");
		goto lb2;
	}

	cf = clk_get(NULL, "fout_epll");
	if (IS_ERR(cf)) {
		printk("failed to get fout_epll\n");
		goto lb2;
	}
	clk_enable(cf);
	if(clk_set_parent(cm, cf)){
		printk("failed to set FOUTepll as parent of MOUTepll\n");
		goto lb1;
	}
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
	clk_put(cf);
	clk_put(cm);
#endif

#if defined(CONFIG_SND_S3C_I2S_V50)
	writel(readl(s3c_i2s.regs + S3C_IISCON) | S3C_IISCON_SWRESET, s3c_i2s.regs + S3C_IISCON);
#else
	writel(S3C_IISCON_I2SACTIVE, s3c_i2s.regs + S3C_IISCON);
#endif


	s3c_snd_txctrl(0);
	s3c_snd_rxctrl(0);


	return 0;

#if USE_CLKAUDIO
lb1:
	clk_put(cf);
lb2:
	clk_put(cm);
lb3:
	clk_put(s3c_i2s.audio_bus);
lb4:
	clk_disable(s3c_i2s.iis_clk);
	clk_put(s3c_i2s.iis_clk);
#endif
lb5:
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);
	
	return -ENODEV;
}

#ifdef CONFIG_PM
static int s3c_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	debug_msg("%s\n", __FUNCTION__);

	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
	s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
	s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);

	clk_disable(s3c_i2s.iis_clk);
	debug_msg("%s-done\n", __FUNCTION__);

	return 0;
}

static int s3c_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	debug_msg("%s\n", __FUNCTION__);

	clk_enable(s3c_i2s.iis_clk);

	writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
	writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC);
	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);

	debug_msg("%s-done\n", __FUNCTION__);

	return 0;
}
#else
#define s3c_i2s_suspend NULL
#define s3c_i2s_resume NULL
#endif

#if defined(CONFIG_S3C_DMA_PL080_SOL)
/* Platform data structure for using DMAC - by jung */
static struct dmac_info i2s0_info[] = {
	{
		.modalias	= "i2s0_tx",
		.dma_port	= 0,
		.conn_num	= I2S0_TX,
		.hw_fifo	= S3C_IIS_PABASE + S3C_IISTXD,
	},

	{
		.modalias	= "i2s0_rx",
		.dma_port	= 0,
		.conn_num	= I2S0_RX,
		.hw_fifo	= S3C_IIS_PABASE + S3C_IISRXD,		
	}
};

struct dmac_conn_info dmac_i2s0_info = {
	.connection_num  = i2s0_info,
	.array_size = ARRAY_SIZE(i2s0_info),
};
#endif

struct snd_soc_dai s3c_i2s_dai = {
	.name = "s3c-i2s",
	.id = 0,
	.probe = s3c_i2s_probe,
	.suspend = s3c_i2s_suspend,
	.resume = s3c_i2s_resume,
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = {
		.hw_params = s3c_i2s_hw_params,
		.prepare   = s3c_i2s_prepare,
		.startup   = s3c_i2s_startup,
		.trigger   = s3c_i2s_trigger,
		.set_fmt = s3c_i2s_set_fmt,
		.set_clkdiv = s3c_i2s_set_clkdiv,
		.set_sysclk = s3c_i2s_set_sysclk,
	},

#ifdef CONFIG_S3C_DMA_PL080_SOL
	.dma_data = &dmac_i2s0_info,
#endif
};
EXPORT_SYMBOL_GPL(s3c_i2s_dai);

static int __init s3c_i2s_init(void)
{
	return snd_soc_register_dai(&s3c_i2s_dai);
}
module_init(s3c_i2s_init);

static void __exit s3c_i2s_exit(void)
{
	snd_soc_unregister_dai(&s3c_i2s_dai);
}
module_exit(s3c_i2s_exit);

/* Module information */
MODULE_AUTHOR("Jaswinder Singh <jassi.brar@samsung.com>");
MODULE_DESCRIPTION(S3C_DESC);
MODULE_LICENSE("GPL");
