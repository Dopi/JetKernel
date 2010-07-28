/*
 * drivers/media/video/samsung/mfc40/s3c_mfc_opr.c
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

#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <plat/regs-mfc.h>

#include "s3c_mfc_common.h"
#include "s3c_mfc_opr.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_fw.h"
#include "s3c_mfc_buffer_manager.h"
#include "s3c_mfc_interface.h"

extern void __iomem *s3c_mfc_sfr_virt_base;
extern dma_addr_t s3c_mfc_phys_data_buf;
extern unsigned char *s3c_mfc_virt_data_buf;

#define READL(offset)		readl(s3c_mfc_sfr_virt_base + (offset))
#define WRITEL(data, offset)	writel((data), s3c_mfc_sfr_virt_base + (offset))

static void s3c_mfc_cmd_reset(void);
static void s3c_mfc_cmd_fw_start(void);
static void s3c_mfc_cmd_dma_start(void);
static void s3c_mfc_cmd_seq_start(void);
static void s3c_mfc_cmd_frame_start(void);
static void s3c_mfc_cmd_sleep(void);
static void s3c_mfc_cmd_wakeup(void);
static void s3c_mfc_backup_context(s3c_mfc_inst_ctx  *MfcCtx);
static void s3c_mfc_restore_context(s3c_mfc_inst_ctx  *MfcCtx);
static void s3c_mfc_set_codec_firmware(s3c_mfc_inst_ctx  *MfcCtx);
static void s3c_mfc_set_encode_init_param(int inst_no, MFC_CODEC_TYPE mfc_codec_type, s3c_mfc_args *args);
static MFC_ERROR_CODE s3c_mfc_set_dec_stream_buffer(int buf_addr, unsigned int buf_size);
static MFC_ERROR_CODE s3c_mfc_set_dec_frame_buffer(s3c_mfc_inst_ctx  *MfcCtx, int buf_addr, unsigned int buf_size);
static MFC_ERROR_CODE s3c_mfc_set_vsp_buffer(int InstNo);
static MFC_ERROR_CODE s3c_mfc_decode_one_frame(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_dec_exe_arg_t *DecArg, unsigned int *consumedStrmSize);


static void s3c_mfc_cmd_reset(void)
{
	WRITEL(1, S3C_FIMV_SW_RESET);
	mdelay(10);
	WRITEL(0, S3C_FIMV_SW_RESET);
	WRITEL(0, S3C_FIMV_LAST_DEC);
}

static void s3c_mfc_cmd_fw_start(void)
{
	WRITEL(1, S3C_FIMV_FW_START);
	WRITEL(1, S3C_FIMV_CPU_RESET);
	mdelay(100);
}

static void s3c_mfc_cmd_dma_start(void)
{
	WRITEL(1, S3C_FIMV_DMA_START);
}

static void s3c_mfc_cmd_seq_start(void)
{
	WRITEL(1, S3C_FIMV_SEQ_START);
}

static void s3c_mfc_cmd_frame_start(void)
{
	WRITEL(1, S3C_FIMV_FRAME_START);
}

static void s3c_mfc_cmd_sleep()
{
	WRITEL(-1, S3C_FIMV_CH_ID);
	WRITEL(MFC_SLEEP, S3C_FIMV_COMMAND_TYPE);
}

static void s3c_mfc_cmd_wakeup()
{
	WRITEL(-1, S3C_FIMV_CH_ID);
	WRITEL(MFC_WAKEUP, S3C_FIMV_COMMAND_TYPE);
	mdelay(100);
}

static void s3c_mfc_backup_context(s3c_mfc_inst_ctx  *MfcCtx)
{
	memcpy(MfcCtx->MfcSfr, s3c_mfc_sfr_virt_base, S3C_FIMV_REG_SIZE);
}

static void s3c_mfc_restore_context(s3c_mfc_inst_ctx  *MfcCtx)
{
	/*
	memcpy(s3c_mfc_sfr_virt_base, MfcCtx->MfcSfr, S3C_FIMV_REG_SIZE);
	*/
}

static MFC_ERROR_CODE s3c_mfc_set_dec_stream_buffer(int buf_addr, unsigned int buf_size)
{
	mfc_debug("buf_addr : 0x%08x  buf_size : %d\n", buf_addr, buf_size);

	WRITEL(buf_addr & 0xfffffff8, S3C_FIMV_EXT_BUF_START_ADDR);
	WRITEL(buf_addr + buf_size + 0x200, S3C_FIMV_EXT_BUF_END_ADDR);
	WRITEL(buf_addr + buf_size + 0x200, S3C_FIMV_HOST_PTR);
	WRITEL(8 - (buf_addr & 0x7), S3C_FIMV_START_BYTE_NUM);
	WRITEL(buf_size, S3C_FIMV_DEC_UNIT_SIZE);

	return MFCINST_RET_OK;
}


static MFC_ERROR_CODE s3c_mfc_set_dec_frame_buffer(s3c_mfc_inst_ctx  *MfcCtx, int buf_addr, unsigned int buf_size)
{
	unsigned int    Width, Height, FrameSize, dec_dpb_addr;


	mfc_debug("buf_addr : 0x%08x  buf_size : %d\n", buf_addr, buf_size);

	Width = (MfcCtx->img_width + 15)/16*16;
	Height = (MfcCtx->img_height + 31)/32*32;
	FrameSize = (Width*Height*3)>>1;

	mfc_debug("width : %d height : %d framesize : %d buf_size : %d MfcCtx->DPBCnt :%d\n", \
								Width, Height, FrameSize, buf_size, MfcCtx->DPBCnt);
	if(buf_size < FrameSize*MfcCtx->totalDPBCnt){
		mfc_err("MFCINST_ERR_FRM_BUF_SIZE\n");
		return MFCINST_ERR_FRM_BUF_SIZE;
	}

	WRITEL(Align(buf_addr, BUF_ALIGN_UNIT), S3C_FIMV_DEC_DPB_ADR);
	dec_dpb_addr = READL(S3C_FIMV_DEC_DPB_ADR);
	WRITEL(Align(dec_dpb_addr + FrameSize*MfcCtx->DPBCnt, BUF_ALIGN_UNIT), S3C_FIMV_DPB_COMV_ADR);

	if((MfcCtx->MfcCodecType == MPEG4_DEC) 
			||(MfcCtx->MfcCodecType == MPEG2_DEC) 
			||(MfcCtx->MfcCodecType == XVID_DEC) 
			||(MfcCtx->MfcCodecType == DIVX_DEC) ) {
		dec_dpb_addr = READL(S3C_FIMV_DEC_DPB_ADR);
		WRITEL(Align(dec_dpb_addr + ((3*FrameSize*MfcCtx->DPBCnt)>>1), BUF_ALIGN_UNIT), S3C_FIMV_POST_ADR);
	}

	mfc_debug("DEC_DPB_ADR : 0x%08x DPB_COMV_ADR : 0x%08x POST_ADR : 0x%08x\n",	\
		READL(S3C_FIMV_DEC_DPB_ADR), READL(S3C_FIMV_DPB_COMV_ADR), READL(S3C_FIMV_POST_ADR));


	return MFCINST_RET_OK;
}

static MFC_ERROR_CODE s3c_mfc_set_vsp_buffer(int InstNo)
{
	unsigned int VSPPhyBuf;

	VSPPhyBuf = s3c_mfc_get_vsp_buf_phys_addr(InstNo);
	WRITEL(Align(VSPPhyBuf, BUF_ALIGN_UNIT), S3C_FIMV_VSP_BUF_ADDR);
	WRITEL(Align(VSPPhyBuf + VSP_BUF_SIZE, BUF_ALIGN_UNIT), S3C_FIMV_DB_STT_ADDR);

	mfc_debug("InstNo : %d VSP_BUF_ADDR : 0x%08x DB_STT_ADDR : 0x%08x\n",	\
			InstNo, READL(S3C_FIMV_VSP_BUF_ADDR), READL(S3C_FIMV_DB_STT_ADDR));

	return MFCINST_RET_OK;
}


static void s3c_mfc_set_codec_firmware(s3c_mfc_inst_ctx  *MfcCtx)
{
	unsigned int FWPhyBuf;

	FWPhyBuf = s3c_mfc_get_fw_buf_phys_addr();

	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(MPEG4_ENC), S3C_FIMV_FW_STT_ADR_0);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(MPEG4_DEC), S3C_FIMV_FW_STT_ADR_1);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(H264_ENC), S3C_FIMV_FW_STT_ADR_2);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(H264_DEC), S3C_FIMV_FW_STT_ADR_3);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(VC1_DEC), S3C_FIMV_FW_STT_ADR_4);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(MPEG2_DEC), S3C_FIMV_FW_STT_ADR_5);
	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(H263_DEC), S3C_FIMV_FW_STT_ADR_6);
	WRITEL(s3c_mfc_get_fw_buf_size(MfcCtx->MfcCodecType), S3C_FIMV_BOOTCODE_SIZE);
}


/* This function sets the MFC SFR values according to the input arguments. */
static void s3c_mfc_set_encode_init_param(int inst_no, MFC_CODEC_TYPE mfc_codec_type, s3c_mfc_args *args)
{
	unsigned int		ms_size;

	s3c_mfc_enc_init_mpeg4_arg_t   *EncInitMpeg4Arg;
	s3c_mfc_enc_init_h264_arg_t    *EncInitH264Arg;

	EncInitMpeg4Arg = (s3c_mfc_enc_init_mpeg4_arg_t *) args;
	EncInitH264Arg  = (s3c_mfc_enc_init_h264_arg_t  *) args;

	mfc_debug("mfc_codec_type : %d\n", mfc_codec_type);

	s3c_mfc_set_vsp_buffer(inst_no);

	/* Set the other SFR */
	WRITEL(EncInitMpeg4Arg->in_dpb_addr, S3C_FIMV_ENC_DPB_ADR);
	WRITEL(EncInitMpeg4Arg->in_width, S3C_FIMV_IMG_SIZE_X);
	WRITEL(EncInitMpeg4Arg->in_height, S3C_FIMV_IMG_SIZE_Y);
	WRITEL(EncInitMpeg4Arg->in_profile_level, S3C_FIMV_PROFILE);
	WRITEL(EncInitMpeg4Arg->in_gop_num, S3C_FIMV_IDR_PERIOD);
	WRITEL(EncInitMpeg4Arg->in_gop_num, S3C_FIMV_I_PERIOD);
	WRITEL(EncInitMpeg4Arg->in_vop_quant, S3C_FIMV_FRAME_QP_INIT);
	WRITEL(0, S3C_FIMV_POST_ON);
	WRITEL(EncInitMpeg4Arg->in_mb_refresh, S3C_FIMV_CIR_MB_NUM);

	/* Rate Control options */
	WRITEL((EncInitMpeg4Arg->in_RC_enable << 8) | (EncInitMpeg4Arg->in_vop_quant & 0x3F), S3C_FIMV_RC_CONFIG);

	if (READL(S3C_FIMV_RC_CONFIG) & 0x0300) {
		WRITEL(EncInitMpeg4Arg->in_RC_framerate, S3C_FIMV_RC_FRAME_RATE);
		WRITEL(EncInitMpeg4Arg->in_RC_bitrate, S3C_FIMV_RC_BIT_RATE);
		WRITEL(EncInitMpeg4Arg->in_RC_qbound, S3C_FIMV_RC_QBOUND);
		WRITEL(EncInitMpeg4Arg->in_RC_rpara, S3C_FIMV_RC_RPARA);
		WRITEL(0, S3C_FIMV_RC_MB_CTRL);
	}

	/* Multi-slice options */
	WRITEL(EncInitMpeg4Arg->in_MS_mode, S3C_FIMV_MSLICE_ENA);

	if (EncInitMpeg4Arg->in_MS_mode) {
		WRITEL(EncInitMpeg4Arg->in_MS_size_mode, S3C_FIMV_MSLICE_SEL);
		if (EncInitMpeg4Arg->in_MS_size_mode == 0) {
			WRITEL(EncInitMpeg4Arg->in_MS_size, S3C_FIMV_MSLICE_MB);
			WRITEL(0, S3C_FIMV_MSLICE_BYTE);
		} else {
			ms_size = (mfc_codec_type == H264_ENC) ? EncInitMpeg4Arg->in_MS_size : 0;
			WRITEL(ms_size, S3C_FIMV_MSLICE_MB);
			WRITEL(EncInitMpeg4Arg->in_MS_size, S3C_FIMV_MSLICE_BYTE);
		}
	}

	switch (mfc_codec_type) {
	case MPEG4_ENC:
		/* MPEG4 encoder */
		WRITEL(0, S3C_FIMV_ENTROPY_CON);
		WRITEL(0, S3C_FIMV_DEBLOCK_FILTER_OPTION);
		WRITEL(0, S3C_FIMV_SHORT_HD_ON);
		break;

	case H263_ENC:
		/* H263 encoder */
		WRITEL(0, S3C_FIMV_ENTROPY_CON);
		WRITEL(0, S3C_FIMV_DEBLOCK_FILTER_OPTION);
		WRITEL(1, S3C_FIMV_SHORT_HD_ON);

		break;

	case H264_ENC:
		/* H.264 encoder */
		WRITEL((EncInitH264Arg->in_symbolmode & 0x1) | (EncInitH264Arg->in_model_number << 2), S3C_FIMV_ENTROPY_CON);
		WRITEL((EncInitH264Arg->in_deblock_filt & 0x3)
				| ((EncInitH264Arg->in_deblock_alpha_C0 & 0x1f) << 7)
				| ((EncInitH264Arg->in_deblock_beta     & 0x1f) << 2), S3C_FIMV_DEBLOCK_FILTER_OPTION);
		WRITEL(0, S3C_FIMV_SHORT_HD_ON);
		break;

	default:
		mfc_err("Invalid MFC codec type\n");
	}

}

BOOL s3c_mfc_load_firmware()
{
	volatile unsigned char *FWVirBuf;

	mfc_debug("s3c_mfc_load_firmware++\n");

	FWVirBuf = s3c_mfc_get_fw_buf_virt_addr();
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(MPEG4_ENC), mp4_enc_mc_fw, sizeof(mp4_enc_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(MPEG4_DEC), mp4_dec_mc_fw, sizeof(mp4_dec_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(H264_ENC), h264_enc_mc_fw, sizeof(h264_enc_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(H264_DEC), h264_dec_mc_fw, sizeof(h264_dec_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(VC1_DEC), vc1_dec_mc_fw, sizeof(vc1_dec_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(MPEG2_DEC), mp2_dec_mc_fw, sizeof(mp2_dec_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(H263_DEC), h263_dec_mc_fw, sizeof(h263_dec_mc_fw));
	memcpy((void *)FWVirBuf + s3c_mfc_get_fw_buf_offset(COM_CTRL), cmd_ctrl_fw, sizeof(cmd_ctrl_fw));

	mfc_debug("s3c_mfc_load_firmware--\n");
	return TRUE;
}

MFC_ERROR_CODE s3c_mfc_init_hw()
{
	unsigned int FWPhyBuf;
	unsigned int VSPPhyBuf;

	mfc_debug("++\n");

	FWPhyBuf = s3c_mfc_get_fw_buf_phys_addr();

	/*
	 * 0. MFC reset
	 */
	s3c_mfc_cmd_reset();

	/* 1. DMA start
	 * 	- load command contrl firmware
	 */
	WRITEL(0, S3C_FIMV_BITS_ENDIAN);
	WRITEL(0, S3C_FIMV_ARM_ENDIAN);
	WRITEL(1, S3C_FIMV_BUS_MASTER);

	WRITEL(FWPhyBuf + s3c_mfc_get_fw_buf_offset(COM_CTRL), S3C_FIMV_DMA_EXTADDR);
	WRITEL(s3c_mfc_get_fw_buf_size(COM_CTRL)/4, S3C_FIMV_BOOTCODE_SIZE);
	WRITEL(0, S3C_FIMV_DMA_INTADDR);

	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(INT_MFC_DMA_DONE, S3C_FIMV_INT_MASK);


	s3c_mfc_cmd_dma_start();

	if(s3c_mfc_wait_for_done(MFC_INTR_DMA_DONE) == 0){
		mfc_err("MFCINST_ERR_FW_DMA_SET_FAIL\n");
		return MFCINST_ERR_FW_DMA_SET_FAIL;
	}

	/* 2. FW start
	 * 	- set VSP buffer for command control
	 * 	- set memory structure
	 */
	VSPPhyBuf = s3c_mfc_get_vsp_buf_phys_addr(MFC_MAX_INSTANCE_NUM);
	WRITEL(Align(VSPPhyBuf, BUF_ALIGN_UNIT), S3C_FIMV_VSP_BUF_ADDR);

	WRITEL(0, S3C_FIMV_BUS_MASTER);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(INT_MFC_FRAME_DONE | INT_MFC_FW_DONE, S3C_FIMV_INT_MASK);
	WRITEL(MEM_STRUCT_LINEAR, S3C_FIMV_TILE_MODE);

	s3c_mfc_cmd_fw_start();

	mfc_debug("--", "VSP_BUF_ADDR : 0x%08x DB_STT_ADDR : 0x%08x\n", \
			READL(S3C_FIMV_VSP_BUF_ADDR), READL(S3C_FIMV_DB_STT_ADDR));

	return MFCINST_RET_OK;
}

MFC_ERROR_CODE s3c_mfc_init_encode(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	mfc_debug("++\n");

	MfcCtx->MfcCodecType = ((MFC_CODEC_TYPE *) args)[0];

	/* 3. CHANNEL SET
	 * 	- set codec firmware
	 * 	- set codec_type/channel_id/post_on
	 */

	s3c_mfc_set_codec_firmware(MfcCtx);

	WRITEL(s3c_mfc_get_codec_type(MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);
	WRITEL(MFC_CHANNEL_SET, S3C_FIMV_COMMAND_TYPE);
	WRITEL(MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(0, S3C_FIMV_POST_ON);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);

	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL((INT_MFC_FRAME_DONE|INT_MFC_FW_DONE), S3C_FIMV_INT_MASK);

	s3c_mfc_cmd_frame_start();

	if(s3c_mfc_wait_for_done(MFC_INTR_FRAME_DONE) == 0){
		mfc_err("MFCINST_ERR_FW_LOAD_FAIL\n");
		return MFCINST_ERR_FW_LOAD_FAIL;
	}

	/* 4. INIT CODEC
	 * 	- change Endian(important!!!)
	 * 	- set Encoder Init SFR
	 */

	s3c_mfc_set_encode_init_param(MfcCtx->InstNo, MfcCtx->MfcCodecType, args);

	WRITEL(s3c_mfc_get_codec_type(MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);
	WRITEL(MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(MFC_INIT_CODEC, S3C_FIMV_COMMAND_TYPE);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL((INT_MFC_FRAME_DONE|INT_MFC_FW_DONE), S3C_FIMV_INT_MASK);

	s3c_mfc_cmd_frame_start();

	if(s3c_mfc_wait_for_done(MFC_INTR_FRAME_DONE) == 0){
		mfc_err("MFCINST_ERR_FW_LOAD_FAIL\n");
		return MFCINST_ERR_FW_LOAD_FAIL;
	}

	s3c_mfc_backup_context(MfcCtx);
	mfc_debug("--\n");
	return MFCINST_RET_OK;
}


MFC_ERROR_CODE s3c_mfc_exe_encode(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	s3c_mfc_enc_exe_arg          *EncExeArg;

	/* 
	 * 5. Encode Frame
	 */

	EncExeArg = (s3c_mfc_enc_exe_arg *) args;
	mfc_debug("++ EncExeArg->in_strm_st : 0x%08x EncExeArg->in_strm_end :0x%08x \r\n", \
								EncExeArg->in_strm_st, EncExeArg->in_strm_end);
	mfc_debug("EncExeArg->in_Y_addr : 0x%08x EncExeArg->in_CbCr_addr :0x%08x \r\n",   \
								EncExeArg->in_Y_addr, EncExeArg->in_CbCr_addr);

	s3c_mfc_restore_context(MfcCtx);

	s3c_mfc_set_vsp_buffer(MfcCtx->InstNo);

	if ((MfcCtx->forceSetFrameType > DONT_CARE) && 		\
		(MfcCtx->forceSetFrameType <= NOT_CODED)) {
		WRITEL(MfcCtx->forceSetFrameType, S3C_FIMV_CODEC_COMMAND);
		MfcCtx->forceSetFrameType = DONT_CARE;
	} else 
		WRITEL(DONT_CARE, S3C_FIMV_CODEC_COMMAND);
	/*
	if((EncExeArg->in_ForceSetFrameType >= DONT_CARE) && (EncExeArg->in_ForceSetFrameType <= NOT_CODED))
		WRITEL(EncExeArg->in_ForceSetFrameType, S3C_FIMV_CODEC_COMMAND);
	*/
	/*
	 * Set Interrupt
	 */

	WRITEL(EncExeArg->in_Y_addr, S3C_FIMV_ENC_CUR_Y_ADR);
	WRITEL(EncExeArg->in_CbCr_addr, S3C_FIMV_ENC_CUR_CBCR_ADR);
	WRITEL(EncExeArg->in_strm_st, S3C_FIMV_EXT_BUF_START_ADDR);
	WRITEL(EncExeArg->in_strm_end, S3C_FIMV_EXT_BUF_END_ADDR);
	WRITEL(EncExeArg->in_strm_st, S3C_FIMV_HOST_PTR);

	WRITEL(MFC_FRAME_RUN, S3C_FIMV_COMMAND_TYPE);
	WRITEL(MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(s3c_mfc_get_codec_type(MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);

	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL((INT_MFC_FRAME_DONE|INT_MFC_FW_DONE), S3C_FIMV_INT_MASK);

	s3c_mfc_cmd_frame_start();

	if (s3c_mfc_wait_for_done(MFC_INTR_FRAME_DONE) == 0) {
		mfc_err("MFCINST_ERR_ENC_ENCODE_DONE_FAIL\n");
		return MFCINST_ERR_ENC_ENCODE_DONE_FAIL;
	}

	EncExeArg->out_frame_type = READL(S3C_FIMV_RET_VALUE);
	EncExeArg->out_encoded_size = READL(S3C_FIMV_ENC_UNIT_SIZE);
	EncExeArg->out_header_size  = READL(S3C_FIMV_ENC_HEADER_SIZE);

	mfc_debug("-- frame type(%d) encodedSize(%d)\r\n", \
		EncExeArg->out_frame_type, EncExeArg->out_encoded_size);

	return MFCINST_RET_OK;
}


MFC_ERROR_CODE s3c_mfc_init_decode(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	MFC_ERROR_CODE   ret;
	s3c_mfc_dec_init_arg_t *InitArg;
	unsigned int FWPhyBuf;

	mfc_debug("++\n");
	InitArg = (s3c_mfc_dec_init_arg_t *)args;
	FWPhyBuf = s3c_mfc_get_fw_buf_phys_addr();

	/* Context setting from input param */
	MfcCtx->MfcCodecType = InitArg->in_codec_type;
	MfcCtx->IsPackedPB = InitArg->in_packed_PB;
	
	/* 3. CHANNEL SET
	 * 	- set codec firmware
	 * 	- set codec_type/channel_id/post_on
	 */

	s3c_mfc_set_codec_firmware(MfcCtx);

	WRITEL(s3c_mfc_get_codec_type(MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);
	WRITEL(MFC_CHANNEL_SET, S3C_FIMV_COMMAND_TYPE);
	WRITEL(MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(MfcCtx->postEnable, S3C_FIMV_POST_ON);
	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(INT_MFC_FRAME_DONE | INT_MFC_FW_DONE, S3C_FIMV_INT_MASK);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);

	s3c_mfc_cmd_frame_start();
	
	if((ret = s3c_mfc_wait_for_done(MFC_INTR_FRAME_DONE)) == 0){
		mfc_err("MFCINST_ERR_FW_LOAD_FAIL\n");
		return MFCINST_ERR_FW_LOAD_FAIL;
	}

	/* 4. INIT CODEC
	 * 	- change Endian(important!!)
	 * 	- set VSP buffer
	 * 	- set Input Stream buffer
	 * 	- set NUM_EXTRA_DPB
	 */
	s3c_mfc_set_vsp_buffer(MfcCtx->InstNo);
	s3c_mfc_set_dec_stream_buffer(InitArg->in_strm_buf, InitArg->in_strm_size);

	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL(MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(s3c_mfc_get_codec_type(MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);
	WRITEL(MFC_INIT_CODEC, S3C_FIMV_COMMAND_TYPE);
	WRITEL((MfcCtx->displayDelay<<16)|(0xFFFF & MfcCtx->extraDPB), S3C_FIMV_NUM_EXTRA_BUF);
	
	s3c_mfc_cmd_frame_start();

	if(s3c_mfc_wait_for_done(MFC_POLLING_HEADER_DONE) == 0){
		mfc_err("MFCINST_ERR_DEC_HEADER_DECODE_FAIL\n");
		return MFCINST_ERR_DEC_HEADER_DECODE_FAIL;
	}

	/* out param & context setting from header decoding result */
	MfcCtx->img_width = READL(S3C_FIMV_IMG_SIZE_X);
	MfcCtx->img_height = READL(S3C_FIMV_IMG_SIZE_Y);

	InitArg->out_img_width = READL(S3C_FIMV_IMG_SIZE_X);
	InitArg->out_img_height = READL(S3C_FIMV_IMG_SIZE_Y);

	/* in the case of VC1 interlace, height will be the multiple of 32
	 * otherwise, height and width is the mupltiple of 16
	 */
	InitArg->out_buf_width = (READL(S3C_FIMV_IMG_SIZE_X)+ 15)/16*16;
	InitArg->out_buf_height = (READL(S3C_FIMV_IMG_SIZE_Y) + 31)/32*32;


	switch (MfcCtx->MfcCodecType) {
	case H264_DEC: 
		InitArg->out_dpb_cnt = (READL(S3C_FIMV_DPB_SIZE)*3)>>1; 
		MfcCtx->DPBCnt = READL(S3C_FIMV_DPB_SIZE);
		break;

	case MPEG4_DEC:
	case MPEG2_DEC: 
	case DIVX_DEC: 
	case XVID_DEC:
		InitArg->out_dpb_cnt = ((NUM_MPEG4_DPB * 3) >> 1) + NUM_POST_DPB + MfcCtx->extraDPB;
		MfcCtx->DPBCnt = NUM_MPEG4_DPB;
		break;

	case VC1_DEC:
		InitArg->out_dpb_cnt = ((NUM_VC1_DPB * 3) >> 1)+ MfcCtx->extraDPB;
		MfcCtx->DPBCnt = NUM_VC1_DPB + MfcCtx->extraDPB;
		break;

	default:
		InitArg->out_dpb_cnt = ((NUM_MPEG4_DPB * 3) >> 1)+ NUM_POST_DPB + MfcCtx->extraDPB;
		MfcCtx->DPBCnt = NUM_MPEG4_DPB;
	}

	MfcCtx->totalDPBCnt = InitArg->out_dpb_cnt;

	mfc_debug("buf_width : %d buf_height : %d out_dpb_cnt : %d MfcCtx->DPBCnt : %d\n", \
				InitArg->out_img_width, InitArg->out_img_height, InitArg->out_dpb_cnt, MfcCtx->DPBCnt);
	mfc_debug("img_width : %d img_height : %d\n", \
				InitArg->out_img_width, InitArg->out_img_height);

	s3c_mfc_backup_context(MfcCtx);

	mfc_debug("--\n");
	return MFCINST_RET_OK;
}


MFC_ERROR_CODE s3c_mfc_start_decode_seq(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args)
{
	int ret;
	s3c_mfc_dec_seq_start_arg_t *seq_arg;
	
	/*
	 * 5. SEQ start
	 *    - set DPB buffer
	 */
	mfc_debug("++\n");

	seq_arg = (s3c_mfc_dec_seq_start_arg_t *)args;

	if ((ret = s3c_mfc_set_dec_frame_buffer(MfcCtx, seq_arg->in_frm_buf, seq_arg->in_frm_size)) != MFCINST_RET_OK)
		return ret;

	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL(INT_MFC_FRAME_DONE | INT_MFC_FW_DONE, S3C_FIMV_INT_MASK);

	s3c_mfc_cmd_seq_start();

	ret = s3c_mfc_wait_for_done(MFC_INTR_FRAME_DONE);
	if(ret == 0)
		return MFCINST_ERR_SEQ_START_FAIL;


	return MFCINST_RET_OK;
}

static MFC_ERROR_CODE s3c_mfc_decode_one_frame(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_dec_exe_arg_t *DecArg, unsigned int *consumedStrmSize)
{
	int ret;
	unsigned int frame_type;
	static int count = 0;

	count++;
	
	mfc_debug("++ IntNo%d(%d)\r\n", MfcCtx->InstNo, count);

	s3c_mfc_restore_context(MfcCtx);

	if(MfcCtx->endOfFrame) {
		WRITEL(1, S3C_FIMV_LAST_DEC);
		MfcCtx->endOfFrame = 0;
	} else {
		WRITEL(0, S3C_FIMV_LAST_DEC);
		//s3c_mfc_set_dec_stream_buffer(DecArg->in_strm_buf, DecArg->in_strm_size);
	}

	s3c_mfc_set_dec_stream_buffer(DecArg->in_strm_buf, DecArg->in_strm_size);

	s3c_mfc_set_dec_frame_buffer(MfcCtx, DecArg->in_frm_buf, DecArg->in_frm_size);

	/* Set VSP */
	s3c_mfc_set_vsp_buffer(MfcCtx->InstNo);

	WRITEL( MfcCtx->InstNo, S3C_FIMV_CH_ID);
	WRITEL(s3c_mfc_get_codec_type( MfcCtx->MfcCodecType), S3C_FIMV_STANDARD_SEL);
	WRITEL(s3c_mfc_get_fw_buf_size(MfcCtx->MfcCodecType), S3C_FIMV_BOOTCODE_SIZE);
	WRITEL(MFC_FRAME_RUN, S3C_FIMV_COMMAND_TYPE);
	WRITEL(INT_LEVEL_BIT, S3C_FIMV_INT_MODE);
	WRITEL(0, S3C_FIMV_INT_OFF);
	WRITEL(1, S3C_FIMV_INT_DONE_CLEAR);
	WRITEL(1, S3C_FIMV_BITS_ENDIAN);
	WRITEL((INT_MFC_FRAME_DONE|MFC_INTR_FW_DONE), S3C_FIMV_INT_MASK);

	s3c_mfc_cmd_frame_start();


	if ((ret = s3c_mfc_wait_for_done(MFC_INTR_FRAME_FW_DONE)) == 0) {
		mfc_err("MFCINST_ERR_DEC_DECODE_DONE_FAIL\n");
		return MFCINST_ERR_DEC_DECODE_DONE_FAIL;
	}

	if ((READL(S3C_FIMV_DISPLAY_STATUS) & 0x3) == DECODING_ONLY) {
		DecArg->out_display_Y_addr = 0;
		DecArg->out_display_C_addr = 0;
	} else {
		DecArg->out_display_Y_addr = READL(S3C_FIMV_DISPLAY_Y_ADR);
		DecArg->out_display_C_addr = READL(S3C_FIMV_DISPLAY_C_ADR);
	}


	if ((ret & MFC_INTR_FW_DONE) == MFC_INTR_FW_DONE) {
		DecArg->out_display_status = 0; /* no more frame to display */
	} else
		DecArg->out_display_status = 1; /* There exist frame to display */

	frame_type = READL(S3C_FIMV_FRAME_TYPE);
	MfcCtx->FrameType = (s3c_mfc_frame_type)(frame_type & 0x3);

	s3c_mfc_backup_context(MfcCtx);

	mfc_debug("(Y_ADDR : 0x%08x  C_ADDR : 0x%08x)\r\n", \
		DecArg->out_display_Y_addr , DecArg->out_display_C_addr);  
	mfc_debug("(in_strmsize : 0x%08x  consumed byte : 0x%08x)\r\n", \
			DecArg->in_strm_size, READL(S3C_FIMV_RET_VALUE));      

	*consumedStrmSize = READL(S3C_FIMV_RET_VALUE);
	return MFCINST_RET_OK;
}


MFC_ERROR_CODE s3c_mfc_exe_decode(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	MFC_ERROR_CODE ret;
	s3c_mfc_dec_exe_arg_t *DecArg;
	unsigned int consumedStrmSize;
	
	/* 6. Decode Frame */
	mfc_debug("++\n");

	DecArg = (s3c_mfc_dec_exe_arg_t *)args;
	ret = s3c_mfc_decode_one_frame(MfcCtx,  DecArg, &consumedStrmSize);

	if((MfcCtx->IsPackedPB) && (MfcCtx->FrameType == MFC_RET_FRAME_P_FRAME) \
		&& (DecArg->in_strm_size - consumedStrmSize > 4)) {
		mfc_debug("Packed PB\n");
		DecArg->in_strm_buf += consumedStrmSize;
		DecArg->in_strm_size -= consumedStrmSize;

		ret = s3c_mfc_decode_one_frame(MfcCtx,  DecArg, &consumedStrmSize);
	}
	mfc_debug("--\n");

	return ret; 
}

MFC_ERROR_CODE s3c_mfc_deinit_hw(s3c_mfc_inst_ctx  *MfcCtx)
{
	s3c_mfc_restore_context(MfcCtx);

	return MFCINST_RET_OK;
}

MFC_ERROR_CODE s3c_mfc_get_config(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	return MFCINST_RET_OK;
}


MFC_ERROR_CODE s3c_mfc_set_config(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	s3c_mfc_set_config_arg_t *set_cnf_arg;
	set_cnf_arg = (s3c_mfc_set_config_arg_t *)args;

	switch (set_cnf_arg->in_config_param) {
	case MFC_DEC_SETCONF_CODECTYPE:
		if (MfcCtx->MfcState < MFCINST_STATE_OPENED || 
			MfcCtx->MfcState >= MFCINST_STATE_DEC_SEQ_START) {
			mfc_err("MFC_DEC_SETCONF_CODECTYPE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		MfcCtx->MfcCodecType = set_cnf_arg->in_config_value[0];
		break;

	case MFC_DEC_SETCONF_POST_ENABLE:
		if (MfcCtx->MfcState >= MFCINST_STATE_DEC_SEQ_START) {
			mfc_err("MFC_DEC_SETCONF_POST_ENABLE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1))
			MfcCtx->postEnable = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("POST_ENABLE should be 0 or 1\n");
			MfcCtx->postEnable = 0;
		}
		break;
	
		
	case MFC_DEC_SETCONF_EXTRA_BUFFER_NUM:
		if (MfcCtx->MfcState >= MFCINST_STATE_DEC_SEQ_START) {
			mfc_err("MFC_DEC_SETCONF_EXTRA_BUFFER_NUM : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}
		if ((set_cnf_arg->in_config_value[0] >= 0) || (set_cnf_arg->in_config_value[0] <= MFC_MAX_EXTRA_DPB)) 
			MfcCtx->extraDPB = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("EXTRA_BUFFER_NUM should be between 0 and 5...It will be set 5 by default\n");
			MfcCtx->extraDPB = MFC_MAX_EXTRA_DPB;
		}
		break;
		
	case MFC_DEC_SETCONF_DISPLAY_DELAY:
		if(MfcCtx->MfcState >= MFCINST_STATE_DEC_SEQ_START) {
		        mfc_err("MFC_DEC_SETCONF_DISPLAY_DELAY : state is invalid\n");
		        return MFCINST_ERR_STATE_INVALID;
		}
		if((set_cnf_arg->in_config_value[0] >= 0) || (set_cnf_arg->in_config_value[0] <= 16))
			MfcCtx->displayDelay = set_cnf_arg->in_config_value[0];
		else{
			mfc_err("DISPLAY_DELAY should be between 0 and 16\n");
			MfcCtx->displayDelay = 0;
		}
		break;
		
	case MFC_DEC_SETCONF_IS_LAST_FRAME:
		if (MfcCtx->MfcState != MFCINST_STATE_DEC_EXE) {
			mfc_err("MFC_DEC_SETCONF_IS_LAST_FRAME : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] == 0) || (set_cnf_arg->in_config_value[0] == 1))
			MfcCtx->endOfFrame = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("IS_LAST_FRAME should be 0 or 1\n");
			MfcCtx->endOfFrame = 0;
		}
		break;
			
	case MFC_ENC_SETCONF_FRAME_TYPE:
		if ((MfcCtx->MfcState < MFCINST_STATE_ENC_INITIALIZE) || (MfcCtx->MfcState > MFCINST_STATE_ENC_EXE)) {
			mfc_err("MFC_ENC_SETCONF_FRAME_TYPE : state is invalid\n");
			return MFCINST_ERR_STATE_INVALID;
		}

		if ((set_cnf_arg->in_config_value[0] < DONT_CARE) || (set_cnf_arg->in_config_value[0] > NOT_CODED))
			MfcCtx->forceSetFrameType = set_cnf_arg->in_config_value[0];
		else {
			mfc_warn("FRAME_TYPE should be between 0 and 2\n");
			MfcCtx->forceSetFrameType = DONT_CARE;
		}
		break;
		
	default:
		mfc_err("invalid config param\n");
		return MFCINST_ERR_SET_CONF;
	}
	
	return MFCINST_RET_OK;
}

