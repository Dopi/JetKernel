#define S5K3BA_COMPLETE
//#undef S5K3BA_COMPLETE
/*
 * Driver for S5K3BA (UXGA camera) from Samsung Electronics
 * 
 * 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (C) 2009, Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __S5K3BA_H__
#define __S5K3BA_H__

struct s5k3ba_reg {
	unsigned char addr;
	unsigned char val;
};

struct s5k3ba_regset_type {
	unsigned char *regset;
	int len;
};

/*
 * Macro
 */
#define REGSET_LENGTH(x)	(sizeof(x)/sizeof(s5k3ba_reg))

/*
 * User defined commands
 */
/* S/W defined features for tune */
#define REG_DELAY	0xFF00	/* in ms */
#define REG_CMD		0xFFFF	/* Followed by command */

/* Following order should not be changed */
enum image_size_s5k3ba {
	/* This SoC supports upto UXGA (1600*1200) */
#if 0
	QQVGA,	/* 160*120*/
	QCIF,	/* 176*144 */
	QVGA,	/* 320*240 */
	CIF,	/* 352*288 */
	VGA,	/* 640*480 */
#endif
	SVGA,	/* 800*600 */
#if 0
	HD720P,	/* 1280*720 */
	SXGA,	/* 1280*1024 */
	UXGA,	/* 1600*1200 */
#endif
};

/*
 * Following values describe controls of camera
 * in user aspect and must be match with index of s5k3ba_regset[]
 * These values indicates each controls and should be used
 * to control each control
 */
enum s5k3ba_control {
	S5K3BA_INIT,
	S5K3BA_EV,
	S5K3BA_AWB,
	S5K3BA_MWB,
	S5K3BA_EFFECT,
	S5K3BA_CONTRAST,
	S5K3BA_SATURATION,
	S5K3BA_SHARPNESS,
};

#define S5K3BA_REGSET(x)	{	\
	.regset = x,			\
	.len = sizeof(x)/sizeof(s5k3ba_reg),}


/*
 * User tuned register setting values
 */
#if 1
static unsigned char s5k3ba_init_reg[][2] = {
	{0xfc, 0x01},
	{0x03, 0x01}, // sw reset

	{0xfc, 0x01},
	{0x04, 0x01},

	{0xfc, 0x02},
	{0x52, 0x80}, // pll_m

	{0xfc, 0x02},
	{0x50, 0x54},
    
	{0xfc, 0x00},
	{0x02, 0x02},

	{0xfc, 0x01},
	{0x02, 0x05},
};
#else
static unsigned char s5k3ba_init_reg[][2] = {
 //1. initial setting 
    {0xfc, 0x01},                                
    {0x04, 0x03},  //ARM Clock divider(1/4)     

    //{0xfc, 0x02},                                
    //{0x52, 0x80},    // PLL M                     
    //{0x50, 0x14},    //1b    // PLL S,P (15fps)    
    //In case of PCLK = 64MHz 

    {0xfc, 0x02},
    {0x52, 0x80},  //PLL M M= 128 setting. 
        /*
        //case 200:
            {0xfc, 0x02},
            {0x50, 0x19},      //PLL S= 0 , P = 25 PCLK=128/2=64Mhz 15 Frmae Setting 
//            {0x50, 0x14},      //PLL S= 0 , P = 20 PCLK=128/2=64Mhz 15 Frmae Setting             
//            {0x50, 0x59},      //PLL S= 1 , P = 25 PCLK=64/2=32Mhz , 7~8Frame Setting
        */
        //case 266:
    {0xfc, 0x02},
    {0x50, 0x1A},
    
    {0xfc, 0x07},
    {0x58, 0x10},
    {0x59, 0x00},
    {0x5A, 0x00},
    {0x5B, 0x6c},

    {0xfc, 0xf0}, 
    {0x00, 0x40}, 

    {0xfc, 0x00},
    {0x62, 0x02},
    {0xbc, 0xe0},  // AWB_AE_DIFF

    {0xfc, 0x03},  //************************************************* 
    {0x2d, 0x03}, 
    {0xfc, 0x01},  
    {0x02, 0x02},   // YCbCr Order

    {0xfc, 0x02},    
    {0x4a, 0xc1},  // SC type selection
    {0x37, 0x18},  //16    // SC type global gain 
    {0x47, 0xc4},     // r-ramp  by chin

    {0xfc, 0x01},  //AWB Window Area (except sky)     
    {0xc4, 0x01},      
    {0xc5, 0x4e},           
    {0xc7, 0x6e},

    {0xfc, 0x02},
    {0x30, 0x84},  //Analog offset

    {0xfc, 0x00},
    {0x3d, 0x10},  //AWB Low Y limit 

    {0xfc, 0x02},
    {0x3d, 0x06},  //ADLC OFF 
    {0x44, 0x5b},  //clamp enable
    {0x55, 0x03},

    {0xfc, 0x06}, 
    {0x0c, 0x01}, 
    {0x0d, 0x4e}, 
    {0x0f, 0x6e},  

    {0xfc, 0x00},
    {0x78, 0x58},  //AGC MAX (30lux_Micron���Y=60code)

    {0xfc, 0x02},
    {0x45, 0x8c},  //CDS timing_������ greenish �ذ�(15fps)
    {0x49, 0x80},  // APS Current 2uA

    {0xfc, 0x01},
    {0x25, 0x14},  //10    //Digital Clamp 

    {0xfc, 0x00},
    {0x6d, 0x01},   //AE target high (Macbeth white=240) 
    {0x6c, 0x00},  //AE target (Macbeth white=240) 
    //{0x6d, 0x00},

    //2. ISP tuning //******************************************
    //ISP_tuning
//    {0xfc, 0x00},  
//    {0x01, 0x00},    // I2C hold mode off 

    {0xfc, 0x01},  
    {0x00, 0x00},      // ISP BPR Off
    {0x0c, 0x02},      // Full YC
    {0xc8, 0x19},  // AWB Y Max

    {0xfc, 0x00},
    {0x81, 0x00},      // AWB G gain suppress disable
    {0x29, 0x04},
    {0x2a, 0x00},
    {0x2b, 0x04},  // color level                  
    {0x2c, 0x00},  

    {0xfc, 0x07},  
    {0x11, 0x00},      // G offset
    {0x37, 0x00},      // Flicker Add

    {0xfc, 0x00},  
    {0x72, 0xa0},      // Flicker for 32MHz
    {0x74, 0x18},      // Flicker
    {0x73, 0x00},  // Frame AE

    {0xfc, 0x05},                   
    {0x64, 0x00},      // Darkslice R
    {0x65, 0x00},      // Darkslice G
    {0x66, 0x00},      // Darkslice B

    //Edge                                  
    {0xfc, 0x05}, 
    {0x2c, 0x0a},  //14    // positive gain                   
    {0x30, 0x0a},  //10    // negative edge gain
    {0x34, 0x1a},   // APTCLP  
    {0x35, 0x10},  //0a   // APTSC  
    {0x36, 0x0b},   // ENHANCE 
    {0x3f, 0x00},   // NON-LIN 
    {0x45, 0x30},   // EGREF  
    {0x47, 0x00},   // LLREF
    {0x48, 0x08},   // by chin
    {0x49, 0x39},   // CSSEL  EGSEL  CS_DLY by 
    {0x40, 0x41},  // Y delay

    ////////////////////////////////////
    {0xfc, 0x00},  
    {0x7e, 0xfc},  
    //s7e8c  //NR GrGb off
    // [7]: BPR [6]:Noise Filter(1D/NR) [4]: GrGb Enable [3]:BPR Data Threshold 
    // [2]: color suppress [1]: Y gain suppress [0]: Digital Clamp
    ///////////////////////////////////
    ////////////////////////////////////
    // GrGb Correction setting
    {0xfc, 0x01},  
    {0x44, 0x0c},       

    //s4400      
    /// [4]: GrGb full [3]: GrGb On
    /// [2]: GrGb Rb On 
    {0xfc, 0x0b},  
    {0x21, 0x00},      // Start AGC
    {0x22, 0x10},      // AGCMIN
    {0x23, 0x50},      // AGCMAX
    {0x24, 0x18},  // G Th AGCMIN(23d)
    {0x25, 0x52},      // G Th AGCMAX(50d)
    {0x26, 0x38},  // RB Th AGCMIN
    {0x27, 0x52},      // RB Th AGCMAX
    // GrGb Correction setting End

    ///////////////////////////////////
    // BPR Setting      
    {0xfc, 0x01},         
    {0x3f, 0x00},      // setting because S/W bug

    {0xfc, 0x0b},  
    {0x0b, 0x00},      // ISP BPR On Start
    {0x0c, 0x00},      // Th13 AGC Min
    {0x0d, 0x5a},  // Th13 AGC Max
    {0x0e, 0x01},  //00    // Th1 Max H for AGCMIN
    {0x0f, 0xff},  //c0    // Th1 Max L for AGCMIN
    {0x10, 0x00},      // Th1 Min H for AGCMAX
    {0x11, 0x10},      //00    // Th1 Min L for AGCMAX
    {0x12, 0xff},      // Th3 Max H for AGCMIN
    {0x13, 0xff},     // Th3 Max L for AGCMIN
    {0x14, 0xff},     // Th3 Min H for AGCMAX
    {0x15, 0xff},     // Th3 Min L for AGCMAX
    ///////////////////////////////////////////

    // NR Setting
    {0xfc, 0x01},  
    {0x4b, 0x01},      // NR Enable
    //s4b00     // NR Enable

    {0xfc, 0x0b},                             
    {0x28, 0x00},      //NR Start AGC             
    {0x29, 0x00},      // SIG Th AGCMIN H   
    {0x2a, 0x0a},  //14    // SIG Th AGCMIN L   
    {0x2b, 0x00},      // SIG Th AGCMAX H   
    {0x2c, 0x0a},  //14    // SIG Th AGCMAX L   
    {0x2d, 0x00},      // PRE Th AGCMIN H   
    {0x2e, 0xc0},  //64    // PRE Th AGCMIN L(100d)   
    {0x2f, 0x01},      // PRE Th AGCMAX H(300d)   
    {0x30, 0x2c},      // PRE Th AGCMAX L   
    {0x31, 0x00},      // POST Th AGCMIN H  
    {0x32, 0xe0},  //64    // POST Th AGCMIN L(100d)  
    {0x33, 0x01},      // POST Th AGCMAX H(300d)
    {0x34, 0x2c},      // POST Th AGCMAX L  
    // NR Setting End

    ////////////////////////////////
    // Color suppress setting
    {0xfc, 0x0b},  
    {0x08, 0x50},      // C suppress AGC MIN
    {0x09, 0x03},      // C suppress MIN H
    {0x0a, 0x80},      // C suppress MIN L
    // C Suppress Setting End

    {0xfc, 0x05},
    {0x4a, 0x00},  //01    // Edge Color Suppress, 9/13
    ///////////////////////////////

    // 1D Y LPF Filter             
    {0xfc, 0x01},                            
    //s05e0     // Default s60        
    {0x05, 0x60},      // Default s60        
    //[7]: Y LPF filter On [6]: Clap On

    {0xfc, 0x0b},                            
    {0x35, 0x00},      // YLPF Start AGC      
    {0x36, 0x50},      // YLPF01 AGCMIN       
    {0x37, 0x50},      // YLPF01 AGCMAX       
    {0x38, 0x00},      // YLPF SIG01 Th AGCMINH
    {0x39, 0x90},  //00    // YLPF SIG01 Th AGCMINL   
    {0x3a, 0x01},   // YLPF SIG01 Th AGCMAXH   
    {0x3b, 0xa0},   // YLPF SIG01 Th AGCMAXL               
    {0x3c, 0x50},      // YLPF02 AGCMIN           
    {0x3d, 0x50},   // YLPF02 AGCMAX           
    {0x3e, 0x00},   // YLPF SIG02 Th AGCMINH   
    {0x3f, 0xa0},  //00   // YLPF SIG02 Th AGCMINL   
    {0x40, 0x01},   // YLPF SIG02 Th AGCMAXH   s73
    {0x41, 0xb0},   // YLPF SIG02 Th AGCMAXL   
    // Y LPF Filter setting End 

    // SET EDGE COLOR SUPPRESS AND Y-LPF(�� å�Ӵ� mail �߰�)************************************ 
    {0xfc, 0x05},
    {0x42, 0x1F},
    {0x43, 0x1F},
    {0x44, 0x0E},
    {0x45, 0x8C},    //�� suppres�ϰ����ϸ� 5a, 0x �� ���ϴ� side effect ������ �ȵ�.
    {0x46, 0x7A},
    {0x47, 0x60},
    {0x48, 0x0C},
    {0x49, 0x39},
    {0x4A, 0x01},
    {0x4B, 0xB1},
    {0x4C, 0x3B},
    {0x4D, 0x14},
    //*******************************************************************************************  
    ///////////////////////////////////////////
    // NR Setting
    {0xfc, 0x01},  
    {0x4b, 0x01},      // NR Enable
    // Set multipliers (which are not suppressed)_(�� å�Ӵ� mail �߰�)**************************
    {0xfc, 0x01},
    {0x48, 0x11},
    // Suppressed parameters

    {0xfc, 0x0B},
    {0x21, 0x00},
    {0x22, 0x10},
    {0x23, 0x60},
    {0x24, 0x10},
    {0x25, 0x28},
    {0x26, 0x08},
    {0x27, 0x20},
    {0x28, 0x00},    //NR Start AGC                    
    {0x29, 0x00},    // SIG Th AGCMIN H              
    {0x2A, 0x02},    // SIG Th AGCMIN L      
    {0x2B, 0x00},    // SIG Th AGCMAX H              
    {0x2C, 0x14},    // SIG Th AGCMAX L      
    {0x2D, 0x03},    // PRE Th AGCMIN H              
    {0x2E, 0x84},    // PRE Th AGCMIN L
    {0x2F, 0x03},    // PRE Th AGCMAX H        
    {0x30, 0x84},    // PRE Th AGCMAX L              
    {0x31, 0x00},    // POST Th AGCMIN H             
    {0x32, 0x00},    // POST Th AGCMIN L
    {0x33, 0x00},    // POST Th AGCMAX H      
    {0x34, 0xC8},    // POST Th AGCMAX L             
    {0x35, 0x00},  //1D Y filter setting 
    {0x36, 0x10},
    {0x37, 0x50},
    {0x38, 0x00},
    {0x39, 0x14},
    {0x3A, 0x00},
    {0x3B, 0x50},
    {0x3C, 0x10},
    {0x3D, 0x50},
    {0x3E, 0x00},
    {0x3F, 0x28},
    {0x40, 0x00},
    {0x41, 0xA0},
    //*******************************************************************************************

    //�� å�Ӵ� mail �߰� ***********************************************************************
    // To avoid AWB tracking @ max AGC gain even though AE is unstable state
    {0xfc, 0x00},
    {0xba, 0x50},   // AE Target minus AE Average
    {0xbb, 0x00},
    {0xbc, 0x00},
    //*******************************************************************************************

    //3. AE weight & etc linear
    // AE Window Weight linear(EVT1)0929   
    {0xfc, 0x20},  // upper window weight zero
    {0x60, 0x11},
    {0x61, 0x11},
    {0x62, 0x11},
    {0x63, 0x11},
    {0x64, 0x11},
    {0x65, 0x11},
    {0x66, 0x11},
    {0x67, 0x11},
    {0x68, 0x11},
    {0x69, 0x11},
    {0x6a, 0x11},
    {0x6b, 0x11},
    {0x6c, 0x11},
    {0x6d, 0x11},
    {0x6e, 0x11},
    {0x6f, 0x11},
    {0x70, 0x11},
    {0x71, 0x11},
    {0x72, 0x11},
    {0x73, 0x11},
    {0x74, 0x11},
    {0x75, 0x11},
    {0x76, 0x11},
    {0x77, 0x11},
    {0x78, 0x11},
    {0x79, 0x11},
    {0x7a, 0x11},
    {0x7b, 0x11},
    {0x7c, 0x11},
    {0x7d, 0x11},
    {0x7e, 0x11},
    {0x7f, 0x11},

    // AE window Weight setting End
    //hue gain linear //
    {0xfc, 0x00},                          
    {0x48, 0x40},     
    {0x49, 0x40},   
    {0x4a, 0x00},     
    {0x4b, 0x00},     
    {0x4c, 0x40},     
    {0x4d, 0x40},     
    {0x4e, 0x00},     
    {0x4f, 0x00},     
    {0x50, 0x40},     
    {0x51, 0x40},     
    {0x52, 0x00},     
    {0x53, 0x00},     
    {0x54, 0x40},     
    {0x55, 0x40},     
    {0x56, 0x00},     
    {0x57, 0x00},     
    {0x58, 0x40},     
    {0x59, 0x40},     
    {0x5a, 0x00},     
    {0x5b, 0x00},    
    {0x5c, 0x40},     
    {0x5d, 0x40},     
    {0x5e, 0x00},     
    {0x5f, 0x00}, 
    {0x62, 0x00},  //hue enable OFF

    //4. shading (Flex�� 3000K manual shading) 
    {0xfc, 0x09},
    // DSP9_SH_WIDTH_H 
    {0x01, 0x06},
    {0x02, 0x40},
    // DSP9_SH_HEIGHT_H 
    {0x03, 0x04},
    {0x04, 0xB0},
    {0x05, 0x03},
    {0x06, 0x13},
    {0x07, 0x02},
    {0x08, 0x5A},
    {0x09, 0x03},
    {0x0A, 0x15},
    {0x0B, 0x02},
    {0x0C, 0x5B},
    {0x0D, 0x03},
    {0x0E, 0x0D},
    {0x0F, 0x02},
    {0x10, 0x5D},
    {0x1D, 0x80},
    {0x1E, 0x00},
    {0x1F, 0x80},
    {0x20, 0x00},
    {0x23, 0x80},
    {0x24, 0x00},
    {0x21, 0x80},
    {0x22, 0x00},
    {0x25, 0x80},
    {0x26, 0x00},
    {0x27, 0x80},
    {0x28, 0x00},
    {0x2B, 0x80},
    {0x2C, 0x00},
    {0x29, 0x80},
    {0x2A, 0x00},
    {0x2D, 0x80},
    {0x2E, 0x00},
    {0x2F, 0x80},
    {0x30, 0x00},
    {0x33, 0x80},
    {0x34, 0x00},
    {0x31, 0x80},
    {0x32, 0x00},
    // DSP9_SH_VAL_R0H 
    {0x35, 0x01},
    {0x36, 0x00},
    {0x37, 0x01},
    {0x38, 0x0F},
    {0x39, 0x01},
    {0x3A, 0x42},
    {0x3B, 0x01},
    {0x3C, 0x9C},
    {0x3D, 0x01},
    {0x3E, 0xD0},
    {0x3F, 0x02},
    {0x40, 0x0F},
    {0x41, 0x02},
    {0x42, 0x3D},
    {0x43, 0x02},
    {0x44, 0x5E},
    {0x45, 0x01},
    {0x46, 0x00},
    {0x47, 0x01},
    {0x48, 0x0A},
    {0x49, 0x01},
    {0x4A, 0x2E},
    {0x4B, 0x01},
    {0x4C, 0x66},
    {0x4D, 0x01},
    {0x4E, 0x89},
    {0x4F, 0x01},
    {0x50, 0xB7},
    {0x51, 0x01},
    {0x52, 0xD8},
    {0x53, 0x01},
    {0x54, 0xFA},
    // DS9_SH_VAL_B0H
    {0x55, 0x01},
    {0x56, 0x00},
    {0x57, 0x01},
    {0x58, 0x0A},
    {0x59, 0x01},
    {0x5A, 0x28},
    {0x5B, 0x01},
    {0x5C, 0x59},
    {0x5D, 0x01},
    {0x5E, 0x7A},
    {0x5F, 0x01},
    {0x60, 0xA1},
    {0x61, 0x01},
    {0x62, 0xC0},
    {0x63, 0x01},
    {0x64, 0xDC},
    // DSP9_SH_M_R2_R1H 
    {0x65, 0x00},
    {0x66, 0x9F},
    {0x67, 0xE6},
    {0x68, 0x02},
    {0x69, 0x7F},
    {0x6A, 0x9B},
    {0x6B, 0x05},
    {0x6C, 0x9F},
    {0x6D, 0x1E},
    {0x6E, 0x07},
    {0x6F, 0xA6},
    {0x70, 0xCC},
    {0x71, 0x09},
    {0x72, 0xFE},
    {0x73, 0x6E},
    {0x74, 0x0C},
    {0x75, 0xA6},
    {0x76, 0x04},
    {0x77, 0x0F},
    {0x78, 0x9D},
    {0x79, 0x8C},
    // DSP9_SH_M_R2_G1H 
    {0x7A, 0x00},
    {0x7B, 0x9F},
    {0x7C, 0x95},
    {0x7D, 0x02},
    {0x7E, 0x7E},
    {0x7F, 0x54},
    {0x80, 0x05},
    {0x81, 0x9C},
    {0x82, 0x3E},
    {0x83, 0x07},
    {0x84, 0xA2},
    {0x85, 0xE3},
    {0x86, 0x09},
    {0x87, 0xF9},
    {0x88, 0x53},
    {0x89, 0x0C},
    {0x8A, 0x9F},
    {0x8B, 0x8D},
    {0x8C, 0x0F},
    {0x8D, 0x95},
    {0x8E, 0x91},
    // DSP9_SH_M_R2_B1H 
    {0x8F, 0x00},
    {0x90, 0xA1},
    {0x91, 0xFF},
    {0x92, 0x02},
    {0x93, 0x87},
    {0x94, 0xFD},
    {0x95, 0x05},
    {0x96, 0xB1},
    {0x97, 0xFA},
    {0x98, 0x07},
    {0x99, 0xC0},
    {0x9A, 0x79},
    {0x9B, 0x0A},
    {0x9C, 0x1F},
    {0x9D, 0xF6},
    {0x9E, 0x0C},
    {0x9F, 0xD0},
    {0xA0, 0x74},
    {0xA1, 0x0F},
    {0xA2, 0xD1},
    {0xA3, 0xF1},
    // DSP9_SH_SUB_RR0H 
    {0xA4, 0x66},
    {0xA5, 0x76},
    {0xA6, 0x22},
    {0xA7, 0x27},
    {0xA8, 0x14},
    {0xA9, 0x7E},
    {0xAA, 0x1F},
    {0xAB, 0x86},
    {0xAC, 0x1B},
    {0xAD, 0x52},
    {0xAE, 0x18},
    {0xAF, 0x1B},
    {0xB0, 0x15},
    {0xB1, 0x92},
    // DSP9_SH_SUB_RG0H 
    {0xB2, 0x66},
    {0xB3, 0xAA},
    {0xB4, 0x22},
    {0xB5, 0x38},
    {0xB6, 0x14},
    {0xB7, 0x88},
    {0xB8, 0x1F},
    {0xB9, 0x97},
    {0xBA, 0x1B},
    {0xBB, 0x60},
    {0xBC, 0x18},
    {0xBD, 0x28},
    {0xBE, 0x15},
    {0xBF, 0x9D},
    // DSP9_SH_SUB_RB0H 
    {0xC0, 0x65},
    {0xC1, 0x23},
    {0xC2, 0x21},
    {0xC3, 0xB6},
    {0xC4, 0x14},
    {0xC5, 0x3A},
    {0xC6, 0x1F},
    {0xC7, 0x1E},
    {0xC8, 0x1A},
    {0xC9, 0xF8},
    {0xCA, 0x17},
    {0xCB, 0xCC},
    {0xCC, 0x15},
    {0xCD, 0x4A},
    {0x00, 0x02},  // shading on

    {0xfc, 0x00},
    {0x79, 0xf4},
    {0x7a, 0x09},

    //5.color correction
    //1229 CCM
    //2.0251    -1.0203 -0.0048 
    //-0.7080   1.8970  -0.1889 
    //-0.468    -0.444  1.912
    {0xfc, 0x01},
    {0x51, 0x08},  //R
    {0x52, 0x18},
    {0x53, 0xfb},
    {0x54, 0xec},
    {0x55, 0xff},
    {0x56, 0xfc},
    {0x57, 0xfd},  //G
    {0x58, 0x2c},
    {0x59, 0x07},
    {0x5a, 0x95},
    {0x5b, 0xff},
    {0x5c, 0x3f},
    {0x5d, 0xfe},  //B
    {0x5e, 0x22},  
    {0x5f, 0xfe},  
    {0x60, 0x3a},  
    {0x61, 0x07},  
    {0x62, 0xa5},  

    //6.gamma 
    //Gamma 
    {0xfc, 0x01},           
    // R                                               
    {0x6F, 0x05},  
    {0x70, 0x14},  
    {0x71, 0x3c},  
    {0x72, 0x96},  
    {0x73, 0x00},  
    {0x74, 0x2c},  
    {0x75, 0xa2},  
    {0x76, 0xfc},  
    {0x77, 0x44},  
    {0x78, 0x56},  
    {0x79, 0x80},  
    {0x7A, 0xb7},  
    {0x7B, 0xed},
    {0x7C, 0x16},
    {0x7D, 0xab},
    {0x7E, 0x3c},
    {0x7F, 0x61},
    {0x80, 0x83},
    {0x81, 0xa4},                            
    {0x82, 0xff},                              
    {0x83, 0xc4},                              
    {0x84, 0xe2},                            
    {0x85, 0xff},                              
    {0x86, 0xff},                              
    // G                                  
    {0x87, 0x05},  
    {0x88, 0x14},  
    {0x89, 0x3c},        
    {0x8A, 0x96},  
    {0x8B, 0x00},  
    {0x8C, 0x2c},  
    {0x8D, 0xa2},  
    {0x8E, 0xfc},  
    {0x8F, 0x44},  
    {0x90, 0x56},  
    {0x91, 0x80},  
    {0x92, 0xb7},  
    {0x93, 0xed},        
    {0x94, 0x16},          
    {0x95, 0xab},          
    {0x96, 0x3c},               
    {0x97, 0x61},               
    {0x98, 0x83},              
    {0x99, 0xa4},               
    {0x9A, 0xff},        
    {0x9B, 0xc4},          
    {0x9C, 0xe2},          
    {0x9D, 0xff},          
    {0x9E, 0xff},          
    //B      
    {0x9F, 0x05},  
    {0xA0, 0x10},  
    {0xA1, 0x30},  
    {0xA2, 0x70},  
    {0xA3, 0x00},  
    {0xA4, 0x2c},  
    {0xA5, 0xa2},  
    {0xA6, 0xfc},  
    {0xA7, 0x44},       
    {0xA8, 0x56},       
                    
    {0xA9, 0x80},       
    {0xAA, 0xb7},       
    {0xAB, 0xed},  
    {0xAC, 0x16},  
    {0xAD, 0xab},  
            
    {0xAE, 0x3c},  
    {0xAF, 0x61},  
    {0xB0, 0x83},  
    {0xB1, 0xa4},  
    {0xB2, 0xff},  
            
    {0xB3, 0xc4},  
    {0xB4, 0xe2},  
    {0xB5, 0xff},  
    {0xB6, 0xff},

    //7.hue 
    {0xFC, 0x00},
    {0x62, 0x00},    // hue auto control off

    {0xFC, 0x05},
    {0x4E, 0x60},
    {0x4F, 0xA0},
    {0x50, 0x35},
    {0x51, 0xA0},
    {0x52, 0x20},
    {0x53, 0x01},
    {0x54, 0xE0},
    {0x55, 0xE0},
    {0x56, 0x54},
    {0x57, 0x20},
    {0x58, 0x20},
    {0x59, 0xF0},

    //8.white point 
    //AWB Start Point
    {0xfc, 0x07},
    {0x05, 0x00},
    {0x06, 0x08},
    {0x07, 0x1b},
    {0x08, 0xf0},
    {0x09, 0x00},  // R
    {0x0a, 0xa8},  
    {0x0b, 0x00},  // B
    {0x0c, 0xb0},
    {0x0d, 0x00},  // G
    {0x0e, 0x40},

    {0xfc, 0x00},
    {0x70, 0x02},

    {0x40, 0x8a},  //2000K
    {0x41, 0xe5},  
    {0x42, 0x95},  //3100K
    {0x43, 0xba},  
    {0x44, 0xbc},  //5100K
    {0x45, 0x99},        

    {0x34, 0x24},
    {0x35, 0x10},
    {0x36, 0x13},
    {0x37, 0x04},
    {0x38, 0x10},
    {0x39, 0x28},
    {0x3a, 0x1e},
    {0x3b, 0x2a},

    {0x31, 0x00},  // skin tone[6], CW delete[5]  
};
#endif

#define S5K3BA_INIT_REGS	(sizeof(s5k3ba_init_reg) / sizeof(s5k3ba_init_reg[0]))

/*
 * EV bias
 */

static const struct s5k3ba_reg s5k3ba_ev_m6[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_m5[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_m4[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_m3[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_m2[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_m1[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_default[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p1[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p2[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p3[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p4[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p5[] = {
};

static const struct s5k3ba_reg s5k3ba_ev_p6[] = {
};

#ifdef S5K3BA_COMPLETE
/* Order of this array should be following the querymenu data */
static const unsigned char *s5k3ba_regs_ev_bias[] = {
	(unsigned char *)s5k3ba_ev_m6, (unsigned char *)s5k3ba_ev_m5,
	(unsigned char *)s5k3ba_ev_m4, (unsigned char *)s5k3ba_ev_m3,
	(unsigned char *)s5k3ba_ev_m2, (unsigned char *)s5k3ba_ev_m1,
	(unsigned char *)s5k3ba_ev_default, (unsigned char *)s5k3ba_ev_p1,
	(unsigned char *)s5k3ba_ev_p2, (unsigned char *)s5k3ba_ev_p3,
	(unsigned char *)s5k3ba_ev_p4, (unsigned char *)s5k3ba_ev_p5,
	(unsigned char *)s5k3ba_ev_p6,
};

/*
 * Auto White Balance configure
 */
static const struct s5k3ba_reg s5k3ba_awb_off[] = {
};

static const struct s5k3ba_reg s5k3ba_awb_on[] = {
};

static const unsigned char *s5k3ba_regs_awb_enable[] = {
	(unsigned char *)s5k3ba_awb_off,
	(unsigned char *)s5k3ba_awb_on,
};

/*
 * Manual White Balance (presets)
 */
static const struct s5k3ba_reg s5k3ba_wb_tungsten[] = {

};

static const struct s5k3ba_reg s5k3ba_wb_fluorescent[] = {

};

static const struct s5k3ba_reg s5k3ba_wb_sunny[] = {

};

static const struct s5k3ba_reg s5k3ba_wb_cloudy[] = {

};

/* Order of this array should be following the querymenu data */
static const unsigned char *s5k3ba_regs_wb_preset[] = {
	(unsigned char *)s5k3ba_wb_tungsten,
	(unsigned char *)s5k3ba_wb_fluorescent,
	(unsigned char *)s5k3ba_wb_sunny,
	(unsigned char *)s5k3ba_wb_cloudy,
};

/*
 * Color Effect (COLORFX)
 */
static const struct s5k3ba_reg s5k3ba_color_sepia[] = {
};

static const struct s5k3ba_reg s5k3ba_color_aqua[] = {
};

static const struct s5k3ba_reg s5k3ba_color_monochrome[] = {
};

static const struct s5k3ba_reg s5k3ba_color_negative[] = {
};

static const struct s5k3ba_reg s5k3ba_color_sketch[] = {
};

/* Order of this array should be following the querymenu data */
static const unsigned char *s5k3ba_regs_color_effect[] = {
	(unsigned char *)s5k3ba_color_sepia,
	(unsigned char *)s5k3ba_color_aqua,
	(unsigned char *)s5k3ba_color_monochrome,
	(unsigned char *)s5k3ba_color_negative,
	(unsigned char *)s5k3ba_color_sketch,
};

/*
 * Contrast bias
 */
static const struct s5k3ba_reg s5k3ba_contrast_m2[] = {
};

static const struct s5k3ba_reg s5k3ba_contrast_m1[] = {
};

static const struct s5k3ba_reg s5k3ba_contrast_default[] = {
};

static const struct s5k3ba_reg s5k3ba_contrast_p1[] = {
};

static const struct s5k3ba_reg s5k3ba_contrast_p2[] = {
};

static const unsigned char *s5k3ba_regs_contrast_bias[] = {
	(unsigned char *)s5k3ba_contrast_m2,
	(unsigned char *)s5k3ba_contrast_m1,
	(unsigned char *)s5k3ba_contrast_default,
	(unsigned char *)s5k3ba_contrast_p1,
	(unsigned char *)s5k3ba_contrast_p2,
};

/*
 * Saturation bias
 */
static const struct s5k3ba_reg s5k3ba_saturation_m2[] = {
};

static const struct s5k3ba_reg s5k3ba_saturation_m1[] = {
};

static const struct s5k3ba_reg s5k3ba_saturation_default[] = {
};

static const struct s5k3ba_reg s5k3ba_saturation_p1[] = {
};

static const struct s5k3ba_reg s5k3ba_saturation_p2[] = {
};

static const unsigned char *s5k3ba_regs_saturation_bias[] = {
	(unsigned char *)s5k3ba_saturation_m2,
	(unsigned char *)s5k3ba_saturation_m1,
	(unsigned char *)s5k3ba_saturation_default,
	(unsigned char *)s5k3ba_saturation_p1,
	(unsigned char *)s5k3ba_saturation_p2,
};

/*
 * Sharpness bias
 */
static const struct s5k3ba_reg s5k3ba_sharpness_m2[] = {
};

static const struct s5k3ba_reg s5k3ba_sharpness_m1[] = {
};

static const struct s5k3ba_reg s5k3ba_sharpness_default[] = {
};

static const struct s5k3ba_reg s5k3ba_sharpness_p1[] = {
};

static const struct s5k3ba_reg s5k3ba_sharpness_p2[] = {
};

static const unsigned char *s5k3ba_regs_sharpness_bias[] = {
	(unsigned char *)s5k3ba_sharpness_m2,
	(unsigned char *)s5k3ba_sharpness_m1,
	(unsigned char *)s5k3ba_sharpness_default,
	(unsigned char *)s5k3ba_sharpness_p1,
	(unsigned char *)s5k3ba_sharpness_p2,
};
#endif /* S5K3BA_COMPLETE */

#endif
