/*
 * drivers/media/video/samsung/mfc50/mfc_opr.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * Jaeryul Oh, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * Change Logs
 *   2009.09.14 - Beautify source code (Key Young, Park)
 *   2009.09.21 - Implement clock & power gating.
 *                including suspend & resume fuction. (Key Young, Park)
 *   2009.11.04 - remove mfc_common.[ch]
 *                seperate buffer alloc & set (Key Young, Park)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MFC_OPR_H_ 
#define _MFC_OPR_H_

#include <plat/regs-mfc.h>
#include "mfc_errorno.h"
#include "mfc_interface.h"
#include "mfc_shared_mem.h"

#define MFC_WARN_START_NO		145
#define MFC_ERR_START_NO			1


#define INT_MFC_FW_DONE     (0x1 << 5)
#define INT_MFC_DMA_DONE    (0x1 << 7)
#define INT_MFC_FRAME_DONE  (0x1 << 8)
/* Interrupt on/off (0x500) */
#define INT_ENABLE_BIT      (0 << 0)
#define INT_DISABLE_BIT     (1 << 0)
/* Interrupt mode (0x504)   */
#define INT_LEVEL_BIT       (0 << 0)
#define INT_PULSE_BIT       (1 << 0)

/* Command Types */
#define MFC_CHANNEL_SET     0
#define MFC_CHANNEL_READ    1
#define MFC_CHANNEL_END     2
#define MFC_INIT_CODEC      3
#define MFC_FRAME_RUN       4
#define MFC_SLEEP           6
#define MFC_WAKEUP          7

/* DPB Count */
#define NUM_MPEG4_DPB       2
#define NUM_POST_DPB        3
#define NUM_VC1_DPB         4

#define ALIGN_TO_16B(x)   ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_64B(x)   ((((x) + (1 <<  6) - 1) >>  6) <<  6)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)   ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_4KB(x)   ((((x) + (1 << 12) - 1) >> 12) << 12)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)  ((((x) + (1 << 16) - 1) >> 16) << 16)
#define ALIGN_TO_128KB(x) ((((x) + (1 << 17) - 1) >> 17) << 17)

#define PIXEL_CACHE_ON_ONLY_P_PICTURE   0
#define PIXEL_CACHE_ON_ONLY_B_PICTURE   1
#define PIXEL_CACHE_ON_BOTH_P_B_PICTURE 2
#define PIXEL_CACHE_DISABLE             3

#define BOUND_MEMORY_SIZE		921600



typedef enum {FALSE, TRUE} BOOL;

typedef enum {
	MFCINST_STATE_NULL = 0,

	/* Instance is created */
	MFCINST_STATE_OPENED = 10,

	/* channel_set and init_codec is completed */
	MFCINST_STATE_DEC_INITIALIZE = 20,

	MFCINST_STATE_DEC_EXE = 30,
	MFCINST_STATE_DEC_EXE_DONE,
	
	/* Instance is initialized for encoding */
	MFCINST_STATE_ENC_INITIALIZE = 40, 
	MFCINST_STATE_ENC_EXE,
	MFCINST_STATE_ENC_EXE_DONE
} mfc_inst_state;

typedef enum {
	MEM_STRUCT_LINEAR = 0,
	MEM_STRUCT_TILE_ENC  = 3  /* 64x32 */
} mfc_mem_type;

typedef enum {
	SEQ_HEADER        = 1,
	FRAME             = 2,
	LAST_FRAME        = 3,
	INIT_BUFFER       = 4,
	FRAME_RUN_REALLOC = 5,
} mfc_dec_type;

typedef enum {
	H2R_CMD_EMPTY          = 0,
	H2R_CMD_OPEN_INSTANCE  = 1,
	H2R_CMD_CLOSE_INSTANCE = 2,
	H2R_CMD_SYS_INIT       = 3,
	H2R_CMD_SLEEP          = 5,
	H2R_CMD_WAKEUP         = 6,
} mfc_facade_cmd;

typedef enum {
    R2H_CMD_EMPTY              = 0,
    R2H_CMD_OPEN_INSTANCE_RET  = 1,
    R2H_CMD_CLOSE_INSTANCE_RET = 2,
    R2H_CMD_ERROR_RET          = 3,
    R2H_CMD_SEQ_DONE_RET       = 4,
    R2H_CMD_FRAME_DONE_RET     = 5,
    R2H_CMD_SLICE_DONE_RET     = 6,
    R2H_CMD_ENC_COMPLETE_RET   = 6,
    R2H_CMD_SYS_INIT_RET       = 8,
    R2H_CMD_FW_STATUS_RET      = 9,
    R2H_CMD_SLEEP_RET          = 10,
    R2H_CMD_WAKEUP_RET         = 11,
    R2H_CMD_INIT_BUFFERS_RET   = 15,
    R2H_CMD_EDFU_INT_RET       = 16,
    R2H_CMD_DECODE_ERR_RET     = 32
} mfc_wait_done_type;

typedef enum {
	DECODING_ONLY    = 0,
	DECODING_DISPLAY = 1,
	DISPLAY_ONLY     = 2,
	DECODING_EMPTY   = 3
} mfc_display_status;

/* In case of decoder */
typedef enum {
	MFC_RET_FRAME_NOT_SET = 0,
	MFC_RET_FRAME_I_FRAME = 1,
	MFC_RET_FRAME_P_FRAME = 2,
	MFC_RET_FRAME_B_FRAME = 3	
} mfc_frame_type;

typedef struct tag_mfc_inst_ctx
{
	int InstNo;
	unsigned int DPBCnt;
	unsigned int totalDPBCnt;
	unsigned int extraDPB;
	unsigned int displayDelay;
	unsigned int postEnable;
	unsigned int endOfFrame;
	unsigned int forceSetFrameType;
	unsigned int img_width;
	unsigned int img_height;
	unsigned int dwAccess;  // for Power Management.
	unsigned int IsPackedPB;
	unsigned int interlace_mode;
	unsigned int sliceEnable;
	unsigned int crcEnable;
	unsigned int widthDivX311;
	unsigned int heightDivX311;
	int mem_inst_no;
	mfc_frame_type FrameType;
	SSBSIP_MFC_CODEC_TYPE MfcCodecType;
	mfc_inst_state MfcState;
	unsigned int port0_mmap_size;
	unsigned int codec_buff_paddr;
	unsigned int pred_buff_paddr;
	mfc_frame_buf_arg_t dec_dpb_buff_paddr;
	unsigned int shared_mem_paddr;
	unsigned int shared_mem_vaddr;
	unsigned int IsStartedIFrame;
	MFC_SHARED_MEM shared_mem;
} mfc_inst_ctx;

int mfc_load_firmware(void);
void mfc_cmd_reset(void);

MFC_ERROR_CODE mfc_init_hw(void);
MFC_ERROR_CODE mfc_init_encode(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_exe_encode(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_init_decode(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_exe_decode(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_get_config(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_set_config(mfc_inst_ctx *mfc_ctx, mfc_args *args);
MFC_ERROR_CODE mfc_deinit_hw(mfc_inst_ctx *mfc_ctx);
MFC_ERROR_CODE mfc_set_sleep(void);
MFC_ERROR_CODE mfc_set_wakeup(void);

int mfc_return_inst_no(int inst_no, SSBSIP_MFC_CODEC_TYPE codec_type);
int mfc_set_state(mfc_inst_ctx *ctx, mfc_inst_state state);
void mfc_init_mem_inst_no(void);
int mfc_get_mem_inst_no(void);
void mfc_return_mem_inst_no(int inst_no);
BOOL mfc_is_running(void);
BOOL is_dec_codec(SSBSIP_MFC_CODEC_TYPE codec_type);

#endif /* _MFC_OPR_H_ */
