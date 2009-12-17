/*
 * drivers/video/samsung/s3cfb_mdj2024wv.c
 *
 * Copyright (C) 2008 Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	S3C Frame Buffer Driver
 *	based on skeletonfb.c, sa1100fb.h, s3c2410fb.c
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-lcd.h>

#include <mach/hardware.h>

#include "s3cfb.h"

#define BACKLIGHT_STATUS_ALC	0x100
#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		1
#define BACKLIGHT_LEVEL_DEFAULT	(BACKLIGHT_STATUS_ALC | 0xFF)	/* Default Setting */
#define BACKLIGHT_LEVEL_MAX		(BACKLIGHT_STATUS_ALC | BACKLIGHT_LEVEL_VALUE)

int lcd_power = OFF;
EXPORT_SYMBOL(lcd_power);

void lcd_power_ctrl(s32 value);
EXPORT_SYMBOL(lcd_power_ctrl);

int backlight_power = OFF;
EXPORT_SYMBOL(backlight_power);

void backlight_power_ctrl(s32 value);
EXPORT_SYMBOL(backlight_power_ctrl);

int backlight_level = BACKLIGHT_LEVEL_DEFAULT;
EXPORT_SYMBOL(backlight_level);

void backlight_level_ctrl(s32 value);
EXPORT_SYMBOL(backlight_level_ctrl);


#define S3C_FB_HFP				10 		/* Front Porch */
#define S3C_FB_HSW				10 		/* Hsync Width */
#define S3C_FB_HBP				10 		/* Back Porch */

#define S3C_FB_VFP				3 		/* Front Porch */
#define S3C_FB_VSW				2 		/* Vsync Width */
#define S3C_FB_VBP				8 		/* Back Porch */

#define S3C_FB_HRES				320 	/* Horizon pixel Resolition */
#define S3C_FB_VRES				480 	/* Vertical pixel Resolution */

#define S3C_FB_HRES_VIRTUAL		640 	/* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL		480 	/* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD			320		/* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD			480 	/* Vertial pixel Resolution */

#define S3C_FB_VFRAME_FREQ  	60		/* Frame Rate Frequency */

#define S3C_FB_PIXEL_CLOCK		(S3C_FB_VFRAME_FREQ * \
								(S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * \
								(S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void s3cfb_set_fimd_info(void)
{
	s3c_fimd.vidcon1	= S3C_VIDCON1_IHSYNC_INVERT |
							S3C_VIDCON1_IVSYNC_INVERT |
							S3C_VIDCON1_IVDEN_NORMAL;

	s3c_fimd.vidtcon0 	= S3C_VIDTCON0_VBPD(S3C_FB_VBP - 1) |
							S3C_VIDTCON0_VFPD(S3C_FB_VFP - 1) |
							S3C_VIDTCON0_VSPW(S3C_FB_VSW - 1);
	s3c_fimd.vidtcon1	= S3C_VIDTCON1_HBPD(S3C_FB_HBP - 1) |
							S3C_VIDTCON1_HFPD(S3C_FB_HFP - 1) |
							S3C_VIDTCON1_HSPW(S3C_FB_HSW - 1);
	s3c_fimd.vidtcon2	= S3C_VIDTCON2_LINEVAL(S3C_FB_VRES - 1) |
							S3C_VIDTCON2_HOZVAL(S3C_FB_HRES - 1);

	s3c_fimd.vidosd0a 	= S3C_VIDOSDxA_OSD_LTX_F(0) |
							S3C_VIDOSDxA_OSD_LTY_F(0);
	s3c_fimd.vidosd0b 	= S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES - 1) |
							S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES - 1);

	s3c_fimd.vidosd1a 	= S3C_VIDOSDxA_OSD_LTX_F(0) |
							S3C_VIDOSDxA_OSD_LTY_F(0);
	s3c_fimd.vidosd1b 	= S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES_OSD - 1) |
							S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES_OSD - 1);

	s3c_fimd.width		= S3C_FB_HRES;
	s3c_fimd.height 	= S3C_FB_VRES;
	s3c_fimd.xres 		= S3C_FB_HRES;
	s3c_fimd.yres 		= S3C_FB_VRES;

#if defined(CONFIG_FB_S3C_VIRTUAL_SCREEN)
	s3c_fimd.xres_virtual = S3C_FB_HRES_VIRTUAL;
	s3c_fimd.yres_virtual = S3C_FB_VRES_VIRTUAL;
#else
	s3c_fimd.xres_virtual = S3C_FB_HRES;
	s3c_fimd.yres_virtual = S3C_FB_VRES;
#endif

	s3c_fimd.osd_width 	= S3C_FB_HRES_OSD;
	s3c_fimd.osd_height = S3C_FB_VRES_OSD;
	s3c_fimd.osd_xres 	= S3C_FB_HRES_OSD;
	s3c_fimd.osd_yres 	= S3C_FB_VRES_OSD;

	s3c_fimd.osd_xres_virtual = S3C_FB_HRES_OSD;
	s3c_fimd.osd_yres_virtual = S3C_FB_VRES_OSD;

    s3c_fimd.pixclock		= S3C_FB_PIXEL_CLOCK;

	s3c_fimd.hsync_len 		= S3C_FB_HSW;
	s3c_fimd.vsync_len 		= S3C_FB_VSW;
	s3c_fimd.left_margin 	= S3C_FB_HFP;
	s3c_fimd.upper_margin 	= S3C_FB_VFP;
	s3c_fimd.right_margin 	= S3C_FB_HBP;
	s3c_fimd.lower_margin 	= S3C_FB_VBP;

	s3c_fimd.set_lcd_power		 = lcd_power_ctrl;
	s3c_fimd.set_backlight_power = backlight_power_ctrl;
	s3c_fimd.set_brightness 	 = backlight_level_ctrl;

	s3c_fimd.backlight_min = BACKLIGHT_LEVEL_MIN;
	s3c_fimd.backlight_max = BACKLIGHT_LEVEL_MAX;
}

static void lcd_gpio_init(void)
{
	/* B(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_B_0, S3C_GPIO_SFN(GPIO_LCD_B_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_1, S3C_GPIO_SFN(GPIO_LCD_B_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_2, S3C_GPIO_SFN(GPIO_LCD_B_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_3, S3C_GPIO_SFN(GPIO_LCD_B_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_4, S3C_GPIO_SFN(GPIO_LCD_B_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_5, S3C_GPIO_SFN(GPIO_LCD_B_5_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_6, S3C_GPIO_SFN(GPIO_LCD_B_6_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_7, S3C_GPIO_SFN(GPIO_LCD_B_7_AF));
	/* G(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_G_0, S3C_GPIO_SFN(GPIO_LCD_G_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_1, S3C_GPIO_SFN(GPIO_LCD_G_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_2, S3C_GPIO_SFN(GPIO_LCD_G_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_3, S3C_GPIO_SFN(GPIO_LCD_G_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_4, S3C_GPIO_SFN(GPIO_LCD_G_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_5, S3C_GPIO_SFN(GPIO_LCD_G_5_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_6, S3C_GPIO_SFN(GPIO_LCD_G_6_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_7, S3C_GPIO_SFN(GPIO_LCD_G_7_AF));
	/* R(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_R_0, S3C_GPIO_SFN(GPIO_LCD_R_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_1, S3C_GPIO_SFN(GPIO_LCD_R_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_2, S3C_GPIO_SFN(GPIO_LCD_R_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_3, S3C_GPIO_SFN(GPIO_LCD_R_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_4, S3C_GPIO_SFN(GPIO_LCD_R_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_5, S3C_GPIO_SFN(GPIO_LCD_R_5_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_6, S3C_GPIO_SFN(GPIO_LCD_R_6_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_7, S3C_GPIO_SFN(GPIO_LCD_R_7_AF));
	
	/* HSYNC */
	s3c_gpio_cfgpin(GPIO_LCD_HSYNC, S3C_GPIO_SFN(GPIO_LCD_HSYNC_AF));
	/* VSYNC */
	s3c_gpio_cfgpin(GPIO_LCD_VSYNC, S3C_GPIO_SFN(GPIO_LCD_VSYNC_AF));
	/* DE */
	s3c_gpio_cfgpin(GPIO_LCD_DE, S3C_GPIO_SFN(GPIO_LCD_DE_AF));
	/* CLK */
	s3c_gpio_cfgpin(GPIO_LCD_CLK, S3C_GPIO_SFN(GPIO_LCD_CLK_AF));
	
#if 0
	/* LCD_ON */
	if (gpio_is_valid(GPIO_LCD_ON)) {
		if (gpio_request(GPIO_LCD_ON, S3C_GPIO_LAVEL(GPIO_LCD_ON))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_ON!\n");
		gpio_direction_output(GPIO_LCD_ON, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_ON, S3C_GPIO_PULL_NONE);
#endif
	/* LCD_RST_N */
	if (gpio_is_valid(GPIO_LCD_RST_N)) {
		if (gpio_request(GPIO_LCD_RST_N, S3C_GPIO_LAVEL(GPIO_LCD_RST_N))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_RST_N!\n");
		gpio_direction_output(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_RST_N, S3C_GPIO_PULL_NONE);
	/* LCD_ID */
	if (gpio_is_valid(GPIO_LCD_ID)) {
		if (gpio_request(GPIO_LCD_ID, S3C_GPIO_LAVEL(GPIO_LCD_ID))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_ID!\n");
		gpio_direction_input(GPIO_LCD_ID);
	}
	s3c_gpio_setpull(GPIO_LCD_ID, S3C_GPIO_PULL_NONE);

	/* LCD_SCLK */
	if (gpio_is_valid(GPIO_LCD_SCLK)) {
		if (gpio_request(GPIO_LCD_SCLK, S3C_GPIO_LAVEL(GPIO_LCD_SCLK))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_SCLK!\n");
		gpio_direction_output(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_SCLK, S3C_GPIO_PULL_NONE);
	/* LCD_CS_N */
	if (gpio_is_valid(GPIO_LCD_CS_N)) {
		if (gpio_request(GPIO_LCD_CS_N, S3C_GPIO_LAVEL(GPIO_LCD_CS_N))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_CS_N!\n");
		gpio_direction_output(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_CS_N, S3C_GPIO_PULL_NONE);
	/* LCD_SDO */
	if (gpio_is_valid(GPIO_LCD_SDO)) {
		if (gpio_request(GPIO_LCD_SDO, S3C_GPIO_LAVEL(GPIO_LCD_SDO))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_SDO!\n");
		gpio_direction_output(GPIO_LCD_SDO, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_SDO, S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	if (gpio_is_valid(GPIO_LCD_SDI)) {
		if (gpio_request(GPIO_LCD_SDI, S3C_GPIO_LAVEL(GPIO_LCD_SDI))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_SDI!\n");
		gpio_direction_output(GPIO_LCD_SDI, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_SDI, S3C_GPIO_PULL_NONE);
}

static void backlight_gpio_init(void)
{
#if 0
	/* ALC_RST_N */
	if (gpio_is_valid(GPIO_ALC_RST_N)) {
		if (gpio_request(GPIO_ALC_RST_N, S3C_GPIO_LAVEL(GPIO_ALC_RST_N))) 
			printk(KERN_ERR "Failed to request GPIO_ALC_RST_N!\n");
		gpio_direction_output(GPIO_ALC_RST_N, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_ALC_RST_N, S3C_GPIO_PULL_NONE);
#endif	
	/* LUM PWM */
	if (gpio_is_valid(GPIO_LUM_PWM)) {
		if (gpio_request(GPIO_LUM_PWM, S3C_GPIO_LAVEL(GPIO_LUM_PWM))) 
			printk(KERN_ERR "Failed to request GPIO_LUM_PWM!\n");
		gpio_direction_output(GPIO_LUM_PWM, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LUM_PWM, S3C_GPIO_PULL_NONE);
}

/*
 * Serial Interface
 */

#define LCD_CSX_HIGH	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW		gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_LOW);

#define LCD_DCX_HIGH	gpio_set_value(GPIO_LCD_SDO, GPIO_LEVEL_HIGH);
#define LCD_DCX_LOW		gpio_set_value(GPIO_LCD_SDO, GPIO_LEVEL_LOW);

#define LCD_WRX_HIGH	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH);
#define LCD_WRX_LOW		gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_LOW);

#define LCD_SDA_HIGH	gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_HIGH);
#define LCD_SDA_LOW		gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_LOW);

#define DEFAULT_UDELAY	50	

#define PWRCTL			0xF3
#define SLPIN			0x10
#define SLPOUT			0x11
#define DISCTL			0xF2
#define VCMCTL			0xF4
#define SRCCTL			0xF5
#define GATECTL			0xFD
#define MADCTL			0x36
#define COLMOD			0x3A
#define GAMCTL1			0xF7
#define GAMCTL2			0xF8
#define BCMODE			0xCB
#define WRCABC			0xCB
#define DCON			0xEF
#define WRCTRLD			0x53

struct setting_table {
	u8 command;	
	u8 parameters;
	u8 parameter[15];
	s32 wait;
};

static struct setting_table power_on_setting_table[] = {
	{  PWRCTL,  9, { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  SLPOUT,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  DISCTL, 11, { 0x16, 0x16, 0x0F, 0x0A, 0x05, 0x0A, 0x05, 0x10, 0x00, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  PWRCTL,  9, { 0x00, 0x01, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  VCMCTL,  5, { 0x1B, 0x1B, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  SRCCTL,  6, { 0x00, 0x00, 0x0A, 0x01, 0x01, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GATECTL,  3, { 0x44, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  PWRCTL,  9, { 0x00, 0x03, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  PWRCTL,  9, { 0x00, 0x07, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  PWRCTL,  9, { 0x00, 0x0F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  PWRCTL,  9, { 0x00, 0x1F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{  PWRCTL,  9, { 0x00, 0x3F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{  PWRCTL,  9, { 0x00, 0x7F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  35 },
	{  MADCTL,  1, { 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  COLMOD,  1, { 0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GAMCTL1, 15, { 0x00, 0x00, 0x00, 0x00, 0x1F, 0x2B, 0x2C, 0x2C, 0x10, 0x10, 0x0F, 0x16, 0x06, 0x22, 0x22 },   0 },
	{ GAMCTL2, 15, { 0x00, 0x00, 0x00, 0x00, 0x1F, 0x2B, 0x2C, 0x2C, 0x10, 0x10, 0x0F, 0x16, 0x06, 0x22, 0x22 },   0 },
	{  BCMODE,  1, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  WRCABC,  1, { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{    DCON,  1, { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	{    DCON,  1, { 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ WRCTRLD,  1, { 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
};

#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
	{    DCON,  1, { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	{    DCON,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{  PWRCTL,  9, { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPIN,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 200 },
};

#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))

static void setting_table_write(struct setting_table *table)
{
	s32 i, j;

	LCD_CSX_HIGH
	LCD_DCX_HIGH
	udelay(DEFAULT_UDELAY);

	/* Mddi Write Command */
	LCD_CSX_LOW
	LCD_DCX_LOW
	udelay(DEFAULT_UDELAY);

   	for (i = 7; i >= 0; i--) { 
		LCD_WRX_LOW
   		udelay(DEFAULT_UDELAY);

		if ((table->command >> i) & 0x1)
			LCD_SDA_HIGH
		else
			LCD_SDA_LOW
		udelay(DEFAULT_UDELAY);	

		LCD_WRX_HIGH
		udelay(DEFAULT_UDELAY);	
	}

	LCD_SDA_HIGH
	udelay(DEFAULT_UDELAY);	

	LCD_DCX_HIGH
	LCD_CSX_HIGH
	udelay(DEFAULT_UDELAY);

	/* Mddi Write Parameter */
	for (j = 0; j < table->parameters; j++) {
		LCD_CSX_LOW
		udelay(DEFAULT_UDELAY);

		for (i = 7; i >= 0; i--) { 
			LCD_WRX_LOW
			udelay(DEFAULT_UDELAY);

			if ((table->parameter[j] >> i) & 0x1)
				LCD_SDA_HIGH
			else
				LCD_SDA_LOW
				udelay(DEFAULT_UDELAY);	

			LCD_WRX_HIGH
			udelay(DEFAULT_UDELAY);	
		}

		LCD_SDA_HIGH
		udelay(DEFAULT_UDELAY);	

		LCD_CSX_HIGH
		udelay(DEFAULT_UDELAY);
	}

	msleep(table->wait);
}

/*
 *	LCD Power Handler
 */

void lcd_power_ctrl(s32 value)
{
	s32 i;	
	
	if (value) {
	
		/* Power On Sequence */
	
		/* Reset Deasseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
#if 0		
		/* Power Enable */
		gpio_set_value(GPIO_LCD_ON, GPIO_LEVEL_HIGH);
#endif		
		msleep(10);

		/* Reset Assert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
		
		msleep(10);

		/* Reset Deasseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
		
		msleep(10);

		for (i = 0; i < POWER_ON_SETTINGS; i++)
			setting_table_write(&power_on_setting_table[i]);	
	}
	else {

		/* Power Off Sequence */
	
		for (i = 0; i < POWER_OFF_SETTINGS; i++)
			setting_table_write(&power_off_setting_table[i]);	

		/* Reset Assert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
		
		msleep(10);
#if 0		
		/* Power Disable */
		gpio_set_value(GPIO_LCD_ON, GPIO_LEVEL_LOW);
#endif
	}

	lcd_power = value;
}

/*
 * BU6091GU Initialize
 */

#define BD6091GU_ID		0xEC

static struct i2c_driver backlight_i2c_driver;
static struct i2c_client *backlight_i2c_client = NULL;


//mk93.lee static unsigned short backlight_normal_i2c[] = { (BD6091GU_ID >> 1), I2C_CLIENT_END };
//mk93.lee static unsigned short backlight_ignore[] = { 1, (BD6091GU_ID >> 1), I2C_CLIENT_END };	/* To Avoid HW Problem */ 
//mk93.lee static unsigned short backlight_probe[] = { 2, (BD6091GU_ID >> 1),I2C_CLIENT_END };
//normal i2c bug?

static unsigned short backlight_normal_i2c[] = {  I2C_CLIENT_END };
static unsigned short backlight_ignore[] = { I2C_CLIENT_END };	/* To Avoid HW Problem */ 
static unsigned short backlight_probe[] = { 2, (BD6091GU_ID >> 1),I2C_CLIENT_END };

static struct i2c_client_address_data backlight_addr_data = {
	.normal_i2c = backlight_normal_i2c,
	.ignore		= backlight_ignore,
	.probe		= backlight_probe,	
};

static int backlight_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "bd6091gu");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &backlight_i2c_driver;

	if ((ret = i2c_attach_client(c)) < 0)
		goto error;

	backlight_i2c_client = c;

error:
	return ret;
}

static int backlight_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &backlight_addr_data, backlight_attach);
}

static struct i2c_driver backlight_i2c_driver = {
	.driver = {
		.name = "bd6091gu",
	},
	.id = BD6091GU_ID,
	.attach_adapter = backlight_attach_adapter,
};

static int backlight_i2c_init(void) 
{
	return i2c_add_driver(&backlight_i2c_driver);
}

/*
 * VOVP(Over Voltage Protextion detect voltage) -> 31V 
 * WPWMEN(External PWM Input "WPWMIN" terminal Enable Control) -> Invalid
 * ALCEN(ALC Function Control) -> On
 * LEDMD(LED Mode Select) -> ALC Mode
 * LEDEN(LED Control) -> On
 * ILED(LED Current Setting at Register mode) -> 25.6 mA
 * THL(LED current Down transition per 0.2 mA step) -> 4.096 ms
 * TLH(LED current Up transition per 0.2 mA step) -> 4.096 ms
 * ADCYC(ADC Measurement Cycle) -> 1.05 s
 * GAIN(Sensor Gain Switching Function Control) -> Auto Change
 * VSB(SBIAS Output Voltage Control) -> 3.0V
 * MDCIR(LED Current Reset Select by Mode Change) -> LED current non-reset at mode change
 * SBIASON(SBIAS Control) -> Measurement cycle synchronous
 */

#define ALC_SETTING_NUM		21

#define ALC_CONTROL_REG		17	
#define ALC_CURRENT_REG		18
#define ALC_TRANSITION_REG	19	
#define ALC_MODE_REG		20

#define ALC_CONTROL_LEDMD	(0x1 << 1)

static u8 alc_setting[ALC_SETTING_NUM][2] = {
	{0x0E, 0x48},	/* 14.6 mA */ 
	{0x0F, 0x49},	/* 14.8 mA */
	{0x10, 0x4B},	/* 15.2 mA */ 
	{0x11, 0x4E},	/* 15.8 mA */
	{0x12, 0x53},	/* 16.8 mA */
	{0x13, 0x59},	/* 18.0 mA */
	{0x14, 0x60},	/* 19.4 mA */
	{0x15, 0x66},	/* 20.6 mA */ 	
	{0x16, 0x6B},	/* 21.6 mA */
	{0x17, 0x70},	/* 22.6 mA */
	{0x18, 0x74},	/* 23.4 mA */
	{0x19, 0x77},	/* 24.0 mA */
	{0x1A, 0x7A},	/* 24.6 mA */	
	{0x1B, 0x7C},	/* 25.0 mA */ 
	{0x1C, 0x7E},	/* 25.4 mA */
	{0x1D, 0x7F},	/* 25.6 mA */
	{0x00, 0x00},	/* SFTRST */
	{0x01, 0x0F},	/* VOVP[6:4], WPWMEN[3], ALCEN[2], LEDMD[1], LEDEN[0] */ 
	{0x03, 0x7F},	/* ILED[6:0] = 25.6 mA */
	{0x08, 0x44},	/* THL[7:4] = 4.096 ms, TLH[3:0] = 4.096 ms */
	{0x0B, 0x40}	/* ADCYC[7:6], GAIN[5:4], VSB[2], MDCIR[1], SBIASON[0] */
};

static void backlight_ctrl(s32 value)
{
	struct i2c_msg msg = { 0, 0, 2, NULL };
	s32 i, ret = 0;

	if (value) {	/* Backlight On Sequence */
	
		if (lcd_power == OFF)
			lcd_power_ctrl(ON);	

		/* LUM_PWM */
		gpio_set_value(GPIO_LUM_PWM, GPIO_LEVEL_HIGH);
#if 0		
		/* ALC_RST_N */
		gpio_set_value(GPIO_ALC_RST_N, GPIO_LEVEL_HIGH);
#endif
		if (backlight_i2c_client == NULL) { 
			if (backlight_i2c_init() < 0) {
				printk("%s -> i2c_init error!\n", __FUNCTION__);	
				return;
			}
		}

		if (value & BACKLIGHT_STATUS_ALC)  
			alc_setting[ALC_CONTROL_REG][1] |= ALC_CONTROL_LEDMD;			/* ALC ON */
		else 
			alc_setting[ALC_CONTROL_REG][1] &= ~ALC_CONTROL_LEDMD;			/* ALC OFF */ 

		alc_setting[ALC_CURRENT_REG][1] = ((value & BACKLIGHT_LEVEL_VALUE) >> 2);		
		
		msg.addr = backlight_i2c_client->addr;
		
		for (i = 0; i < ALC_SETTING_NUM; i++) { 
			msg.buf = &alc_setting[i][0];
			if ((ret = i2c_transfer(backlight_i2c_client->adapter, &msg, 1)) < 0) 
				goto backlight_i2c_error;
		}
		
		backlight_power = ON;
	}
	else {	/* Backlight Off Sequence */

		/* LUM_PWM */
		gpio_set_value(GPIO_LUM_PWM, GPIO_LEVEL_LOW);
#if 0		
		/* ALC_RST_N */
		gpio_set_value(GPIO_ALC_RST_N, GPIO_LEVEL_LOW);
#endif	
		backlight_power = OFF;
	
		lcd_power_ctrl(OFF);	
	}

backlight_i2c_error:

	if (ret < 0)
		printk("%s -> i2c_transfer error! (ret = %d)\n", __FUNCTION__, ret);	
}

void backlight_level_ctrl(s32 value)
{
	if ((value < BACKLIGHT_LEVEL_MIN) ||	/* Invalid Value */
		(value > BACKLIGHT_LEVEL_MAX) ||
		(value == backlight_level))	/* Same Value */
		return;

	if (backlight_power)
		backlight_ctrl(value);	
	
	backlight_level = value;	
}

void backlight_power_ctrl(s32 value)
{
	if ((value < OFF) ||	/* Invalid Value */
		(value > ON) ||
		(value == backlight_power))	/* Same Value */
		return;

	backlight_ctrl((value ? backlight_level : OFF));	
	
	backlight_power = value;	
}

#define BD6091GU_DEFAULT_BACKLIGHT_BRIGHTNESS	255

static s32 bd6091gu_backlight_off;
static s32 bd6091gu_backlight_brightness = BD6091GU_DEFAULT_BACKLIGHT_BRIGHTNESS;
static u8 bd6091gu_backlight_last_level = 33;
static DEFINE_MUTEX(bd6091gu_backlight_lock);

static void bd6091gu_set_backlight_level(u8 level)
{
	if (bd6091gu_backlight_last_level == level)
		return;

	backlight_ctrl(level);	
	
	bd6091gu_backlight_last_level = level;
}

static void bd6091gu_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	mutex_lock(&bd6091gu_backlight_lock);
	bd6091gu_backlight_brightness = value;
	if(!bd6091gu_backlight_off)
		bd6091gu_set_backlight_level(bd6091gu_backlight_brightness);
	mutex_unlock(&bd6091gu_backlight_lock);
}

static struct led_classdev bd6091gu_backlight_led  = {
	.name		= "lcd-backlight",
	.brightness = BD6091GU_DEFAULT_BACKLIGHT_BRIGHTNESS,
	.brightness_set = bd6091gu_brightness_set,
};

static int bd6091gu_backlight_probe(struct platform_device *pdev)
{
	led_classdev_register(&pdev->dev, &bd6091gu_backlight_led);
	return 0;
}

static int bd6091gu_backlight_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&bd6091gu_backlight_led);
	return 0;
}

static struct platform_driver bd6091gu_backlight_driver = {
	.probe		= bd6091gu_backlight_probe,
	.remove		= bd6091gu_backlight_remove,
	.driver		= {
		.name		= "bd6091gu-backlight",
		.owner		= THIS_MODULE,
	},
};

static int __init bd6091gu_backlight_init(void)
{
	return platform_driver_register(&bd6091gu_backlight_driver);
}

static void __exit bd6091gu_backlight_exit(void)
{
	platform_driver_unregister(&bd6091gu_backlight_driver);
}

module_init(bd6091gu_backlight_init);
module_exit(bd6091gu_backlight_exit);

void s3cfb_init_hw(void)
{
	s3cfb_set_fimd_info();

	s3cfb_set_gpio();
#ifdef CONFIG_FB_S3C_LCD_INIT	
	lcd_gpio_init();
	
	backlight_gpio_init();

	lcd_power_ctrl(ON);

	backlight_level_ctrl(BACKLIGHT_LEVEL_DEFAULT);

	backlight_power_ctrl(ON); 
#else
	lcd_gpio_init();
	
	backlight_gpio_init();
	
	lcd_power = ON;

	backlight_level = BACKLIGHT_LEVEL_DEFAULT;

	backlight_power = ON;
#endif
}

#define LOGO_MEM_BASE		(0x50000000 + 0x0D000000 - 0x100000)	/* SDRAM_BASE + SRAM_SIZE(208MB) - 1MB */
#define LOGO_MEM_SIZE		(S3C_FB_HRES * S3C_FB_VRES * 2)

void s3cfb_display_logo(int win_num)
{
	s3c_fb_info_t *fbi = &s3c_fb_info[0];
	u8 *logo_virt_buf;
	
	logo_virt_buf = ioremap_nocache(LOGO_MEM_BASE, LOGO_MEM_SIZE);

	memcpy(fbi->map_cpu_f1, logo_virt_buf, LOGO_MEM_SIZE);	

	iounmap(logo_virt_buf);
}

#include "s3cfb_progress.h"

static int progress = 0;

static int progress_flag = OFF;

static struct timer_list progress_timer;

static void progress_timer_handler(unsigned long data)
{
	s3c_fb_info_t *fbi = &s3c_fb_info[1];
	unsigned short *bar_src, *bar_dst;	
	int	i, j, p;

	/* 1 * 12 R5G5B5 BMP (Aligned 4 Bytes) */
	bar_dst = (unsigned short *)(fbi->map_cpu_f1 + (((320 * 416) + 41) * 2));
	bar_src = (unsigned short *)(progress_bar + sizeof(progress_bar) - 4);

	for (i = 0; i < 12; i++) {
		for (j = 0; j < 2; j++) {
			p = ((320 * i) + (progress * 2) + j);
			*(bar_dst + p) = (*(bar_src - (i * 2)) | 0x8000);
		}
	}	

	progress++;

	if (progress > 118) {
		del_timer(&progress_timer);
	}
	else {
		progress_timer.expires = (get_jiffies_64() + (HZ/15)); 
		progress_timer.function = progress_timer_handler; 
		add_timer(&progress_timer);
	}
}

static unsigned int new_wincon1; 
static unsigned int old_wincon1; 

void s3cfb_start_progress(void)
{
	s3c_fb_info_t *fbi = &s3c_fb_info[1];
	unsigned short *bg_src, *bg_dst;	
	int	i, j, p;
	
	memset(fbi->map_cpu_f1, 0x00, LOGO_MEM_SIZE);	

	/* 320 * 25 R5G5B5 BMP */
	bg_dst = (unsigned short *)(fbi->map_cpu_f1 + ((320 * 410) * 2));
	bg_src = (unsigned short *)(progress_bg + sizeof(progress_bg) - 2);

	for (i = 0; i < 25; i++) {
		for (j = 0; j < 320; j++) {
			p = ((320 * i) + j);
			if ((*(bg_src - p) & 0x7FFF) == 0x0000)
				*(bg_dst + p) = (*(bg_src - p) & ~0x8000);
			else
				*(bg_dst + p) = (*(bg_src - p) | 0x8000);
		}
	}	

	old_wincon1 = readl(S3C_WINCON1);

	new_wincon1 = S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_BUFSEL_0 | S3C_WINCONx_BUFAUTOEN_DISABLE | \
	           S3C_WINCONx_BITSWP_DISABLE | S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE | \
	           S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BLD_PIX_PIXEL | S3C_WINCONx_BPPMODE_F_16BPP_A555 | \
	           S3C_WINCONx_ALPHA_SEL_0 | S3C_WINCONx_ENWIN_F_ENABLE,

	writel(new_wincon1, S3C_WINCON1);

	init_timer(&progress_timer);
	progress_timer.expires = (get_jiffies_64() + (HZ/10)); 
	progress_timer.function = progress_timer_handler; 
	add_timer(&progress_timer);

	progress_flag = ON;
}

void s3cfb_stop_progress(void)
{
	if (progress_flag == OFF)
		return;

	del_timer(&progress_timer);
	
	writel(old_wincon1, S3C_WINCON1);
	
	progress_flag = OFF;
}
