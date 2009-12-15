/* linux/arch/arm/mach-s3c6400/include/mach/dma.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S3C6400 - DMA support
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H __FILE__

#include <linux/sysdev.h>
#include <mach/hardware.h>

/*
 * This is the maximum DMA address(physical address) that can be DMAd to.
 *
 */
#define MAX_DMA_ADDRESS		0x40000000
#define MAX_DMA_TRANSFER_SIZE   0x100000 /* Data Unit is half word  */

/* We use `virtual` dma channels to hide the fact we have only a limited
 * number of DMA channels, and not of all of them (dependant on the device)
 * can be attached to any DMA source. We therefore let the DMA core handle
 * the allocation of hardware channels to clients.
*/

enum dma_ch {
	DMACH_XD0,
	DMACH_XD1,
	DMACH_SDI,
	DMACH_SPI0,
	DMACH_SPI_TX,
	DMACH_SPI_RX,
	DMACH_SPI1,
	DMACH_UART0,
	DMACH_UART1,
	DMACH_UART2,
	DMACH_TIMER,
	DMACH_I2S_IN,
	DMACH_I2S_OUT,
	DMACH_I2S_IN_1,		/* s3c2450 iis_1 rx */
	DMACH_I2S_OUT_1,	/* s3c2450 iis_1 tx */
	DMACH_I2S_V40_IN,
	DMACH_I2S_V40_OUT,
	DMACH_PCM_IN,
	DMACH_PCM_OUT,
	DMACH_MIC_IN,
	DMACH_USB_EP1,
	DMACH_USB_EP2,
	DMACH_USB_EP3,
	DMACH_USB_EP4,
	DMACH_UART0_SRC2,	/* s3c2412 second uart sources */
	DMACH_UART1_SRC2,
	DMACH_UART2_SRC2,
	DMACH_UART3,		/* s3c2443 has extra uart */
	DMACH_UART3_SRC2,
	DMACH_I2S1_IN,		/* S3C6400 */
	DMACH_I2S1_OUT,
	DMACH_SPI0_IN,
	DMACH_SPI0_OUT,
	DMACH_SPI1_IN,
	DMACH_SPI1_OUT,
	DMACH_AC97_PCM_OUT,
	DMACH_AC97_PCM_IN,
	DMACH_AC97_MIC_IN,
	DMACH_ONENAND_IN,
	DMACH_3D_M2M,
	DMACH_MAX,		/* the end entry */
};

#define DMACH_LOW_LEVEL	(1<<28)	/* use this to specifiy hardware ch no */

/* Number of dma channels */
#if defined (CONFIG_CPU_S3C2443)
#define S3C2410_DMA_CHANNELS		(6)

#elif defined (CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
#define S3C2410_DMA_CHANNELS		(8)

#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
/* We have 4 dma controllers - DMA0, DMA1, SDMA0, SDMA1 */
#define S3C_DMA_CONTROLLERS        	(4)
#define S3C_CHANNELS_PER_DMA       	(8)
#define S3C_CANDIDATE_CHANNELS_PER_DMA  (16)
#define S3C2410_DMA_CHANNELS		(S3C_DMA_CONTROLLERS*S3C_CHANNELS_PER_DMA)

#else
#define S3C2410_DMA_CHANNELS		(4)
#endif

/* types */

enum s3c2410_dma_state {
	S3C2410_DMA_IDLE,
	S3C2410_DMA_RUNNING,
	S3C2410_DMA_PAUSED
};


/* enum s3c2410_dma_loadst
 *
 * This represents the state of the DMA engine, wrt to the loaded / running
 * transfers. Since we don't have any way of knowing exactly the state of
 * the DMA transfers, we need to know the state to make decisions on wether
 * we can
 *
 * S3C2410_DMA_NONE
 *
 * There are no buffers loaded (the channel should be inactive)
 *
 * S3C2410_DMA_1LOADED
 *
 * There is one buffer loaded, however it has not been confirmed to be
 * loaded by the DMA engine. This may be because the channel is not
 * yet running, or the DMA driver decided that it was too costly to
 * sit and wait for it to happen.
 *
 * S3C2410_DMA_1RUNNING
 *
 * The buffer has been confirmed running, and not finisged
 *
 * S3C2410_DMA_1LOADED_1RUNNING
 *
 * There is a buffer waiting to be loaded by the DMA engine, and one
 * currently running.
*/

enum s3c2410_dma_loadst {
	S3C2410_DMALOAD_NONE,
	S3C2410_DMALOAD_1LOADED,
	S3C2410_DMALOAD_1RUNNING,
	S3C2410_DMALOAD_1LOADED_1RUNNING,
};

enum s3c2410_dma_buffresult {
	S3C2410_RES_OK,
	S3C2410_RES_ERR,
	S3C2410_RES_ABORT
};

enum s3c2410_dmasrc {
	S3C2410_DMASRC_HW,		/* source is memory */
	S3C2410_DMASRC_MEM,		/* source is hardware */
	S3C_DMA_MEM2MEM,      		/* source is memory - READ/WRITE */
	S3C_DMA_MEM2MEM_P,      	/* source is hardware - READ/WRITE */
	S3C_DMA_PER2PER      		/* source is hardware - READ/WRITE */
};

/* enum s3c2410_chan_op
 *
 * operation codes passed to the DMA code by the user, and also used
 * to inform the current channel owner of any changes to the system state
*/

enum s3c2410_chan_op {
	S3C2410_DMAOP_START,
	S3C2410_DMAOP_STOP,
	S3C2410_DMAOP_PAUSE,
	S3C2410_DMAOP_RESUME,
	S3C2410_DMAOP_FLUSH,
	S3C2410_DMAOP_TIMEOUT,		/* internal signal to handler */
	S3C2410_DMAOP_STARTED,		/* indicate channel started */
};

/* flags */

#define S3C2410_DMAF_SLOW         (1<<0)   /* slow, so don't worry about
					    * waiting for reloads */
#define S3C2410_DMAF_AUTOSTART    (1<<1)   /* auto-start if buffer queued */

/* dma buffer */

struct s3c2410_dma_client {
	char                *name;
};

/* s3c2410_dma_buf_s
 *
 * internally used buffer structure to describe a queued or running
 * buffer.
*/

struct s3c2410_dma_buf;
struct s3c2410_dma_buf {
	struct s3c2410_dma_buf	*next;
	int			 magic;		/* magic */
	int			 size;		/* buffer size in bytes */
	dma_addr_t		 data;		/* start of DMA data */
	dma_addr_t		 ptr;		/* where the DMA got to [1] */
	void			*id;		/* client's id */
};

/* [1] is this updated for both recv/send modes? */

struct s3c2410_dma_chan;

/* s3c2410_dma_cbfn_t
 *
 * buffer callback routine type
*/

typedef void (*s3c2410_dma_cbfn_t)(struct s3c2410_dma_chan *,
				   void *buf, int size,
				   enum s3c2410_dma_buffresult result);

typedef int  (*s3c2410_dma_opfn_t)(struct s3c2410_dma_chan *,
				   enum s3c2410_chan_op );

struct s3c2410_dma_stats {
	unsigned long		loads;
	unsigned long		timeout_longest;
	unsigned long		timeout_shortest;
	unsigned long		timeout_avg;
	unsigned long		timeout_failed;
};

struct s3c2410_dma_map;

/* struct s3c2410_dma_chan
 *
 * full state information for each DMA channel
*/

/*========================== S3C6400 ===========================================*/
typedef struct s3c_dma_controller s3c_dma_controller_t;
struct s3c_dma_controller {
	/* channel state flags and information */
	unsigned char          number;      /* number of this dma channel */
	unsigned char          in_use;      /* channel allocated and how many channel are used */
	unsigned char          irq_claimed; /* irq claimed for channel */
	unsigned char          irq_enabled; /* irq enabled for channel */
	unsigned char          xfer_unit;   /* size of an transfer */

	/* channel state */

	enum s3c2410_dma_state    state;
	enum s3c2410_dma_loadst   load_state;
	struct s3c2410_dma_client  *client;

	/* channel configuration */
	unsigned long          dev_addr;
	unsigned long          load_timeout;
	unsigned int           flags;        /* channel flags */

	/* channel's hardware position and configuration */
	void __iomem           *regs;        /* channels registers */
	void __iomem           *addr_reg;    /* data address register */
	unsigned int           irq;          /* channel irq */
	unsigned long          dcon;         /* default value of DCON */

};

struct s3c2410_dma_chan {
	/* channel state flags and information */
	unsigned char		 number;      /* number of this dma channel */
	unsigned char		 in_use;      /* channel allocated */
	unsigned char		 irq_claimed; /* irq claimed for channel */
	unsigned char		 irq_enabled; /* irq enabled for channel */
	unsigned char		 xfer_unit;   /* size of an transfer */

	/* channel state */

	enum s3c2410_dma_state	 state;
	enum s3c2410_dma_loadst	 load_state;
	struct s3c2410_dma_client *client;

	/* channel configuration */
	enum s3c2410_dmasrc	 source;
	unsigned long		 dev_addr;
	unsigned long		 load_timeout;
	unsigned int		 flags;		/* channel flags */

	struct s3c24xx_dma_map	*map;		/* channel hw maps */

	/* channel's hardware position and configuration */
	void __iomem		*regs;		/* channels registers */
	void __iomem		*addr_reg;	/* data address register */
	unsigned int		 irq;		/* channel irq */
	unsigned long		 dcon;		/* default value of DCON */

	/* driver handles */
	s3c2410_dma_cbfn_t	 callback_fn;	/* buffer done callback */
	s3c2410_dma_opfn_t	 op_fn;		/* channel op callback */

	/* stats gathering */
	struct s3c2410_dma_stats *stats;
	struct s3c2410_dma_stats  stats_store;

	/* buffer list and information */
	struct s3c2410_dma_buf	*curr;		/* current dma buffer */
	struct s3c2410_dma_buf	*next;		/* next buffer to load */
	struct s3c2410_dma_buf	*end;		/* end of queue */

	/* system device */
	struct sys_device	dev;
	
#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410) 
	unsigned int            index;        	/* channel index */
	unsigned int            config_flags;        /* channel flags */
	unsigned int            control_flags;        /* channel flags */
	s3c_dma_controller_t	*dma_con;
#endif
};

/* the currently allocated channel information */
extern struct s3c2410_dma_chan s3c2410_chans[];

/* note, we don't really use dma_device_t at the moment */
typedef unsigned long dma_device_t;


struct s3c_pl080_sg_list {
	unsigned long	uSrcAddr;
	unsigned long	uDstAddr;
	unsigned long	uNextLLI;
	unsigned long	uCxControl0;
	unsigned long	uCxControl1;
};

#define	s3c_pl080_sg_address(sg)	(virt_to_phys(sg))


/* functions --------------------------------------------------------------- */

/* s3c2410_dma_request
 *
 * request a dma channel exclusivley
*/

extern int s3c2410_dma_request(dmach_t channel,
			       struct s3c2410_dma_client *, void *dev);


/* s3c2410_dma_ctrl
 *
 * change the state of the dma channel
*/

extern int s3c2410_dma_ctrl(dmach_t channel, enum s3c2410_chan_op op);


/* s3c2410_dma_setflags
 *
 * set the channel's flags to a given state
*/

extern int s3c2410_dma_setflags(dmach_t channel,
				unsigned int flags);


/* s3c2410_dma_free
 *
 * free the dma channel (will also abort any outstanding operations)
*/

extern int s3c2410_dma_free(dmach_t channel, struct s3c2410_dma_client *);


/* s3c2410_dma_enqueue
 *
 * place the given buffer onto the queue of operations for the channel.
 * The buffer must be allocated from dma coherent memory, or the Dcache/WB
 * drained before the buffer is given to the DMA system.
*/

extern int s3c2410_dma_enqueue(dmach_t channel, void *id,
			       dma_addr_t data, int size);

extern int s3c2410_dma_enqueue_sg(dmach_t channel, void *id,
			       dma_addr_t data, int size, struct s3c_pl080_sg_list *sg_list);

/* s3c2410_dma_config
 *
 * configure the dma channel
*/

extern int s3c2410_dma_config(dmach_t channel, int xferunit, int dcon);

/* s3c2410_dma_devconfig
 *
 * configure the device we're talking to
*/

extern int s3c2410_dma_devconfig(int channel, enum s3c2410_dmasrc source,
				 int hwcfg, unsigned long devaddr);

/* s3c2410_dma_getposition
 *
 * get the position that the dma transfer is currently at
*/

extern int s3c2410_dma_getposition(dmach_t channel,
				   dma_addr_t *src, dma_addr_t *dest);


extern int s3c2410_dma_set_opfn(dmach_t, s3c2410_dma_opfn_t rtn);
extern int s3c2410_dma_set_buffdone_fn(dmach_t, s3c2410_dma_cbfn_t rtn);


/* DMA Register definitions */

#define S3C2410_DMA_DISRC       (0x00)
#define S3C2410_DMA_DISRCC      (0x04)
#define S3C2410_DMA_DIDST       (0x08)
#define S3C2410_DMA_DIDSTC      (0x0C)
#define S3C2410_DMA_DCON        (0x10)
#define S3C2410_DMA_DSTAT       (0x14)
#define S3C2410_DMA_DCSRC       (0x18)
#define S3C2410_DMA_DCDST       (0x1C)
#define S3C2410_DMA_DMASKTRIG   (0x20)
#define S3C2412_DMA_DMAREQSEL	(0x24)
#define S3C2443_DMA_DMAREQSEL	(0x24)

#define S3C2410_DISRCC_INC	(1<<0)
#define S3C2410_DISRCC_APB	(1<<1)

#define S3C2410_DMASKTRIG_STOP   (1<<2)
#define S3C2410_DMASKTRIG_ON     (1<<1)
#define S3C2410_DMASKTRIG_SWTRIG (1<<0)

#define S3C2410_DCON_DEMAND     (0<<31)
#define S3C2410_DCON_HANDSHAKE  (1<<31)
#define S3C2410_DCON_SYNC_PCLK  (0<<30)
#define S3C2410_DCON_SYNC_HCLK  (1<<30)

#define S3C2410_DCON_INTREQ     (1<<29)

#define S3C2410_DCON_CH0_XDREQ0	(0<<24)
#define S3C2410_DCON_CH0_UART0	(1<<24)
#define S3C2410_DCON_CH0_SDI	(2<<24)
#define S3C2410_DCON_CH0_TIMER	(3<<24)
#define S3C2410_DCON_CH0_USBEP1	(4<<24)

#define S3C2410_DCON_CH1_XDREQ1	(0<<24)
#define S3C2410_DCON_CH1_UART1	(1<<24)
#define S3C2410_DCON_CH1_I2SSDI	(2<<24)
#define S3C2410_DCON_CH1_SPI	(3<<24)
#define S3C2410_DCON_CH1_USBEP2	(4<<24)

#define S3C2410_DCON_CH2_I2SSDO	(0<<24)
#define S3C2410_DCON_CH2_I2SSDI	(1<<24)
#define S3C2410_DCON_CH2_SDI	(2<<24)
#define S3C2410_DCON_CH2_TIMER	(3<<24)
#define S3C2410_DCON_CH2_USBEP3	(4<<24)

#define S3C2410_DCON_CH3_UART2	(0<<24)
#define S3C2410_DCON_CH3_SDI	(1<<24)
#define S3C2410_DCON_CH3_SPI	(2<<24)
#define S3C2410_DCON_CH3_TIMER	(3<<24)
#define S3C2410_DCON_CH3_USBEP4	(4<<24)

#define S3C2410_DCON_SRCSHIFT   (24)
#define S3C2410_DCON_SRCMASK	(7<<24)

#define S3C2410_DCON_BYTE       (0<<20)
#define S3C2410_DCON_HALFWORD   (1<<20)
#define S3C2410_DCON_WORD       (2<<20)

#define S3C2410_DCON_AUTORELOAD (0<<22)
#define S3C2410_DCON_NORELOAD   (1<<22)
#define S3C2410_DCON_HWTRIG     (1<<23)

#ifdef CONFIG_CPU_S3C2440
#define S3C2440_DIDSTC_CHKINT	(1<<2)

#define S3C2440_DCON_CH0_I2SSDO	(5<<24)
#define S3C2440_DCON_CH0_PCMIN	(6<<24)

#define S3C2440_DCON_CH1_PCMOUT	(5<<24)
#define S3C2440_DCON_CH1_SDI	(6<<24)

#define S3C2440_DCON_CH2_PCMIN	(5<<24)
#define S3C2440_DCON_CH2_MICIN	(6<<24)

#define S3C2440_DCON_CH3_MICIN	(5<<24)
#define S3C2440_DCON_CH3_PCMOUT	(6<<24)
#endif

#ifdef CONFIG_CPU_S3C2412

#define S3C2412_DMAREQSEL_SRC(x)	((x)<<1)

#define S3C2412_DMAREQSEL_HW		(1)

#define S3C2412_DMAREQSEL_SPI0TX	S3C2412_DMAREQSEL_SRC(0)
#define S3C2412_DMAREQSEL_SPI0RX	S3C2412_DMAREQSEL_SRC(1)
#define S3C2412_DMAREQSEL_SPI1TX	S3C2412_DMAREQSEL_SRC(2)
#define S3C2412_DMAREQSEL_SPI1RX	S3C2412_DMAREQSEL_SRC(3)
#define S3C2412_DMAREQSEL_I2STX		S3C2412_DMAREQSEL_SRC(4)
#define S3C2412_DMAREQSEL_I2SRX		S3C2412_DMAREQSEL_SRC(5)
#define S3C2412_DMAREQSEL_TIMER		S3C2412_DMAREQSEL_SRC(9)
#define S3C2412_DMAREQSEL_SDI		S3C2412_DMAREQSEL_SRC(10)
#define S3C2412_DMAREQSEL_USBEP1	S3C2412_DMAREQSEL_SRC(13)
#define S3C2412_DMAREQSEL_USBEP2	S3C2412_DMAREQSEL_SRC(14)
#define S3C2412_DMAREQSEL_USBEP3	S3C2412_DMAREQSEL_SRC(15)
#define S3C2412_DMAREQSEL_USBEP4	S3C2412_DMAREQSEL_SRC(16)
#define S3C2412_DMAREQSEL_XDREQ0	S3C2412_DMAREQSEL_SRC(17)
#define S3C2412_DMAREQSEL_XDREQ1	S3C2412_DMAREQSEL_SRC(18)
#define S3C2412_DMAREQSEL_UART0_0	S3C2412_DMAREQSEL_SRC(19)
#define S3C2412_DMAREQSEL_UART0_1	S3C2412_DMAREQSEL_SRC(20)
#define S3C2412_DMAREQSEL_UART1_0	S3C2412_DMAREQSEL_SRC(21)
#define S3C2412_DMAREQSEL_UART1_1	S3C2412_DMAREQSEL_SRC(22)
#define S3C2412_DMAREQSEL_UART2_0	S3C2412_DMAREQSEL_SRC(23)
#define S3C2412_DMAREQSEL_UART2_1	S3C2412_DMAREQSEL_SRC(24)

#endif

#define S3C2443_DMAREQSEL_SRC(x)	((x)<<1)

#define S3C2443_DMAREQSEL_HW		(1)

#define S3C2443_DMAREQSEL_SPI0TX	S3C2443_DMAREQSEL_SRC(0)
#define S3C2443_DMAREQSEL_SPI0RX	S3C2443_DMAREQSEL_SRC(1)
#define S3C2443_DMAREQSEL_SPI1TX	S3C2443_DMAREQSEL_SRC(2)
#define S3C2443_DMAREQSEL_SPI1RX	S3C2443_DMAREQSEL_SRC(3)
#define S3C2443_DMAREQSEL_I2STX		S3C2443_DMAREQSEL_SRC(4)
#define S3C2443_DMAREQSEL_I2SRX		S3C2443_DMAREQSEL_SRC(5)
#define S3C2443_DMAREQSEL_TIMER		S3C2443_DMAREQSEL_SRC(9)
#define S3C2443_DMAREQSEL_SDI		S3C2443_DMAREQSEL_SRC(10)
#define S3C2443_DMAREQSEL_XDREQ0	S3C2443_DMAREQSEL_SRC(17)
#define S3C2443_DMAREQSEL_XDREQ1	S3C2443_DMAREQSEL_SRC(18)
#define S3C2443_DMAREQSEL_UART0_0	S3C2443_DMAREQSEL_SRC(19)
#define S3C2443_DMAREQSEL_UART0_1	S3C2443_DMAREQSEL_SRC(20)
#define S3C2443_DMAREQSEL_UART1_0	S3C2443_DMAREQSEL_SRC(21)
#define S3C2443_DMAREQSEL_UART1_1	S3C2443_DMAREQSEL_SRC(22)
#define S3C2443_DMAREQSEL_UART2_0	S3C2443_DMAREQSEL_SRC(23)
#define S3C2443_DMAREQSEL_UART2_1	S3C2443_DMAREQSEL_SRC(24)
#define S3C2443_DMAREQSEL_UART3_0	S3C2443_DMAREQSEL_SRC(25)
#define S3C2443_DMAREQSEL_UART3_1	S3C2443_DMAREQSEL_SRC(26)
#define S3C2443_DMAREQSEL_PCMOUT	S3C2443_DMAREQSEL_SRC(27)
#define S3C2443_DMAREQSEL_PCMIN 	S3C2443_DMAREQSEL_SRC(28)
#define S3C2443_DMAREQSEL_MICIN		S3C2443_DMAREQSEL_SRC(29)


/*=================================================*/
/*   DMA Register Definitions for S3C6400          */

#define S3C_DMAC_INT_STATUS   		(0x00)
#define S3C_DMAC_INT_TCSTATUS   	(0x04)
#define S3C_DMAC_INT_TCCLEAR   		(0x08)
#define S3C_DMAC_INT_ERRORSTATUS   	(0x0c)
#define S3C_DMAC_INT_ERRORCLEAR   	(0x10)
#define S3C_DMAC_RAW_INTTCSTATUS   	(0x14)
#define S3C_DMAC_RAW_INTERRORSTATUS   	(0x18)
#define S3C_DMAC_ENBLD_CHANNELS	   	(0x1c)
#define S3C_DMAC_SOFTBREQ	   	(0x20)
#define S3C_DMAC_SOFTSREQ	   	(0x24)
#define S3C_DMAC_SOFTLBREQ	   	(0x28)
#define S3C_DMAC_SOFTLSREQ	   	(0x2c)
#define S3C_DMAC_CONFIGURATION   	(0x30)
#define S3C_DMAC_SYNC   		(0x34)

#define S3C_DMAC_CxSRCADDR   		(0x00)
#define S3C_DMAC_CxDESTADDR   		(0x04)
#define S3C_DMAC_CxLLI   		(0x08)
#define S3C_DMAC_CxCONTROL0   		(0x0C)
#define S3C_DMAC_CxCONTROL1   		(0x10)
#define S3C_DMAC_CxCONFIGURATION   	(0x14)

#define S3C_DMAC_C0SRCADDR   		(0x100)
#define S3C_DMAC_C0DESTADDR   		(0x104)
#define S3C_DMAC_C0LLI   		(0x108)
#define S3C_DMAC_C0CONTROL0   		(0x10C)
#define S3C_DMAC_C0CONTROL1   		(0x110)
#define S3C_DMAC_C0CONFIGURATION   	(0x114)

#define S3C_DMAC_C1SRCADDR   		(0x120)
#define S3C_DMAC_C1DESTADDR   		(0x124)
#define S3C_DMAC_C1LLI   		(0x128)
#define S3C_DMAC_C1CONTROL0   		(0x12C)
#define S3C_DMAC_C1CONTROL1   		(0x130)
#define S3C_DMAC_C1CONFIGURATION   	(0x134)

#define S3C_DMAC_C2SRCADDR   		(0x140)
#define S3C_DMAC_C2DESTADDR   		(0x144)
#define S3C_DMAC_C2LLI   		(0x148)
#define S3C_DMAC_C2CONTROL0   		(0x14C)
#define S3C_DMAC_C2CONTROL1   		(0x150)
#define S3C_DMAC_C2CONFIGURATION   	(0x154)

#define S3C_DMAC_C3SRCADDR   		(0x160)
#define S3C_DMAC_C3DESTADDR   		(0x164)
#define S3C_DMAC_C3LLI   		(0x168)
#define S3C_DMAC_C3CONTROL0   		(0x16C)
#define S3C_DMAC_C3CONTROL1   		(0x170)
#define S3C_DMAC_C3CONFIGURATION   	(0x174)

#define S3C_DMAC_C4SRCADDR   		(0x180)
#define S3C_DMAC_C4DESTADDR   		(0x184)
#define S3C_DMAC_C4LLI   		(0x188)
#define S3C_DMAC_C4CONTROL0   		(0x18C)
#define S3C_DMAC_C4CONTROL1   		(0x190)
#define S3C_DMAC_C4CONFIGURATION   	(0x194)

#define S3C_DMAC_C5SRCADDR   		(0x1A0)
#define S3C_DMAC_C5DESTADDR   		(0x1A4)
#define S3C_DMAC_C5LLI   		(0x1A8)
#define S3C_DMAC_C5CONTROL0   		(0x1AC)
#define S3C_DMAC_C5CONTROL1   		(0x1B0)
#define S3C_DMAC_C5CONFIGURATION   	(0x1B4)

#define S3C_DMAC_C6SRCADDR   		(0x1C0)
#define S3C_DMAC_C6DESTADDR   		(0x1C4)
#define S3C_DMAC_C6LLI   		(0x1C8)
#define S3C_DMAC_C6CONTROL0   		(0x1CC)
#define S3C_DMAC_C6CONTROL1   		(0x1D0)
#define S3C_DMAC_C6CONFIGURATION   	(0x1D4)

#define S3C_DMAC_C7SRCADDR   		(0x1E0)
#define S3C_DMAC_C7DESTADDR   		(0x1E4)
#define S3C_DMAC_C7LLI   		(0x1E8)
#define S3C_DMAC_C7CONTROL0   		(0x1EC)
#define S3C_DMAC_C7CONTROL1   		(0x1F0)
#define S3C_DMAC_C7CONFIGURATION   	(0x1F4)

/*DMACConfiguration(0x30)*/
#define S3C_DMA_CONTROLLER_ENABLE 	(1<<0)		

/*DMACCxControl0 : Channel control register 0*/
#define S3C_DMACONTROL_TC_INT_ENABLE 	(1<<31)	
#define S3C_DMACONTROL_DEST_NO_INC	(0<<27)	
#define S3C_DMACONTROL_DEST_INC		(1<<27)
#define S3C_DMACONTROL_SRC_NO_INC	(0<<26)
#define S3C_DMACONTROL_SRC_INC		(1<<26)
#define S3C_DMACONTROL_DEST_AXI_SPINE	(0<<25)
#define S3C_DMACONTROL_DEST_AXI_PERI	(1<<25)
#define S3C_DMACONTROL_SRC_AXI_SPINE	(0<<24)
#define S3C_DMACONTROL_SRC_AXI_PERI	(1<<24)
#define S3C_DMACONTROL_DEST_WIDTH_BYTE	(0<<21)
#define S3C_DMACONTROL_DEST_WIDTH_HWORD	(1<<21)
#define S3C_DMACONTROL_DEST_WIDTH_WORD	(2<<21)
#define S3C_DMACONTROL_SRC_WIDTH_BYTE	(0<<18)
#define S3C_DMACONTROL_SRC_WIDTH_HWORD	(1<<18)
#define S3C_DMACONTROL_SRC_WIDTH_WORD	(2<<18)

#define S3C_DMACONTROL_DBSIZE_1		(0<<15)
#define S3C_DMACONTROL_DBSIZE_4		(1<<15)
#define S3C_DMACONTROL_DBSIZE_8		(2<<15)
#define S3C_DMACONTROL_DBSIZE_16	(3<<15)
#define S3C_DMACONTROL_DBSIZE_32	(4<<15)
#define S3C_DMACONTROL_DBSIZE_64	(5<<15)
#define S3C_DMACONTROL_DBSIZE_128	(6<<15)
#define S3C_DMACONTROL_DBSIZE_256	(7<<15)

#define S3C_DMACONTROL_SBSIZE_1		(0<<12)
#define S3C_DMACONTROL_SBSIZE_4		(1<<12)
#define S3C_DMACONTROL_SBSIZE_8		(2<<12)
#define S3C_DMACONTROL_SBSIZE_16	(3<<12)
#define S3C_DMACONTROL_SBSIZE_32	(4<<12)
#define S3C_DMACONTROL_SBSIZE_64	(5<<12)
#define S3C_DMACONTROL_SBSIZE_128	(6<<12)
#define S3C_DMACONTROL_SBSIZE_256	(7<<12)


/*Channel configuration register, DMACCxConfiguration*/
#define S3C_DMACONFIG_HALT		(1<<18) /*The contents of the channels FIFO are drained*/
#define S3C_DMACONFIG_ACTIVE		(1<<17) /*Check channel fifo has data or not*/
#define S3C_DMACONFIG_LOCK		(1<<16)
#define S3C_DMACONFIG_TCMASK	 	(1<<15) /*Terminal count interrupt mask*/
#define S3C_DMACONFIG_ERRORMASK	 	(1<<14) /*Interrup error mask*/
#define S3C_DMACONFIG_FLOWCTRL_MEM2MEM	(0<<11)
#define S3C_DMACONFIG_FLOWCTRL_MEM2PER	(1<<11)
#define S3C_DMACONFIG_FLOWCTRL_PER2MEM	(2<<11)
#define S3C_DMACONFIG_FLOWCTRL_PER2PER	(3<<11)
#define S3C_DMACONFIG_ONENANDMODEDST	(1<<10)	/* Reserved: OneNandModeDst */
#define S3C_DMACONFIG_DESTPERIPHERAL(x)	((x)<<6)
#define S3C_DMACONFIG_ONENANDMODESRC	(1<<5)	/* Reserved: OneNandModeSrc */
#define S3C_DMACONFIG_SRCPERIPHERAL(x)	((x)<<1)
#define S3C_DMACONFIG_CHANNEL_ENABLE	(1<<0)

/* DMAC0 DMA request sources */
#define S3C_DMA0_UART0CH0	0
#define S3C_DMA0_UART0CH1	1
#define S3C_DMA0_UART1CH0	2
#define S3C_DMA0_UART1CH1	3
#define S3C_DMA0_ONENAND_RX	3	/* Memory to Memory DMA */
#define S3C_DMA0_UART2CH0	4
#define S3C_DMA0_UART2CH1	5
#define S3C_DMA0_UART3CH0	6
#define S3C_DMA0_UART3CH1	7
#define S3C_DMA0_PCM0_TX	8
#define S3C_DMA0_PCM0_RX	9
#define S3C_DMA0_I2S0_TX	10
#define S3C_DMA0_I2S0_RX	11
#define S3C_DMA0_SPI0_TX	12
#define S3C_DMA0_SPI0_RX	13
#define S3C_DMA0_HSI_TX		14
#define S3C_DMA0_HSI_RX		15

/* DMAC1 DMA request sources */
#define S3C_DMA1_PCM1_TX	0
#define S3C_DMA1_PCM1_RX	1
#define S3C_DMA1_I2S1_TX	2
#define S3C_DMA1_I2S1_RX	3
#define S3C_DMA1_SPI1_TX	4
#define S3C_DMA1_SPI1_RX	5
#define S3C_DMA1_AC97_PCMOUT	6
#define S3C_DMA1_AC97_PCMIN	7
#define S3C_DMA1_AC97_MICIN	8
#define S3C_DMA1_PWM		9
#define S3C_DMA1_IRDA		10
#define S3C_DMA1_EXT		11

#define S3C_DMA1		16

#define S3C_DEST_SHIFT 		6
#define S3C_SRC_SHIFT 		1


//#define S3C_DMAC_CSRCADDR(ch)   	S3C_DMAC_C##ch##SRCADDR
#define S3C_DMAC_CSRCADDR(ch)   	(S3C_DMAC_C0SRCADDR+ch*0x20)
#define S3C_DMAC_CDESTADDR(ch)   	(S3C_DMAC_C0DESTADDR+ch*0x20)
#define S3C_DMAC_CLLI(ch)   		(S3C_DMAC_C0LLI+ch*0x20)
#define S3C_DMAC_CCONTROL0(ch)   	(S3C_DMAC_C0CONTROL0+ch*0x20)
#define S3C_DMAC_CCONTROL1(ch)   	(S3C_DMAC_C0CONTROL1+ch*0x20)
#define S3C_DMAC_CCONFIGURATION(ch)   	(S3C_DMAC_C0CONFIGURATION+ch*0x20)

#endif /* __ASM_ARCH_IRQ_H */
