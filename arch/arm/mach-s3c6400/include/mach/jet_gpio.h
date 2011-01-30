/*
 *  linux/include/asm-arm/arch-s3c6400/jet_gpio.h
 *
 *  Author:	dopi711@googlemail.com
 *  Created:	07, Jun, 2010
 *  Copyright:  JetDroid project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef ASM_MACH_JET_GPIO_H

#define ASM_MACH_JET_GPIO_H

/*
 * Memory Configuration (Reserved Memory Setting)
 */

#define	PHYS_SIZE					(208 * 1024 * 1024)		/* 208MB */

#define CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA

/*
 * GPIO Configuration
 */

#define GPIO_LEVEL_LOW      		0
#define GPIO_LEVEL_HIGH     		1
#define GPIO_LEVEL_NONE     		2

/* S3C64XX_GPA(0) ~ S3C64XX_GPA(7) */

#define GPIO_BT_RXD				S3C64XX_GPA(4)
#define GPIO_BT_RXD_AF				2		// UART RXD[1]

#define GPIO_BT_TXD				S3C64XX_GPA(5)
#define GPIO_BT_TXD_AF				2		// UART TXD[1]

#define GPIO_BT_CTS				S3C64XX_GPA(6)
#define GPIO_BT_CTS_AF				2		// UART CTSn[1]

#define GPIO_BT_RTS				S3C64XX_GPA(7)
#define GPIO_BT_RTS_AF				2		// UART RTSn[1]

/* S3C64XX_GPB(0) ~ S3C64XX_GPB(6) */

#define GPIO_AP_RXD				S3C64XX_GPB(0) 
#define GPIO_AP_RXD_AF				2

#define GPIO_AP_TXD				S3C64XX_GPB(1)
#define GPIO_AP_TXD_AF				2

#define GPIO_AP_FLM_RXD				S3C64XX_GPB(2) 	
#define GPIO_AP_FLM_RXD_AF			2

#define GPIO_AP_FLM_TXD				S3C64XX_GPB(3)
#define GPIO_AP_FLM_TXD_AF			2

// S3C64XX_GPB(4)	N.C.

#define GPIO_AP_SCL_3V		    		S3C64XX_GPB(5)
#define GPIO_AP_SCL_3V_AF   			2

#define GPIO_AP_SDA_3V		    		S3C64XX_GPB(6)
#define GPIO_AP_SDA_3V_AF			2


/* S3C64XX_GPC(0) ~ S3C64XX_GPC(7) */

// S3C64XX_GPC(0)	N.C.

#define GPIO_BT_RST_N				S3C64XX_GPC(1)	// BT_RESET
#define GPIO_BT_RST_N_AF			1

#define GPIO_WLAN_BT_SHUTDOWN			S3C64XX_GPC(2)	// GPIO_BT_WLAN_REG_ON
#define GPIO_WLAN_BT_SHUTDOWN_AF		1

#define GPIO_WLAN_RST_N				S3C64XX_GPC(3)	// WLAN_RESET
#define GPIO_WLAN_RST_N_AF			1

#define GPIO_WLAN_CMD				S3C64XX_GPC(4)
#define GPIO_WLAN_CMD_AF			3

#define GPIO_WLAN_CLK				S3C64XX_GPC(5)
#define GPIO_WLAN_CLK_AF			3

// S3C64XX_GPC(6)	N.C.

// S3C64XX_GPC(7)	N.C.


/* S3C64XX_GPD(0) ~ S3C64XX_GPD(4) */

#define GPIO_I2S_CLK				S3C64XX_GPD(0)
#define GPIO_I2S_CLK_AF				3		// I2S CLK[0]

// S3C64XX_GPD(1)	N.C.

#define GPIO_I2S_LRCLK				S3C64XX_GPD(2)  // AP_I2S_SYNC
#define GPIO_I2S_LRCLK_AF			3		// I2S LRCLK[0]

#define GPIO_I2S_DI				S3C64XX_GPD(3)
#define GPIO_I2S_DI_AF				3		// I2S DI[0]

#define GPIO_I2S_DO				S3C64XX_GPD(4)
#define GPIO_I2S_DO_AF				3		// I2S DO[0]


/* S3C64XX_GPE(0) ~ S3C64XX_GPE(4) */

#define GPIO_VT_PCM_CLK				S3C64XX_GPE(0)
#define GPIO_VT_PCM_CLK_AF			2		// PCM SCLK[1] 

#define GPIO_WLAN_WAKE				S3C64XX_GPE(1)
#define GPIO_WLAN_WAKE_AF			1  		// 1 = output

#define GPIO_VT_PCM_SYNC			S3C64XX_GPE(2)
#define GPIO_VT_PCM_SYNC_AF			2		// PCM SYNC[1]

#define GPIO_VT_PCM_IN				S3C64XX_GPE(3)
#define GPIO_VT_PCM_IN_AF			2		// PCM IN[1]

#define GPIO_VT_PCM_OUT				S3C64XX_GPE(4)
#define GPIO_VT_PCM_OUT_AF			2		// PCM OUT[1]


/* S3C64XX_GPF(0) ~ S3C64XX_GPF(15) */

#define GPIO_CAM_MCLK				S3C64XX_GPF(0)
#define GPIO_CAM_MCLK_AF			2		// CAMIF CLK

#define GPIO_CAM_HSYNC				S3C64XX_GPF(1)
#define GPIO_CAM_HSYNC_AF			2		// CAMIF HREF

#define GPIO_CAM_PCLK				S3C64XX_GPF(2)
#define GPIO_CAM_PCLK_AF			2		// CAMIF PCLK

#define GPIO_MCAM_RST_N				S3C64XX_GPF(3)
#define GPIO_MCAM_RST_N_AF			1		// 1 = Output, 2 = CAMIF RSTn

#define GPIO_CAM_VSYNC				S3C64XX_GPF(4)
#define GPIO_CAM_VSYNC_AF			2		// CAMIF VSYNC

#define GPIO_CAM_D_0				S3C64XX_GPF(5)
#define GPIO_CAM_D_0_AF				2		// CAMIF YDATA[0]

#define GPIO_CAM_D_1				S3C64XX_GPF(6)
#define GPIO_CAM_D_1_AF				2		// CAMIF YDATA[1]

#define GPIO_CAM_D_2				S3C64XX_GPF(7)
#define GPIO_CAM_D_2_AF				2		// CAMIF YDATA[2]

#define GPIO_CAM_D_3				S3C64XX_GPF(8)
#define GPIO_CAM_D_3_AF				2		// CAMIF YDATA[3]

#define GPIO_CAM_D_4				S3C64XX_GPF(9)
#define GPIO_CAM_D_4_AF				2		// CAMIF YDATA[4]

#define GPIO_CAM_D_5				S3C64XX_GPF(10)
#define GPIO_CAM_D_5_AF				2		// CAMIF YDATA[5]

#define GPIO_CAM_D_6				S3C64XX_GPF(11)
#define GPIO_CAM_D_6_AF				2		// CAMIF YDATA[6]

#define GPIO_CAM_D_7				S3C64XX_GPF(12)
#define GPIO_CAM_D_7_AF				2		// CAMIF YDATA[7]

// S3C64XX_GPD(13)	N.C.

// S3C64XX_GPD(14)	N.C.

#define GPIO_VIBTONE_PWM			S3C64XX_GPF(15)  // VIBTONE_PWM
#define GPIO_VIBTONE_PWM_AF			2


/* S3C64XX_GPG(0) ~ S3C64XX_GPG(6) */

#define GPIO_TF_CLK				S3C64XX_GPG(0)
#define GPIO_TF_CLK_AF				2

#define GPIO_TF_CMD				S3C64XX_GPG(1)
#define GPIO_TF_CMD_AF				2

#define GPIO_TF_D_0				S3C64XX_GPG(2)
#define GPIO_TF_D_0_AF				2

#define GPIO_TF_D_1				S3C64XX_GPG(3)
#define GPIO_TF_D_1_AF				2

#define GPIO_TF_D_2				S3C64XX_GPG(4)
#define GPIO_TF_D_2_AF				2

#define GPIO_TF_D_3				S3C64XX_GPG(5)
#define GPIO_TF_D_3_AF				2

#define GPIO_TFLASH_EN				S3C64XX_GPG(6)
#define GPIO_TFLASH_EN_AF			1		// 1 = Output

/* S3C64XX_GPH(1) ~ S3C64XX_GPH(9) */

#define GPIO_NAND_CLK				S3C64XX_GPH(0)
#define GPIO_NAND_CLK_AF			2		// MMC CLK1

#define GPIO_NAND_CMD				S3C64XX_GPH(1)
#define GPIO_NAND_CMD_AF			2		// MMC CMD1

#define GPIO_NAND_D_0				S3C64XX_GPH(2)
#define GPIO_NAND_D_0_AF			2		// MMC DATA1[0]

#define GPIO_NAND_D_1				S3C64XX_GPH(3)
#define GPIO_NAND_D_1_AF			2		// MMC DATA1[1]

#define GPIO_NAND_D_2				S3C64XX_GPH(4)
#define GPIO_NAND_D_2_AF			2		// MMC DATA1[2]

#define GPIO_NAND_D_3				S3C64XX_GPH(5)
#define GPIO_NAND_D_3_AF			2		// MMC DATA1[3]

#define GPIO_WLAN_D_0				S3C64XX_GPH(6)  // SDIO_DATA(0)
#define GPIO_WLAN_D_0_AF			3		// MMC DATA2[0]

#define GPIO_WLAN_D_1				S3C64XX_GPH(7)  // SDIO_DATA(1)
#define GPIO_WLAN_D_1_AF			3		// MMC DATA2[1]

#define GPIO_WLAN_D_2				S3C64XX_GPH(8)  // SDIO_DATA(2)
#define GPIO_WLAN_D_2_AF			3		// MMC DATA2[2]

#define GPIO_WLAN_D_3				S3C64XX_GPH(9)  // SDIO_DATA(3)
#define GPIO_WLAN_D_3_AF			3		// MMC DATA2[2]

/* S3C64XX_GPI(0) ~ S3C64XX_GPI(15) */
/* S3C64XX_GPJ(0) ~ S3C64XX_GPJ(11) */

#define GPIO_LCD_B_0				S3C64XX_GPI(0)
#define GPIO_LCD_B_0_AF				2

#define GPIO_LCD_B_1				S3C64XX_GPI(1)
#define GPIO_LCD_B_1_AF				2

#define GPIO_LCD_B_2				S3C64XX_GPI(2)
#define GPIO_LCD_B_2_AF				2

#define GPIO_LCD_B_3				S3C64XX_GPI(3)
#define GPIO_LCD_B_3_AF				2

#define GPIO_LCD_B_4				S3C64XX_GPI(4)
#define GPIO_LCD_B_4_AF				2

#define GPIO_LCD_B_5				S3C64XX_GPI(5)
#define GPIO_LCD_B_5_AF				2

#define GPIO_LCD_B_6				S3C64XX_GPI(6)
#define GPIO_LCD_B_6_AF				2

#define GPIO_LCD_B_7				S3C64XX_GPI(7)
#define GPIO_LCD_B_7_AF				2

#define GPIO_LCD_G_0				S3C64XX_GPI(8)
#define GPIO_LCD_G_0_AF				2

#define GPIO_LCD_G_1				S3C64XX_GPI(9)
#define GPIO_LCD_G_1_AF				2

#define GPIO_LCD_G_2				S3C64XX_GPI(10)
#define GPIO_LCD_G_2_AF				2

#define GPIO_LCD_G_3				S3C64XX_GPI(11)
#define GPIO_LCD_G_3_AF				2

#define GPIO_LCD_G_4				S3C64XX_GPI(12)
#define GPIO_LCD_G_4_AF				2

#define GPIO_LCD_G_5				S3C64XX_GPI(13)
#define GPIO_LCD_G_5_AF				2

#define GPIO_LCD_G_6				S3C64XX_GPI(14)
#define GPIO_LCD_G_6_AF				2

#define GPIO_LCD_G_7				S3C64XX_GPI(15)
#define GPIO_LCD_G_7_AF				2

#define GPIO_LCD_R_0				S3C64XX_GPJ(0)
#define GPIO_LCD_R_0_AF				2

#define GPIO_LCD_R_1				S3C64XX_GPJ(1)
#define GPIO_LCD_R_1_AF				2

#define GPIO_LCD_R_2				S3C64XX_GPJ(2)
#define GPIO_LCD_R_2_AF				2

#define GPIO_LCD_R_3				S3C64XX_GPJ(3)
#define GPIO_LCD_R_3_AF				2

#define GPIO_LCD_R_4				S3C64XX_GPJ(4)
#define GPIO_LCD_R_4_AF				2

#define GPIO_LCD_R_5				S3C64XX_GPJ(5)
#define GPIO_LCD_R_5_AF				2

#define GPIO_LCD_R_6				S3C64XX_GPJ(6)
#define GPIO_LCD_R_6_AF				2

#define GPIO_LCD_R_7				S3C64XX_GPJ(7)
#define GPIO_LCD_R_7_AF				2

#define GPIO_LCD_HSYNC				S3C64XX_GPJ(8)
#define GPIO_LCD_HSYNC_AF			2

#define GPIO_LCD_VSYNC				S3C64XX_GPJ(9)
#define GPIO_LCD_VSYNC_AF			2

#define GPIO_LCD_DE				S3C64XX_GPJ(10)
#define GPIO_LCD_DE_AF				2

#define GPIO_LCD_CLK				S3C64XX_GPJ(11)
#define GPIO_LCD_CLK_AF				2

/* S3C64XX_GPK(0) ~ S3C64XX_GPK(15) */

#define GPIO_TA_SEL				S3C64XX_GPK(0)
#define GPIO_TA_SEL_AF				0		// input (FIXME: eigentlich irq IRQ ???)

#define GPIO_CAM_EN				S3C64XX_GPK(1)
#define GPIO_CAM_EN_AF				1

#define GPIO_EARPATH_SEL			S3C64XX_GPK(2)
#define GPIO_EARPATH_SEL_AF			1

#define GPIO_EAR_CP_CODEC_SW 			S3C64XX_GPK(3)
#define GPIO_EAR_CP_CODEC_SW_AF			1

#define GPIO_MAIN_CP_CODEC_SW 			S3C64XX_GPK(4)
#define GPIO_MAIN_CP_CODEC_SW_AF		1

#define GPIO_FM_RST				S3C64XX_GPK(5)
#define GPIO_FM_RST_AF				1

#define GPIO_USBSW_SCL_3V0			S3C64XX_GPK(6)
#define GPIO_USBSW_SCL_3V0_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_USBSW_SDA_3V0			S3C64XX_GPK(7)
#define GPIO_USBSW_SDA_3V0_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_KEYSENSE_0				S3C64XX_GPK(8)
#define GPIO_KEYSENSE_0_AF			3

#define GPIO_KEYSENSE_1				S3C64XX_GPK(9)
#define GPIO_KEYSENSE_1_AF			3

#define GPIO_KEYSENSE_2				S3C64XX_GPK(10)
#define GPIO_KEYSENSE_2_AF			3

#define GPIO_KEYSENSE_3				S3C64XX_GPK(11)
#define GPIO_KEYSENSE_3_AF			3

#define GPIO_CAM_VGA_nRST			S3C64XX_GPK(12)
#define GPIO_CAM_VGA_nRST_AF			1		// 1 = output

#define GPIO_CAM_VGA_nSTBY			S3C64XX_GPK(13)
#define GPIO_CAM_VGA_nSTBY_AF			1		// 1 = output

#define GPIO_MSENSE_RST_N			S3C64XX_GPK(14)
#define GPIO_MSENSE_RST_N_AF			1

#define GPIO_USIM_BOOT				S3C64XX_GPK(15)
#define GPIO_USIM_BOOT_AF			1 		// 1 = Output

/* S3C64XX_GPL(0) ~ S3C64XX_GPL(14) */

#define GPIO_KEYSCAN_0				S3C64XX_GPL(0)
#define GPIO_KEYSCAN_0_AF			3

#define GPIO_KEYSCAN_1				S3C64XX_GPL(1)
#define GPIO_KEYSCAN_1_AF			3

#define GPIO_KEYSCAN_2				S3C64XX_GPL(2)
#define GPIO_KEYSCAN_2_AF			3

#define GPIO_5M_EN				S3C64XX_GPL(3)
#define GPIO_5M_EN_AF				1

#define GPIO_VIBTONE_EN				S3C64XX_GPL(4)
#define GPIO_VIBTONE_EN_AF			1

#define GPIO_USB_SEL				S3C64XX_GPL(5)
#define GPIO_USB_SEL_AF				1

#define GPIO_TV_EN				S3C64XX_GPL(6)
#define GPIO_TV_EN_AF 				1

#define GPIO_TOUCH_EN				S3C64XX_GPL(7)
#define GPIO_TOUCH_EN_AF			1

#define GPIO_T_FLASH_DETECT			S3C64XX_GPL(8)
#define GPIO_T_FLASH_DETECT_AF			3		// Ext. Interrupt [16] 

#define GPIO_DET_35				S3C64XX_GPL(9)
#define GPIO_DET_35_AF				3		// Ext. Interrupt [17]

#define GPIO_FM_INT				S3C64XX_GPL(10)
#define GPIO_FM_INT_AF				3		// Ext. Interrupt [18] 

#define GPIO_EXTWKUP 				S3C64XX_GPL(11)
#define GPIO_EXTWKUP_AF				3 // in/out/int ???

#define GPIO_PS_VOUT				S3C64XX_GPL(12)	// GPIO_PS_VOUT_30 
#define GPIO_PS_VOUT_AF				3		// Ext. Interrupt [20] 

#define GPIO_BOOT_MODE				S3C64XX_GPL(13)
#define GPIO_BOOT_MODE_AF			0	

#define GPIO_ACC_INT				S3C64XX_GPL(14)
#define GPIO_ACC_INT_AF				2

/* S3C64XX_GPM(0) ~ S3C64XX_GPM(5) */

#define GPIO_FM_I2C_SCL				S3C64XX_GPM(0)
#define GPIO_FM_I2C_SCL_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_FM_I2C_SDA				S3C64XX_GPM(1)
#define GPIO_FM_I2C_SDA_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_WLAN_HOST_WAKE			S3C64XX_GPM(2)
#define GPIO_WLAN_HOST_WAKE_AF			3

#define GPIO_BT_HOST_WAKE			S3C64XX_GPM(3)
#define GPIO_BT_HOST_WAKE_AF			3

#define GPIO_BT_WAKE				S3C64XX_GPM(4)
#define GPIO_BT_WAKE_AF				1  // output

#define GPIO_ALPS_ON				S3C64XX_GPM(5) 	
#define GPIO_ALPS_ON_AF				1

/* S3C64XX_GPN(0) ~ S3C64XX_GPN(15) */

#define GPIO_ONEDRAM_INT_N			S3C64XX_GPN(0)
#define GPIO_ONEDRAM_INT_N_AF			2

#define GPIO_MAX8906_AMP_EN			S3C64XX_GPN(1)	
#define GPIO_MAX8906_AMP_EN_AF			1		// 1 = output

#define GPIO_MSENSE_INT				S3C64XX_GPN(2)
#define GPIO_MSENSE_INT_AF			2

#define GPIO_PDA_ACTIVE				S3C64XX_GPN(3)
#define GPIO_PDA_ACTIVE_AF			1

#define GPIO_FM_LDO_ON				S3C64XX_GPN(4)
#define GPIO_FM_LDO_ON_AF			1		// 1 = output

#define GPIO_POWER_N				S3C64XX_GPN(5)
#define GPIO_POWER_N_AF				2		

#define GPIO_PHONE_ON				S3C64XX_GPN(6)
#define GPIO_PHONE_ON_AF			1		// 1 = Output

#define GPIO_PHONE_ACTIVE			S3C64XX_GPN(7)
#define GPIO_PHONE_ACTIVE_AF			2		// 2 = Interrupt

#define GPIO_PMIC_INT_N				S3C64XX_GPN(8)
#define GPIO_PMIC_INT_N_AF			2

#define GPIO_PWR_I2C_SCL			S3C64XX_GPN(9)	// PWR_SCL_3.0V
#define GPIO_PWR_I2C_SCL_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_PWR_I2C_SDA			S3C64XX_GPN(10)	// PWR_SDA_3.0V
#define GPIO_PWR_I2C_SDA_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_EAR_SEND_END			S3C64XX_GPN(11)
#define GPIO_EAR_SEND_END_AF			2

#define GPIO_JACK_INT_N				S3C64XX_GPN(12)
#define GPIO_JACK_INT_N_AF			2

#define GPIO_BOOT_EINT13			S3C64XX_GPN(13)
#define GPIO_BOOT_EINT13_AF			2

#define GPIO_BOOT_EINT14			S3C64XX_GPN(14)
#define GPIO_BOOT_EINT14_AF			2

#define GPIO_BOOT_EINT15			S3C64XX_GPN(15)
#define GPIO_BOOT_EINT15_AF			2

/* S3C64XX_GPO(0) ~ S3C64XX_GPO(15) */

#define GPIO_NAND_CS_N				S3C64XX_GPO(0)
#define GPIO_NAND_CS_N_AF			2

// S3C64XX_GPO(1)

#define GPIO_CP_BOOT_SEL			S3C64XX_GPO(2)	// aka. as FLM_SEL
#define GPIO_CP_BOOT_SEL_AF			1		// 1 = Output

#define GPIO_RESOUT_N				S3C64XX_GPO(3)
#define GPIO_RESOUT_N_AF			3		// Ext. Interrupt Group7 [3]

// S3C64XX_GPO(4)

#define GPIO_PCM_SEL				S3C64XX_GPO(5)	// switch to select PCM connection from/to BT module (CP=MSM or VT=S3C6410_PCM1)
#define GPIO_PCM_SEL_AF				1		// 1 = output

#define GPIO_LCD_CS_N			    	S3C64XX_GPO(6)
#define GPIO_LCD_CS_N_AF			1		// 1 = output

#define GPIO_LCD_SDI			    	S3C64XX_GPO(7)
#define GPIO_LCD_SDI_AF				1

#define GPIO_AP_SCL_1V8			    	S3C64XX_GPO(8)
#define GPIO_AP_SCL_1V8_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_AP_SDA_1V8			    	S3C64XX_GPO(9)
#define GPIO_AP_SDA_1V8_AF			0		// should be output (1) but all other sources init it as input (0)

#define GPIO_LCD_RST_N				S3C64XX_GPO(10)
#define GPIO_LCD_RST_N_AF			1

#define GPIO_LCD_DET				S3C64XX_GPO(11)	
#define GPIO_LCD_DET_AF				1	

#define GPIO_LCD_SCLK				S3C64XX_GPO(12)
#define GPIO_LCD_SCLK_AF			1

#define GPIO_LCD_SDO			    	S3C64XX_GPO(13)	// this is actually an NC pin
#define GPIO_LCD_SDO_AF				1

// S3C64XX_GPO(13)

// S3C64XX_GPO(14)

// S3C64XX_GPO(15)


/* S3C64XX_GPP(0) ~ S3C64XX_GPP(14) */

#define GPIO_PDA_PS_HOLD			S3C64XX_GPP(13)
#define GPIO_PDA_PS_HOLD_AF			1

/* S3C64XX_GPQ(0) ~ S3C64XX_GPQ(8) */

#define GPIO_UART_SEL				S3C64XX_GPQ(2)
#define GPIO_UART_SEL_AF			1

#define GPIO_VT_CP_SW				S3C64XX_GPQ(4)	// switch to select source for speaker output (CP=MSM or VT=MAX9880) 
#define GPIO_VT_CP_SW_AF			1		// 1 = output	

#define GPIO_LCD_ID				S3C64XX_GPQ(5)
#define GPIO_LCD_ID_AF				0

/*
 * Partition Information
 */

#define BOOT_PART_ID			0x0
#define CSA_PART_ID			0xF		/* Not Used */
#define SBL_PART_ID			0x1
#define PARAM_PART_ID			0x2
#define KERNEL_PART_ID			0x3
#define RAMDISK_PART_ID			0xF		/* Not Used */
#define FILESYSTEM_PART_ID		0x4
#define FILESYSTEM1_PART_ID		0x5
#define FILESYSTEM2_PART_ID		0x6
#define TEMP_PART_ID			0x7		/* Temp Area for FOTA */
#define MODEM_IMG_PART_ID		0x8
#define MODEM_EFS_PART_ID		0x9		/* Modem EFS Area for OneDRAM */

#endif	/* ASM_MACH_JET_GPIO_H */

