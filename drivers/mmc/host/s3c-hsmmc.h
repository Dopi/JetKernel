/*
 *  linux/drivers/mmc/s3c-hsmmc.h - Samsung S3C HS-MMC Interface driver
 *
 * $Id: s3c-hsmmc.h,v 1.1 2008/03/03 00:40:28 ihlee215 Exp $
 *
 *  Copyright (C) 2004 Thomas Kleffel, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MHZ
#define MHZ (1000*1000)
#endif

#define s3c_hsmmc_readl(x)		readl((host->base)+(x))
#define s3c_hsmmc_readw(x)	readw((host->base)+(x))
#define s3c_hsmmc_readb(x)	readb((host->base)+(x))

#define s3c_hsmmc_writel(v,x)	writel((v),(host->base)+(x))
#define s3c_hsmmc_writew(v,x)	writew((v),(host->base)+(x))
#define s3c_hsmmc_writeb(v,x)	writeb((v),(host->base)+(x))

#define S3C_HSMMC_CLOCK_ON	1
#define S3C_HSMMC_CLOCK_OFF	0

#define	SPEED_NORMAL		0
#define	SPEED_HIGH		1


#ifndef CONFIG_S3C_HSMMC_MAX_HW_SEGS
#define CONFIG_S3C_HSMMC_MAX_HW_SEGS	32
#endif


/* For SDMA */
struct s3c_hsmmc_dma_blk {
	dma_addr_t	dma_address;	/* dma address			*/
	uint		length;		/* length			*/
	uint		boundary;	/* Host DMA Buffer Boundary	*/
	void		*original;
};

/* For ADMA2 */
struct s3c_hsmmc_adma_descr {
	volatile u32	length_attr;	/* length + attribute	*/
	volatile u32	dma_address;	/* dma address	        */
} __packed;


struct s3c_hsmmc_host {
	void __iomem			*base;
	struct mmc_request	*mrq;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_host		*mmc;
	struct clk				*clk_io;
	struct clk				*clk[3];
	struct resource		*mem;
	struct platform_device 	*pdev;

	struct timer_list	timer;

	struct s3c_sdhci_platdata *plat_data;

	int			irq;
	int			irq_cd;
	int 			num_clksrc;
	int			on_bord;

	spinlock_t		lock;

	struct tasklet_struct	card_tasklet;	/* Tasklet structures	*/
	struct tasklet_struct	finish_tasklet;

	unsigned int		clock;		/* Current clock (MHz)	*/
	unsigned int		ctrl2;	
	unsigned int		ctrl3[3];	/* 0: normal, 1: high	*/
	unsigned int		ctrl4;	

#define S3C_HSMMC_USE_DMA	(1<<0)
	int			flags;		/* Host attributes	*/

	uint			dma_dir;
	
#ifdef CONFIG_HSMMC_SCATTERGATHER
	uint			sg_len;		/* size of scatter list	*/

	/* For SDMA */
	uint			dma_blk;	/* total dmablk number	*/
	uint			next_blk;	/* next block to send	*/
	struct s3c_hsmmc_dma_blk dblk[CONFIG_S3C_HSMMC_MAX_HW_SEGS*4];

	/* when pseudo algo cannot deal with sglist */
#define S3C_HSMMC_MALLOC_SIZE	PAGE_SIZE
#define S3C_HSMMC_MAX_SUB_BUF	CONFIG_S3C_HSMMC_MAX_HW_SEGS
	void			*sub_block[S3C_HSMMC_MAX_SUB_BUF];

	/* For ADMA2 */
	struct s3c_hsmmc_adma_descr sdma_descr_tbl[CONFIG_S3C_HSMMC_MAX_HW_SEGS];
#endif

};

