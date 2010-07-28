/* 
 * drivers/media/video/samsung/mfc40/s3c_mfc_memory.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_MEMORY_H_
#define _S3C_MFC_MEMORY_H_

#include "s3c_mfc_common.h"
#include "s3c_mfc_types.h"

#ifdef CONFIG_VIDEO_MFC_MAX_INSTANCE
#define MFC_MAX_INSTANCE_NUM (CONFIG_VIDEO_MFC_MAX_INSTANCE + 1)
#endif

#define MFC_MAX_FW_NUM		(8)
#define MFC_MAX_WIDTH		(1280)
#define MFC_MAX_HEIGHT		(720)

/* All buffer size have to be aligned to 64 */
#define FIRMWARE_CODE_SIZE	(98304) /* 98,304 byte */
#define VSP_BUF_SIZE		(393216) /* 393,216 byte */
#define DB_STT_SIZE		(MFC_MAX_WIDTH*4*32) /* 163,840 byte */


#define MFC_SFR_BUF_SIZE	sizeof(S5PC100_MFC_SFR)
#define MFC_FW_BUF_SIZE		((MFC_MAX_FW_NUM * FIRMWARE_CODE_SIZE) + (MFC_MAX_INSTANCE_NUM + 1) * (VSP_BUF_SIZE + DB_STT_SIZE)) /* 3,014,656 */

volatile unsigned char *s3c_mfc_get_fw_buf_virt_addr(void);
volatile unsigned char *s3c_mfc_get_vsp_buf_virt_addr(int instNo);
unsigned int s3c_mfc_get_sfr_phys_addr(void);
unsigned int s3c_mfc_get_fw_buf_phys_addr(void);
unsigned int s3c_mfc_get_vsp_buf_phys_addr(int instNo);
unsigned int s3c_mfc_get_data_buffer_size(void);

#endif /* _S3C_MFC_MEMORY_H_ */
