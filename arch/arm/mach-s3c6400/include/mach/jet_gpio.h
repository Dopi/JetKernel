/*
 *  linux/include/asm-arm/arch-s3c2410/jet.h
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

#define GPIO_AP_SCL		    		S3C64XX_GPA(2)
#define GPIO_AP_SCL_AF	    			1

#define GPIO_AP_SDA		    		S3C64XX_GPA(3)
#define GPIO_AP_SDA_AF	    			1

#define GPIO_BT_RXD				S3C64XX_GPA(4)
#define GPIO_BT_RXD_AF				2

#define GPIO_BT_TXD				S3C64XX_GPA(5)
#define GPIO_BT_TXD_AF				2

#define GPIO_BT_CTS				S3C64XX_GPA(6)
#define GPIO_BT_CTS_AF				2

#define GPIO_BT_RTS				S3C64XX_GPA(7)
#define GPIO_BT_RTS_AF				2

/* S3C64XX_GPB(0) ~ S3C64XX_GPB(6) */

#define GPIO_PDA_RXD				S3C64XX_GPB(0)  // AP_RXD
#define GPIO_PDA_RXD_AF				2

#define GPIO_PDA_TXD				S3C64XX_GPB(1)  // AP_TXD
#define GPIO_PDA_TXD_AF				2

#define GPIO_I2C1_SCL				S3C64XX_GPB(2)  // CAM_SCL
#define GPIO_I2C1_SCL_AF			6

#define GPIO_I2C1_SDA				S3C64XX_GPB(3)  // CAM_SDA
#define GPIO_I2C1_SDA_AF			6

// S3C64XX_GPB(4)

#define GPIO_I2C0_SCL				S3C64XX_GPB(5)  // TOUCH_I2C_SCL_F
#define GPIO_I2C0_SCL_AF			2

#define GPIO_I2C0_SDA				S3C64XX_GPB(6)  // TOUCH_I2C_SDA_F
#define GPIO_I2C0_SDA_AF			2

/* S3C64XX_GPC(0) ~ S3C64XX_GPC(7) */

#define GPIO_OJ_SPI_MISO			S3C64XX_GPC(0)
#define GPIO_OJ_SPI_MISO_AF			0

#define GPIO_OJ_SPI_CLK				S3C64XX_GPC(1)
#define GPIO_OJ_SPI_CLK_AF			1

#define GPIO_OJ_SPI_MOSI			S3C64XX_GPC(2)
#define GPIO_OJ_SPI_MOSI_AF			1

#define GPIO_OJ_CS				S3C64XX_GPC(3)
#define GPIO_OJ_CS_AF				1

#define GPIO_WLAN_CMD				S3C64XX_GPC(4)
#define GPIO_WLAN_CMD_AF			3

#define GPIO_WLAN_CLK				S3C64XX_GPC(5)
#define GPIO_WLAN_CLK_AF			3

#define GPIO_WLAN_WAKE				S3C64XX_GPC(6)
#define GPIO_WLAN_WAKE_AF			1  // output

#define GPIO_BT_WAKE				S3C64XX_GPC(7)
#define GPIO_BT_WAKE_AF				1  // output

/* S3C64XX_GPD(0) ~ S3C64XX_GPD(4) */

#define GPIO_I2S_CLK				S3C64XX_GPD(0)
#define GPIO_I2S_CLK_AF				3

#define GPIO_WLAN_BT_SHUTDOWN			S3C64XX_GPD(1)	// GPIO_BT_WLAN_REG_ON
#define GPIO_WLAN_BT_SHUTDOWN_AF		1

#define GPIO_I2S_LRCLK				S3C64XX_GPD(2)  // AP_I2S_SYNC
#define GPIO_I2S_LRCLK_AF			3

#define GPIO_I2S_DI				S3C64XX_GPD(3)
#define GPIO_I2S_DI_AF				3

#define GPIO_I2S_DO				S3C64XX_GPD(4)
#define GPIO_I2S_DO_AF				3

/* S3C64XX_GPE(0) ~ S3C64XX_GPE(4) */

#define GPIO_BT_RST_N				S3C64XX_GPE(0)
#define GPIO_BT_RST_N_AF			1

#define GPIO_BOOT				S3C64XX_GPE(1)
#define GPIO_BOOT_AF				1

#define GPIO_WLAN_RST_N				S3C64XX_GPE(2)
#define GPIO_WLAN_RST_N_AF			1

#define GPIO_PWR_I2C_SCL			S3C64XX_GPE(3)
#define GPIO_PWR_I2C_SCL_AF			1

#define GPIO_PWR_I2C_SDA			S3C64XX_GPE(4)
#define GPIO_PWR_I2C_SDA_AF			1

/* S3C64XX_GPF(0) ~ S3C64XX_GPF(15) */

#define GPIO_CAM_MCLK				S3C64XX_GPF(0)
#define GPIO_CAM_MCLK_AF			2

#define GPIO_CAM_HSYNC				S3C64XX_GPF(1)
#define GPIO_CAM_HSYNC_AF			2

#define GPIO_CAM_PCLK				S3C64XX_GPF(2)
#define GPIO_CAM_PCLK_AF			2

#define GPIO_MCAM_RST_N				S3C64XX_GPF(3)
#define GPIO_MCAM_RST_N_AF			1

#define GPIO_CAM_VSYNC				S3C64XX_GPF(4)
#define GPIO_CAM_VSYNC_AF			2

#define GPIO_CAM_D_0				S3C64XX_GPF(5)
#define GPIO_CAM_D_0_AF				2

#define GPIO_CAM_D_1				S3C64XX_GPF(6)
#define GPIO_CAM_D_1_AF				2

#define GPIO_CAM_D_2				S3C64XX_GPF(7)
#define GPIO_CAM_D_2_AF				2

#define GPIO_CAM_D_3				S3C64XX_GPF(8)
#define GPIO_CAM_D_3_AF				2

#define GPIO_CAM_D_4				S3C64XX_GPF(9)
#define GPIO_CAM_D_4_AF				2

#define GPIO_CAM_D_5				S3C64XX_GPF(10)
#define GPIO_CAM_D_5_AF				2

#define GPIO_CAM_D_6				S3C64XX_GPF(11)
#define GPIO_CAM_D_6_AF				2

#define GPIO_CAM_D_7				S3C64XX_GPF(12)
#define GPIO_CAM_D_7_AF				2

/* this macro is for rev04 */
#define GPIO_MIC_SEL_EN_REV04			S3C64XX_GPF(13)
#define GPIO_MIC_SEL_EN_REV04_AF		1

#define GPIO_LUM_PWM				S3C64XX_GPF(14)
#define GPIO_LUM_PWM_AF				1

#define GPIO_PWM1_TOUT				S3C64XX_GPF(15)  // VIBTONE_PWM
#define GPIO_PWM1_TOUT_AF			2

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
#define GPIO_TFLASH_EN_AF			1

/* S3C64XX_GPH(1) ~ S3C64XX_GPH(9) */

#define GPIO_NAND_CLK				S3C64XX_GPH(0)
#define GPIO_NAND_CLK_AF			2

#define GPIO_NAND_CMD				S3C64XX_GPH(1)
#define GPIO_NAND_CMD_AF			2

#define GPIO_NAND_D_0				S3C64XX_GPH(2)
#define GPIO_NAND_D_0_AF			2

#define GPIO_NAND_D_1				S3C64XX_GPH(3)
#define GPIO_NAND_D_1_AF			2

#define GPIO_NAND_D_2				S3C64XX_GPH(4)
#define GPIO_NAND_D_2_AF			2

#define GPIO_NAND_D_3				S3C64XX_GPH(5)
#define GPIO_NAND_D_3_AF			2

#define GPIO_WLAN_D_0				S3C64XX_GPH(6)  // SDIO_DATA(0)
#define GPIO_WLAN_D_0_AF			3

#define GPIO_WLAN_D_1				S3C64XX_GPH(7)  // SDIO_DATA(1)
#define GPIO_WLAN_D_1_AF			3

#define GPIO_WLAN_D_2				S3C64XX_GPH(8)  // SDIO_DATA(2)
#define GPIO_WLAN_D_2_AF			3

#define GPIO_WLAN_D_3				S3C64XX_GPH(9)  // SDIO_DATA(3)
#define GPIO_WLAN_D_3_AF			3

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

#define GPIO_CHG_EN				S3C64XX_GPK(0)
#define GPIO_CHG_EN_AF				1

#define GPIO_AUDIO_EN				S3C64XX_GPK(1)
#define GPIO_AUDIO_EN_AF			1

#define GPIO_EAR_MIC_BIAS			S3C64XX_GPK(2)
#define GPIO_EAR_MIC_BIAS_AF			1

#define GPIO_MICBIAS_EN				S3C64XX_GPK(3)
#define GPIO_MICBIAS_EN_AF			1

#define GPIO_UART_SEL				S3C64XX_GPK(4)
#define GPIO_UART_SEL_AF			1

#define GPIO_MONOHEAD_DET			S3C64XX_GPK(5)
#define GPIO_MONOHEAD_DET_AF			0

#define GPIO_CAM_EN				S3C64XX_GPK(6)
#define GPIO_CAM_EN_AF				1

#define GPIO_PHONE_RST_N			S3C64XX_GPK(7)  // CP_RST
#define GPIO_PHONE_RST_N_AF			1

#define GPIO_KEYSENSE_0				S3C64XX_GPK(8)
#define GPIO_KEYSENSE_0_AF			3

#define GPIO_KEYSENSE_1				S3C64XX_GPK(9)
#define GPIO_KEYSENSE_1_AF			3

#define GPIO_KEYSENSE_2				S3C64XX_GPK(10)
#define GPIO_KEYSENSE_2_AF			3

#define GPIO_KEYSENSE_3				S3C64XX_GPK(11)
#define GPIO_KEYSENSE_3_AF			3

//#define GPIO_KEYSENSE_4				S3C64XX_GPK(12)
//#define GPIO_KEYSENSE_4_AF			3

//#define GPIO_KEYSENSE_5				S3C64XX_GPK(13)
//#define GPIO_KEYSENSE_5_AF			3

//#define GPIO_KEYSENSE_6				S3C64XX_GPK(14)
//#define GPIO_KEYSENSE_6_AF			3

#define GPIO_VMSMP_26V				S3C64XX_GPK(15)
#define GPIO_VMSMP_26V_AF			0

/* S3C64XX_GPL(0) ~ S3C64XX_GPL(14) */

#define GPIO_KEYSCAN_0				S3C64XX_GPL(0)
#define GPIO_KEYSCAN_0_AF			3

#define GPIO_KEYSCAN_1				S3C64XX_GPL(1)
#define GPIO_KEYSCAN_1_AF			3

#define GPIO_KEYSCAN_2				S3C64XX_GPL(2)
#define GPIO_KEYSCAN_2_AF			3

#define GPIO_KEYSCAN_3				S3C64XX_GPL(3)
#define GPIO_KEYSCAN_3_AF			3

//#define GPIO_KEYSCAN_4				S3C64XX_GPL(4)
//#define GPIO_KEYSCAN_4_AF			3

//#define GPIO_KEYSCAN_5				S3C64XX_GPL(5)
//#define GPIO_KEYSCAN_5_AF			3

//#define GPIO_KEYSCAN_6				S3C64XX_GPL(6)
//#define GPIO_KEYSCAN_6_AF			3

#define GPIO_TOUCH_EN				S3C64XX_GPL(7)
#define GPIO_TOUCH_EN_AF			0

#define GPIO_T_FLASH_DETECT			S3C64XX_GPL(8)
#define GPIO_T_FLASH_DETECT_AF			1

#define GPIO_PHONE_ON				S3C64XX_GPL(9)
#define GPIO_PHONE_ON_AF			1

#define GPIO_VIB_EN				S3C64XX_GPL(10)
#define GPIO_VIB_EN_AF				1

#define GPIO_TA_CONNECTED_N			S3C64XX_GPL(11)
#define GPIO_TA_CONNECTED_N_AF			3

#define GPIO_PS_VOUT				S3C64XX_GPL(12)
#define GPIO_PS_VOUT_AF				3

//#define GPIO_TA_USB_SEL				S3C64XX_GPL(13)
//#define GPIO_TA_USB_SEL_AF			1

//#define GPIO_BT_HOST_WAKE			S3C64XX_GPL(14)
//#define GPIO_BT_HOST_WAKE_AF			3

/* S3C64XX_GPM(0) ~ S3C64XX_GPM(5) */

#define GPIO_FM_I2C_SCL				S3C64XX_GPM(0)
#define GPIO_FM_I2C_SCL_AF			1

#define GPIO_FM_I2C_SDA				S3C64XX_GPM(1)
#define GPIO_FM_I2C_SDA_AF			1

#define GPIO_WLAN_HOST_WAKE			S3C64XX_GPM(2)
#define GPIO_WLAN_HOST_WAKE_AF			3

#define GPIO_BT_HOST_WAKE			S3C64XX_GPM(3)
#define GPIO_BT_HOST_WAKE_AF			3

//#define GPIO_OJ_MOTION				S3C64XX_GPM(4)
//#define GPIO_OJ_MOTION_AF			3

#define GPIO_MSENSE_RST_N			S3C64XX_GPM(5)
#define GPIO_MSENSE_RST_N_AF			1

/* S3C64XX_GPN(0) ~ S3C64XX_GPN(15) */

#define GPIO_ONEDRAM_INT_N			S3C64XX_GPN(0)
#define GPIO_ONEDRAM_INT_N_AF			2

//#define GPIO_WLAN_HOST_WAKE			S3C64XX_GPN(1)
//#define GPIO_WLAN_HOST_WAKE_AF			2

#define GPIO_MSENSE_INT				S3C64XX_GPN(2)
#define GPIO_MSENSE_INT_AF			2

#define GPIO_ACC_INT				S3C64XX_GPN(3)
#define GPIO_ACC_INT_AF				2

#define GPIO_HALL_SW				S3C64XX_GPN(4)  // dgahn.smd: TOCHK
#define GPIO_HALL_SW_AF				2

#define GPIO_POWER_N				S3C64XX_GPN(5)
#define GPIO_POWER_N_AF				2

//#define GPIO_TF_DETECT				S3C64XX_GPN(6)
//#define GPIO_TF_DETECT_AF			2

#define GPIO_PHONE_ACTIVE			S3C64XX_GPN(7)
#define GPIO_PHONE_ACTIVE_AF			2

#define GPIO_TOUCH_INT				S3C64XX_GPN(8)
#define GPIO_TOUCH_INT_AF			2

#define GPIO_JACK_INT_N				S3C64XX_GPN(9)
#define GPIO_JACK_INT_N_AF			2

#define GPIO_DET_35				S3C64XX_GPN(10)
#define GPIO_DET_35_AF				2

#define GPIO_EAR_SEND_END			S3C64XX_GPN(11)
#define GPIO_EAR_SEND_END_AF			2

#define GPIO_RESOUT_N				S3C64XX_GPN(12)
#define GPIO_RESOUT_N_AF			2

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

#define GPIO_LCD_RST_N				S3C64XX_GPO(2)
#define GPIO_LCD_RST_N_AF			1

// S3C64XX_GPO(3) ~ S3C64XX_GPO(5)

#define GPIO_LCD_CS_N				S3C64XX_GPO(6)
#define GPIO_LCD_CS_N_AF			1

#define GPIO_LCD_SDI			    	S3C64XX_GPO(7)
#define GPIO_LCD_SDI_AF				1

// S3C64XX_GPO(8) ~ S3C64XX_GPO(11)

#define GPIO_LCD_SDO			    	S3C64XX_GPO(12)
#define GPIO_LCD_SDO_AF				1

#define GPIO_LCD_SCLK				S3C64XX_GPO(13)
#define GPIO_LCD_SCLK_AF			1

#define GPIO_LCD_ID				S3C64XX_GPO(14)
#define GPIO_LCD_ID_AF				0

// S3C64XX_GPO(15)

/* S3C64XX_GPP(0) ~ S3C64XX_GPP(14) */

#define GPIO_PDA_PS_HOLD			S3C64XX_GPP(13)
#define GPIO_PDA_PS_HOLD_AF			1

/* S3C64XX_GPQ(0) ~ S3C64XX_GPQ(8) */


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

