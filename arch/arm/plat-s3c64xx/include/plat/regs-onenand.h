/* arch/arm/plat-s3c/include/plat/regs-onenand.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef ___ASM_ARCH_REGS_ONENAND_H
#define ___ASM_ARCH_REGS_ONENAND_H

#include <plat/map-base.h>

/***************************************************************************/
/* ONENAND Registers for S3C2443/2450/S3C6400/6410 */
#define S3C_ONENANDREG(x)	((x) + S3C_VA_ONENAND)
#define S3C_ONENANDREG2(x)	((x) + (0x80000) + S3C_VA_ONENAND)

#define S3C_MEM_CFG0		S3C_ONENANDREG(0x00)	/* Bank0 Memory Device Configuration Register */
#define S3C_BURST_LEN0		S3C_ONENANDREG(0x10)	/* Bank0 Burst Length Register */
#define S3C_MEM_RESET0		S3C_ONENANDREG(0x20)	/* Bank0 Memory Reset Register */
#define S3C_INT_ERR_STAT0	S3C_ONENANDREG(0x30)	/* Bank0 Interrupt Error Status Register */
#define S3C_INT_ERR_MASK0	S3C_ONENANDREG(0x40)	/* Bank0 Interrupt Error Mask Register */
#define S3C_INT_ERR_ACK0	S3C_ONENANDREG(0x50)	/* Bank0 Interrupt Error Acknowledge Register */
#define S3C_ECC_ERR_STAT0	S3C_ONENANDREG(0x60)	/* Bank0 ECC Error Status Register */
#define S3C_MANUFACT_ID0	S3C_ONENANDREG(0x70)	/* Bank0 Manufacturer ID Register */
#define S3C_DEVICE_ID0		S3C_ONENANDREG(0x80)	/* Bank0 Device ID Register */
#define S3C_DATA_BUF_SIZE0	S3C_ONENANDREG(0x90)	/* Bank0 Data Buffer Size Register */
#define S3C_BOOT_BUF_SIZE0   	S3C_ONENANDREG(0xA0)	/* Bank0 Boot Buffer Size Register */
#define S3C_BUF_AMOUNT0      	S3C_ONENANDREG(0xB0)	/* Bank0 Amount of Buffer Register */
#define S3C_TECH0            	S3C_ONENANDREG(0xC0)	/* Bank0 Technology Register */
#define S3C_FBA_WIDTH0       	S3C_ONENANDREG(0xD0)	/* Bank0 FBA Width Register */
#define S3C_FPA_WIDTH0       	S3C_ONENANDREG(0xE0)	/* Bank0 FPA Width Register */
#define S3C_FSA_WIDTH0       	S3C_ONENANDREG(0xF0)	/* Bank0 FSA Width Register */
#define S3C_REVISION0        	S3C_ONENANDREG(0x100)	/* Bank0 Revision Register */
#define S3C_DATARAM00        	S3C_ONENANDREG(0x110)	/* Bank0 DataRAM0 Code Register */
#define S3C_DATARAM10        	S3C_ONENANDREG(0x120)	/* Bank0 DataRAM1 Code Register */
#define S3C_SYNC_MODE0       	S3C_ONENANDREG(0x130)	/* Bank0 Synchronous Mode Register */
#define S3C_TRANS_SPARE0     	S3C_ONENANDREG(0x140)	/* Bank0 Transfer Size Register */
#define S3C_DBS_DFS_WIDTH0  	S3C_ONENANDREG(0x160)	/* Bank0 DBS_DFS width Register */
#define S3C_PAGE_CNT0        	S3C_ONENANDREG(0x170)	/* Bank0 Page Count Register */
#define S3C_ERR_PAGE_ADDR0   	S3C_ONENANDREG(0x180)	/* Bank0 Error Page Address Register */
#define S3C_BURST_RD_LAT0    	S3C_ONENANDREG(0x190)	/* Bank0 Burst Read Latency Register */
#define S3C_INT_PIN_ENABLE0  	S3C_ONENANDREG(0x1A0)	/* Bank0 Interrupt Pin Enable Register */
#define S3C_INT_MON_CYC0     	S3C_ONENANDREG(0x1B0)	/* Bank0 Interrupt Monitor Cycle Count Register */
#define S3C_ACC_CLOCK0       	S3C_ONENANDREG(0x1C0)	/* Bank0 Access Clock Register */
#define S3C_SLOW_RD_PATH0    	S3C_ONENANDREG(0x1D0)	/* Bank0 Slow Read Path Register */
#define S3C_ERR_BLK_ADDR0    	S3C_ONENANDREG(0x1E0)	/* Bank0 Error Block Address Register */
#define S3C_FLASH_VER_ID0    	S3C_ONENANDREG(0x1F0)	/* Bank0 Flash Version ID Register */
#define S3C_FLASH_AUX_CNTRL0 	S3C_ONENANDREG(0x300)	/* Bank0 Flash Auxiliary control register */
#define S3C_FLASH_AFIFO_CNT0 	S3C_ONENANDREG(0x310)	/* Number of data in asynchronous FIFO in flash controller 0. */

#define S3C_MEM_CFG1		S3C_ONENANDREG2(0x00)	/* Bank1 Memory Device Configuration Register */
#define S3C_BURST_LEN1		S3C_ONENANDREG2(0x10)	/* Bank1 Burst Length Register */
#define S3C_MEM_RESET1		S3C_ONENANDREG2(0x20)	/* Bank1 Memory Reset Register */
#define S3C_INT_ERR_STAT1	S3C_ONENANDREG2(0x30)	/* Bank1 Interrupt Error Status Register */
#define S3C_INT_ERR_MASK1	S3C_ONENANDREG2(0x40)	/* Bank1 Interrupt Error Mask Register */
#define S3C_INT_ERR_ACK1	S3C_ONENANDREG2(0x50)	/* Bank1 Interrupt Error Acknowledge Register */
#define S3C_ECC_ERR_STAT1	S3C_ONENANDREG2(0x60)	/* Bank1 ECC Error Status Register */
#define S3C_MANUFACT_ID1	S3C_ONENANDREG2(0x70)	/* Bank1 Manufacturer ID Register */
#define S3C_DEVICE_ID1		S3C_ONENANDREG2(0x80)	/* Bank1 Device ID Register */
#define S3C_DATA_BUF_SIZE1	S3C_ONENANDREG2(0x90)	/* Bank1 Data Buffer Size Register */
#define S3C_BOOT_BUF_SIZE1   	S3C_ONENANDREG2(0xA0)	/* Bank1 Boot Buffer Size Register */
#define S3C_BUF_AMOUNT1      	S3C_ONENANDREG2(0xB0)	/* Bank1 Amount of Buffer Register */
#define S3C_TECH1            	S3C_ONENANDREG2(0xC0)	/* Bank1 Technology Register */
#define S3C_FBA_WIDTH1       	S3C_ONENANDREG2(0xD0)	/* Bank1 FBA Width Register */
#define S3C_FPA_WIDTH1       	S3C_ONENANDREG2(0xE0)	/* Bank1 FPA Width Register */
#define S3C_FSA_WIDTH1       	S3C_ONENANDREG2(0xF0)	/* Bank1 FSA Width Register */
#define S3C_REVISION1        	S3C_ONENANDREG2(0x100)	/* Bank1 Revision Register */
#define S3C_DATARAM01        	S3C_ONENANDREG2(0x110)	/* Bank1 DataRAM0 Code Register */
#define S3C_DATARAM11        	S3C_ONENANDREG2(0x120)	/* Bank1 DataRAM1 Code Register */
#define S3C_SYNC_MODE1       	S3C_ONENANDREG2(0x130)	/* Bank1 Synchronous Mode Register */
#define S3C_TRANS_SPARE1     	S3C_ONENANDREG2(0x140)	/* Bank1 Transfer Size Register */
#define S3C_DBS_DFS_WIDTH1  	S3C_ONENANDREG2(0x160)	/* Bank1 DBS_DFS width Register */
#define S3C_PAGE_CNT1        	S3C_ONENANDREG2(0x170)	/* Bank1 Page Count Register */
#define S3C_ERR_PAGE_ADDR1   	S3C_ONENANDREG2(0x180)	/* Bank1 Error Page Address Register */
#define S3C_BURST_RD_LAT1    	S3C_ONENANDREG2(0x190)	/* Bank1 Burst Read Latency Register */
#define S3C_INT_PIN_ENABLE1  	S3C_ONENANDREG2(0x1A0)	/* Bank1 Interrupt Pin Enable Register */
#define S3C_INT_MON_CYC1     	S3C_ONENANDREG2(0x1B0)	/* Bank1 Interrupt Monitor Cycle Count Register */
#define S3C_ACC_CLOCK1       	S3C_ONENANDREG2(0x1C0)	/* Bank1 Access Clock Register */
#define S3C_SLOW_RD_PATH1    	S3C_ONENANDREG2(0x1D0)	/* Bank1 Slow Read Path Register */
#define S3C_ERR_BLK_ADDR1    	S3C_ONENANDREG2(0x1E0)	/* Bank1 Error Block Address Register */
#define S3C_FLASH_VER_ID1    	S3C_ONENANDREG2(0x1F0)	/* Bank1 Flash Version ID Register */
#define S3C_FLASH_AUX_CNTRL1 	S3C_ONENANDREG2(0x300)	/* Bank1 Flash Auxiliary control register */
#define S3C_FLASH_AFIFO_CNT1 	S3C_ONENANDREG2(0x310)	/* Number of data in asynchronous FIFO in flash controller 1. */

#endif /*  __ASM_ARCH_REGS_ONENAND_H */
