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

#include <linux/i2c/maximi2c.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-lcd.h>

#include <mach/hardware.h>

#include "s3cfb.h"

#define BACKLIGHT_MODE_NORMAL	0x000
#define BACKLIGHT_MODE_ALC		0x100
#define BACKLIGHT_MODE_CABC		0x200

#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		0
#define BACKLIGHT_LEVEL_MAX		(BACKLIGHT_MODE_NORMAL | BACKLIGHT_LEVEL_VALUE)

#define BACKLIGHT_LEVEL_DEFAULT	BACKLIGHT_LEVEL_MAX		/* Default Setting */

/* sec_bsp_tsim 2009.08.12 : reset lcd before reboot this machine. */
void lcd_reset(void)
{
	gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
};
EXPORT_SYMBOL(lcd_reset);

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


#define S3C_FB_HRES_VIRTUAL     S3C_FB_HRES     /* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL     (S3C_FB_VRES * 2) 	/* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD			320		/* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD			480 	/* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD_VIRTUAL S3C_FB_HRES_OSD     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD_VIRTUAL (S3C_FB_VRES_OSD * 2) 	/* Vertial pixel Resolution */

#define S3C_FB_VFRAME_FREQ      60      /* Frame Rate Frequency */

#define S3C_FB_PIXEL_CLOCK      (S3C_FB_VFRAME_FREQ * \
                                (S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * \
                                (S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void s3cfb_set_fimd_info(void)
{
	s3c_fimd.vidcon1    = S3C_VIDCON1_IVCLK_RISE_EDGE | 
						  S3C_VIDCON1_IHSYNC_INVERT |
	                      S3C_VIDCON1_IVSYNC_INVERT |
	                      S3C_VIDCON1_IVDEN_NORMAL;

	s3c_fimd.vidtcon0   = S3C_VIDTCON0_VBPD(S3C_FB_VBP - 1) |
	                      S3C_VIDTCON0_VFPD(S3C_FB_VFP - 1) |
	                      S3C_VIDTCON0_VSPW(S3C_FB_VSW - 1);
	s3c_fimd.vidtcon1   = S3C_VIDTCON1_HBPD(S3C_FB_HBP - 1) |
	                      S3C_VIDTCON1_HFPD(S3C_FB_HFP - 1) |
	                      S3C_VIDTCON1_HSPW(S3C_FB_HSW - 1);
	s3c_fimd.vidtcon2   = S3C_VIDTCON2_LINEVAL(S3C_FB_VRES - 1) |
	                      S3C_VIDTCON2_HOZVAL(S3C_FB_HRES - 1);

	s3c_fimd.vidosd0a   = S3C_VIDOSDxA_OSD_LTX_F(0) |
	                      S3C_VIDOSDxA_OSD_LTY_F(0);
	s3c_fimd.vidosd0b   = S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES - 1) |
	                      S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES - 1);

	s3c_fimd.vidosd1a   = S3C_VIDOSDxA_OSD_LTX_F(0) |
	                      S3C_VIDOSDxA_OSD_LTY_F(0);
	s3c_fimd.vidosd1b   = S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES_OSD - 1) |
	                      S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES_OSD - 1);

	s3c_fimd.width		= 45;
	s3c_fimd.height 	= 68;
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

#if defined(CONFIG_FB_S3C_VIRTUAL_SCREEN)
	s3c_fimd.osd_xres_virtual = S3C_FB_HRES_OSD_VIRTUAL;
	s3c_fimd.osd_yres_virtual = S3C_FB_VRES_OSD_VIRTUAL;
#else
	s3c_fimd.osd_xres_virtual = S3C_FB_HRES_OSD;
	s3c_fimd.osd_yres_virtual = S3C_FB_VRES_OSD;
#endif

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
	/* B(0:5) */
	s3c_gpio_cfgpin(GPIO_LCD_B_0, S3C_GPIO_SFN(GPIO_LCD_B_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_1, S3C_GPIO_SFN(GPIO_LCD_B_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_2, S3C_GPIO_SFN(GPIO_LCD_B_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_3, S3C_GPIO_SFN(GPIO_LCD_B_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_4, S3C_GPIO_SFN(GPIO_LCD_B_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_5, S3C_GPIO_SFN(GPIO_LCD_B_5_AF));
	/* G(0:5) */
	s3c_gpio_cfgpin(GPIO_LCD_G_0, S3C_GPIO_SFN(GPIO_LCD_G_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_1, S3C_GPIO_SFN(GPIO_LCD_G_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_2, S3C_GPIO_SFN(GPIO_LCD_G_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_3, S3C_GPIO_SFN(GPIO_LCD_G_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_4, S3C_GPIO_SFN(GPIO_LCD_G_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_5, S3C_GPIO_SFN(GPIO_LCD_G_5_AF));
	/* R(0:5) */
	s3c_gpio_cfgpin(GPIO_LCD_R_0, S3C_GPIO_SFN(GPIO_LCD_R_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_1, S3C_GPIO_SFN(GPIO_LCD_R_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_2, S3C_GPIO_SFN(GPIO_LCD_R_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_3, S3C_GPIO_SFN(GPIO_LCD_R_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_4, S3C_GPIO_SFN(GPIO_LCD_R_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_5, S3C_GPIO_SFN(GPIO_LCD_R_5_AF));
	/* HSYNC */
	s3c_gpio_cfgpin(GPIO_LCD_HSYNC, S3C_GPIO_SFN(GPIO_LCD_HSYNC_AF));
	/* VSYNC */
	s3c_gpio_cfgpin(GPIO_LCD_VSYNC, S3C_GPIO_SFN(GPIO_LCD_VSYNC_AF));
	/* DE */
	s3c_gpio_cfgpin(GPIO_LCD_DE, S3C_GPIO_SFN(GPIO_LCD_DE_AF));
	/* CLK */
	s3c_gpio_cfgpin(GPIO_LCD_CLK, S3C_GPIO_SFN(GPIO_LCD_CLK_AF));
	
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
}

/*
 * Serial Interface
 */

#define LCD_CSX_HIGH	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW		gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_LOW);

#define LCD_SCL_HIGH	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH);
#define LCD_SCL_LOW		gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW		gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_LOW);



#define DEFAULT_USLEEP	5	

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
#define GAMCTL3			0xF9
#define GAMCTL4			0xFA
#define GAMCTL5			0xFB
#define GAMCTL6			0xFC
#define BCMODE			0xCB
#define WRCABC			0x55
#define DCON			0xEF
#define WRCTRLD			0x53
#define WRDISBV         0x51
#define WRCABCMB		0x5E
#define MIECTL1			0xCA
#define MIECTL2			0xCC
#define MIECTL3			0xCD





struct setting_table {
	u8 command;	
	u8 parameters;
	u8 parameter[15];
	s32 wait;
};

static struct setting_table power_on_setting_table[] = {
	{ PWRCTL,  9,  { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ SLPOUT,  0,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	
	{ DISCTL,  11, { 0x16, 0x16, 0x0F, 0x0A, 0x05, 0x0A, 0x05, 0x10, 0x00, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ PWRCTL,  9,  { 0x00, 0x01, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ VCMCTL,  5,  { 0x1A, 0x1A, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ SRCCTL,  6,  { 0x00, 0x00, 0x0A, 0x01, 0x01, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GATECTL, 3,  { 0x44, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	
	{ PWRCTL,  9,  { 0x00, 0x03, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x07, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x0F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x1F, 0x2A, 0x00, 0x02, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  15 },
	{ PWRCTL,  9,  { 0x00, 0x3F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{ PWRCTL,  9,  { 0x00, 0x7F, 0x2A, 0x00, 0x08, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  35 },
	
	{ MADCTL,  1,  { 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ COLMOD,  1,  { 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ GAMCTL1, 15, { 0x00, 0x00, 0x00, 0x14, 0x27, 0x2D, 0x2C, 0x2D, 0x10, 0x11, 0x10, 0x16, 0x04, 0x22, 0x22 },   0 },
	{ GAMCTL2, 15, { 0x00, 0x00, 0x00, 0x14, 0x27, 0x2D, 0x2C, 0x2D, 0x10, 0x11, 0x10, 0x16, 0x04, 0x22, 0x22 },   0 },
	{ GAMCTL3, 15, { 0x96, 0x00, 0x00, 0x00, 0x00, 0x15, 0x1E, 0x23, 0x16, 0x0D, 0x07, 0x10, 0x00, 0x81, 0x42 },   0 },
	{ GAMCTL4, 15, { 0x80, 0x16, 0x00, 0x00, 0x00, 0x15, 0x1E, 0x23, 0x16, 0x0D, 0x07, 0x10, 0x00, 0x81, 0x42 },   0 },
	{ GAMCTL5, 15, { 0x00, 0x00, 0x34, 0x30, 0x2F, 0x2F, 0x2E, 0x2F, 0x0E, 0x0D, 0x09, 0x0E, 0x00, 0x22, 0x12 },   0 },
	{ GAMCTL6, 15, { 0x00, 0x00, 0x34, 0x30, 0x2F, 0x2F, 0x2E, 0x2F, 0x0E, 0x0D, 0x09, 0x0E, 0x00, 0x22, 0x12 },   0 },
	{ BCMODE,  1,  { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{ MIECTL3, 2,  { 0x7C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{ WRDISBV, 1,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
//	{ MIECTL1, 3,  { 0x80, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
//	{ MIECTL2, 3,  { 0x20, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
//	{ WRCABC,  1,  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{ DCON,    1,  { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	
	{ DCON,    1,  { 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ WRCTRLD, 1,  { 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
};


#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
	{    DCON,  1, { 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	{    DCON,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  25 },
	{  PWRCTL,  9, { 0x00, 0x00, 0x2A, 0x00, 0x00, 0x33, 0x29, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPIN,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 200 },
};

#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))

#if 0
static struct setting_table cabc_on_setting_table[] = 
	{  WRCABC,  1, { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 };

static struct setting_table cabc_off_setting_table[] = 
	{  WRCABC,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 };
#endif
static struct setting_table backlight_setting_table = 
	{ WRDISBV,  1, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 };

static void setting_table_write(struct setting_table *table)
{
	s32 i, j;

		LCD_CSX_HIGH
		udelay(DEFAULT_USLEEP);
		LCD_SCL_HIGH 
		udelay(DEFAULT_USLEEP);
	
		/* Write Command */
		LCD_CSX_LOW
		udelay(DEFAULT_USLEEP);
		
		LCD_SCL_LOW 
		udelay(DEFAULT_USLEEP); 	
		LCD_SDI_LOW 
		udelay(DEFAULT_USLEEP);
		LCD_SCL_HIGH 
		udelay(DEFAULT_USLEEP); 
	
		for (i = 7; i >= 0; i--) { 
			LCD_SCL_LOW
			udelay(DEFAULT_USLEEP);
			if ((table->command >> i) & 0x1)
				LCD_SDI_HIGH
			else
				LCD_SDI_LOW
			udelay(DEFAULT_USLEEP); 
			LCD_SCL_HIGH
			udelay(DEFAULT_USLEEP); 
		}
	
		LCD_CSX_HIGH
		udelay(DEFAULT_USLEEP); 
	
		/* Write Parameter */
		if ((table->parameters) > 0) {
			for (j = 0; j < table->parameters; j++) {
				LCD_CSX_LOW 
				udelay(DEFAULT_USLEEP); 	
			
				LCD_SCL_LOW 
				udelay(DEFAULT_USLEEP); 	
				LCD_SDI_HIGH 
				udelay(DEFAULT_USLEEP);
				LCD_SCL_HIGH 
				udelay(DEFAULT_USLEEP); 	
	
				for (i = 7; i >= 0; i--) { 
					LCD_SCL_LOW
					udelay(DEFAULT_USLEEP); 
					if ((table->parameter[j] >> i) & 0x1)
						LCD_SDI_HIGH
					else
						LCD_SDI_LOW
					udelay(DEFAULT_USLEEP); 
					LCD_SCL_HIGH
					udelay(DEFAULT_USLEEP); 				
				}
			
				LCD_CSX_HIGH 
				udelay(DEFAULT_USLEEP); 	
			}
		}
		msleep(table->wait);
}

/*
 *	LCD Power Handler
 */

#define MAX8698_ID		0xCC

#define ONOFF2			0x01

#define ONOFF2_ELDO6	(0x01 << 7)
#define ONOFF2_ELDO7	(0x03 << 6)

void lcd_power_ctrl(s32 value)
{
	s32 i;	
	u8 data;

	// To prevent from another request while working.
	if(lcd_power > ON)
		{
		printk("LCD POWER CONTROL called with (%d) while working\n", value);
		return;
		}
	lcd_power = ON + 1;
	
	printk("#%s:%d s\n",__func__,value);

	if (value) {
	
		/* Power On Sequence */
	
		/* Power Enable */
		// Turn on VLCD1.8V first
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't read the status from PMIC to turn on 1.8V\n");
			}
		
		data |= ONOFF2_ELDO6;		// TURN ON VLCD_1.8V
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't write the command to PMIC to turn on 1.8V\n");
			}

		// wait 0s < t1 < 1ms
		udelay(10);

		// Turn on VLCD3.0V
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't read the status from PMIC to turn on 3.0V\n");
			}
		
		data |= ONOFF2_ELDO7;		// TURN ON VLCD_3.0V
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't write the command to PMIC to turn on 3.0V\n");
			}

		// wait longer than 1ms
		msleep(2);
		
		/* Reset Asseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
		msleep(1);	
		
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
		msleep(1);		// hold low level longer than 10us

		/* Reset Deasseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);

		msleep(10);		// wait longer than 10ms before writing setting table

		for (i = 0; i < POWER_ON_SETTINGS; i++)
			setting_table_write(&power_on_setting_table[i]);	
	}
	else {

		/* Power Off Sequence */
		for (i = 0; i < POWER_OFF_SETTINGS; i++)
			setting_table_write(&power_off_setting_table[i]);	

		/* Reset Assert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);

		// Turn off VLCD3.0V first
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't read the status from PMIC to turn off 3.0V\n");
			}
		
		data &= ~ONOFF2_ELDO7;
		
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't write the command to PMIC to turn off 3.0V\n");
			}

		/* Power Disable */
		// Turn off VLCD1.8V next
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't read the status from PMIC to turn off 1.8V\n");
			}
		
		data &= ~ONOFF2_ELDO6;
		
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS)
			{
			printk("LCD POWER CONTROL can't write the command to PMIC to turn off 1.8V\n");
			}

		// While sleep in state, VDDI couldn't drop to 1.8V because it was affected by CSX and SCL,
		// so we need to make low them.
		LCD_CSX_LOW
		LCD_SCL_LOW
		
	}

	lcd_power = value;
	
	printk("#%s:%d e\n",__func__,value);
}

void backlight_ctrl(s32 value)
{
	if (value) {
		/* Backlight On Sequence */
		
		if (lcd_power == OFF) {
			if(!s3cfb_is_clock_on()) {
				s3cfb_enable_clock_power();
			}
			lcd_power_ctrl(ON);
		}

		backlight_setting_table.parameter[0] = (value & BACKLIGHT_LEVEL_VALUE);
		
//		printk("LCD Backlight level setting value ==> %d \n",value);
		
		setting_table_write(&backlight_setting_table);	
	}
	else {
		/* Backlight Off Sequence */

		lcd_power_ctrl(OFF);	
	}

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
		(value > ON))
		return;

	backlight_ctrl((value ? backlight_level : OFF));	
	
	backlight_power = (value ? ON : OFF);
}

#define S6D05A_DEFAULT_BACKLIGHT_BRIGHTNESS	255

static s32 s6d05a_backlight_off;
static s32 s6d05a_backlight_brightness = S6D05A_DEFAULT_BACKLIGHT_BRIGHTNESS;
static u8 s6d05a_backlight_last_level = 33;
static DEFINE_MUTEX(s6d05a_backlight_lock);

static void s6d05a_set_backlight_level(u8 level)
{
	if (backlight_level == level)
		return;

	backlight_ctrl(level);	
	
	backlight_level = level;
}

static void s6d05a_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	mutex_lock(&s6d05a_backlight_lock);
	s6d05a_backlight_brightness = value;
	s6d05a_set_backlight_level(s6d05a_backlight_brightness);
	mutex_unlock(&s6d05a_backlight_lock);
}

static struct led_classdev s6d05a_backlight_led  = {
	.name		= "lcd-backlight",
	.brightness = S6D05A_DEFAULT_BACKLIGHT_BRIGHTNESS,
	.brightness_set = s6d05a_brightness_set,
};

static int s6d05a_backlight_probe(struct platform_device *pdev)
{
	led_classdev_register(&pdev->dev, &s6d05a_backlight_led);
	return 0;
}

static int s6d05a_backlight_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&s6d05a_backlight_led);
	return 0;
}

static struct platform_driver s6d05a_backlight_driver = {
	.probe		= s6d05a_backlight_probe,
	.remove		= s6d05a_backlight_remove,
	.driver		= {
		.name		= "s6d05a-backlight",
		.owner		= THIS_MODULE,
	},
};

static int __init s6d05a_backlight_init(void)
{
	return platform_driver_register(&s6d05a_backlight_driver);
}

static void __exit s6d05a_backlight_exit(void)
{
	platform_driver_unregister(&s6d05a_backlight_driver);
}
module_init(s6d05a_backlight_init);
module_exit(s6d05a_backlight_exit);

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

//	backlight_level = BACKLIGHT_LEVEL_DEFAULT;
	backlight_level = -1;	// GA_KSS : If we use pre-defined value for backlight level, user can see darker screen just after power on when backlight leve was set as maximum.
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

#include "s3cfb_progress_hvga.h"

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
#ifdef CONFIG_FB_S3C_BPP_24
	writel(s3c_fimd.wincon0,    S3C_WINCON0);
  	s3cfb_onoff_win(&s3c_fb_info[0], ON);
#endif
	writel(old_wincon1, S3C_WINCON1);
	
	progress_flag = OFF;
}
