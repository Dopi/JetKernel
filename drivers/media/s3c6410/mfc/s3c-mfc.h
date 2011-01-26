/* mfc/s3c-mfc.h
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
 
#ifndef __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__
#define __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__

#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif


#define IOCTL_MFC_MPEG4_DEC_INIT			(0x00800001)
#define IOCTL_MFC_MPEG4_ENC_INIT			(0x00800002)
#define IOCTL_MFC_MPEG4_DEC_EXE				(0x00800003)
#define IOCTL_MFC_MPEG4_ENC_EXE				(0x00800004)

#define IOCTL_MFC_H264_DEC_INIT				(0x00800005)
#define IOCTL_MFC_H264_ENC_INIT				(0x00800006)
#define IOCTL_MFC_H264_DEC_EXE				(0x00800007)
#define IOCTL_MFC_H264_ENC_EXE				(0x00800008)

#define IOCTL_MFC_H263_DEC_INIT				(0x00800009)
#define IOCTL_MFC_H263_ENC_INIT				(0x0080000A)
#define IOCTL_MFC_H263_DEC_EXE				(0x0080000B)
#define IOCTL_MFC_H263_ENC_EXE				(0x0080000C)

#define IOCTL_MFC_VC1_DEC_INIT				(0x0080000D)
#define IOCTL_MFC_VC1_DEC_EXE				(0x0080000E)

#define IOCTL_MFC_GET_LINE_BUF_ADDR			(0x0080000F)
#define IOCTL_MFC_GET_RING_BUF_ADDR			(0x00800010)
#define IOCTL_MFC_GET_FRAM_BUF_ADDR			(0x00800011)
#define IOCTL_MFC_GET_POST_BUF_ADDR			(0x00800012)
#define IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR		(0x00800013)
#define IOCTL_MFC_GET_CONFIG				(0x00800016)
#define IOCTL_MFC_GET_MPEG4_ASP_PARAM		(0x00800017)

#define IOCTL_MFC_SET_H263_MULTIPLE_SLICE	(0x00800014)
#define IOCTL_MFC_SET_CONFIG				(0x00800015)

#define IOCTL_MFC_GET_DBK_BUF_ADDR			(0x00800018)	// yj

#define IOCTL_MFC_SET_DISP_CONFIG			(0x00800111)
#define IOCTL_MFC_GET_FRAME_SIZE			(0x00800112)
#define IOCTL_MFC_SET_PP_DISP_SIZE			(0x00800113)
#define IOCTL_MFC_SET_DEC_INBUF_TYPE		(0x00800114)

#define IOCTL_VIRT_TO_PHYS					0x12345678

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
#define IOCTL_CACHE_FLUSH_B_FRAME              (0x00800115)
#define IOCTL_MFC_GET_PHY_B_FRAME_BUF_ADDR     (0x00800116)
#define IOCTL_MFC_GET_B_FRAME_BUF_ADDR         (0x00800117)
#endif

typedef struct
{
	int  rotate;
	int  deblockenable;
} MFC_DECODE_OPTIONS;

#define MFCDRV_RET_OK						(0)
#define MFCDRV_RET_ERR_INVALID_PARAM		(-1001)
#define MFCDRV_RET_ERR_HANDLE_INVALIDATED	(-1004)
#define MFCDRV_RET_ERR_OTHERS				(-9001)

#endif /* __SAMSUNG_SYSLSI_APDEV_S3C_MFC_H__ */
