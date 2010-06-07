/*
 * s3c-pcm.c  --  ALSA Soc Audio Layer
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *	Ryu Euiyoul <ryu.real@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    11th Dec 2006   Merged with Simtec driver
 *    10th Nov 2006   Initial version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

//#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/dma.h>
#include <mach/audio.h>

#include "s3c-pcm.h"

#if defined CONFIG_SND_S3C6400_SOC_AC97
#define MAIN_DMA_CH 1
#else /*S3C6400 I2S */ 
#define MAIN_DMA_CH 0
#endif

#define ANDROID_BUF_SIZE	4096

//#define CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define s3cdbg(x...) printk(x)
#else
#define s3cdbg(x...)
#endif

static const struct snd_pcm_hardware s3c24xx_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				    SNDRV_PCM_INFO_BLOCK_TRANSFER |
				    SNDRV_PCM_INFO_MMAP |
				    SNDRV_PCM_INFO_MMAP_VALID,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |
				    SNDRV_PCM_FMTBIT_U16_LE |
				    SNDRV_PCM_FMTBIT_U8 |
				    SNDRV_PCM_FMTBIT_S24_LE |
				    SNDRV_PCM_FMTBIT_S8,
	.channels_min		= 2,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= 128,
	.period_bytes_max	= 16*1024,
	.periods_min		= 2,
	.periods_max		= 128,
	.fifo_size		= 32,
};

struct s3c24xx_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct s3c24xx_pcm_dma_params *params;
};

extern unsigned int ring_buf_index;
extern unsigned int period_index;

/* s3c24xx_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
*/
static void s3c24xx_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	unsigned long len = prtd->dma_period;
	dma_addr_t    pos = prtd->dma_pos;
	int ret;
	unsigned long next_len = 0;
#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
	/* Next length prediction */
	dma_addr_t pred_pos;
#endif
	
	s3cdbg("Entered %s\n", __FUNCTION__);

	if ((pos + len) > prtd->dma_end) {
		len  = prtd->dma_end - pos;
		s3cdbg(KERN_DEBUG "%s: corrected dma len %ld\n", __FUNCTION__, len);
	}
#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
    /* DMA with I2S might be unstable when length is too short. */
    pred_pos = pos + prtd->dma_period;

    next_len = prtd->dma_period;

    if ((pred_pos + next_len) > prtd->dma_end) {
        next_len  = prtd->dma_end - pred_pos;
    }

    if (next_len <= 32) { /* next transfer is too short */
        len += next_len; /* transfer with next small period */
        ret = s3c2410_dma_enqueue(prtd->params->channel, 
                           substream, pos, len);
        pos += next_len;
    }
    else
        ret = s3c2410_dma_enqueue(prtd->params->channel, 
			        substream, pos, len);
#else
		ret = s3c2410_dma_enqueue(prtd->params->channel, 
			substream, pos, len);
#endif

	prtd->dma_pos = pos;
}

static void s3c24xx_audio_buffdone(struct s3c2410_dma_chan *channel,
				void *dev_id, int size,
				enum s3c2410_dma_buffresult result)
{
	struct snd_pcm_substream *substream = dev_id;
	struct s3c24xx_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (result == S3C2410_RES_ABORT || result == S3C2410_RES_ERR){
		return;
	}
	else {
		
		if (!substream)
			return;

		prtd = substream->runtime->private_data;
		
		prtd->dma_pos += prtd->dma_period;
		if (prtd->dma_pos >= prtd->dma_end)
			prtd->dma_pos = prtd->dma_start;

		snd_pcm_period_elapsed(substream);

		spin_lock(&prtd->lock);

		if (prtd->state & ST_RUNNING) 
			s3c24xx_pcm_enqueue(substream);

		spin_unlock(&prtd->lock);
	}
}

static int s3c24xx_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s3c24xx_pcm_dma_params *dma = rtd->dai->cpu_dai->dma_data;
	unsigned long totbytes;
	int ret=0;
	
	s3cdbg("Entered %s, params = %p \n", __FUNCTION__, prtd->params);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		totbytes = params_buffer_bytes(params) * ANDROID_BUF_NUM;
	
	else 
		totbytes = params_buffer_bytes(params);

//	printk("[%d]:ring_buf_num %d\n", substream->stream, ring_buf_num);

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!dma)
		return 0;

	/* this may get called several times by oss emulation
	 * with different params */
	if (prtd->params == NULL) {
		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);


		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	} else if (prtd->params != dma) {
		s3c2410_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = dma;
		s3cdbg("params %p, client %p, channel %d\n", prtd->params,
			prtd->params->client, prtd->params->channel);


		/* prepare DMA */
		ret = s3c2410_dma_request(prtd->params->channel,
					  prtd->params->client, NULL);

		if (ret) {
			printk(KERN_ERR "failed to get dma channel\n");
			return ret;
		}
	}

	/* channel needs configuring for mem=>device, increment memory addr,
	 * sync to pclk, half-word transfers to the IIS-FIFO. */
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410)  && !defined(CONFIG_CPU_S5PC100) && !defined (CONFIG_CPU_S5P6440)
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_MEM, S3C2410_DISRCC_INC |
				S3C2410_DISRCC_APB, prtd->params->dma_addr);

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size,
				S3C2410_DCON_SYNC_PCLK | 
				S3C2410_DCON_HANDSHAKE);
	} else {
		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size,
				S3C2410_DCON_HANDSHAKE | 
				S3C2410_DCON_SYNC_PCLK);

		s3c2410_dma_devconfig(prtd->params->channel,
					S3C2410_DMASRC_HW, 0x3,
					prtd->params->dma_addr);
	}

#else
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_MEM, 0,
				prtd->params->dma_addr);

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);
	} else {
		s3c2410_dma_devconfig(prtd->params->channel,
				S3C2410_DMASRC_HW, 0,
				prtd->params->dma_addr);		

		s3c2410_dma_config(prtd->params->channel,
				prtd->params->dma_size, 0);
	}
#endif

	s3c2410_dma_set_buffdone_fn(prtd->params->channel,
				    s3c24xx_audio_buffdone);

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = totbytes;

	spin_lock_irq(&prtd->lock);
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);

	s3cdbg("Entered %s, line %d \n", __FUNCTION__, __LINE__);
	return 0;
}

static int s3c24xx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;

	s3cdbg("Entered %s\n", __FUNCTION__);

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	if (prtd->params) {
		s3c2410_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = NULL;
	}

	return 0;
}

static int s3c24xx_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) 
	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!prtd->params)
	 	return 0;
#endif

	/* flush the DMA channel */
	s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_FLUSH);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ring_buf_index 	 = 0;
		period_index	 = 0;
	}

	prtd->dma_pos = prtd->dma_start;

	/* enqueue dma buffers */
	s3c24xx_pcm_enqueue(substream);

	return ret;
}

static int s3c24xx_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
		
		s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_START);
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) && !defined (CONFIG_CPU_S5P6440)
		s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STARTED);
#endif		
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STOP);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t 
	s3c24xx_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;
	unsigned long res;

	s3cdbg("Entered %s\n", __FUNCTION__);

	spin_lock(&prtd->lock);

#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)  
		res = prtd->dma_pos - prtd->dma_start;
	else 
		res = prtd->dma_pos - prtd->dma_start;	
#else
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		res = dst - prtd->dma_start;
	else
		res = src - prtd->dma_start;
#endif

	spin_unlock(&prtd->lock);

	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */
	
	/* Playback mode */	
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {	
		if (res >= (snd_pcm_lib_buffer_bytes(substream) * ANDROID_BUF_NUM)) {
			if (res == (snd_pcm_lib_buffer_bytes(substream) * ANDROID_BUF_NUM))
				res = 0;
		}
	}

	/* Capture mode */	
	else {	
		if (res >= (snd_pcm_lib_buffer_bytes(substream))) {
			if (res == (snd_pcm_lib_buffer_bytes(substream)))
				res = 0;
		}
	}

	return bytes_to_frames(substream->runtime, res);
}

static int s3c24xx_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);

	snd_soc_set_runtime_hwparams(substream, &s3c24xx_pcm_hardware);

	prtd = kzalloc(sizeof(struct s3c24xx_runtime_data), GFP_KERNEL);
//	printk("[%d]: prtd addr 0x%x\n", substream->stream, prtd);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;
	return 0;
}

static int s3c24xx_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;

	s3cdbg("Entered %s, prtd = %p\n", __FUNCTION__, prtd);

	if (prtd)
		kfree(prtd);
	else
		printk("s3c24xx_pcm_close called with prtd == NULL\n");

	return 0;
}

static int s3c24xx_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	s3cdbg("Entered %s\n", __FUNCTION__);

	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
                                     runtime->dma_area,
                                     runtime->dma_addr,
                                     runtime->dma_bytes);
}

static struct snd_pcm_ops s3c24xx_pcm_ops = {
	.open		= s3c24xx_pcm_open,
	.close		= s3c24xx_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= s3c24xx_pcm_hw_params,
	.hw_free	= s3c24xx_pcm_hw_free,
	.prepare	= s3c24xx_pcm_prepare,
	.trigger	= s3c24xx_pcm_trigger,
	.pointer	= s3c24xx_pcm_pointer,
	.mmap		= s3c24xx_pcm_mmap,
};

static int s3c24xx_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = s3c24xx_pcm_hardware.buffer_bytes_max;

	s3cdbg("Entered %s\n", __FUNCTION__);

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	return 0;
}

static void s3c24xx_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	s3cdbg("Entered %s\n", __FUNCTION__);

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 s3c24xx_pcm_dmamask = DMA_32BIT_MASK;

static int s3c24xx_pcm_new(struct snd_card *card, 
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &s3c24xx_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (dai->playback.channels_min) {
		ret = s3c24xx_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (dai->capture.channels_min) {
		ret = s3c24xx_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

struct snd_soc_platform s3c24xx_soc_platform = {
	.name		= "s3c24xx-audio",
	.pcm_ops 	= &s3c24xx_pcm_ops,
	.pcm_new	= s3c24xx_pcm_new,
	.pcm_free	= s3c24xx_pcm_free_dma_buffers,
};

EXPORT_SYMBOL_GPL(s3c24xx_soc_platform);

static int __init s3c_soc_platform_init(void)
{
	return snd_soc_register_platform(&s3c24xx_soc_platform);
}	

module_init(s3c_soc_platform_init);

static void __exit s3c_soc_platform_exit(void)
{
	snd_soc_unregister_platform(&s3c24xx_soc_platform);
}
module_exit(s3c_soc_platform_exit);

MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("Samsung S3C24XX PCM DMA module");
MODULE_LICENSE("GPL");
