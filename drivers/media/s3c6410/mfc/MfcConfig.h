/* mfc/MFC_Instance.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C MFC driver
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
 */
#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__


#include <linux/version.h>
#include <asm/memory.h>
#include <mach/memory.h>
#include <mach/hardware.h>
#include <plat/reserved_mem.h>


//#define ZERO_COPY

#ifdef ZERO_COPY
#define ZERO_COPY_HDR_SIZE 32
#else
#define ZERO_COPY_HDR_SIZE 0
#endif

// Physical Base Address for the MFC Host I/F Registers
#define S3C6400_BASEADDR_MFC_SFR			0x7e002000


// Physical Base Address for the MFC Shared Buffer
//   Shared Buffer = {CODE_BUF, WORK_BUF, PARA_BUF}
#define S3C6400_BASEADDR_MFC_BITPROC_BUF	MFC_RESERVED_MEM_START


// Physical Base Address for the MFC Data Buffer
//   Data Buffer = {STRM_BUF, FRME_BUF}
#define S3C6400_BASEADDR_MFC_DATA_BUF		(MFC_RESERVED_MEM_START + 0x116000)



// Physical Base Address for the MFC Host I/F Registers
#define S3C6400_BASEADDR_POST_SFR			0x77000000



//////////////////////////////////
/////                        /////
/////    MFC BITPROC_BUF     /////
/////                        /////
//////////////////////////////////
//the following three buffers have fixed size
//firware buffer is to download boot code and firmware code
#define MFC_CODE_BUF_SIZE	81920	// It is fixed depending on the MFC'S FIRMWARE CODE (refer to 'Prism_S_V133.h' file)

//working buffer is uded for video codec operations by MFC
#define MFC_WORK_BUF_SIZE	1048576 // 1024KB = 1024 * 1024


//Parameter buffer is allocated to store yuv frame address of output frame buffer.
#define MFC_PARA_BUF_SIZE	8192  //Stores the base address of Y , Cb , Cr for each decoded frame

#define MFC_BITPROC_BUF_SIZE		\
							(	MFC_CODE_BUF_SIZE + \
								MFC_PARA_BUF_SIZE    + \
								MFC_WORK_BUF_SIZE)


///////////////////////////////////
/////                         /////
/////      MFC DATA_BUF       /////
/////                         /////
///////////////////////////////////
#define MFC_NUM_INSTANCES_MAX	3	// MFC Driver supports 4 instances MAX.

// Determine if 'Post Rotate Mode' is enabled.
// If it is enabled, the memory size of SD YUV420(720x576x1.5 bytes) is required more.
// In case of linux driver, reserved buffer size will be changed.
#define MFC_ROTATE_ENABLE		1

#define MFC_LINE_RING_SHARE		1	// Determine if the LINE_BUF is shared with RING_BUF

#if (MFC_LINE_RING_SHARE == 1)

/*
 * stream buffer size must be a multiple of 512bytes 
 * becasue minimun data transfer unit between stream buffer and internal bitstream handling block 
 * in MFC core is 512bytes
 */
#define MFC_LINE_BUF_SIZE_PER_INSTANCE		(307200)


#define MFC_LINE_BUF_SIZE		(MFC_NUM_INSTANCES_MAX * MFC_LINE_BUF_SIZE_PER_INSTANCE)
#define MFC_RING_BUF_SIZE		((307200)  *  3)		// 768,000 Bytes
#define MFC_FRAM_BUF_SIZE		(720*480*3*4)


#define MFC_RING_BUF_PARTUNIT_SIZE			(MFC_RING_BUF_SIZE / 3)	// RING_BUF consists of 3 PARTs.

#define MFC_STRM_BUF_SIZE		(MFC_LINE_BUF_SIZE)

/*
  * If Mp4DbkOn is enabled, the memory size of DATA_BUF is required more.
  * 
  * 2009.5.12 by yj (yunji.kim@samsung.com)
  */
#define MFC_DBK_BUF_SIZE		((176*144*3) >> 1)
#define MFC_DATA_BUF_SIZE		(MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE + MFC_DBK_BUF_SIZE)	

#elif (MFC_LINE_RING_SHARE == 0)


/*
 * stream buffer size must be a multiple of 512bytes 
 * becasue minimun data transfer unit between stream buffer and internal bitstream handling block 
 * in MFC core is 512bytes
 */
#define MFC_LINE_BUF_SIZE_PER_INSTANCE		(307200)


#define MFC_LINE_BUF_SIZE		(MFC_NUM_INSTANCES_MAX * MFC_LINE_BUF_SIZE_PER_INSTANCE)

#define MFC_RING_BUF_SIZE		((307200)  *  3)		// 768,000 Bytes
//#define MFC_FRAM_BUF_SIZE		(3732480)
#define MFC_FRAM_BUF_SIZE		(720*480*3*4)

#define MFC_RING_BUF_PARTUNIT_SIZE			(MFC_RING_BUF_SIZE / 3)	// RING_BUF consists of 3 PARTs.

#define MFC_STRM_BUF_SIZE		(MFC_LINE_BUF_SIZE + MFC_RING_BUF_SIZE)

/*
  * If Mp4DbkOn is enabled, the memory size of DATA_BUF is required more.
  * 
  * 2009.5.12 by yj (yunji.kim@samsung.com)
  */
#define MFC_DBK_BUF_SIZE		((176*144*3) >> 1)
#define MFC_DATA_BUF_SIZE		(MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE + MFC_DBK_BUF_SIZE)	

#endif

#if (MFC_RING_BUF_PARTUNIT_SIZE & 0x1FF)
#error "MFC_RING_BUF_PARTUNIT_SIZE value must be 512-byte aligned."
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__ */
