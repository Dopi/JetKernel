/*
 *  linux/include/asm-arm/arch-s3c6410/dma.h
 *
 *  Copyright (C) 2007 Samsung Electronics TLD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *
 */

#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/spinlock.h>

/************************************************************************************
 * DMA Controller Number for S3C6410
 ***********************************************************************************/
#define DMAC_NR				4
/***********************************************************************************************/
/*		Etc Informations										                               */
/***********************************************************************************************/
#define MAX_DMA_CHANNELS    8       		/* Max channels of DMAC */
#define DMA_DEFAULT    		2				/* DMA status for active */				
#define DMA_USED        	1
#define DMA_NOUSED      	0
/***********************************************************************************************/
/*		Registers Offset										                               */
/***********************************************************************************************/
#define	DMACINTSTAT			0x00				/* Interrupt status register */
#define	DMACINTTCSTAT		0x04				/* Interrupt terminal count status register */
#define	DMACINTTCCLR		0x08				/* Interrupt terminal count clear register */
#define	DMACINTERRSTAT		0x0C				/* Interrupt error status register */
#define	DMACINTERRCLR		0x10				/* Interrupt error clear register */
#define	DMACRAWINTTCSTAT	0x14				/* RAW Interrupt TC status register */
#define	DMACRAWINTERRSTAT	0x18				/* RAW Interrupt error clear register */
#define	DMACENBLDCHAN		0x1C				/* Enabled channel status register */
#define	DMACSOFTBREQ		0x20				/* Software Burst Request register */
#define	DMACSOFTSREQ		0x24				/* Software Single Request register */
#define	DMACSOFTLBREQ		0x28				/* Software Last Burst Request register */ /
#define	DMACSOFTLSREQ		0x2C				/* Software Last Single Request register */
#define	DMACCFG				0x30				/* Configuration register */ 
#define	DMACSYNC			0x34				/* Syncronization register */
#define DMACCxSRCADDR(x)	0x100 + ((x) << 5)	/* Channel Source Address Register */
#define DMACCxDSTADDR(x)	0x104 + ((x) << 5)	/* Channel Destination Address Register */
#define DMACCxLLI(x)		0x108 + ((x) << 5)	/* Channel LLI Register */
#define DMACCxCTL0(x)		0x10C + ((x) << 5)	/* Channel Control Register 0 */
#define DMACCxCTL1(x)		0x110 + ((x) << 5)	/* Channel Control Register 1 */
#define DMACCxCFG(x)		0x114 + ((x) << 5)	/* Channel Configuration Register */

/***********************************************************************************************/
/*		DMACINTSTAT : Status of the DMA interrupts after masking(R/O, [7:0])                   */
/***********************************************************************************************/
#define	INT_STAT_CH(x)			(0x1 << x)			

/***********************************************************************************************/
/*		DMACINTTCSTAT : Interrupt terminal count request status(R/O, [7:0])					   */
/***********************************************************************************************/
#define	INTTC_STAT_CH(x)		(0x1 << x)		

/***********************************************************************************************/
/*		DMACINTTCCLR : Terminal count request clear(W/O, [7:0])								   */
/***********************************************************************************************/
#define	INTTC_CLR_CH(x)			(0x1 << x)	

/***********************************************************************************************/
/*		DMACINTERRSTAT : Interrupt error status(R/O, [7:0])						    	       */
/***********************************************************************************************/
#define	ERR_INT_STAT_CH(x)		(0x1 << x)	

/***********************************************************************************************/
/*		DMACINTERRCLR : Interrupt error clear(W/O, [7:0])                   				   */
/***********************************************************************************************/
#define	ERR_INT_CLR_CH(x)		(0x1 << x)	

/***********************************************************************************************/
/*		DMACRAWINTTCSTAT : Status of the terminal count interrupt prior to masking(R/O, [7:0]) */
/***********************************************************************************************/
#define	RAW_INTTC_STAT_CH(x)	(0x1 << x)	

/***********************************************************************************************/
/*		DMACRAWINTERRSTAT : Status of the error interrupt prior to masking(R/O, [7:0])         */
/***********************************************************************************************/
#define	RAW_ERR_INT_STAT_CH(x)	(0x1 << x)

/***********************************************************************************************/
/*		DMACENBLDCHAN : Channel enable status(R/O, [7:0])	        	                       */
/***********************************************************************************************/
#define	ENABLE_STAT_CH(x)		(0x1 << x)	

/***********************************************************************************************/
/*		DMACSOFTBREQ : Software burst request for each device connections                      */
/*					   (R/W, [15:0]) x is connection number.								   */
/***********************************************************************************************/
#define	Soft_BREQ(x)			(0x1 << x)

/***********************************************************************************************/
/*		DMACSOFTSREQ : Software single request for each device connections                     */
/*					   (R/W, [15:0]) x is connection number.								   */
/***********************************************************************************************/
#define	Soft_SREQ(x)			(0x1 << x)

/***********************************************************************************************/
/*		DMACSOFTLBREQ : Software last burst reqeust for each device connections                */
/*					    (R/W, [15:0]) x is connection number.								   */
/***********************************************************************************************/
#define	Soft_LBREQ(x)			(0x1 << x)

/***********************************************************************************************/
/*		DMACSOFTLSREQ : Software last single reqeust for each device connections               */
/*					    (R/W, [15:0]) x is connection number.								   */
/***********************************************************************************************/
#define	Soft_LSREQ(x)			(0x1 << x)	

/***********************************************************************************************/
/*		DMACCFG : DMAC Configuration register 					                               */
/***********************************************************************************************/
#define	DMAC_ENABLE				(0x1 << 0) 	  
#define DMAC_DISABLE		   ~(DMAC_ENABLE) /* DMAC enable: 
											  0 = disabled
											  1 = enabled.
											  This bit is reset to 0.
											  Disabling the DMAC reduces power consumption mode */

#define M1_ENDIAN				(0x1 << 1) /* AHB Master 1 endianness configuration for PL080
											  0 = little-endian mode
											  1 = big-endian mode.
											  This bit is reset to 0.							*/

#define M2_ENDIAN				(0x1 << 2) /* AHB Master 2 endianness configuration for PL080
											  0 = little-endian mode
											  1 = big-endian mode.
											  This bit is reset to 0.							*/

/***********************************************************************************************/
/*		DMACSYNC : Synchronization Register								                       */
/***********************************************************************************************/
#define	DMAC_SYNC(x)			(0x1 << x)	/* DMA synchronization logic for DMA request 
											   signals enabled or disabled. A LOW bit indicates 
											   that the synchronization logic for the request 
											   signals is enabled. A HIGH bit indicates that 
											   the synchronization logic is disabled. 		   */	

/***********************************************************************************************/
/*		DMACCxLLI(x) : Channel Linked List Item Register				                       */
/***********************************************************************************************/
#define SELECT_MASTER1_LLI		(0x0 << 0) /* AHB Master1 select for Next LLI				   */
#define SELECT_MASTER2_LLI		(0x1 << 0) /* AHB Master2 select for Next LLI				   */
#define NONE_LLI				(0x0)
/***********************************************************************************************/
/*		DMACCxCTL0(x) : Channel Control Register										                               */
/***********************************************************************************************/
#define SRC_BSIZE_1		(0x0 << 12)			/* Source Burst size = 1byte */
#define SRC_BSIZE_4		(0x1 << 12)			/* Source Burst size = 4byte */
#define SRC_BSIZE_8		(0x2 << 12)			/* Source Burst size = 8byte */
#define SRC_BSIZE_16	(0x3 << 12)			/* Source Burst size = 16byte */
#define SRC_BSIZE_32	(0x4 << 12)			/* Source Burst size = 32byte */
#define SRC_BSIZE_64	(0x5 << 12)			/* Source Burst size = 64byte */
#define SRC_BSIZE_128	(0x6 << 12)			/* Source Burst size = 128byte */
#define SRC_BSIZE_256	(0x7 << 12)			/* Source Burst size = 256byte */
#define DST_BSIZE_1		(0x0 << 15)			/* Destination Burst size = 1byte */
#define DST_BSIZE_4		(0x1 << 15)			/* Destination Burst size = 4byte */
#define DST_BSIZE_8		(0x2 << 15)			/* Destination Burst size = 8byte */
#define DST_BSIZE_16	(0x3 << 15)			/* Destination Burst size = 16byte */
#define DST_BSIZE_32	(0x4 << 15)			/* Destination Burst size = 32byte */
#define DST_BSIZE_64	(0x5 << 15)			/* Destination Burst size = 64byte */
#define DST_BSIZE_128	(0x6 << 15)			/* Destination Burst size = 128byte */
#define DST_BSIZE_256	(0x7 << 15)			/* Destination Burst size = 256byte */
#define SRC_TWIDTH_8	(0x0 << 18)			/* 8 bit */
#define SRC_TWIDTH_16	(0x1 << 18)			/* 16 bit */
#define SRC_TWIDTH_32	(0x2 << 18)			/* 32 bit */
#define DST_TWIDTH_8	(0x0 << 21)			/* 8 bit */
#define DST_TWIDTH_16	(0x1 << 21)			/* 16 bit */
#define DST_TWIDTH_32	(0x2 << 21)			/* 32 bit */
#define SRC_AHB_SYSTEM	(0x0 << 24)			/* Source      AHB master select(0 : AXI_SYSTEM) */
#define SRC_AHB_PERI	(0x1 << 24)			/* Source      AHB master select(1 : AXI_PERI)   */
#define DST_AHB_SYSTEM	(0x0 << 25)			/* Destination AHB master select(0 : AXI_SYSTEM) */
#define DST_AHB_PERI	(0x1 << 25)			/* Destination AHB master select(1 : AXI_PERI)   */
#define SRC_INC			(0x1 << 26)			/* Source address increment after transfer */
#define DST_INC			(0x1 << 27)			/* Destination address increment after transfer */
#define PROT_ENABLE		(0x1 << 28)			/* Protection */
#define PROT_DISABLE   ~(PROT_ENABLE)
#define TC_INT_ENABLE	(0x1 << 31)			/* Terminal Interrupt Enable */
#define TC_INT_DISABLE ~(TC_INT_EN)

/***********************************************************************************************/
/*		DMACCxCTL1(x) : Channel Control Register										                               */
/***********************************************************************************************/
#define TSFR_SIZE4(x)	(x >> 2)			/* [24:0] ex) Transfer Size(x) = 256Byte -> 0x40 */
#define TSFR_SIZE2(x)	(x >> 1)			/* [24:0] ex) Transfer Size(x) = 256Byte -> 0x40 */

/***********************************************************************************************/
/*		DMACCxCFG(x) : Channel Configuration Register										                               */
/***********************************************************************************************/
#define	CHAN_ENABLE		(0x1 << 0)			/* Enables this DMA Channel */
#define CHAN_DISABLE    (0x0 << 0)			/* Disables this DMA Channel */
#define SRC_CONN(x)		(x << 1)			/* Source peripheral number */
#define DST_CONN(x)		(x << 6)			/* Destination peripheral number */
#define FCTL_DMA_M2M	(0x0 << 11)			/* Flow control(DMA) 	: Memory to Memory */
#define FCTL_DMA_M2P	(0x1 << 11)			/* Flow control(DMA) 	: Memory to Peri. */
#define FCTL_DMA_P2M	(0x2 << 11)			/* Flow control(DMA) 	: Peri.  to Memory */
#define FCTL_DMA_P2P	(0x3 << 11)			/* Flow control(DMA) 	: Peri.  to Peri.  */
#define FCTL_DPERI_P2P	(0x4 << 11)			/* Flow control(D-PERI) : Peri. to Peri. */
#define FCTL_PERI_M2P	(0x5 << 11)			/* Flow control(PERI) 	: Memory to Peri. */
#define FCTL_PERI_P2M	(0x6 << 11)			/* Flow control(PERI) 	: Peri. to Memory */
#define FCTL_SPERI_P2P	(0x7 << 11)			/* Flow control(S-PERI) : Peri to Peri */
#define INT_ERR_MASK	(0x1 << 14)			/* Error interrupt mask */
#define INT_ERR_UMASK  ~(INT_ERR_MASK)		/* Error interupt unmask */
#define INT_TC_MASK		(0x1 << 15)			/* TC interrupt mask */
#define INT_TC_UMASK   ~(INT_TC_MASK)		/* TC interrupt unmask */		
#define LOCK_SET		(0x1 << 16)			/* LOCK transfer enable */
#define LOCK_CLR	   ~(LOCK_SET)			/* LOCK transfer disable */
#define ACTIVE_SET		(0x1 << 17)			/* There is no data in the FIFO of the channle */
#define ACTIVE_CLR	   ~(ACTIVE_SET)		/* The FIFO of the channel has data */
#define HALT_SET		(0x1 << 18)			/* Ignore subsequent source DMA request */
#define HALT_CLR	   ~(HALT_SET)			/* Enable DMA Request */

/***********************************************************************************************/
/*		DMAC information for other device						                               */
/***********************************************************************************************/
#define CONNECTION_DEVICE_NUM		32

/***********************************************************************************************/
/*		DMAC Source(Connection Num)						                               		   */
/***********************************************************************************************/
#define	I2S0_TX		10
#define	I2S0_RX		11

extern spinlock_t  s3c6410_dma_spin_lock;					/* spinlock for only dma */
    
static inline unsigned long s3c6410_dma_lock(void)
{
	unsigned long flags;
	spin_lock_irqsave(&s3c6410_dma_spin_lock, flags);
	return flags;
}

static inline void s3c6410_dma_unlock(unsigned long flags)
{  
	spin_unlock_irqrestore(&s3c6410_dma_spin_lock, flags);
}   

struct dmac_sel {
	const char		*name;
	unsigned long	 dma_sel;
};

/* If you want to inteface with dma controller, should be add in below structure */
static struct dmac_sel s3c6410_dmac_sel[CONNECTION_DEVICE_NUM] = {
	[I2S0_TX] = {
		.name 		= "i2s0-out",
		.dma_sel	= 1 << I2S0_TX,
	},

	[I2S0_RX] = {
		.name 		= "i2s0-in",
		.dma_sel	= 1 << I2S0_RX,
	},	
};

struct dmac_info {
	char			modalias[20];
	unsigned int	dma_port;
	unsigned int	conn_num;
	unsigned int	hw_fifo;
};

struct dmac_conn_info {
	struct   dmac_info	*connection_num;
	unsigned long		array_size;
};

/* structure for making the LLI List */
struct dmac_lli {
	unsigned int src_lli;
	unsigned int dst_lli;
	unsigned int next_lli;
	unsigned int chan_ctrl0;
	unsigned int chan_ctrl1;
};

/* Data Structure of DMA information for each devices */
struct dma_data {
	void				*dma_lli_v;							/* Linked List Item, Virtual address */
	unsigned int		dma_port;							/* DMA Controller port information */
	unsigned int		chan_num;							/* DMA Channel number */
	unsigned int		conn_num;							/* Device & DMA connection information */
	unsigned int		dst_addr;							/* Destination address for DMAC */ 
	unsigned int		src_addr;							/* Source address for DMAC */ 
	unsigned int		lli_addr;							/* LLI address for DMAC */
	unsigned int 		dmac_cfg;							/* Set mode for flow control bit */
	unsigned int		dmac_ctrl0;							/* DMACCxControl0 register value (Burst size, Data width..) */
	unsigned int 		dmac_ctrl1;							/* DMACCxControl1 register value (Transfer Size)*/
	unsigned int		dmac_bytes;							/* Total Transfer size for DMAC*/
	void				(*dma_dev_handler)(void *); /* DMAC interrupt handler pointer for each devices */
	unsigned int		active;								/* DMA active flag */
	void				*private_data;
};

extern int s3c6410_dmac_request(struct dma_data *dmadata, void * id);
extern int s3c6410_dmac_enable(struct dma_data *dmadata);
extern int s3c6410_dmac_disable(struct dma_data *dmadata);
extern int s3c6410_dmac_free(struct dma_data *dmadata);
extern int s3c6410_dmac_halt(struct dma_data *dmadata, unsigned int val);
extern unsigned int s3c6410_dmac_get_base(unsigned int port);

