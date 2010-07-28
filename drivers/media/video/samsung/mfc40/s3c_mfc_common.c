/*
 * drivers/media/video/samsung/mfc40/s3c_mfc_common.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/mm.h>

#include "s3c_mfc_common.h"
#include "s3c_mfc_interface.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_fw.h"

static int s3c_mfc_inst_no[MFC_MAX_INSTANCE_NUM];

unsigned int s3c_mfc_get_codec_type(MFC_CODEC_TYPE codec_type)
{
	unsigned int standardSel = 0;

	switch(codec_type) {
	case MPEG4_DEC: 
	case DIVX_DEC:
	case XVID_DEC:
		standardSel = ((0 << 4) | (0 << 0)); 
		break;

	case H263_ENC:
	case MPEG4_ENC: 
		standardSel = ((1 << 4) | (0 << 0)); 
		break;

	case H264_DEC: 
		standardSel = ((0 << 4) | (1 << 0)); 
		break;

	case H264_ENC: 
		standardSel = ((1 << 4) | (1 << 0)); 
		break;

	case H263_DEC: 
		standardSel = ((0 << 4) | (4 << 0)); 
		break;

	case MPEG2_DEC: 
		standardSel = ((0 << 4) | (5 << 0)); 
		break;

	case VC1_DEC: 
		standardSel = ((0 << 4) | ( 6 << 0)); 
		break;

	default:
		break;
	}

	return standardSel;

}

int s3c_mfc_get_fw_buf_offset(MFC_CODEC_TYPE codecType)
{
	int offset;

	switch(codecType) {
	case MPEG4_ENC:
	case H263_ENC:
		offset = 0; 
		break;

	case DIVX_DEC:
	case XVID_DEC:
	case MPEG4_DEC:
		offset = FIRMWARE_CODE_SIZE; 
		break;

	case H264_ENC: 
		offset = 2 * FIRMWARE_CODE_SIZE; 
		break;

	case H264_DEC: 
		offset = 3 * FIRMWARE_CODE_SIZE; 
		break;

	case VC1_DEC: 
		offset = 4 * FIRMWARE_CODE_SIZE; 
		break;

	case MPEG2_DEC:
		offset = 5 * FIRMWARE_CODE_SIZE; 
		break;

	case H263_DEC: 
		offset = 6 * FIRMWARE_CODE_SIZE; 
		break;

	case COM_CTRL: 
		offset = 7 * FIRMWARE_CODE_SIZE; 
		break;

	default: 
		offset = -1;
		mfc_err("unknown codec type\n");
	}

	return offset;
}

int s3c_mfc_get_fw_buf_size(MFC_CODEC_TYPE codecType)
{
	int bufSize;

	switch(codecType) {
	case MPEG4_ENC:
	case H263_ENC: 
		bufSize = sizeof(mp4_enc_mc_fw); 
		break;

	case DIVX_DEC:
	case XVID_DEC:
	case MPEG4_DEC: 
		bufSize = sizeof(mp4_dec_mc_fw); 
		break;

	case H264_ENC:
		bufSize = sizeof(h264_enc_mc_fw); 
		break;

	case H264_DEC: 
		bufSize = sizeof(h264_dec_mc_fw); 
		break;

	case VC1_DEC: 
		bufSize = sizeof(vc1_dec_mc_fw); 
		break;

	case MPEG2_DEC: 
		bufSize = sizeof(mp2_dec_mc_fw); 
		break;

	case H263_DEC: 
		bufSize = sizeof(h263_dec_mc_fw); 
		break;

	case COM_CTRL: 
		bufSize = sizeof(cmd_ctrl_fw); 
		break;

	default: bufSize = -1;
		 mfc_err("unknown codec type\n");
	}

	return bufSize;
}

void  s3c_mfc_init_inst_no(void)
{
	memset(&s3c_mfc_inst_no, 0x00, sizeof(s3c_mfc_inst_no));
}

int s3c_mfc_get_inst_no(void)
{
	unsigned int i;

	for(i = 0; i < MFC_MAX_INSTANCE_NUM; i++)
		if (s3c_mfc_inst_no[i] == 0) {
			s3c_mfc_inst_no[i] = 1;
			return i;
		}

	return -1;   
}


void s3c_mfc_return_inst_no(int inst_no)
{
	if ((inst_no >= 0) && (inst_no < MFC_MAX_INSTANCE_NUM))
		s3c_mfc_inst_no[inst_no] = 0;

}


BOOL s3c_mfc_is_running(void)
{
	unsigned int    i;
	BOOL ret = FALSE;

	for(i = 1; i < MFC_MAX_INSTANCE_NUM; i++)
		if(s3c_mfc_inst_no[i] == 1)
			ret = TRUE;

	return ret;  
}

int s3c_mfc_set_state(s3c_mfc_inst_ctx *ctx, s3c_mfc_inst_state state)
{

	if(ctx->MfcState > state)
		return 0;

	ctx->MfcState = state;
	return  1;

}

