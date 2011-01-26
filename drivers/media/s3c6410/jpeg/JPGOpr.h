/* jpeg/JPGOpr.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C JPEG driver
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

#ifndef __JPG_OPR_H__
#define __JPG_OPR_H__


typedef enum tagJPG_RETURN_STATUS{
	JPG_FAIL,
	JPG_SUCCESS,
	OK_HD_PARSING,
	ERR_HD_PARSING,
	OK_ENC_OR_DEC,
	ERR_ENC_OR_DEC,
	ERR_UNKNOWN
}JPG_RETURN_STATUS;

typedef enum tagIMAGE_TYPE_T{
	JPG_RGB16,
	JPG_YCBYCR,
	JPG_TYPE_UNKNOWN
}IMAGE_TYPE_T;

typedef enum tagSAMPLE_MODE_T{
	JPG_444,
	JPG_422,
	JPG_420, 
	JPG_411,
	JPG_400,
	JPG_SAMPLE_UNKNOWN
}SAMPLE_MODE_T;

typedef enum tagENCDEC_TYPE_T{
	JPG_MAIN,
	JPG_THUMBNAIL
}ENCDEC_TYPE_T;

typedef enum tagIMAGE_QUALITY_TYPE_T{
	JPG_QUALITY_LEVEL_1 = 0, /*high quality*/
	JPG_QUALITY_LEVEL_2,
	JPG_QUALITY_LEVEL_3,
	JPG_QUALITY_LEVEL_4     /*low quality*/
}IMAGE_QUALITY_TYPE_T;

typedef struct tagJPG_DEC_PROC_PARAM{
	SAMPLE_MODE_T	sampleMode;
	ENCDEC_TYPE_T	decType;
	UINT32	width;
	UINT32	height;
	UINT32	dataSize;
	UINT32	fileSize;
} JPG_DEC_PROC_PARAM;

typedef struct tagJPG_ENC_PROC_PARAM{
	SAMPLE_MODE_T	sampleMode;
	ENCDEC_TYPE_T	encType;
	IMAGE_QUALITY_TYPE_T quality;
	UINT32	width;
	UINT32	height;
	UINT32	dataSize;
	UINT32	fileSize;
} JPG_ENC_PROC_PARAM;

JPG_RETURN_STATUS decodeJPG(S3C6400_JPG_CTX *jCTX, JPG_DEC_PROC_PARAM *decParam);
void resetJPG(S3C6400_JPG_CTX *jCTX);
void decodeHeader(S3C6400_JPG_CTX *jCTX);
void decodeBody(S3C6400_JPG_CTX *jCTX);
JPG_RETURN_STATUS waitForIRQ(S3C6400_JPG_CTX *jCTX);
SAMPLE_MODE_T getSampleType(S3C6400_JPG_CTX *jCTX);
void getXY(S3C6400_JPG_CTX *jCTX, UINT32 *x, UINT32 *y);
UINT32 getYUVSize(SAMPLE_MODE_T sampleMode, UINT32 width, UINT32 height);
BOOL isCorrectHeader(SAMPLE_MODE_T sampleMode, UINT32 *width, UINT32 *height);
void rewriteHeader(S3C6400_JPG_CTX *jCTX, UINT32 file_size, UINT32 width, UINT32 height);
void rewriteYUV(S3C6400_JPG_CTX *jCTX, UINT32 width, UINT32 orgwidth, UINT32 height, UINT32 orgheight);
JPG_RETURN_STATUS encodeJPG(S3C6400_JPG_CTX *jCTX, JPG_ENC_PROC_PARAM	*EncParam);

#endif
