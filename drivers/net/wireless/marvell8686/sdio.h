/*File sdio.h
 * This file contains the structure definations for the low level driver
 * And the error response related code
 *
 *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
 */

#ifndef __SDIO_H__
#define __SDIO_H__

#include	<linux/spinlock.h>      /* For read write semaphores */
#include	<linux/semaphore.h>
#include	<linux/completion.h>
#include	<asm/dma.h>
#ifdef CONFIG_MARVELL_8686_PROC_FS
	#include 	<linux/proc_fs.h>
#endif

#include 	"os_defs.h"

#ifdef	DEBUG_SDIO_LEVEL2
#ifndef DEBUG_LEVEL1
#define	DEBUG_LEVEL1
#endif
#define	_ENTER() printk(KERN_DEBUG "Enter: %s, %s linux %i\n", __FUNCTION__, \
			__FILE__, __LINE__)
#define	_LEAVE() printk(KERN_DEBUG "Leave: %s, %s linux %i\n", __FUNCTION__, \
			__FILE__, __LINE__)
#else
#define _ENTER()
#define _LEAVE()
#endif

#ifdef	DEBUG_SDIO_LEVEL1
#define	_DBGMSG(x...)		printk(KERN_DEBUG x)
#define	_WARNING(x...)		printk(KERN_DEBUG x)
#else
#define	_DBGMSG(x...)
#define	_WARNING(x...)
#endif

#ifdef DEBUG_SDIO_LEVEL0
#define	_PRINTK(x...)		printk(x)
#define	_ERROR(x...)		printk(KERN_ERR x)
#else
#define	_PRINTK(x...)
#define	_ERROR(x...)
#endif

typedef struct _card_capability
{
	u8 num_of_io_funcs;	    /* Number of i/o functions */
	u8 memory_yes;		    /* Memory present ? */
	u16 rca;		    /* Relative Card Address */
	u32 ocr;		    /* Operation Condition register */
	u16 fnblksz[8];
	u32 cisptr[8];
} card_capability;

typedef struct _dummy_tmpl
{
	int irq_line;
} dummy_tmpl;

typedef struct _sdio_host *mmc_controller_t;

typedef enum _sdio_fsm
{
	SDIO_FSM_IDLE = 1,
	SDIO_FSM_CLK_OFF,
	SDIO_FSM_END_CMD,
	SDIO_FSM_BUFFER_IN_TRANSIT,
	SDIO_FSM_END_BUFFER,
	SDIO_FSM_END_IO,
	SDIO_FSM_END_PRG,
	SDIO_FSM_ERROR
} sdio_fsm_state;

typedef struct _sdio_host
{
	char name[16];
	int bus_width;
} __attribute__ ((aligned)) sdio_ctrller;

typedef struct _sdio_operations
{
	char name[16];
} sdio_operations;

typedef struct _iorw_extended_t
{
	u8 rw_flag;	     /** If 0 command is READ; else if 1 command is WRITE */
	u8 func_num;
	u8 blkmode;
	u8 op_code;
	u32 reg_addr;
	u32 byte_cnt;
	u32 blk_size;
	u8 *buf;
} iorw_extended_t;

#define BUS_INTERFACE_CONTROL_REG 	0x07
#define CARD_CAPABILITY_REG		0x08
#define COMMON_CIS_POINTER_0_REG	0x09
#define COMMON_CIS_POINTER_1_REG	0x0a
#define COMMON_CIS_POINTER_2_REG	0x0b
#define BUS_SUSPEND_REG			0x0c
#define FUNCTION_SELECT_REG		0x0d
#define EXEC_FLAGS_REG			0x0e
#define READY_FLAGS_REG			0x0f

#endif /* __SDIO__H */
