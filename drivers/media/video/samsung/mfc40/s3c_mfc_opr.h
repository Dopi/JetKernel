/* 
 * drivers/media/video/samsung/mfc40/s3c_mfc_opr.h
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

#ifndef _S3C_MFC_OPR_H_ 
#define _S3C_MFC_OPR_H_

#include "s3c_mfc_errorno.h"
#include "s3c_mfc_interface.h"
#include "s3c_mfc_types.h"

#define     INT_MFC_FW_DONE		(0x1 << 5)
#define     INT_MFC_DMA_DONE		(0x1 << 7)
#define     INT_MFC_FRAME_DONE		(0x1 << 8)
/* Interrupt on/off (0x500) */
#define     INT_ENABLE_BIT		(0 << 0)
#define     INT_DISABLE_BIT		(1 << 0)
/* Interrupt mode (0x504) */
#define     INT_LEVEL_BIT		(0 << 0)
#define     INT_PULSE_BIT		(1 << 0)

/* Command Types */
#define 	MFC_CHANNEL_SET		0
#define 	MFC_CHANNEL_READ	1
#define 	MFC_CHANNEL_END		2
#define 	MFC_INIT_CODEC		3
#define 	MFC_FRAME_RUN		4
#define 	MFC_SLEEP		6
#define 	MFC_WAKEUP		7

/* DPB Count */
#define		NUM_MPEG4_DPB		(2)
#define		NUM_POST_DPB		(3)
#define		NUM_VC1_DPB		(4)

BOOL s3c_mfc_load_firmware(void);
MFC_ERROR_CODE s3c_mfc_init_hw(void);
MFC_ERROR_CODE s3c_mfc_init_encode(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_exe_encode(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_init_decode(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_start_decode_seq(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_exe_decode(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_get_config(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_set_config(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args);
MFC_ERROR_CODE s3c_mfc_deinit_hw(s3c_mfc_inst_ctx *MfcCtx);
MFC_ERROR_CODE s3c_mfc_set_sleep(s3c_mfc_inst_ctx *MfcCtx);
MFC_ERROR_CODE s3c_mfc_set_wakeup(s3c_mfc_inst_ctx *MfcCtx);

#endif /* _S3C_MFC_OPR_H_ */

