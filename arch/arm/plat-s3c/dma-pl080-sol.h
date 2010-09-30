/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

struct s3c6410_dmac_port {
	struct s3c6410_dmac		*dmac[MAX_DMA_CHANNELS];
	void __iomem			*dmac_base;
    unsigned int			irq;
};

struct s3c6410_dmac {
    struct platform_device      *pdev;
    struct dma_data             *dmadata;
	void						*private_data;
    spinlock_t                   lock;
};

#define dmac_readl(x, offset)			readl(dport[x].dmac_base + offset)
#define dmac_writel(val, x, offset)	writel(val, dport[x].dmac_base + offset)


