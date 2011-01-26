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

#include <asm/io.h>
#include <mach/hardware.h>
#include <plat/s3c6410-dma.h>
#include <mach/audio.h>

#include "s3c-pcm.h"

#if defined CONFIG_SND_S3C6400_SOC_AC97
#define MAIN_DMA_CH 1
#else /*S3C6400 I2S */ 
#define MAIN_DMA_CH 0
#endif

#define PLAYBACK			0
#define CAPTURE				1
#define ANDROID_BUF_SIZE	4096

#define USE_LLI_INTERFACE	
#undef USE_LLI_INTERFACE	

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
	unsigned long dma_totsize;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct dma_data			*dma_param;
	struct dmac_conn_info	*dinfo;

#ifdef USE_LLI_INTERFACE
	struct dmac_lli				*lli_data;
	unsigned int 				num_lli;
#endif
};

extern unsigned int ring_buf_index;
extern unsigned int period_index;

/* s3c6410_pcm_dma_param_init
 *
 * Initiaize parameters for using dma controller.
*/
static void s3c6410_pcm_dma_param_init(struct s3c24xx_runtime_data *prtd)
{
	/* Initialize DMA Param */
    prtd->dma_param->dma_lli_v  = NULL;
    prtd->dma_param->src_addr   = 0;
    prtd->dma_param->dst_addr   = 0;
    prtd->dma_param->lli_addr   = 0;
    prtd->dma_param->dmac_cfg   = 0;
    prtd->dma_param->dmac_ctrl0 = 0;
    prtd->dma_param->dmac_ctrl1 = 0;
    prtd->dma_param->dmac_bytes = 0;
    prtd->dma_param->active     = DMA_DEFAULT;
    prtd->dma_param->chan_num   = MAX_DMA_CHANNELS;
}


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
 
	/* DMA with I2S might be unstable when length is too short. */
    pred_pos = pos + prtd->dma_period;

    next_len = prtd->dma_period;

    if ((pred_pos + next_len) > prtd->dma_end) {
        next_len  = prtd->dma_end - pred_pos;
    }

    if (next_len <= 32) { 			/* next transfer is too short */
        len += next_len; 			/* transfer with next small period */
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			prtd->dma_param->dmac_ctrl1 = TSFR_SIZE4(len);
			prtd->dma_param->src_addr	= pos;
		}
		else {
			prtd->dma_param->dmac_ctrl1 = TSFR_SIZE2(len);
			prtd->dma_param->dst_addr	= pos;
		}
		s3c6410_dmac_enable(prtd->dma_param);
		pos += next_len;
    }

    else {
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			prtd->dma_param->dmac_ctrl1 = TSFR_SIZE4(len);
			prtd->dma_param->src_addr	= pos;
		}
		else {
			prtd->dma_param->dmac_ctrl1 = TSFR_SIZE2(len);
			prtd->dma_param->dst_addr	= pos;
		}
		s3c6410_dmac_enable(prtd->dma_param);
	}

	prtd->dma_pos = pos;
}

static void s3c6410_audio_buffdone(void *id)
{
	struct snd_pcm_substream *substream = id;
	struct s3c24xx_runtime_data *prtd;

	s3cdbg("Entered %s\n", __FUNCTION__);
		
	if (!substream)
		return;

	prtd = substream->runtime->private_data;
		
	prtd->dma_pos += prtd->dma_period;
	if (prtd->dma_pos >= prtd->dma_end)
		prtd->dma_pos = prtd->dma_start;

	snd_pcm_period_elapsed(substream);

	spin_lock(&prtd->lock);

#ifdef USE_LLI_INTERFACE
	/* We don't have something to do */

#else
	if (prtd->state & ST_RUNNING) 
		s3c24xx_pcm_enqueue(substream);
#endif

	spin_unlock(&prtd->lock);
}

#ifdef USE_LLI_INTERFACE
static struct dmac_lli *s3c6410_audio_make_dmalli(struct snd_pcm_substream *substream, struct dma_data *dmadata, 
												  unsigned int num_lli)
{
	struct snd_pcm_runtime		*runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd 	 = runtime->private_data;
	struct dmac_lli		*dma_lli = dmadata->dma_lli_v;
	int i;

	for(i = 0 ; i < num_lli - 1 ; i++){
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dma_lli[i].src_lli    = dmadata->src_addr + (i * prtd->dma_period);
			dma_lli[i].dst_lli    = dmadata->dst_addr;
		}else {
			dma_lli[i].dst_lli    = dmadata->dst_addr + (i * prtd->dma_period);
			dma_lli[i].src_lli    = dmadata->src_addr;
		}	

		dma_lli[i].chan_ctrl0 = dmadata->dmac_ctrl0 | TC_INT_ENABLE;
		dma_lli[i].chan_ctrl1 = dmadata->dmac_ctrl1;
		dma_lli[i].next_lli   = dmadata->lli_addr + ((i * 20) + 0x14);
	}
	
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_lli[i].src_lli    = dmadata->src_addr + (i * prtd->dma_period); 
		dma_lli[i].dst_lli    = dmadata->dst_addr;
	} else {
		dma_lli[i].dst_lli    = dmadata->dst_addr + (i * prtd->dma_period);
		dma_lli[i].src_lli    = dmadata->src_addr;
	}

	dma_lli[i].chan_ctrl0 = dmadata->dmac_ctrl0 | TC_INT_ENABLE;
	dma_lli[i].chan_ctrl1 = dmadata->dmac_ctrl1;
	dma_lli[i].next_lli   = dmadata->lli_addr;

	return dma_lli;
}
#endif

static int s3c24xx_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime 		*runtime  = substream->runtime;
	struct s3c24xx_runtime_data *prtd 	  = runtime->private_data;
	struct snd_soc_pcm_runtime 	*rtd 	  = substream->private_data;
	struct dma_data		   		*dma_data = prtd->dma_param;
	struct dmac_conn_info 		*dinfo	  = rtd->dai->cpu_dai->dma_data;

	s3cdbg("Entered %s, params = %p \n", __FUNCTION__, prtd->params);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		prtd->dma_totsize = params_buffer_bytes(params) * CONFIG_ANDROID_BUF_NUM;
	
	else 
		prtd->dma_totsize = params_buffer_bytes(params);

	prtd->dinfo = dinfo;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data->dma_port = prtd->dinfo->connection_num[PLAYBACK].dma_port;
		dma_data->conn_num = prtd->dinfo->connection_num[PLAYBACK].conn_num;
	}else {
		dma_data->dma_port = prtd->dinfo->connection_num[CAPTURE].dma_port;
		dma_data->conn_num = prtd->dinfo->connection_num[CAPTURE].conn_num;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = prtd->dma_totsize;

	spin_lock_irq(&prtd->lock);
	prtd->dma_limit = runtime->hw.periods_min;
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + prtd->dma_totsize;
	spin_unlock_irq(&prtd->lock);

	dma_data->dma_dev_handler = (void *)s3c6410_audio_buffdone;

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data->src_addr      = prtd->dma_start;
		dma_data->dst_addr      = dinfo->connection_num[PLAYBACK].hw_fifo;
		dma_data->dmac_cfg      = FCTL_DMA_M2P | DST_CONN(dma_data->conn_num) | INT_TC_MASK;
		dma_data->dmac_ctrl0    = SRC_BSIZE_1 | DST_BSIZE_1 | SRC_TWIDTH_32 | DST_TWIDTH_32 | SRC_INC | 
								  DST_AHB_PERI | SRC_AHB_SYSTEM;
		dma_data->dmac_ctrl1	= TSFR_SIZE4(prtd->dma_period);
	}
	
	else {
		dma_data->src_addr      = dinfo->connection_num[CAPTURE].hw_fifo;
	    dma_data->dst_addr      = prtd->dma_start;
	    dma_data->dmac_cfg      = FCTL_DMA_P2M | SRC_CONN(dma_data->conn_num) | INT_TC_MASK;
	    dma_data->dmac_ctrl0    = SRC_BSIZE_1 | DST_BSIZE_1 | SRC_TWIDTH_16 | DST_TWIDTH_16 | DST_INC |
								  SRC_AHB_PERI | DST_AHB_SYSTEM;
		dma_data->dmac_ctrl1	= TSFR_SIZE2(prtd->dma_period);
	}

#ifdef USE_LLI_INTERFACE
	prtd->num_lli = runtime->dma_bytes / prtd->dma_period;

	prtd->lli_data = dma_alloc_coherent(substream->pcm->card->dev, sizeof(struct dmac_lli) * prtd->num_lli, 
									 	&dma_data->lli_addr, GFP_KERNEL | GFP_DMA);
	if(!prtd->lli_data)
		printk("Failed to allocate memory for using dmac lli interface\n");

	dma_data->dma_lli_v = (void *)prtd->lli_data;
	prtd->lli_data = s3c6410_audio_make_dmalli(substream, dma_data, prtd->num_lli);

#else
	dma_data->dmac_ctrl0 |= TC_INT_ENABLE;

#endif

	s3c6410_dmac_request(dma_data, substream);

	s3cdbg("Entered %s, line %d \n", __FUNCTION__, __LINE__);

	return 0;
}

static int s3c24xx_pcm_hw_free(struct snd_pcm_substream *substream)
{
	s3cdbg("Entered %s\n", __FUNCTION__);

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int s3c24xx_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct s3c24xx_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s3cdbg("Entered %s\n", __FUNCTION__);

	if(!prtd->dinfo)
		return 0;
	
	/* Flush dma area */
	memset(substream->runtime->dma_area, 0, prtd->dma_totsize);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ring_buf_index 	 = 0;
		period_index	 = 0;
	}

	prtd->dma_pos = prtd->dma_start;
		
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
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		s3c6410_dmac_disable(prtd->dma_param);
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
		if (res >= (snd_pcm_lib_buffer_bytes(substream) * CONFIG_ANDROID_BUF_NUM)) {
			if (res == (snd_pcm_lib_buffer_bytes(substream) * CONFIG_ANDROID_BUF_NUM))
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

	prtd = kmalloc(sizeof(struct s3c24xx_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	prtd->dma_param = (struct dma_data *)kzalloc(sizeof(struct dma_data), GFP_KERNEL);
	if (prtd->dma_param == NULL)
		return -ENOMEM;

	s3c6410_pcm_dma_param_init(prtd);

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;

	return 0;
}

static int s3c24xx_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s3c24xx_runtime_data *prtd = runtime->private_data;

	s3cdbg("Entered %s, prtd = %p\n", __FUNCTION__, prtd);

#ifdef USE_LLI_INTERFACE
	if(prtd->lli_data)
		dma_free_coherent(substream->pcm->card->dev, sizeof(struct dmac_lli) * prtd->num_lli, prtd->dma_param->dma_lli_v,
						  prtd->dma_param->lli_addr);
#endif

	s3c6410_dmac_free(prtd->dma_param);
	s3c6410_pcm_dma_param_init(prtd);

	if (prtd->dma_param)
		kfree(prtd->dma_param);

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
