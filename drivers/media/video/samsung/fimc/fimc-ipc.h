/* linux/drivers/media/video/samsung/ipc.h
 *
 * Header file for Samsung IPC driver
 *
 * Youngmok Song, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3C_IPC_H
#define _S3C_IPC_H

#define S3C_IPC_NAME		"s3c-ipc"

#define info(args...)	do { printk(KERN_INFO S3C_IPC_NAME ": " args); } while (0)
#define err(args...)	do { printk(KERN_ERR  S3C_IPC_NAME ": " args); } while (0)


typedef enum {
	DISABLED,		
	ENABLED
}ipc_enoff;

typedef enum {
	IPC_TOP_FIELD,
	IPC_BOTTOM_FIELD
}ipc_field_id;

typedef enum {
	INTERNAL,
	CAM_FIELD_SIG
}ipc_field_id_sel;

typedef enum {
	BYUSER,
	AUTO
}ipc_field_id_togl;

typedef enum {
	IPC_HDS, // Horizontal Double scaling
	IPC_2D // 2D IPC
	
}ipc_2d;

typedef enum {
	PROGRESSIVE
	
}scan_mode;

typedef enum {
	_1PIX_PER_1CYCLE,
	_1PIX_PER_2CYCLE,
	_1PIX_PER_3CYCLE,
	_1PIX_PER_4CYCLE
}ipc_pel_rate_ctrl;

typedef enum {
	NO_EFFECT,
	MIN_EDGE,
	MODERATE_EDGE,
	MAX_EDGE
}ipc_sharpness;

typedef enum {
	IPC_PP_LINEEQ_0 = 0,
	IPC_PP_LINEEQ_1,
	IPC_PP_LINEEQ_2,
	IPC_PP_LINEEQ_3,
	IPC_PP_LINEEQ_4,
	IPC_PP_LINEEQ_5,
	IPC_PP_LINEEQ_6,
	IPC_PP_LINEEQ_7,
	IPC_PP_LINEEQ_ALL
}ipc_pp_lineeq_val;

typedef enum {
	/* Don't change the order and the value */
	IPC_PP_H_NORMAL = 0,
	IPC_PP_H_8_9,		/* 720 to 640 */
	IPC_PP_H_1_2,
	IPC_PP_H_1_3,
	IPC_PP_H_1_4
}ipc_filter_h_pp;

typedef enum {
	/* Don't change the order and the value */
	IPC_PP_V_NORMAL = 0,
	IPC_PP_V_5_6,		/* PAL to NTSC */
	IPC_PP_V_3_4,
	IPC_PP_V_1_2,
	IPC_PP_V_1_3,
	IPC_PP_V_1_4
}ipc_filter_v_pp;

typedef enum {
	STEP0 = 0, // 0/16 = 0
	STEP1, // 1/16 = 0.0625
	STEP2, // 2/16 = 0.125
	STEP3, // 3/16 = 0.1875
	STEP4, // 4/16 = 0.25
	STEP5, // 5/16 = 0.3125
	STEP6, // 6/16 = 0.375
	STEP7, // 7/16 = 0.4375
	STEP8, // 8/16 = 0.5
	STEP9, // 9/16 = 0.5625
	STEP10, // 10/16 = 0.625
	STEP11, // 11/16 = 0.6875
	STEP12, // 12/16 = 0.75
	STEP13, // 13/16 = 0.8125
	STEP14, // 14/16 = 0.875
	STEP15 // 15/16 = 0.9375
}ipc_src_x_pos_fraction_step;

////////////////////////////////////////////////
// Source Image Processing Infomation
////////////////////////////////////////////////
typedef struct {
//	IMG_FMT	srcbpp;
	u32 srcstaddr;
	u32 imghsz;
	u32 imgvsz;
	u32 srcxpos;
	u32 srcypos;
	u32 srchsz;
	u32 srcvsz; 
	u32 srcnumoffrm;	
	u32 lastfrmbufidx;	
	ipc_src_x_pos_fraction_step Step;	
}ipc_source;

//extern oVP_Source oIPC_SrcInf;

////////////////////////////////////////////////
// Destination Image Processing Infomation
////////////////////////////////////////////////	
typedef struct {
	scan_mode scanmode;
	u32 orgdsthsz;
	u32 orgdstvsz;	
	u32 dstxpos;
	u32 dstypos;
	u32 dsthsz;
	u32 dstvsz; 
//	IMG_RESOLUTION 	ImgSz;
}ipc_destination;

//extern oVP_Destination oVP_DstInf;

////////////////////////////////////////////////
// Variable Structure(Control, Source, Destination, Enhancing)
////////////////////////////////////////////////
typedef struct {
	u32 modeval;
	u32 lineeqval;
	u32 scanconversionidx;
}ipc_controlvariable;

typedef struct {
	u32 contrast[8];
	u32 brightness[8];
	u32 saturation;
	ipc_sharpness sharpness;
	u32 thhnoise;
	u32 brightoffset;
}ipc_enhancingvariable;

typedef enum {
	REG_READ=0x1, 
	REG_WRITE=0x2
}reg_type;
/*
typedef struct {
	u8 name[32];
	u32 offset;
	reg_type type;
	u32 readableBits;
	u32 writebleBits;
	u32 resetValue;
	u32 resetValueMask; // if((register real value)&(resetValueMask) == resetValue){the reset value is OK}
	u32 tmp; //this is temp.
}register;
*/

typedef enum {
	IPC_ENABLE	= 0x0,
	IPC_SRESET	= 0x04,
	IPC_SHADOW_UPDATE	= 0x08,
	IPC_FIELD_ID	= 0x0C,
	IPC_MODE	= 0x10,
	IPC_PEL_RATE_CTRL	= 0x1C,
	IPC_SRC_WIDTH	= 0x4C,
	IPC_SRC_HEIGHT	= 0x50,
	IPC_DST_WIDTH	= 0x5C,
	IPC_DST_HEIGHT	= 0x60,
	IPC_H_RATIO	= 0x64,
	IPC_V_RATIO	= 0x68,


	IPC_POLY8_Y0_LL	= 0x6C,
	IPC_POLY8_Y0_LH	= 0x70,
	IPC_POLY8_Y0_HL	= 0x74,
	IPC_POLY8_Y0_HH	= 0x78,
	
	IPC_POLY8_Y1_LL	= 0x7C,
	IPC_POLY8_Y1_LH	= 0x80,
	IPC_POLY8_Y1_HL	= 0x84,
	IPC_POLY8_Y1_HH	= 0x88,
	
	IPC_POLY8_Y2_LL	= 0x8C,
	IPC_POLY8_Y2_LH	= 0x90,
	IPC_POLY8_Y2_HL	= 0x94,
	IPC_POLY8_Y2_HH	= 0x98,

	IPC_POLY8_Y3_LL	= 0x9C,
	IPC_POLY8_Y3_LH	= 0xA0,
	IPC_POLY8_Y3_HL	= 0xA4,
	IPC_POLY8_Y3_HH	= 0xA8,

	IPC_POLY4_Y0_LL	= 0xEC,
	IPC_POLY4_Y0_LH	= 0xF0,
	IPC_POLY4_Y0_HL	= 0xF4,
	IPC_POLY4_Y0_HH	= 0xF8,	

	IPC_POLY4_Y1_LL	= 0xFC,
	IPC_POLY4_Y1_LH	= 0x100,
	IPC_POLY4_Y1_HL	= 0x104,
	IPC_POLY4_Y1_HH	= 0x108,	

	IPC_POLY4_Y2_LL	= 0x10C,
	IPC_POLY4_Y2_LH	= 0x110,
	IPC_POLY4_Y2_HL	= 0x114,
	IPC_POLY4_Y2_HH	= 0x118,

	IPC_POLY4_Y3_LL	= 0x11C,
	IPC_POLY4_Y3_LH	= 0x120,
	IPC_POLY4_Y3_HL	= 0x124,
	IPC_POLY4_Y3_HH	= 0x128,

	IPC_POLY4_C0_LL	= 0x12C,
	IPC_POLY4_C0_LH	= 0x130,
	IPC_POLY4_C0_HL	= 0x134,
	IPC_POLY4_C0_HH	= 0x138,
	IPC_POLY4_C1_LL	= 0x13C,
	IPC_POLY4_C1_LH	= 0x140,
	IPC_POLY4_C1_HL	= 0x144,
	IPC_POLY4_C1_HH	= 0x148,

	IPC_BYPASS	= 0x200,
	IPC_PP_SATURATION	=  0x20C,
	IPC_PP_SHARPNESS	= 0x210,
                               
	IPC_PP_LINE_EQ0	= 0x218,
	IPC_PP_LINE_EQ1	= 0x21C,
	IPC_PP_LINE_EQ2	= 0x220,
	IPC_PP_LINE_EQ3	= 0x224,
	IPC_PP_LINE_EQ4	= 0x228,
	IPC_PP_LINE_EQ5	= 0x22C,
	IPC_PP_LINE_EQ6	= 0x230,
	IPC_PP_LINE_EQ7	= 0x234,
                               
	IPC_PP_BRIGHT_OFFSET	= 0x238,	
	IPC_VERSION_INFO	= 0x3FC
}ipc_sfr;

/* start ipc */
extern int ipc_initip(u32 input_width, u32 input_height,  ipc_2d ipc2d);
extern void ipc_field_id_control(ipc_field_id id);
extern void ipc_field_id_mode(ipc_field_id_sel sel, ipc_field_id_togl toggle);
extern void ipc_on(void);

/* stop ipc */
extern void ipc_off(void);

	
struct s3c_ipc_info {
	char 		name[16];
	struct device	*dev;
	struct clk	*clock;	
	void __iomem	*regs;
	int		irq;
};

#endif /* _S3C_IPC_H */

