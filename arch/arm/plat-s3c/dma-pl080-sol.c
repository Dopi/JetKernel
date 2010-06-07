/*
 * linux/arch/arm/pla-s3c/dma-pl080-sol.c
 *
 * Copyright (C) 2009 Samsung Electronics
 * Copyright (C) 2006 ARM Ltd, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/mach-types.h>

#include <plat/regs-clock.h>
#include <plat/s3c6410-dma.h>
#include <mach/map.h>
#include "dma-pl080-sol.h"

#define s3c6410_dmac_suspend	NULL
#define s3c6410_dmac_resume		NULL
#define DRIVER_NAME 			"s3c-dmac"

#if defined(CONFIG_S3C6410_EB_DMAC_DEBUG)
#define DBG(args...)                                printk(args)
#define IFDBG(condition, args...)   if(condition)   printk(args)
#else
#define DBG(args...)                do{} while(0)
#define IFDBG(condition, args...)   do{} while(0)
#endif

DEFINE_SPINLOCK(s3c6410_dma_spin_lock);

static struct s3c6410_dmac_port dport[DMAC_NR];
static unsigned int errCtr = 0;

static int s3c6410_select_dmac(unsigned long dma_sel)
{
	unsigned long tmp = __raw_readl(S3C_SDMA_SEL);

	tmp |= dma_sel;
	__raw_writel(tmp, S3C_SDMA_SEL);

	return 0;
}

static void s3c6410_dmac_post_irq(unsigned int dma_port, unsigned int dma_chan){

	dmac_writel((1 << dma_chan), dma_port, DMACINTTCCLR);
	dmac_writel((1 << dma_chan), dma_port, DMACINTERRCLR);
}

int s3c6410_dmac_transfer_setup(struct dma_data *dmadata)
{
	unsigned int dma_cfg   = 0;
	unsigned int dma_ctrl0 = 0;
	unsigned int dma_ctrl1 = 0;
	unsigned int flow_ctrl = 0;

	dma_cfg   = dmadata->dmac_cfg;
	dma_ctrl0 = dmadata->dmac_ctrl0;
	dma_ctrl1 = dmadata->dmac_ctrl1; 

	flow_ctrl = (dmadata->dmac_cfg & 0x3800);
	if(flow_ctrl == FCTL_DMA_M2P)
		dmac_writel(dmadata->src_addr, dmadata->dma_port, DMACCxSRCADDR(dmadata->chan_num));
	
	else if(flow_ctrl == FCTL_DMA_P2M)
		dmac_writel(dmadata->dst_addr, dmadata->dma_port, DMACCxDSTADDR(dmadata->chan_num));

	dmac_writel(dma_ctrl0,	 	   dmadata->dma_port, DMACCxCTL0   (dmadata->chan_num));
	dmac_writel(dma_ctrl1,	 	   dmadata->dma_port, DMACCxCTL1   (dmadata->chan_num));
	dmac_writel(dma_cfg, 		   dmadata->dma_port, DMACCxCFG	   (dmadata->chan_num));

	return 0;
}

unsigned int get_dma_channel(unsigned int dma_port)
{
	unsigned int i, channel = MAX_DMA_CHANNELS;

	/* Searching Active DMA channel */
	for(i = 0; i < MAX_DMA_CHANNELS; i++){
		if(dport[dma_port].dmac[i]->dmadata == NULL){
			channel = i;
			break;
		}
	}	
	return channel;
}

int s3c6410_dmac_halt(struct dma_data *dmadata, unsigned int val)
{
	unsigned int tmp;
	if(val == HALT_SET) {
		tmp = dmac_readl(dmadata->dma_port, DMACCxCFG(dmadata->chan_num)); 
		dmac_writel(tmp | val, dmadata->dma_port, DMACCxCFG(dmadata->chan_num));
	}

	else {
		tmp = dmac_readl(dmadata->dma_port, DMACCxCFG(dmadata->chan_num)); 
		dmac_writel(tmp & val, dmadata->dma_port, DMACCxCFG(dmadata->chan_num));
	}
			
	DBG("%s: Pass dmac_halt(), [chan:%d]\n", DRIVER_NAME, dmadata->chan_num);
	return 0;
}	
EXPORT_SYMBOL(s3c6410_dmac_halt);

unsigned int s3c6410_dmac_get_base(unsigned int port)
{
	unsigned int base_addr = (unsigned int) dport[port].dmac_base;
	DBG("%s: Pass dmac_get_base(), address[0x%x]\n", DRIVER_NAME, base_addr);
	return base_addr;
}
EXPORT_SYMBOL(s3c6410_dmac_get_base);

int s3c6410_dmac_request(struct dma_data *dmadata, void *id)
{
	unsigned int 				channel = MAX_DMA_CHANNELS;
	unsigned int 				flow_ctrl = 0;
	unsigned int				val;

	if(dmadata->active == DMA_USED) {
		printk(KERN_WARNING "%s: Already allocated dma channel\n", DRIVER_NAME);
		return -EBUSY;
	}

	channel = get_dma_channel(dmadata->dma_port);
	DBG("%s: allocated dma channel[%d] for %s\n", DRIVER_NAME, channel, pdev->name);
	if(channel == MAX_DMA_CHANNELS) {
		printk(KERN_ERR "%s: Can't get dma channel\n", DRIVER_NAME);
		return -EBUSY;
	}

	dmadata->active = DMA_USED;
	dmadata->chan_num = channel;
	dmadata->private_data = id;

	/* Get the dmadata for the device which requested DMA */
	dport[dmadata->dma_port].dmac[channel]->dmadata = dmadata;

	dmac_writel(INTTC_CLR_CH(channel), 	 dmadata->dma_port, DMACINTTCCLR);
	dmac_writel(ERR_INT_CLR_CH(channel), dmadata->dma_port, DMACINTERRCLR);

	val = dmac_readl(dmadata->dma_port, DMACCFG);
	if(!(val && DMAC_ENABLE)){
		val |= DMAC_ENABLE;
		dmac_writel(val, dmadata->dma_port, DMACCFG);
	}
	
	/* Configuration SRC/DST Addr, LLI Register */
	flow_ctrl = (dmadata->dmac_cfg & 0x3800);
	
	if(flow_ctrl == FCTL_DMA_M2P)
		dmac_writel(dmadata->dst_addr, dmadata->dma_port, DMACCxDSTADDR(dmadata->chan_num));
	
	else if(flow_ctrl == FCTL_DMA_P2M)
		dmac_writel(dmadata->src_addr, dmadata->dma_port, DMACCxSRCADDR(dmadata->chan_num));

	dmac_writel(dmadata->lli_addr, dmadata->dma_port, DMACCxLLI	   (dmadata->chan_num));

	DBG("%s: Pass dmac_request()\n", DRIVER_NAME);
	
    return 0;	
}
EXPORT_SYMBOL(s3c6410_dmac_request);

int s3c6410_dmac_free(struct dma_data *dmadata)
{
	unsigned int i, active_count = 0;
	unsigned int dma_port, chan_num;

	DBG("%s: %s\n", DRIVER_NAME, __func__);

	dma_port = dmadata->dma_port;
	chan_num = dmadata->chan_num;
	
	/* DMAC Global data init for dmadata */
	dport[dma_port].dmac[chan_num]->dmadata = NULL;
	
	for(i = 0; i < MAX_DMA_CHANNELS; i++){
		if(dport[dma_port].dmac[i]->dmadata != NULL)
			active_count += dport[dma_port].dmac[i]->dmadata->active;
	}

	/* Channel Disable */
	dmac_writel(CHAN_DISABLE, dma_port, DMACCxCFG(chan_num));
	dmadata->active = DMA_NOUSED;

	/* For Power saving */ 
	if(!active_count) {
		unsigned int val;
		val = dmac_readl(dma_port, DMACCFG);
		val &= DMAC_DISABLE;
		dmac_writel(val, dma_port, DMACCFG);
	}
	DBG("%s: Pass dmac_free(), [chan:%d]\n", DRIVER_NAME, chan_num);

	return 0;
}
EXPORT_SYMBOL(s3c6410_dmac_free);

int s3c6410_dmac_enable(struct dma_data *dmadata)
{
	volatile unsigned int r;
	unsigned int 		  channel = dmadata->chan_num;
	unsigned int 		  dma_port  = dmadata->dma_port;

	s3c6410_dmac_transfer_setup(dmadata);

	r = dmac_readl(dma_port,  DMACCxCFG(channel));
	dmac_writel((r | CHAN_ENABLE) & HALT_CLR, dma_port, DMACCxCFG(channel));
	DBG("%s: Pass dmac_enable(), [chan:%d]\n", DRIVER_NAME, channel);

	return 0;
}
EXPORT_SYMBOL(s3c6410_dmac_enable);

int s3c6410_dmac_disable(struct dma_data *dmadata)
{
	volatile unsigned int r;
	unsigned int 		  channel = dmadata->chan_num;
	unsigned int 		  dma_port  = dmadata->dma_port;

	r = dmac_readl(dma_port, DMACCxCFG(channel));
	dmac_writel((r & ~(CHAN_ENABLE)) | HALT_SET, dma_port, DMACCxCFG(channel));
	DBG("%s: Pass dmac_disable(), [chan:%d]\n", DRIVER_NAME, channel);

	return 0;
}
EXPORT_SYMBOL(s3c6410_dmac_disable);

static inline int find_channel(unsigned int x)
{   
	int r = 1;
    if (!x)
        return -1;
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r - 1;
}   

static irqreturn_t s3c6410_dmac_irq(int irq, void *devid)
{
//	struct platform_device		*pdev;
	volatile unsigned int 		r;
	unsigned int 				channel;
	unsigned int 				dma_port;
	unsigned long				flags;
	void						*id;

	flags = s3c6410_dma_lock();
	r = dmac_readl(0, DMACINTTCSTAT);
	channel = find_channel(r);
	dma_port = 0;
	if(channel == -1){
		r = dmac_readl(1, DMACINTTCSTAT);
		channel = find_channel(r);
		dma_port = 1;
	}
	
	if(channel == -1) {
 		r = dmac_readl(0, DMACINTERRSTAT);
		channel = find_channel(r);
		dma_port = 0;
		if(channel == -1){
			r = dmac_readl(1, DMACINTERRSTAT);
			channel = find_channel(r);
			dma_port = 1;
		}	
		if((1 << channel) & r){
			errCtr++;
			DBG("%s: DMA Error, channel %d\n", DRIVER_NAME, channel);
	    }
	}	
	
	else {
		id = dport[dma_port].dmac[channel]->dmadata->private_data;
		dport[dma_port].dmac[channel]->dmadata->dma_dev_handler(id);
	}		
	
	/* e.g clear interrupt on the DMAC */
	s3c6410_dmac_post_irq(dma_port, channel);

	s3c6410_dma_unlock(flags);
	return IRQ_HANDLED;
}

static int s3c6410_dmac_probe(struct platform_device *pdev)
{
	struct	 resource	 		*res;
	unsigned int 				 ret = 0;
	unsigned int 				 i;
	unsigned int 				 j;

	for(i = 0 ; i < DMAC_NR ; i++) {
		if(dport[i].dmac_base == NULL)
			break;
	}

	if(i == DMAC_NR) {
		ret = -EBUSY;
		goto out;
	}

	for(j = 0 ; j < MAX_DMA_CHANNELS ; j++) {
		dport[i].dmac[j] = (struct s3c6410_dmac *)kmalloc(sizeof(struct s3c6410_dmac), GFP_KERNEL);
		if(&dport[i].dmac[j] == NULL) {
			ret = -ENOMEM;
			goto out;
		}
		dport[i].dmac[j]->dmadata = NULL;
	}
	
	/* Get device resource(s3c6410-dmac) */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk("%s: Couldn't Get Resource\n", DRIVER_NAME);
		ret = -ENODEV;
		goto out;
	}

	/* Allocate a memory region for DMAC */
	if(!request_mem_region(res->start, (res->end - res->start) + 1, DRIVER_NAME)) {
		printk("%s: Couldn't Get IO mem region for\n", DRIVER_NAME);
		ret = -ENOMEM;
		goto out;
	}
	
	/* Remapping for Change with virtual address */
	dport[i].dmac_base = ioremap(res->start, (res->end - res->start) + 1);
	if(!dport[i].dmac_base) {
		printk("%s: Couldn't get virtual io address\n", DRIVER_NAME);
		ret = -ENOMEM;
		goto release_mem;
	}

	dport[i].irq = platform_get_irq(pdev, 0);
	ret = request_irq(dport[i].irq, s3c6410_dmac_irq, IRQF_SHARED|IRQF_DISABLED, DRIVER_NAME, pdev);
	if(ret){
		printk("%s: fail to request_irq() for dmac...\n", DRIVER_NAME);
		goto free;
	}	

	printk("%s: Samsung S3C6410 DMA Controller base address at [phy]:0x%x, [virt]:0x%x, irq %d\n", 
				DRIVER_NAME, res->start, (unsigned int)dport[i].dmac_base, dport[i].irq);
	DBG("%s: Pass dmac_probe()\n", DRIVER_NAME);
	return ret;

free:
	iounmap(dport[i].dmac_base);

release_mem:
	release_mem_region(res->start, (res->end - res->start) + 1);
out:
	return ret;
}

static int __devexit s3c6410_dmac_remove(struct platform_device *pdev)
{
	DBG("%s: Pass dmac_remove\n", DRIVER_NAME);
	return 0;
}

static struct platform_driver s3c6410_dmac_driver = {
	.driver = {
		.name 	= DRIVER_NAME,
	},
	.probe		= s3c6410_dmac_probe,
	.remove		= __devexit_p(s3c6410_dmac_remove),
	.suspend	= s3c6410_dmac_suspend,
	.resume		= s3c6410_dmac_resume,
};

static int __init s3c6410_dmac_init(void)
{
	unsigned long 		 dma_sel = 0;
	unsigned int  		 i;

	/* Find mapping of DMAC (DMA?, SDMA?) */
	for(i = 0; i < CONNECTION_DEVICE_NUM; i++) {
		if(s3c6410_dmac_sel[i].name) {
			dma_sel |= s3c6410_dmac_sel[i].dma_sel;	
		}
	}

	s3c6410_select_dmac(dma_sel);

	return platform_driver_register(&s3c6410_dmac_driver);
}

static void __exit s3c6410_dmac_exit(void)
{
	platform_driver_unregister(&s3c6410_dmac_driver);
}

MODULE_DESCRIPTION ("Samsung S3C6410 PL080 DMAC Device driver");
MODULE_LICENSE ("GPL");

module_init(s3c6410_dmac_init);
module_exit(s3c6410_dmac_exit);

