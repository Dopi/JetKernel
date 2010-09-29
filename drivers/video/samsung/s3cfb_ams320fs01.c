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

#define BACKLIGHT_STATUS_ALC	0x100
#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		0
#define BACKLIGHT_LEVEL_MAX		BACKLIGHT_LEVEL_VALUE

#define BACKLIGHT_LEVEL_DEFAULT	BACKLIGHT_LEVEL_MAX		/* Default Setting */

#define OFFSET_LCD_ON           (0x1 << 7)

#if defined(CONFIG_MACH_VINSQ) || defined(CONFIG_MACH_QUATTRO)
#define POWCTL			0xF3
#define VCMCTL			0xF4
#define SRCCTL			0xF5
#define SLPOUT			0x11
#define MADCTL			0x36
#define COLMOD			0x3A
#define DISCTL			0xF2
#define IFCTL			0xF6
#define GATECTL			0xFD
#define WRDISBV			0x51
#define WRCABCMB		0x5E
#define MIECTL1			0xCA
#define BCMODE			0xCB
#define MIECTL2			0xCC
#define MIDCTL3			0xCD
#define RPGAMCTL		0xF7
#define RNGAMCTL		0xF8
#define GPGAMCTL		0xF9
#define GNGAMCTL		0xFA
#define BPGAMCTL		0xFB
#define BNGAMCTL		0xFC
#define CASET			0x2A
#define PASET			0x2B
#define RAMWR           0x2C
#define WRCTRLD			0x53
#define WRCABC			0x55
#define DISPON			0x29
#define DISPOFF			0x28
#define SLPIN			0x10
#define TEON            0x35

extern int lcd_late_resume;

/* sec_bsp_tsim 2009.08.12 : reset lcd before reboot this machine. */
void lcd_reset(void)
{
	gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
};
EXPORT_SYMBOL(lcd_reset);

extern void s3c_bat_set_compensation_for_drv(int mode,int offset);

int lcd_power = OFF;
EXPORT_SYMBOL(lcd_power);

int lcd_power_ctrl(s32 value);
EXPORT_SYMBOL(lcd_power_ctrl);

int backlight_power = OFF;
EXPORT_SYMBOL(backlight_power);

void backlight_power_ctrl(s32 value);
EXPORT_SYMBOL(backlight_power_ctrl);

int backlight_level = BACKLIGHT_LEVEL_DEFAULT;
EXPORT_SYMBOL(backlight_level);

void backlight_level_ctrl(s32 value);
EXPORT_SYMBOL(backlight_level_ctrl);

#define S3C_FB_HFP			64 		/* Front Porch */
#define S3C_FB_HSW			2 		/* Hsync Width */
#define S3C_FB_HBP			62 		/* Back Porch */

#define S3C_FB_VFP			8 		/* Front Porch */
#define S3C_FB_VSW			2 		/* Vsync Width */
#define S3C_FB_VBP			6 		/* Back Porch */

#define S3C_FB_HRES             240 //vinsq.boot 320     /* Horizon pixel Resolition */
#define S3C_FB_VRES             400 //vinsq.boot 480     /* Vertical pixel Resolution */
#define S3C_FB_HRES_VIRTUAL     S3C_FB_HRES     /* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL     S3C_FB_VRES * 2 /* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD         240 //vinsq.boot 320     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD         400 //vinsq.boot 480     /* Vertial pixel Resolution */
#define S3C_FB_HRES_OSD_VIRTUAL S3C_FB_HRES_OSD     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD_VIRTUAL S3C_FB_VRES_OSD * 2 /* Vertial pixel Resolution */

#define S3C_FB_VFRAME_FREQ  	60		/* Frame Rate Frequency */

#define S3C_FB_PIXEL_CLOCK		(S3C_FB_VFRAME_FREQ * \
								(S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * \
								(S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void s3cfb_set_fimd_info(void)
{
#if 1 //vinsq.boot
	s3c_fimd.vidcon1	= S3C_VIDCON1_IHSYNC_INVERT |
							S3C_VIDCON1_IVSYNC_INVERT |
							S3C_VIDCON1_IVDEN_NORMAL|S3C_VIDCON1_IVCLK_RISE_EDGE;
#else
	s3c_fimd.vidcon1	= S3C_VIDCON1_IHSYNC_INVERT |
							S3C_VIDCON1_IVSYNC_INVERT |
							S3C_VIDCON1_IVDEN_INVERT|S3C_VIDCON1_IVCLK_RISE_EDGE;
#endif
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
  printk("<<--- lcd_gpio_init --->>>\n");
	/* B(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_B_0, S3C_GPIO_SFN(GPIO_LCD_B_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_1, S3C_GPIO_SFN(GPIO_LCD_B_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_2, S3C_GPIO_SFN(GPIO_LCD_B_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_3, S3C_GPIO_SFN(GPIO_LCD_B_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_B_4, S3C_GPIO_SFN(GPIO_LCD_B_4_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_B_5, S3C_GPIO_SFN(GPIO_LCD_B_5_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_B_6, S3C_GPIO_SFN(GPIO_LCD_B_6_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_B_7, S3C_GPIO_SFN(GPIO_LCD_B_7_AF));
	/* G(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_G_0, S3C_GPIO_SFN(GPIO_LCD_G_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_1, S3C_GPIO_SFN(GPIO_LCD_G_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_2, S3C_GPIO_SFN(GPIO_LCD_G_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_3, S3C_GPIO_SFN(GPIO_LCD_G_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_4, S3C_GPIO_SFN(GPIO_LCD_G_4_AF));
	s3c_gpio_cfgpin(GPIO_LCD_G_5, S3C_GPIO_SFN(GPIO_LCD_G_5_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_G_6, S3C_GPIO_SFN(GPIO_LCD_G_6_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_G_7, S3C_GPIO_SFN(GPIO_LCD_G_7_AF));
	/* R(0:7) */
	s3c_gpio_cfgpin(GPIO_LCD_R_0, S3C_GPIO_SFN(GPIO_LCD_R_0_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_1, S3C_GPIO_SFN(GPIO_LCD_R_1_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_2, S3C_GPIO_SFN(GPIO_LCD_R_2_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_3, S3C_GPIO_SFN(GPIO_LCD_R_3_AF));
	s3c_gpio_cfgpin(GPIO_LCD_R_4, S3C_GPIO_SFN(GPIO_LCD_R_4_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_R_5, S3C_GPIO_SFN(GPIO_LCD_R_5_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_R_6, S3C_GPIO_SFN(GPIO_LCD_R_6_AF));
	//vinsq.boot s3c_gpio_cfgpin(GPIO_LCD_R_7, S3C_GPIO_SFN(GPIO_LCD_R_7_AF));
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
    s3c_gpio_cfgpin(GPIO_LCD_BL_EN, GPIO_LCD_BL_EN_AF);


	if (gpio_is_valid(GPIO_LCD_BL_EN)) {
		if (gpio_request(GPIO_LCD_BL_EN, S3C_GPIO_LAVEL(GPIO_LCD_BL_EN))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_BL_EN!\n");
		gpio_direction_output(GPIO_LCD_BL_EN, GPIO_LEVEL_HIGH);
	}    
}

#define LCD_CSX_HIGH	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW		gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_LOW);

#define LCD_SCL_HIGH	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH);
#define LCD_SCL_LOW		gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW		gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_LOW);
#define DEFAULT_USLEEP	10	


struct setting_table {
	u8 command;	
	u8 parameters;
	u8 parameter[15];
	s32 wait;
};

//vinsq.boot  COLMOD 55 > 66
#if 1
static struct setting_table power_on_setting_table[] = {
	{   POWCTL,  7, { 0x80, 0x00, 0x00, 0x0B, 0x33, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   VCMCTL,  5, { 0x6E, 0x6E, 0x7F, 0x7F, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SRCCTL,  5, { 0x12, 0x00, 0x03, 0xF0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPOUT,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
//	{     TEON,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   MADCTL,  1, { 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{   COLMOD,  1, { 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  30 },	
	{   DISCTL, 11, { 0x14, 0x14, 0x03, 0x03, 0x04, 0x03, 0x04, 0x10, 0x04, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{    IFCTL,  4, { 0x00, 0x81, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  GATECTL,  2, { 0x22, 0x01, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	

	{ RPGAMCTL, 15, { 0x00, 0x23, 0x15, 0x15, 0x1C, 0x19, 0x18, 0x1E, 0x24, 0x25, 0x25, 0x20, 0x10, 0x22, 0x21 },   0 },	
	{ RNGAMCTL, 15, { 0x19, 0x00, 0x15, 0x15, 0x1C, 0x1F, 0x1E, 0x24, 0x1E, 0x1F, 0x25, 0x20, 0x10, 0x22, 0x21 },   0 },	

	{ GPGAMCTL, 15, { 0x06, 0x23, 0x14, 0x14, 0x1D, 0x1A, 0x19, 0x1F, 0x24, 0x26, 0x30, 0x1E, 0x1E, 0x22, 0x21 },   0 },	
	{ GNGAMCTL, 15, { 0x19, 0x06, 0x14, 0x14, 0x1D, 0x20, 0x1F, 0x25, 0x1E, 0x20, 0x30, 0x1E, 0x1E, 0x22, 0x21 },   0 },	

	{ BPGAMCTL, 15, { 0x2C, 0x23, 0x20, 0x20, 0x23, 0x2F, 0x30, 0x39, 0x09, 0x09, 0x18, 0x13, 0x13, 0x22, 0x21 },   0 },	
	{ BNGAMCTL, 15, { 0x19, 0x2C, 0x20, 0x20, 0x23, 0x35, 0x36, 0x3F, 0x03, 0x03, 0x18, 0x13, 0x13, 0x22, 0x21 },   0 },	

	{    CASET,  4, { 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    PASET,  4, { 0x00, 0x00, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    RAMWR,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   DISPON,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  50 },	
};
#else
static struct setting_table power_on_setting_table[] = {
	{   POWCTL,  8, { 0xFF, 0x2A, 0x2A, 0x0A, 0x22, 0x72, 0x72, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   VCMCTL,  5, { 0x59, 0x59, 0x52, 0x52, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SRCCTL,  5, { 0x12, 0x00, 0x03, 0xF0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPOUT,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
	{     TEON,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   MADCTL,  1, { 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{   COLMOD,  1, { 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  30 },	
	{   DISCTL, 11, { 0x14, 0x14, 0x03, 0x02, 0x03, 0x02, 0x03, 0x10, 0x04, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{    IFCTL,  4, { 0x00, 0x81, 0x30, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  GATECTL,  2, { 0x22, 0x01, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{ RPGAMCTL, 15, { 0x0D, 0x00, 0x03, 0x0E, 0x1C, 0x29, 0x2D, 0x34, 0x0E, 0x12, 0x24, 0x1E, 0x07, 0x22, 0x22 },   0 },	
	{ RNGAMCTL, 15, { 0x0D, 0x00, 0x03, 0x0E, 0x1C, 0x29, 0x2D, 0x34, 0x0E, 0x12, 0x24, 0x1E, 0x07, 0x22, 0x22 },   0 },	
	{ GPGAMCTL, 15, { 0x1E, 0x00, 0x0A, 0x19, 0x23, 0x31, 0x37, 0x3F, 0x01, 0x03, 0x16, 0x19, 0x07, 0x22, 0x22 },   0 },	
	{ GNGAMCTL, 15, { 0x0D, 0x11, 0x0A, 0x19, 0x23, 0x31, 0x37, 0x3F, 0x01, 0x03, 0x16, 0x19, 0x07, 0x22, 0x22 },   0 },	
	{ BPGAMCTL, 15, { 0x0D, 0x35, 0x03, 0x0E, 0x1C, 0x29, 0x2D, 0x34, 0x0E, 0x12, 0x24, 0x1E, 0x07, 0x22, 0x22 },   0 },	
	{ BNGAMCTL, 15, { 0x0D, 0x35, 0x03, 0x0E, 0x1C, 0x29, 0x2D, 0x34, 0x0E, 0x12, 0x24, 0x1E, 0x07, 0x22, 0x22 },   0 },	
	{    CASET,  4, { 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    PASET,  4, { 0x00, 0x00, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    RAMWR,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   DISPON,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  50 },	
};
#endif

#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))


#define usleep(t)  udelay(t)
static void setting_table_write(struct setting_table *table)
{
	s32 i, j;

	LCD_CSX_HIGH
	usleep(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	usleep(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	usleep(DEFAULT_USLEEP);
	
	LCD_SCL_LOW 
	usleep(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	usleep(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	usleep(DEFAULT_USLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		usleep(DEFAULT_USLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		usleep(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		usleep(DEFAULT_USLEEP);	
	}

	LCD_CSX_HIGH
	usleep(DEFAULT_USLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
		for (j = 0; j < table->parameters; j++) {
			LCD_CSX_LOW 
			usleep(DEFAULT_USLEEP); 	
		
			LCD_SCL_LOW 
			usleep(DEFAULT_USLEEP); 	
			LCD_SDI_HIGH 
			usleep(DEFAULT_USLEEP);
			LCD_SCL_HIGH 
			usleep(DEFAULT_USLEEP); 	

			for (i = 7; i >= 0; i--) { 
				LCD_SCL_LOW
				usleep(DEFAULT_USLEEP);	
				if ((table->parameter[j] >> i) & 0x1)
					LCD_SDI_HIGH
				else
					LCD_SDI_LOW
				usleep(DEFAULT_USLEEP);	
				LCD_SCL_HIGH
				usleep(DEFAULT_USLEEP);					
			}
		
			LCD_CSX_HIGH 
			usleep(DEFAULT_USLEEP); 	
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

int lcd_power_ctrl(s32 value)
{
	s32 i;	
	u8 data;
	u32 timeout = 100;
	
	if (value) {
		while (timeout-- > 0) {
			if (lcd_late_resume == 1)
				break;
			msleep(50);
		}
		
		if (timeout == -1) {
			printk(KERN_ERR "lcd power control time out\n");
			return -1;
		}
	  printk("<<--- lcd_power_ctrl --->>>\n");
		printk("Lcd power on sequence start\n");
		/* Power On Sequence */
		s3c_bat_set_compensation_for_drv(1,OFFSET_LCD_ON);
		/* Reset Asseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
	
		/* Power Enable */
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't read the status from PMIC\n");
			return -1;
		}
		data |= (ONOFF2_ELDO6 | ONOFF2_ELDO7);
			
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't write the command to PMIC\n");
			return -1;
		}
	
		msleep(100);

		/* Reset Deasseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
	
		msleep(120);
	
		for (i = 0; i < POWER_ON_SETTINGS; i++)
			setting_table_write(&power_on_setting_table[i]);

		printk("Lcd power on sequence end\n");
	}
	else {
		printk("Lcd power off sequence start\n");

		s3c_bat_set_compensation_for_drv(0,OFFSET_LCD_ON);

    
		/* Reset Assert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
			
		/* Power Disable */
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't read the status from PMIC\n");
			return -1;
		}
		data &= ~(ONOFF2_ELDO6 | ONOFF2_ELDO7);
	
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't write the command to PMIC\n");
			return -1;
		}

		printk("Lcd power off sequence end\n");
	}
	
	lcd_power = value;

	return 0;
}

static s32 old_level = 0;

void backlight_ctrl(s32 value)
{
    printk("backlight_ctrl(%d)", value);

    
    if(value == 0)
    {
    	gpio_set_value(GPIO_LCD_BL_EN, GPIO_LEVEL_LOW);
        lcd_power_ctrl(OFF);
    }
    else
    {
    	if (lcd_power == OFF)
    		lcd_power_ctrl(ON);	
    	gpio_set_value(GPIO_LCD_BL_EN, GPIO_LEVEL_HIGH);
    }
}

#else //CONFIG_MACH_INSTINCTQ

extern int lcd_late_resume;

/* sec_bsp_tsim 2009.08.12 : reset lcd before reboot this machine. */
void lcd_reset(void)
{
	gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
};
EXPORT_SYMBOL(lcd_reset);

extern void s3c_bat_set_compensation_for_drv(int mode,int offset);

int lcd_power = OFF;
EXPORT_SYMBOL(lcd_power);

int lcd_power_ctrl(s32 value);
EXPORT_SYMBOL(lcd_power_ctrl);

int backlight_power = OFF;
EXPORT_SYMBOL(backlight_power);

void backlight_power_ctrl(s32 value);
EXPORT_SYMBOL(backlight_power_ctrl);

int backlight_level = BACKLIGHT_LEVEL_DEFAULT;
EXPORT_SYMBOL(backlight_level);

void backlight_level_ctrl(s32 value);
EXPORT_SYMBOL(backlight_level_ctrl);

#define S3C_FB_HFP			64 		/* Front Porch */
#define S3C_FB_HSW			2 		/* Hsync Width */
#define S3C_FB_HBP			62 		/* Back Porch */

#define S3C_FB_VFP			8 		/* Front Porch */
#define S3C_FB_VSW			2 		/* Vsync Width */
#define S3C_FB_VBP			6 		/* Back Porch */

#define S3C_FB_HRES             320     /* Horizon pixel Resolition */
#define S3C_FB_VRES             480     /* Vertical pixel Resolution */
#define S3C_FB_HRES_VIRTUAL     S3C_FB_HRES     /* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL     S3C_FB_VRES * 2 /* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD         320     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD         480     /* Vertial pixel Resolution */
#define S3C_FB_HRES_OSD_VIRTUAL S3C_FB_HRES_OSD     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD_VIRTUAL S3C_FB_VRES_OSD * 2 /* Vertial pixel Resolution */

#define S3C_FB_VFRAME_FREQ  	60		/* Frame Rate Frequency */

#define S3C_FB_PIXEL_CLOCK		(S3C_FB_VFRAME_FREQ * \
								(S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * \
								(S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void s3cfb_set_fimd_info(void)
{
	s3c_fimd.vidcon1	= S3C_VIDCON1_IHSYNC_INVERT |
							S3C_VIDCON1_IVSYNC_INVERT |
							S3C_VIDCON1_IVDEN_INVERT|S3C_VIDCON1_IVCLK_RISE_EDGE;

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

//Fix for G0100137946. /**Market > Game > Robo Defense Download and Execute String is small. */
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
}

/*
 * Serial Interface
 */

#define LCD_CS_N_HIGH	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH);
#define LCD_CS_N_LOW	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_LOW);

#define LCD_SCLK_HIGH	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH);
#define LCD_SCLK_LOW	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW	    gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_LOW);

#define DEFAULT_UDELAY	5	

static void spi_write(u16 reg_data)
{	
	s32 i;

	LCD_SCLK_HIGH
	udelay(DEFAULT_UDELAY);

	 LCD_CS_N_HIGH
	 udelay(DEFAULT_UDELAY);
	
	LCD_CS_N_LOW
	udelay(DEFAULT_UDELAY);

	 LCD_SCLK_HIGH
	 udelay(DEFAULT_UDELAY);
	
	
	
	for (i = 15; i >= 0; i--) { 
		LCD_SCLK_LOW
		udelay(DEFAULT_UDELAY);
	
		if ((reg_data >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_UDELAY);	
	
		LCD_SCLK_HIGH
		udelay(DEFAULT_UDELAY);	
	}

	
	 LCD_SCLK_HIGH
	 udelay(DEFAULT_UDELAY);
	
	 LCD_SDI_HIGH
	 udelay(DEFAULT_UDELAY); 
	
	LCD_CS_N_HIGH
	udelay(DEFAULT_UDELAY);
	 

}


struct setting_table {
	u16 reg_data;	
	s32 wait;
};

static struct setting_table power_on_setting_table[] = {
    { 0x0100 ,   0 },
    { 0x2133 ,   0 },
    { 0x2208 ,   0 },
    { 0x2300 ,   0 },
    { 0x2433 ,   0 },
    { 0x2533 ,   0 },
    { 0x2606 ,   0 },
    { 0x2742 ,   0 },
    { 0x2F02 ,   0 },
    { 0x0501 ,  200 },
    { 0x0401 ,  10 },

    { 0x0644 ,   0 },
    { 0x0704 ,   0 },
    { 0x0801 ,   0 },
    { 0x0906 ,   0 },
    { 0x0A11 ,   0 },
    { 0x0C00 ,   0 },
    { 0x0D14 ,   0 },
    { 0x0E00 ,   0 },
    { 0x0F1E ,   0 },
    { 0x1000 ,   0 },
    { 0x1C08 ,   0 },
    { 0x1D05 ,   0 },
    { 0x1F00 ,   0 },
};

#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_on_setting_table[] = {
   	{ 0x0405 ,  20 },
    { 0x0407 ,  15 },
	{ 0x0405 ,  25 },
	{ 0x0407 ,  15 },
	{ 0x0405 ,  25 },
	{ 0x0407 ,   0 },
};

#define DISPLAY_ON_SETTINGS	(int)(sizeof(display_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_off_setting_table[] = {
    { 0x0403, 100 },
    { 0x0401,  20 },
    { 0x0400,   0 },
};


#define DISPLAY_OFF_SETTINGS	(int)(sizeof(display_off_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
    { 0x0500,   0 },
	{ 0x0302,	10 },

};


#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))



#define CAMMA_LEVELS_REV04	11

#define GAMMA_SETTINGS_REV04	18

static struct setting_table gamma_setting_table_rev04[CAMMA_LEVELS_REV04][GAMMA_SETTINGS_REV04] = {
{	/*  50 Candela */		
	{ 0x303E,   0 },	
	{ 0x3139,   0 },	
	{ 0x323F,   0 },	
	{ 0x3326,   0 },	
	{ 0x3437,   0 },	
	{ 0x3531,   0 },	
	{ 0x361A,   0 },	
	{ 0x371E,   0 },	
	{ 0x381A,   0 },	
	{ 0x3918,   0 },	
	{ 0x3A1F,   0 },	
	{ 0x3B19,   0 },	
	{ 0x3C00,   0 },	
	{ 0x3D15,   0 },	
	{ 0x3E00,   0 },	
	{ 0x3F02,   0 },	
	{ 0x4012,   0 },	
	{ 0x4100,   0 },       
},
{	/* 120 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3338,   0 },
	{ 0x344C,   0 },	
	{ 0x354E,   0 },
	{ 0x361A,   0 },
	{ 0x371C,   0 },
	{ 0x381A,   0 },
	{ 0x391A,   0 },
	{ 0x3A1F,   0 },
	{ 0x3B1A,   0 },
	{ 0x3C07,   0 },
	{ 0x3D19,   0 },
	{ 0x3E09,   0 },
	{ 0x3F01,   0 },
	{ 0x4012,   0 },	
	{ 0x4100,   0 },
},
{	/* 150 Candela */
	{ 0x303E,   0 },
	{ 0x3139,   0 },	
	{ 0x323F,   0 },	
	{ 0x333F,   0 },	
	{ 0x3454,   0 },	
	{ 0x3557,   0 },	
	{ 0x3619,   0 },	
	{ 0x371B,   0 },	
	{ 0x3818,   0 },	
	{ 0x391B,   0 },	
	{ 0x3A20,   0 },	
	{ 0x3B1C,   0 },	
	{ 0x3C08,   0 },	
	{ 0x3D19,   0 },	
	{ 0x3E09,   0 },	
	{ 0x3F05,   0 },	
	{ 0x4017,   0 },	
	{ 0x4105,   0 },       
},
{	/* 180 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3345,   0 },
	{ 0x345C,   0 },
	{ 0x355F,   0 },
	{ 0x3619,   0 },
	{ 0x371A,   0 },
	{ 0x3818,   0 },
	{ 0x391B,   0 },
	{ 0x3A20,   0 },
	{ 0x3B1B,   0 },
	{ 0x3C0B,   0 },
	{ 0x3D1B,   0 },
	{ 0x3E0D,   0 },
	{ 0x3F06,   0 },
	{ 0x4017,   0 },
	{ 0x4106,   0 },
},
{	/* 210 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x334A,   0 },
	{ 0x3463,   0 },
	{ 0x3566,   0 },
	{ 0x3619,   0 },
	{ 0x371A,   0 },
	{ 0x3818,   0 },
	{ 0x391B,   0 },
	{ 0x3A1F,   0 },
	{ 0x3B1B,   0 },
	{ 0x3C0E,   0 },
	{ 0x3D1D,   0 },
	{ 0x3E0F,   0 },
	{ 0x3F0C,   0 },
	{ 0x401D,   0 },
	{ 0x410D,   0 },
},
{	/* 240 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3350,   0 },
	{ 0x346A,   0 },
	{ 0x356D,   0 },
	{ 0x3618,   0 },
	{ 0x3719,   0 },
	{ 0x3817,   0 },
	{ 0x391B,   0 },
	{ 0x3A1F,   0 },
	{ 0x3B1B,   0 },
	{ 0x3C0E,   0 },
	{ 0x3D1D,   0 },
	{ 0x3E10,   0 },
	{ 0x3F10,   0 },
	{ 0x4021,   0 },
	{ 0x4111,   0 },
},
{	/* 270 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3354,   0 },
	{ 0x3470,   0 },
	{ 0x3573,   0 },
	{ 0x3618,   0 },
	{ 0x3719,   0 },
	{ 0x3817,   0 },
	{ 0x391D,   0 },
	{ 0x3A20,   0 },
	{ 0x3B1D,   0 },
	{ 0x3C0F,   0 },
	{ 0x3D1D,   0 },
	{ 0x3E0F,   0 },
	{ 0x3F10,   0 },
	{ 0x4021,   0 },
	{ 0x4112,   0 },
},
{	/* 300 Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3358,   0 },
	{ 0x3475,   0 },
	{ 0x3578,   0 },
	{ 0x3618,   0 },
	{ 0x3719,   0 },
	{ 0x3818,   0 },
	{ 0x391C,   0 },
	{ 0x3A1F,   0 },
	{ 0x3B1B,   0 },
	{ 0x3C0D,   0 },
	{ 0x3D1C,   0 },
	{ 0x3E0E,   0 },
	{ 0x3F10,   0 },
	{ 0x4021,   0 },
	{ 0x4112,   0 },
},
{	/*  240(CAM1) Candela */		
	{ 0x303E,   0 },	
	{ 0x3139,   0 },	
	{ 0x323F,   0 },	
	{ 0x3350,   0 },	
	{ 0x346A,   0 },	
	{ 0x356D,   0 },	
	{ 0x3621,   0 },	
	{ 0x3721,   0 },	
	{ 0x3820,   0 },	
	{ 0x3925,   0 },	
	{ 0x3A29,   0 },	
	{ 0x3B25,   0 },	
	{ 0x3C2B,   0 },	
	{ 0x3D35,   0 },	
	{ 0x3E2C,   0 },	
	{ 0x3F31,   0 },	
	{ 0x403C,   0 },	
	{ 0x4131,   0 },       
},
{	/* 270(CAM2) Candela */	
	{ 0x303E,   0 },	
	{ 0x3139,   0 },
	{ 0x323F,   0 },
	{ 0x3354,   0 },
	{ 0x3470,   0 },
	{ 0x3573,   0 },
	{ 0x3621,   0 },
	{ 0x3721,   0 },
	{ 0x3820,   0 },
	{ 0x3926,   0 },
	{ 0x3A28,   0 },
	{ 0x3B25,   0 },
	{ 0x3C2C,   0 },
	{ 0x3D37,   0 },
	{ 0x3E2E,   0 },
	{ 0x3F33,   0 },
	{ 0x403D,   0 },
	{ 0x4133,   0 },
},
{	/*  300(CAM3) Candela */		
	{ 0x303E,   0 },	
	{ 0x3139,   0 },	
	{ 0x323F,   0 },	
	{ 0x3358,   0 },	
	{ 0x3475,   0 },	
	{ 0x3578,   0 },	
	{ 0x3621,   0 },	
	{ 0x3721,   0 },	
	{ 0x3820,   0 },	
	{ 0x3927,   0 },	
	{ 0x3A29,   0 },	
	{ 0x3B26,   0 },	
	{ 0x3C29,   0 },	
	{ 0x3D34,   0 },	
	{ 0x3E2B,   0 },	
	{ 0x3F33,   0 },	
	{ 0x403D,   0 },	
	{ 0x4133,   0 },       
},
};



/*
 * Gamma Register Stting Value
 */

#define CAMMA_LEVELS	12

#define GAMMA_SETTINGS	18

static struct setting_table gamma_setting_table[CAMMA_LEVELS][GAMMA_SETTINGS] = {
{	/*  40 Candela */	
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3323,   0 },
	{ 0x342A,   0 },
	{ 0x3531,   0 },
	{ 0x3626,   0 },
	{ 0x3721,   0 },
	{ 0x3820,   0 },
	{ 0x392B,   0 },
	{ 0x3A21,   0 },
	{ 0x3B25,   0 },
	{ 0x3C31,   0 },
	{ 0x3D1C,   0 },
	{ 0x3E22,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 120 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x333E,   0 },
	{ 0x3446,   0 },
	{ 0x3551,   0 },
	{ 0x3621,   0 },
	{ 0x371F,   0 },
	{ 0x381C,   0 },
	{ 0x392A,   0 },
	{ 0x3A23,   0 },
	{ 0x3B24,   0 },
	{ 0x3C31,   0 },
	{ 0x3D1C,   0 },
	{ 0x3E22,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 140 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3342,   0 },
	{ 0x344B,   0 },
	{ 0x3556,   0 },
	{ 0x3621,   0 },
	{ 0x371F,   0 },
	{ 0x381C,   0 },
	{ 0x392A,   0 },
	{ 0x3A23,   0 },
	{ 0x3B25,   0 },
	{ 0x3C32,   0 },
	{ 0x3D1D,   0 },
	{ 0x3E23,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 160 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3346,   0 },
	{ 0x3450,   0 },
	{ 0x355B,   0 },
	{ 0x3621,   0 },
	{ 0x371D,   0 },
	{ 0x381C,   0 },
	{ 0x3929,   0 },
	{ 0x3A23,   0 },
	{ 0x3B24,   0 },
	{ 0x3C32,   0 },
	{ 0x3D1D,   0 },
	{ 0x3E23,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },    
},
{	/* 180 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x334A,   0 },
	{ 0x3454,   0 },
	{ 0x3560,   0 },
	{ 0x3620,   0 },
	{ 0x371D,   0 },
	{ 0x381C,   0 },
	{ 0x3929,   0 },
	{ 0x3A23,   0 },
	{ 0x3B23,   0 },
	{ 0x3C32,   0 },
	{ 0x3D1E,   0 },
	{ 0x3E23,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 200 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x334D,   0 },
	{ 0x3458,   0 },
	{ 0x3564,   0 },
	{ 0x3620,   0 },
	{ 0x371D,   0 },
	{ 0x381B,   0 },
	{ 0x3929,   0 },
	{ 0x3A23,   0 },
	{ 0x3B24,   0 },
	{ 0x3C32,   0 },
	{ 0x3D1F,   0 },
	{ 0x3E23,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 220 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3350,   0 },
	{ 0x345C,   0 },
	{ 0x3568,   0 },
	{ 0x3620,   0 },
	{ 0x371C,   0 },
	{ 0x381B,   0 },
	{ 0x3928,   0 },
	{ 0x3A23,   0 },
	{ 0x3B23,   0 },
	{ 0x3C32,   0 },
	{ 0x3D1F,   0 },
	{ 0x3E24,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },  
},
{	/* 240 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3353,   0 },
	{ 0x3460,   0 },
	{ 0x356C,   0 },
	{ 0x3620,   0 },
	{ 0x371C,   0 },
	{ 0x381B,   0 },
	{ 0x3928,   0 },
	{ 0x3A22,   0 },
	{ 0x3B23,   0 },
	{ 0x3C32,   0 },
	{ 0x3D20,   0 },
	{ 0x3E24,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 260 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3356,   0 },
	{ 0x3464,   0 },
	{ 0x3570,   0 },
	{ 0x3620,   0 },
	{ 0x371B,   0 },
	{ 0x381B,   0 },
	{ 0x3927,   0 },
	{ 0x3A22,   0 },
	{ 0x3B22,   0 },
	{ 0x3C32,   0 },
	{ 0x3D21,   0 },
	{ 0x3E24,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },       
},
{	/* 280 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3358,   0 },
	{ 0x3467,   0 },
	{ 0x3572,   0 },
	{ 0x3620,   0 },
	{ 0x371B,   0 },
	{ 0x381C,   0 },
	{ 0x3927,   0 },
	{ 0x3A21,   0 },
	{ 0x3B22,   0 },
	{ 0x3C34,   0 },
	{ 0x3D22,   0 },
	{ 0x3E25,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x4129,   0 },   
},
{	/* 300 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x335B,   0 },
	{ 0x346B,   0 },
	{ 0x3575,   0 },
	{ 0x361E,   0 },
	{ 0x371A,   0 },
	{ 0x381A,   0 },
	{ 0x3927,   0 },
	{ 0x3A22,   0 },
	{ 0x3B22,   0 },
	{ 0x3C33,   0 },
	{ 0x3D21,   0 },
	{ 0x3E25,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x412A,   0 },  
},
{	/* 350 Candela */
	{ 0x3032,   0 },
	{ 0x313A,   0 },
	{ 0x323A,   0 },
	{ 0x3362,   0 },
	{ 0x3472,   0 },
	{ 0x357D,   0 },
	{ 0x361E,   0 },
	{ 0x371A,   0 },
	{ 0x381B,   0 },
	{ 0x3927,   0 },
	{ 0x3A22,   0 },
	{ 0x3B22,   0 },
	{ 0x3C32,   0 },
	{ 0x3D20,   0 },
	{ 0x3E24,   0 },
	{ 0x3F3A,   0 },
	{ 0x401C,   0 },
	{ 0x412A,   0 },       
},
};

static void setting_table_write(struct setting_table *table)
{
	spi_write(table->reg_data);
	if(table->wait)
		msleep(table->wait);
}

/*
 *	LCD Power Handler
 */

#define MAX8698_ID		0xCC

#define ONOFF2			0x01

#define ONOFF2_ELDO6	(0x01 << 7)
#define ONOFF2_ELDO7	(0x03 << 6)
static DEFINE_MUTEX(lcd_power_ctrl_mutex);
//the mutex is used to prevent two or more simultaneous instances of the lcd_power_ctrl function
int lcd_power_ctrl(s32 value)
{
	s32 i;	
	u8 data;
	int power_ctrl_return=0;
	mutex_lock(&lcd_power_ctrl_mutex);
	if(value==lcd_power)
		goto return_val;
	if (value) {
		if(!lcd_late_resume){
			power_ctrl_return = -1;
		goto return_val;
			}
	if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
		printk(KERN_ERR "LCD POWER CONTROL can't read the status from PMIC\n");
		power_ctrl_return = -1;
		goto return_val;
	}
	data |= (ONOFF2_ELDO6 | ONOFF2_ELDO7);
		
	if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
		printk(KERN_ERR "LCD POWER CONTROL can't write the command to PMIC\n");
		power_ctrl_return = -1;
		goto return_val;
		}
	
		printk("Lcd power on sequence start\n");
		/* Power On Sequence */
		s3c_bat_set_compensation_for_drv(1,OFFSET_LCD_ON);
		/* Reset Asseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
	

		/* Reset Deasseert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
	
		msleep(10);
	
		for (i = 0; i < POWER_ON_SETTINGS; i++)
			setting_table_write(&power_on_setting_table[i]);

		if (system_rev >= 0x40) {
			for (i = 0; i < GAMMA_SETTINGS_REV04; i++)
				setting_table_write(&gamma_setting_table_rev04[1][i]);
		}
		else {
			for (i = 0; i < GAMMA_SETTINGS; i++)
				setting_table_write(&gamma_setting_table[1][i]);
		}
		
		for (i = 0; i < DISPLAY_ON_SETTINGS; i++)
			setting_table_write(&display_on_setting_table[i]);

		
		printk("Lcd power on sequence end\n");
	}
	else {
		printk("Lcd power off sequence start\n");

		s3c_bat_set_compensation_for_drv(0,OFFSET_LCD_ON);

		/* Power Off Sequence */
		for (i = 0; i < DISPLAY_OFF_SETTINGS; i++)
			setting_table_write(&display_off_setting_table[i]);
		
		for (i = 0; i < POWER_OFF_SETTINGS; i++)
			setting_table_write(&power_off_setting_table[i]);
	
		/* Reset Assert */
		gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
			
		/* Power Disable */
		if(pmic_read(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't read the status from PMIC\n");
			power_ctrl_return = -1;
		goto return_val;
		}
		data &= ~(ONOFF2_ELDO6 | ONOFF2_ELDO7);
	
		if(pmic_write(MAX8698_ID, ONOFF2, &data, 1) != PMIC_PASS) {
			printk(KERN_ERR "LCD POWER CONTROL can't write the command to PMIC\n");
			power_ctrl_return = -1;
		goto return_val;
		}

		printk("Lcd power off sequence end\n");
	}
	
	lcd_power = value;
return_val:
	mutex_unlock(&lcd_power_ctrl_mutex);
	return power_ctrl_return;
}

static s32 old_level = 0;

void backlight_ctrl(s32 value)
{
	s32 i, level;
	u8 data;
	int ret;

	value &= BACKLIGHT_LEVEL_VALUE;

	if (system_rev >= 0x40)
	{
	
		if (value == 0)
			level = 0;
		else if (value == 177)
			level = 9;
		else if (value == 209)
			level = 10;
		else if (value == 241)
			level = 11;	
		else if ((value > 0) && (value < 32))
			level = 1;
		else	
			level = (((value - 32) / 32) + 2); 
	}
	else
	{
		if (value == 0)
			level = 0;
		else if ((value > 0) && (value < 30))
			level = 1;
		else	
			level = (((value - 30) / 21) + 2); 
	}

	if (level) {	
		if (level != old_level) {
		/* Power & Backlight On Sequence */
			if (lcd_power == OFF) {
				if(!s3cfb_is_clock_on()) {
					s3cfb_enable_clock_power();
				}
				ret = lcd_power_ctrl(ON);
				if (ret != 0) {
					printk(KERN_ERR "lcd power on control is failed\n");
					return;
				}
			}

			printk("LCD Backlight level setting value ==> %d  , level ==> %d \n",value,level);
			
			if(system_rev >= 0x40) {
				for (i = 0; i < GAMMA_SETTINGS_REV04; i++)
					setting_table_write(&gamma_setting_table_rev04[(level - 1)][i]);
			}
			else {
				for (i = 0; i < GAMMA_SETTINGS; i++)
					setting_table_write(&gamma_setting_table[(level - 1)][i]);
			}
		}
	}
	else {
		ret = lcd_power_ctrl(OFF);
		if (ret != 0) {
			printk(KERN_ERR "lcd power off control is failed\n");
			return;
		}
	}
}
#endif /*CONFIG_MACH_VINSQ*/

void backlight_level_ctrl(s32 value)
{
	if ((value < BACKLIGHT_LEVEL_MIN) ||	/* Invalid Value */
		(value > BACKLIGHT_LEVEL_MAX) ||
		(value == backlight_level))	/* Same Value */
		return;

	if (lcd_late_resume == 0) {
		printk(KERN_ERR "backlight control is not allowed after early suspend\n");
	   	return;
	}

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

#define AMS320FS01_DEFAULT_BACKLIGHT_BRIGHTNESS		255

static s32 ams320fs01_backlight_off;
static s32 ams320fs01_backlight_brightness = AMS320FS01_DEFAULT_BACKLIGHT_BRIGHTNESS;
static u8 ams320fs01_backlight_last_level = 33;
static DEFINE_MUTEX(ams320fs01_backlight_lock);

static void ams320fs01_set_backlight_level(u8 level)
{	
	if (backlight_level == level)
		return;

	backlight_ctrl(level);

	backlight_level = level;
}

static void ams320fs01_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	mutex_lock(&ams320fs01_backlight_lock);
	ams320fs01_backlight_brightness = value;
	ams320fs01_set_backlight_level(ams320fs01_backlight_brightness);
	mutex_unlock(&ams320fs01_backlight_lock);
}

static struct led_classdev ams320fs01_backlight_led  = {
	.name		= "lcd-backlight",
	.brightness = AMS320FS01_DEFAULT_BACKLIGHT_BRIGHTNESS,
	.brightness_set = ams320fs01_brightness_set,
};

static int ams320fs01_backlight_probe(struct platform_device *pdev)
{
	led_classdev_register(&pdev->dev, &ams320fs01_backlight_led);
	return 0;
}

static int ams320fs01_backlight_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&ams320fs01_backlight_led); 
	return 0;
}

static struct platform_driver ams320fs01_backlight_driver = {
	.probe		= ams320fs01_backlight_probe,
	.remove		= ams320fs01_backlight_remove,
	.driver		= {
		.name		= "ams320fs01-backlight",
		.owner		= THIS_MODULE,
	},
};

static int __init ams320fs01_backlight_init(void)
{
	return platform_driver_register(&ams320fs01_backlight_driver);
}

static void __exit ams320fs01_backlight_exit(void)
{
	platform_driver_unregister(&ams320fs01_backlight_driver); 
}
module_init(ams320fs01_backlight_init);
module_exit(ams320fs01_backlight_exit);

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
		progress_timer.expires = (get_jiffies_64() + (HZ/30)); 
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
