/*
 *  arch/arm/mach-s3c6400/include/mach/volans_gpio_table.h
 *
 *  Author:		Samsung Electronics
 *  Created:	19, Jun, 2009
 *  Copyright:	Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __VOLANS_GPIO_TABLE_H__
#define __VOLANS_GPIO_TABLE_H__

#include <mach/hardware.h>

static struct __gpio_config volans_gpio_table[] = {
	/* GPA Group */
	{
		.gpio = GPIO_AP_FLM_RXD,	.af = GPIO_AP_FLM_RXD_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_AP_FLM_TXD,	.af = GPIO_AP_FLM_TXD_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_AP_BT_RXD,		.af = GPIO_AP_BT_RXD_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_AP_BT_TXD,		.af = GPIO_AP_BT_TXD_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_AP_BT_CTS,		.af = GPIO_AP_BT_CTS_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_AP_BT_RTS,		.af = GPIO_AP_BT_RTS_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPB Group */
	{
		.gpio = GPIO_AP_RXD,		.af = GPIO_AP_RXD_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_AP_TXD,		.af = GPIO_AP_TXD_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_AP_SCL,		.af = GPIO_AP_SCL_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_AP_SDA,		.af = GPIO_AP_SDA_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FM_SCL,		.af = GPIO_FM_SCL_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FM_SDA,		.af = GPIO_FM_SDA_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPC Group */
	{
		.gpio = GPIO_LCD_READ,		.af = GPIO_LCD_READ_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_CLK,		.af = GPIO_LCD_CLK_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LCD_SI,		.af = GPIO_LCD_SI_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LCD_nCS,		.af = GPIO_LCD_nCS_AF,			.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_TOUCH_SCL,		.af = GPIO_TOUCH_SCL_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_TOUCH_SDA,		.af = GPIO_TOUCH_SDA_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_TOUCH_RST,		.af = GPIO_TOUCH_RST_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_TOUCH_IRQ,		.af = GPIO_TOUCH_IRQ_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPD Group */
	{
		.gpio = GPIO_I2S_SCLK,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_I2S_SYNC,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_I2S_SDI,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_I2S_SDO,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPE Group */
	{
		.gpio = GPIO_BT_nRST,		.af = GPIO_BT_nRST_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_BOOT_MODE,		.af = GPIO_BOOT_MODE_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_PWR_I2C_SCL,	.af = GPIO_PWR_I2C_SCL_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PWR_I2C_SDA,	.af = GPIO_PWR_I2C_SDA_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPF Group */
	{
		.gpio = GPIO_CAM_MCLK,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_HSYNC,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{ 
		.gpio = GPIO_CAM_PCLK,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_3M_nRST,	.af = GPIO_CAM_3M_nRST_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_VSYNC,		.af = S3C_GPIO_INPUT,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_0,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_1,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_2,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_3,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_4,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_5,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_6,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_CAM_D_7,		.af = S3C_GPIO_INPUT,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_VIB_PWM,		.af = GPIO_VIB_PWM_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPG Group */
	{
		.gpio = GPIO_T_FLASH_CLK,	.af = GPIO_T_FLASH_CLK_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{ 
		.gpio = GPIO_T_FLASH_CMD,	.af = GPIO_T_FLASH_CMD_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_T_FLASH_D0,	.af = GPIO_T_FLASH_D0_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_T_FLASH_D1,	.af = GPIO_T_FLASH_D1_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_T_FLASH_D2,	.af = GPIO_T_FLASH_D2_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_T_FLASH_D3,	.af = GPIO_T_FLASH_D3_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPH Group */
	{
		.gpio = GPIO_NAND_CLK,		.af = GPIO_NAND_CLK_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_NAND_CMD,		.af = GPIO_NAND_CMD_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_NAND_D0,		.af = GPIO_NAND_D0_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_NAND_D1,		.af = GPIO_NAND_D1_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_NAND_D2,		.af = GPIO_NAND_D2_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_NAND_D3,		.af = GPIO_NAND_D3_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_nRST,		.af = GPIO_LCD_nRST_AF,			.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LCD_ID,		.af = GPIO_LCD_ID_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPI Group */
	{
		.gpio = GPIO_LCD_D0,		.af = GPIO_LCD_D0_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D1,		.af = GPIO_LCD_D1_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D2,		.af = GPIO_LCD_D2_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D3,		.af = GPIO_LCD_D3_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D4,		.af = GPIO_LCD_D4_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D5,		.af = GPIO_LCD_D5_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D6,		.af = GPIO_LCD_D6_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D7,		.af = GPIO_LCD_D7_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D8,		.af = GPIO_LCD_D8_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D9,		.af = GPIO_LCD_D9_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D10,		.af = GPIO_LCD_D10_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D11,		.af = GPIO_LCD_D11_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D12,		.af = GPIO_LCD_D12_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D13,		.af = GPIO_LCD_D13_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D14,		.af = GPIO_LCD_D14_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D15,		.af = GPIO_LCD_D15_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPJ Group */
	{
		.gpio = GPIO_LCD_D16,		.af = GPIO_LCD_D16_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D17,		.af = GPIO_LCD_D17_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D18,		.af = GPIO_LCD_D18_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D19,		.af = GPIO_LCD_D19_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D20,		.af = GPIO_LCD_D20_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D21,		.af = GPIO_LCD_D21_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D22,		.af = GPIO_LCD_D22_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_D23,		.af = GPIO_LCD_D23_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_HSYNC,		.af = GPIO_LCD_HSYNC_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_VSYNC,		.af = GPIO_LCD_VSYNC_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_DE,		.af = GPIO_LCD_DE_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_LCD_MCLK,		.af = GPIO_LCD_MCLK_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	/* GPK Group - Alive */
	{
		.gpio = GPIO_TA_EN,			.af = GPIO_TA_EN_AF,			.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE 
	},
	{
		.gpio = GPIO_AUDIO_EN,		.af = GPIO_AUDIO_EN_AF,			.level = GPIO_LEVEL_LOW, 
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FM_LDO_ON,		.af = GPIO_FM_LDO_ON_AF,		.level = GPIO_LEVEL_LOW, 
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_MICBIAS_EN,	.af = GPIO_MICBIAS_EN_AF,		.level = GPIO_LEVEL_LOW, 
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_UART_SEL,		.af = GPIO_UART_SEL_AF,			.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FM_nRST,		.af = GPIO_FM_nRST_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_EN,		.af = GPIO_CAM_EN_AF,			.level = GPIO_LEVEL_LOW, 
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PHONE_RST_N,		.af = GPIO_PHONE_RST_N_AF,			.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_KBR0,			.af = GPIO_KBR0_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_KBR1,			.af = GPIO_KBR1_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_KBR2,			.af = GPIO_KBR2_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FLM_SEL,		.af = GPIO_FLM_SEL_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_CIF_nRST,	.af = GPIO_CAM_CIF_nRST_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_CIF_nSTBY,	.af = GPIO_CAM_CIF_nSTBY_AF,	.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LED_DRV_EN,	.af = GPIO_LED_DRV_EN_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_VREG_MSMP_26V,	.af = GPIO_VREG_MSMP_26V_AF,	.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPL Group - Alive */
	{
		.gpio = GPIO_KBC0,			.af = GPIO_KBC0_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_KBC1,			.af = GPIO_KBC1_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_KBC2,			.af = GPIO_KBC2_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_3M_nSTBY,	.af = GPIO_CAM_3M_nSTBY_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_VIB_EN,		.af = GPIO_VIB_EN_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CP_USB_ON,		.af = GPIO_CP_USB_ON_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PHONE_ON,		.af = GPIO_PHONE_ON_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_USIM_BOOT,		.af = GPIO_USIM_BOOT_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LCD_EN,		.af = GPIO_LCD_EN_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_MIC_SEL,		.af = GPIO_MIC_SEL_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_FM_INT,		.af = GPIO_FM_INT_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_TA_nCONNECTED,	.af = GPIO_TA_nCONNECTED_AF,	.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LED_DRV_SEL,	.af = GPIO_LED_DRV_SEL_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_TF_EN,			.af = GPIO_TF_EN_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPM Group - Alive */
	{
		.gpio = GPIO_CAM_SCL,		.af = GPIO_CAM_SCL_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_CAM_SDA,		.af = GPIO_CAM_SDA_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_TA_nCHG,		.af = GPIO_TA_nCHG_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PDA_ACTIVE,	.af = GPIO_PDA_ACTIVE_AF,		.level = GPIO_LEVEL_HIGH,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PS_VOUT,		.af = GPIO_PS_VOUT_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPN Group - Alive */
	{
		.gpio = GPIO_nONED_INT_AP,	.af = GPIO_nONED_INT_AP_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_HOST_WAKE,		.af = GPIO_HOST_WAKE_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_ACC_INT,		.af = GPIO_ACC_INT_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_BT_EN,			.af = GPIO_BT_EN_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_nPOWER,		.af = GPIO_nPOWER_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_T_FLASH_DETECT,.af = GPIO_T_FLASH_DETECT_AF,	.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PHONE_ACTIVE_AP,.af = GPIO_PHONE_ACTIVE_AP_AF,	.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_PM_INT_N,		.af = GPIO_PM_INT_N_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_INTB,			.af = GPIO_INTB_AF,				.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_DET_35,		.af = GPIO_DET_35_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_EAR_SEND_END,	.af = GPIO_EAR_SEND_END_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_RESOUT_N_AP,	.af = GPIO_RESOUT_N_AP_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_DOWN,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_DOWN
	},
	{
		.gpio = GPIO_BOOT_EINT13,	.af = GPIO_BOOT_EINT13_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_BOOT_EINT14,	.af = GPIO_BOOT_EINT14_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_BOOT_EINT15,	.af = GPIO_BOOT_EINT15_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPO Group */
	{
		.gpio = GPIO_REV_2,		.af = GPIO_REV_2_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_LUM_PWM,		.af = GPIO_LUM_PWM_AF,			.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_INPUT,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_TOUCH_LDO_EN,	.af = GPIO_TOUCH_LDO_EN_AF,		.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPP Group */
	{
		.gpio = GPIO_PS_HOLD_PDA,	.af = GPIO_PS_HOLD_PDA_AF,		.level = GPIO_LEVEL_NONE,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT1,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	/* GPQ Group */
	{
		.gpio = GPIO_SET_DVS3,		.af = GPIO_SET_DVS3_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_SET_DVS2,		.af = GPIO_SET_DVS2_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
	{
		.gpio = GPIO_SET_DVS1,		.af = GPIO_SET_DVS1_AF,			.level = GPIO_LEVEL_LOW,
		.pull = S3C_GPIO_PULL_NONE,	.slp_con = S3C_GPIO_SLP_OUT0,	.slp_pull = S3C_GPIO_PULL_NONE
	},
};

#endif	/* __VOLANS_GPIO_TABLE_H__ */

