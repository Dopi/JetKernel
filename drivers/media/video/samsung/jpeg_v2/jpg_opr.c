/* linux/drivers/media/video/samsung/jpeg_v2/jpg_opr.c
 *
 * Driver file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <asm/io.h>

#include "jpg_mem.h"
#include "jpg_misc.h"
#include "jpg_opr.h"
#include "jpg_conf.h"

#include "regs-jpeg.h"

extern void __iomem		*s3c_jpeg_base;
extern int			jpg_irq_reason;

jpg_return_status wait_for_interrupt(void)
{
	if (interruptible_sleep_on_timeout(&wait_queue_jpeg, INT_TIMEOUT) == 0) {
		jpg_err("waiting for interrupt is timeout\n");
	}

	return jpg_irq_reason;
}

jpg_return_status decode_jpg(sspc100_jpg_ctx *jpg_ctx,
			     jpg_dec_proc_param *dec_param)
{
	volatile int		ret;
	sample_mode_t sample_mode;
	UINT32	width, height;
	jpg_dbg("enter decode_jpg function\n");

	if (jpg_ctx)
		reset_jpg(jpg_ctx);
	else {
		jpg_err("jpg ctx is NULL\n");
		return JPG_FAIL;
	}
#ifdef CONFIG_CPU_S5PC100
	////////////////////////////////////////
	// Header Parsing		      //
	////////////////////////////////////////

	decode_header(jpg_ctx, dec_param);
	ret = wait_for_interrupt();

	if (ret != OK_HD_PARSING) {
		jpg_err("DD::JPG Header Parsing Error(%d)\r\n", ret);
		return JPG_FAIL;
	}

	sample_mode = get_sample_type(jpg_ctx);
	jpg_dbg("sample_mode : %d\n", sample_mode);

	if (sample_mode == JPG_SAMPLE_UNKNOWN) {
		jpg_err("DD::JPG has invalid sample_mode\r\n");
		return JPG_FAIL;
	}

	dec_param->sample_mode = sample_mode;

	get_xy(jpg_ctx, &width, &height);
	jpg_dbg("decode_jpg", "DD:: width : %d height : %d\n", width, height);

	if (width <= 0 || width > MAX_JPG_WIDTH || height <= 0 || height > MAX_JPG_HEIGHT) {
		jpg_err("DD::JPG has invalid width(%d)/height(%d)\n",width, height);
		return JPG_FAIL;
	}

	//////////////////////////////////////////
	// Body Decoding		  	//
	//////////////////////////////////////////

	decode_body(jpg_ctx);

	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("DD::JPG Body Decoding Error(%d)\n", ret);
		return JPG_FAIL;
	}
#else //CONFIG_CPU_S5PC110
/* set jpeg clock register : power on */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) |
			(S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE),
			s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	/* set jpeg mod register : decode */
	writel(readl(s3c_jpeg_base + S3C_JPEG_MOD_REG) |
			(S3C_JPEG_MOD_REG_PROC_DEC),
			s3c_jpeg_base + S3C_JPEG_MOD_REG);
	/* set jpeg interrupt setting register */
	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) |
			(S3C_JPEG_INTSE_REG_RSTM_INT_EN	|
			S3C_JPEG_INTSE_REG_DATA_NUM_INT_EN |
			S3C_JPEG_INTSE_REG_FINAL_MCU_NUM_INT_EN),
			s3c_jpeg_base + S3C_JPEG_INTSE_REG);
	printk("interrupt setting register value %d",readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG));

	/* set jpeg deocde ouput format register */
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) &
			~(S3C_JPEG_OUTFORM_REG_YCBCY420),
			s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) |
			(dec_param->out_format << 0),
			s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);
	printk("jpg out format register value: %d\n",readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG));
	
	/* set the address of compressed input data */
	writel(jpg_ctx->img_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);

	/* set the address of decompressed image */
	writel(jpg_ctx->jpg_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);

	/* start decoding */

	writel(readl(s3c_jpeg_base + S3C_JPEG_JRSTART_REG) |
			S3C_JPEG_JRSTART_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_JSTART_REG);

	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("jpg decode error(%d)\n", ret);
		return JPG_FAIL;
	}

	sample_mode = get_sample_type(jpg_ctx);
	jpg_dbg("sample_mode : %d\n", sample_mode);

	if (sample_mode == JPG_SAMPLE_UNKNOWN) {
		jpg_err("jpg has invalid sample_mode\r\n");
		return JPG_FAIL;
	}

	dec_param->sample_mode = sample_mode;

	get_xy(jpg_ctx, &width, &height);
	jpg_dbg("decode size:: width : %d height : %d\n", width, height);
#endif
	dec_param->data_size = get_yuv_size(dec_param->sample_mode,dec_param->out_format,width,height);
	dec_param->width = width;
	dec_param->height = height;

	printk("WIDTH: %d HEIGHT: %d SAMPLEMODE: %d,DATA_SIZE: %d\n",width,height,sample_mode,dec_param->data_size);
	return JPG_SUCCESS;
}

void reset_jpg(sspc100_jpg_ctx *jpg_ctx)
{
#ifdef CONFIG_CPU_S5PC100
	jpg_dbg("s3c_jpeg_base 0x%08x \n", s3c_jpeg_base);
	writel(S3C_JPEG_SW_RESET_REG_ENABLE, s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);

	do {
		writel(S3C_JPEG_SW_RESET_REG_ENABLE, s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);
	} while (((readl(s3c_jpeg_base + S3C_JPEG_SW_RESET_REG)) & S3C_JPEG_SW_RESET_REG_ENABLE) == S3C_JPEG_SW_RESET_REG_ENABLE);
#else //CONFIG_CPU_S5PC110
jpg_dbg("s3c_jpeg_base %p \n", s3c_jpeg_base);
	writel(S3C_JPEG_SW_RESET_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);

	do {
		writel(S3C_JPEG_SW_RESET_REG_ENABLE,
				s3c_jpeg_base + S3C_JPEG_SW_RESET_REG);
	} while (((readl(s3c_jpeg_base + S3C_JPEG_SW_RESET_REG))
		& S3C_JPEG_SW_RESET_REG_ENABLE) == S3C_JPEG_SW_RESET_REG_ENABLE);
#endif
}

#ifdef CONFIG_CPU_S5PC100
void decode_header(sspc100_jpg_ctx *jpg_ctx, jpg_dec_proc_param *dec_param)
{
	jpg_dbg("decode_header function\n");
	writel(jpg_ctx->jpg_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);

	writel(readl(s3c_jpeg_base + S3C_JPEG_MOD_REG) | (S3C_JPEG_MOD_REG_PROC_DEC), s3c_jpeg_base + S3C_JPEG_MOD_REG);	//decoding mode
	writel(readl(s3c_jpeg_base + S3C_JPEG_CMOD_REG) & (~S3C_JPEG_CMOD_REG_MOD_HALF_EN_HALF), s3c_jpeg_base + S3C_JPEG_CMOD_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) | (S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE), s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) & ~(0x7f << 0), s3c_jpeg_base + S3C_JPEG_INTSE_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) | (S3C_JPEG_INTSE_REG_ERR_INT_EN	| S3C_JPEG_INTSE_REG_HEAD_INT_EN_ENABLE | S3C_JPEG_INTSE_REG_INT_EN), s3c_jpeg_base + S3C_JPEG_INTSE_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) & ~(S3C_JPEG_OUTFORM_REG_YCBCY420), s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_OUTFORM_REG) | (dec_param->out_format << 0), s3c_jpeg_base + S3C_JPEG_OUTFORM_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_DEC_STREAM_SIZE_REG) & ~(S3C_JPEG_DEC_STREAM_SIZE_REG_PROHIBIT), s3c_jpeg_base + S3C_JPEG_DEC_STREAM_SIZE_REG);
	//writel(dec_param->file_size, s3c_jpeg_base + S3C_JPEG_DEC_STREAM_SIZE_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_JSTART_REG) | S3C_JPEG_JSTART_REG_ENABLE, s3c_jpeg_base + S3C_JPEG_JSTART_REG);
}
void decode_body(sspc100_jpg_ctx *jpg_ctx)
{
	jpg_dbg("decode_body function\n");
	writel(jpg_ctx->img_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_JRSTART_REG) | S3C_JPEG_JRSTART_REG_ENABLE, s3c_jpeg_base + S3C_JPEG_JRSTART_REG);
}
#endif

sample_mode_t get_sample_type(sspc100_jpg_ctx *jpg_ctx)
{
	ULONG		jpgMode;
	sample_mode_t   sample_mode = JPG_SAMPLE_UNKNOWN;

	jpgMode = readl(s3c_jpeg_base + S3C_JPEG_MOD_REG);
	sample_mode =
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_444) ? JPG_444 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_422) ? JPG_422 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_420) ? JPG_420 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_400) ? JPG_400 :
		((jpgMode & JPG_SMPL_MODE_MASK) == JPG_411) ? JPG_411 : JPG_SAMPLE_UNKNOWN;

	return(sample_mode);
}

void get_xy(sspc100_jpg_ctx *jpg_ctx, UINT32 *x, UINT32 *y)
{
#ifdef CONFIG_CPU_S5PC100
	*x = readl(s3c_jpeg_base + S3C_JPEG_X_REG);
	*y = readl(s3c_jpeg_base + S3C_JPEG_Y_REG);
#else //CONFIG_CPU_S5PC110
	*x = (readl(s3c_jpeg_base + S3C_JPEG_X_U_REG)<<8)|
		readl(s3c_jpeg_base + S3C_JPEG_X_L_REG);
	*y = (readl(s3c_jpeg_base + S3C_JPEG_Y_U_REG)<<8)|
		readl(s3c_jpeg_base + S3C_JPEG_Y_L_REG);
#endif
}

UINT32 get_yuv_size(sample_mode_t sample_mode,out_mode_t out_format,UINT32 width, UINT32 height)
{
	switch (sample_mode) {
	case JPG_444 : 
                printk("Enter case YCBCR_444\n");
                if (width % 8 != 0){
                        width += 8 - (width % 8);
                }

                if (height % 8 != 0){
                        height += 8 - (height % 8);
                }
                break;

	case JPG_422 :

		printk("Enter case YCBCR_422\n");
		if (width % 16 != 0){
			width += 16 - (width % 16);
		}
		if (height % 8 != 0){
			height += 8 - (height % 8);
		}
		break;

	case JPG_420 :
		printk("Enter case YCBCR_420\n");
		if (width % 16 != 0){
                        width += 16 - (width % 16);
                }

                if (height % 16 != 0){
                        height += 16 - (height % 16);
                }
                break;
	default:
		printk("Enter case unknown\n");
		break;
	}

	printk("get_yuv_size width(%d) height(%d)\n", width, height);

	switch (out_format) {
	case YCBCR_422 :
		return (width*height*2);
	case YCBCR_420 :
		return ((width*height)+(width*height >> 1));
	default :
		return(0);
	}
}

jpg_return_status encode_jpg(sspc100_jpg_ctx *jpg_ctx,
			     jpg_enc_proc_param	*enc_param)
{

	UINT	i, ret;
	UINT32	cmd_val;

#ifdef CONFIG_CPU_S5PC100
	reset_jpg(jpg_ctx);
	cmd_val = (enc_param->sample_mode == JPG_422) ? (S3C_JPEG_MOD_REG_SUBSAMPLE_422) : (S3C_JPEG_MOD_REG_SUBSAMPLE_420);
	writel(cmd_val, s3c_jpeg_base + S3C_JPEG_MOD_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_CMOD_REG)& (~S3C_JPEG_CMOD_REG_MOD_HALF_EN_HALF), s3c_jpeg_base + S3C_JPEG_CMOD_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) | (S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE), s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	writel(readl(s3c_jpeg_base + S3C_JPEG_CMOD_REG) | (enc_param->in_format << JPG_MODE_SEL_BIT), s3c_jpeg_base + S3C_JPEG_CMOD_REG);
	writel(JPG_RESTART_INTRAVEL, s3c_jpeg_base + S3C_JPEG_DRI_REG);
	writel(enc_param->width, s3c_jpeg_base + S3C_JPEG_X_REG);
	writel(enc_param->height, s3c_jpeg_base + S3C_JPEG_Y_REG);

	jpg_dbg("enc_param->enc_type : %d\n", enc_param->enc_type);

	if (enc_param->enc_type == JPG_MAIN) {
		jpg_dbg("encode image size width: %d, height: %d\n", enc_param->width, enc_param->height);
		writel(jpg_ctx->img_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	} else { // thumbnail encoding
		jpg_dbg("thumb image size width: %d, height: %d\n", enc_param->width, enc_param->height);
		writel(jpg_ctx->img_thumb_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_thumb_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	}

	writel(COEF1_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF1_REG); 	// Coefficient value 1 for RGB to YCbCr
	writel(COEF2_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF2_REG); 	// Coefficient value 2 for RGB to YCbCr
	writel(COEF3_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF3_REG);	// Coefficient value 3 for RGB to YCbCr

	// Quantiazation and Huffman Table setting
	for (i = 0; i < 64; i++)
		writel((UINT32)qtbl_luminance[enc_param->quality][i], s3c_jpeg_base + S3C_JPEG_QTBL0_REG + (i*0x04));

	for (i = 0; i < 64; i++)
		writel((UINT32)qtbl_chrominance[enc_param->quality][i], s3c_jpeg_base + S3C_JPEG_QTBL1_REG + (i*0x04));

	for (i = 0; i < 16; i++)
		writel((UINT32)hdctbl0[i], s3c_jpeg_base + S3C_JPEG_HDCTBL0_REG + (i*0x04));

	for (i = 0; i < 12; i++)
		writel((UINT32)hdctblg0[i], s3c_jpeg_base + S3C_JPEG_HDCTBLG0_REG + (i*0x04));

	for (i = 0; i < 16; i++)
		writel((UINT32)hactbl0[i], s3c_jpeg_base + S3C_JPEG_HACTBL0_REG + (i*0x04));

	for (i = 0; i < 162; i++)
		writel((UINT32)hactblg0[i], s3c_jpeg_base + S3C_JPEG_HACTBLG0_REG + (i*0x04));

	writel(S3C_JPEG_QHTBL_REG_QT_NUM2 | S3C_JPEG_QHTBL_REG_QT_NUM3, s3c_jpeg_base + S3C_JPEG_QHTBL_REG);

	writel(readl(s3c_jpeg_base + S3C_JPEG_JSTART_REG) | S3C_JPEG_JSTART_REG_ENABLE, s3c_jpeg_base + S3C_JPEG_JSTART_REG);
	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("DD::JPG Encoding Error(%d)\n", ret);
		return JPG_FAIL;
	}

	enc_param->file_size = readl(s3c_jpeg_base + S3C_JPEG_CNT_REG);
	jpg_dbg("encoded file size : %d\n", enc_param->file_size);
#else //CONFIG_CPU_S5PC110
/* SW reset */
	if (jpg_ctx)
		reset_jpg(jpg_ctx);
	else {
		jpg_err("::jpg ctx is NULL\n");
		return JPG_FAIL;
	}
	/* set jpeg clock register : power on */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CLKCON_REG) |
			(S3C_JPEG_CLKCON_REG_POWER_ON_ACTIVATE),
			s3c_jpeg_base + S3C_JPEG_CLKCON_REG);
	/* set jpeg mod register : encode */
	writel(readl(s3c_jpeg_base + S3C_JPEG_CMOD_REG) |
			(enc_param->in_format << JPG_MODE_SEL_BIT),
			s3c_jpeg_base + S3C_JPEG_CMOD_REG);
	cmd_val = (enc_param->sample_mode == JPG_422) ?
		(S3C_JPEG_MOD_REG_SUBSAMPLE_422) : (S3C_JPEG_MOD_REG_SUBSAMPLE_420);
	writel(cmd_val | S3C_JPEG_MOD_REG_PROC_ENC, s3c_jpeg_base + S3C_JPEG_MOD_REG);

	/* set DRI(Define Restart Interval) */
	writel(JPG_RESTART_INTRAVEL, s3c_jpeg_base + S3C_JPEG_DRI_L_REG);
	writel((JPG_RESTART_INTRAVEL>>8), s3c_jpeg_base + S3C_JPEG_DRI_U_REG);

	writel(S3C_JPEG_QHTBL_REG_QT_NUM1, s3c_jpeg_base + S3C_JPEG_QTBL_REG);
	writel(0x00, s3c_jpeg_base + S3C_JPEG_HTBL_REG);

	/* Horizontal resolution */
	writel((enc_param->width>>8), s3c_jpeg_base + S3C_JPEG_X_U_REG);
	writel(enc_param->width, s3c_jpeg_base + S3C_JPEG_X_L_REG);
	/* Vertical resolution */
	writel((enc_param->height>>8), s3c_jpeg_base + S3C_JPEG_Y_U_REG);
	writel(enc_param->height, s3c_jpeg_base + S3C_JPEG_Y_L_REG);

	jpg_dbg("enc_param->enc_type : %d\n", enc_param->enc_type);
	printk("enc_param->enc_type : %d\n", enc_param->enc_type);

	if (enc_param->enc_type == JPG_MAIN) {
		jpg_dbg("encode image size width: %d, height: %d\n",
				enc_param->width, enc_param->height);
		printk("encode image size width: %d, height: %d\n",
                                enc_param->width, enc_param->height);
		writel(jpg_ctx->img_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	} else { // thumbnail encoding
		jpg_dbg("thumb image size width: %d, height: %d\n",
				enc_param->width, enc_param->height);
		printk("thumb image size width: %d, height: %d\n",
                                enc_param->width, enc_param->height);
		writel(jpg_ctx->img_thumb_data_addr, s3c_jpeg_base + S3C_JPEG_IMGADR_REG);
		writel(jpg_ctx->jpg_thumb_data_addr, s3c_jpeg_base + S3C_JPEG_JPGADR_REG);
	}

	/*  Coefficient value 1~3 for RGB to YCbCr */
	writel(COEF1_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF1_REG);
	writel(COEF2_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF2_REG);
	writel(COEF3_RGB_2_YUV, s3c_jpeg_base + S3C_JPEG_COEF3_REG);
	// Quantiazation and Huffman Table setting
	for (i = 0; i < 64; i++)
		writel((UINT32)qtbl_luminance[enc_param->quality][i],
			s3c_jpeg_base + S3C_JPEG_QTBL0_REG + (i*0x04));

	for (i = 0; i < 64; i++)
		writel((UINT32)qtbl_chrominance[enc_param->quality][i],
			s3c_jpeg_base + S3C_JPEG_QTBL1_REG + (i*0x04));

	for (i = 0; i < 16; i++)
		writel((UINT32)hdctbl0[i], s3c_jpeg_base + S3C_JPEG_HDCTBL0_REG + (i*0x04));

	for (i = 0; i < 12; i++)
		writel((UINT32)hdctblg0[i], s3c_jpeg_base + S3C_JPEG_HDCTBLG0_REG + (i*0x04));

	for (i = 0; i < 16; i++)
		writel((UINT32)hactbl0[i], s3c_jpeg_base + S3C_JPEG_HACTBL0_REG + (i*0x04));

	for (i = 0; i < 162; i++)
		writel((UINT32)hactblg0[i], s3c_jpeg_base + S3C_JPEG_HACTBLG0_REG + (i*0x04));

	writel(readl(s3c_jpeg_base + S3C_JPEG_INTSE_REG) |
			(S3C_JPEG_INTSE_REG_RSTM_INT_EN	|
			S3C_JPEG_INTSE_REG_DATA_NUM_INT_EN |
			S3C_JPEG_INTSE_REG_FINAL_MCU_NUM_INT_EN),
			s3c_jpeg_base + S3C_JPEG_INTSE_REG);

	writel(readl(s3c_jpeg_base + S3C_JPEG_JSTART_REG) | S3C_JPEG_JSTART_REG_ENABLE,
			s3c_jpeg_base + S3C_JPEG_JSTART_REG);
	ret = wait_for_interrupt();

	if (ret != OK_ENC_OR_DEC) {
		jpg_err("DD::JPG Encoding Error(%d)\n", ret);
		return JPG_FAIL;
	}

	enc_param->file_size = readl(s3c_jpeg_base + S3C_JPEG_CNT_U_REG) << 16;
	enc_param->file_size |= readl(s3c_jpeg_base + S3C_JPEG_CNT_M_REG) << 8;
	enc_param->file_size |= readl(s3c_jpeg_base + S3C_JPEG_CNT_L_REG);
	printk("encoded file size %d\n",enc_param->file_size);
#endif
	return JPG_SUCCESS;

}
