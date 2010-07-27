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

#include <linux/miscdevice.h>
#include "s3cfb_ams320fs01_ioctl.h"

#include <mach/param.h>

#define BACKLIGHT_STATUS_ALC	0x100
#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		0
#define BACKLIGHT_LEVEL_MAX		BACKLIGHT_LEVEL_VALUE

#define BACKLIGHT_LEVEL_DEFAULT	100		/* Default Setting */

extern void s3cfb_enable_clock_power(void);
extern int s3cfb_is_clock_on(void);

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

#define S3C_FB_HFP			64 		/* Front Porch */
#define S3C_FB_HSW			2 		/* Hsync Width */
#define S3C_FB_HBP			62 		/* Back Porch */

#define S3C_FB_VFP			8 		/* Front Porch */
#define S3C_FB_VSW			2 		/* Vsync Width */
#define S3C_FB_VBP			6 		/* Back Porch */

//#define S3C_FB_HRES             320     /* Horizontal pixel Resolition */
//#define S3C_FB_VRES             480     /* Vertical pixel Resolution */
#define S3C_FB_HRES             480     /* Horizontal pixel Resolition */
#define S3C_FB_VRES             800     /* Vertical pixel Resolution */
#define S3C_FB_HRES_VIRTUAL     S3C_FB_HRES     /* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL     S3C_FB_VRES * 2 /* Vertial pixel Resolution */
#define S3C_FB_WIDTH             40      /* Horizontal screen size in mm */
#define S3C_FB_HEIGHT            67      /* Vertical screen size in mm */

//#define S3C_FB_HRES_OSD         320     /* Horizontal pixel Resolition */
//#define S3C_FB_VRES_OSD         480     /* Vertial pixel Resolution */
#define S3C_FB_HRES_OSD         480     /* Horizontal pixel Resolition */
#define S3C_FB_VRES_OSD         800     /* Vertial pixel Resolution */
#define S3C_FB_HRES_OSD_VIRTUAL S3C_FB_HRES_OSD     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD_VIRTUAL S3C_FB_VRES_OSD * 2 /* Vertial pixel Resolution */
#define S3C_FB_WIDTH_OSD             40      /* Horizontal screen size in mm */
#define S3C_FB_HEIGHT_OSD            67      /* Vertical screen size in mm */

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

	s3c_fimd.width		= S3C_FB_HRES;	//S3C_FB_WIDTH;
	s3c_fimd.height 	= S3C_FB_VRES;	//S3C_FB_HEIGHT;
	s3c_fimd.xres 		= S3C_FB_HRES;
	s3c_fimd.yres 		= S3C_FB_VRES;

#if defined(CONFIG_FB_S3C_VIRTUAL_SCREEN)
	s3c_fimd.xres_virtual = S3C_FB_HRES_VIRTUAL;
	s3c_fimd.yres_virtual = S3C_FB_VRES_VIRTUAL;
#else
	s3c_fimd.xres_virtual = S3C_FB_HRES;
	s3c_fimd.yres_virtual = S3C_FB_VRES;
#endif

	s3c_fimd.osd_width 	= S3C_FB_HRES_OSD; //S3C_FB_WIDTH_OSD;
	s3c_fimd.osd_height 	= S3C_FB_VRES_OSD; //S3C_FB_HEIGHT_OSD;
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

	s3c_fimd.set_lcd_power		= lcd_power_ctrl;
	s3c_fimd.set_backlight_power 	= backlight_power_ctrl;
	s3c_fimd.set_brightness 	= backlight_level_ctrl;

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

static struct setting_table standby_off_table[] = {
   	{ 0x0300 ,  0 },
};

#define STANDBY_OFF	(int)(sizeof(standby_off_table)/sizeof(struct setting_table))


static struct setting_table power_on_setting_table[] = {
/* power setting sequence */
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

/* initializing sequence */
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
#if 0
	{ 0x0405 ,  25 },
	{ 0x0407 ,  15 },
	{ 0x0405 ,  25 },
	{ 0x0407 ,   0 },
#endif
};

#define DISPLAY_ON_SETTINGS	(int)(sizeof(display_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_off_setting_table[] = {
    { 0x0403, 100 },
    { 0x0401,  20 },
    { 0x0400, 10 },
};


#define DISPLAY_OFF_SETTINGS	(int)(sizeof(display_off_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
    { 0x0500,   0 },
	{ 0x0302,	10 },

};


#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))



#define CAMMA_LEVELS	23

#define GAMMA_SETTINGS	18

static struct setting_table gamma_setting_table[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/*  60 Candela */		
		{ 0x303E,   0 },	
		{ 0x3139,   0 },	
		{ 0x323F,   0 },	
		{ 0x331E,   0 },	
		{ 0x3435,   0 },	
		{ 0x352C,   0 },	
		{ 0x361A,   0 },	
		{ 0x371E,   0 },	
		{ 0x381A,   0 },	
		{ 0x3916,   0 },	
		{ 0x3A1E,   0 },	
		{ 0x3B17,   0 },	
		{ 0x3C00,   0 },	
		{ 0x3D15,   0 },	
		{ 0x3E00,   0 },	
		{ 0x3F02,   0 },	
		{ 0x4012,   0 },	
		{ 0x4100,   0 },       
	},
	{	/* 90 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x332F,   0 },
		{ 0x3440,   0 },
		{ 0x3543,   0 },
		{ 0x361A,   0 },
		{ 0x371D,   0 },
		{ 0x3819,   0 },
		{ 0x3919,   0 },
		{ 0x3A1F,   0 },
		{ 0x3B19,   0 },
		{ 0x3C01,   0 },
		{ 0x3D16,   0 },
		{ 0x3E04,   0 },
		{ 0x3F00,   0 },
		{ 0x4012,   0 },
		{ 0x4100,   0 },
	},
	{	/* 100 Candela */	
		{ 0x303E,   0 },
		{ 0x3139,   0 },	
		{ 0x323F,   0 },	
		{ 0x3331,   0 },
		{ 0x3443,   0 },
		{ 0x3545,   0 },
		{ 0x361B,   0 },
		{ 0x371D,   0 },
		{ 0x381B,   0 },
		{ 0x391A,   0 },	
		{ 0x3A20,   0 },	
		{ 0x3B1A,   0 },
		{ 0x3C01,   0 },
		{ 0x3D16,   0 },	
		{ 0x3E04,   0 },	
		{ 0x3F00,   0 },	
		{ 0x4012,   0 },	
		{ 0x4100,   0 },       
	},
	{	/* 110 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x3334,   0 },
		{ 0x3447,   0 },
		{ 0x3549,   0 },
		{ 0x361A,   0 },
		{ 0x371C,   0 },
		{ 0x381A,   0 },
		{ 0x391B,   0 },
		{ 0x3A20,   0 },
		{ 0x3B1B,   0 },
		{ 0x3C01,   0 },
		{ 0x3D16,   0 },
		{ 0x3E04,   0 },
		{ 0x3F00,   0 },
		{ 0x4012,   0 },
		{ 0x4100,   0 },
	},
	{	/* 120 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x3336,   0 },
		{ 0x3449,   0 },
		{ 0x354C,   0 },
		{ 0x3619,   0 },
		{ 0x371C,   0 },
		{ 0x3819,   0 },
		{ 0x391B,   0 },
		{ 0x3A20,   0 },
		{ 0x3B1B,   0 },
		{ 0x3C04,   0 },
		{ 0x3D18,   0 },
		{ 0x3E06,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 130 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x3339,   0 },
		{ 0x344D,   0 },
		{ 0x3550,   0 },
		{ 0x361A,   0 },
		{ 0x371C,   0 },
		{ 0x3819,   0 },
		{ 0x391B,   0 },
		{ 0x3A20,   0 },
		{ 0x3B1B,   0 },
		{ 0x3C06,   0 },
		{ 0x3D18,   0 },
		{ 0x3E08,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 140 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x333B,   0 },
		{ 0x344F,   0 },
		{ 0x3552,   0 },
		{ 0x3619,   0 },
		{ 0x371C,   0 },
		{ 0x3819,   0 },
		{ 0x391B,   0 },
		{ 0x3A20,   0 },
		{ 0x3B1B,   0 },
		{ 0x3C06,   0 },
		{ 0x3D18,   0 },
		{ 0x3E08,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 150 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x333D,   0 },
		{ 0x3452,   0 },
		{ 0x3555,   0 },
		{ 0x3619,   0 },
		{ 0x371B,   0 },
		{ 0x3819,   0 },
		{ 0x391A,   0 },
		{ 0x3A1F,   0 },
		{ 0x3B1A,   0 },
		{ 0x3C08,   0 },
		{ 0x3D1A,   0 },
		{ 0x3E09,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 160 Candela */	
		{ 0x303E,   0 },
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x333F,   0 },
		{ 0x3454,   0 },
		{ 0x3557,   0 },
		{ 0x3619,   0 },
		{ 0x371B,   0 },
		{ 0x3819,   0 },
		{ 0x391A,   0 },
		{ 0x3A1F,   0 },
		{ 0x3B1A,   0 },
		{ 0x3C09,   0 },
		{ 0x3D1B,   0 },
		{ 0x3E0B,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 170 Candela */	
		{ 0x303E,   0 },
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x3341,   0 },
		{ 0x3457,   0 },
		{ 0x355A,   0 },
		{ 0x3619,   0 },
		{ 0x371B,   0 },
		{ 0x3819,   0 },
		{ 0x391A,   0 },
		{ 0x3A1F,   0 },
		{ 0x3B1A,   0 },
		{ 0x3C0C,   0 },
		{ 0x3D1C,   0 },
		{ 0x3E0D,   0 },
		{ 0x3F00,   0 },
		{ 0x4013,   0 },
		{ 0x4100,   0 },
	},
	{	/* 180 Candela */	
		{ 0x303E,   0 },
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x3342,   0 },
		{ 0x3459,   0 },
		{ 0x355C,   0 },
		{ 0x361A,   0 },
		{ 0x371B,   0 },
		{ 0x3819,   0 },
		{ 0x391A,   0 },
		{ 0x3A1F,   0 },
		{ 0x3B1A,   0 },
		{ 0x3C0C,   0 },
		{ 0x3D1C,   0 },
		{ 0x3E0D,   0 },
		{ 0x3F03,   0 },
		{ 0x4015,   0 },
		{ 0x4103,   0 },
	},
	{	/* 190 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3344,	0 },
		{ 0x345B,	0 },
		{ 0x355E,	0 },
		{ 0x361A,	0 },
		{ 0x371B,	0 },
		{ 0x3819,	0 },
		{ 0x391A,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0C,	0 },
		{ 0x3D1C,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F05,	0 },
		{ 0x4017,	0 },
		{ 0x4105,	0 },
	},
	{	/* 200 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3346,	0 },
		{ 0x345E,	0 },
		{ 0x3561,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391A,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0C,	0 },
		{ 0x3D1C,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F06,	0 },
		{ 0x4019,	0 },
		{ 0x4107,	0 },
	},
	{	/* 210 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3348,	0 },
		{ 0x3460,	0 },
		{ 0x3563,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391A,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0C,	0 },
		{ 0x3D1C,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F06,	0 },
		{ 0x4019,	0 },
		{ 0x4107,	0 },
	},
	{	/* 220 Candela */
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3349,	0 },
		{ 0x3462,	0 },
		{ 0x3565,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0B,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0C,	0 },
		{ 0x3F09,	0 },
		{ 0x401C,	0 },
		{ 0x410A,	0 },
	},
	{	/* 230 Candela */
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334B,	0 },
		{ 0x3464,	0 },
		{ 0x3567,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0B,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0C,	0 },
		{ 0x3F09,	0 },
		{ 0x401C,	0 },
		{ 0x410A,	0 },
	},
	{	/* 240 Candela */
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334D,	0 },
		{ 0x3466,	0 },
		{ 0x3569,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0B,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F09,	0 },
		{ 0x401C,	0 },
		{ 0x410A,	0 },
	},
	{	/* 250 Candela */
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334E,	0 },
		{ 0x3468,	0 },
		{ 0x356B,	0 },
		{ 0x3619,	0 },
		{ 0x371A,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0C,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F0D,	0 },
		{ 0x401F,	0 },
		{ 0x410E,	0 },
	},
	{	/* 260 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3350,	0 },
		{ 0x346A,	0 },
		{ 0x356D,	0 },
		{ 0x3618,	0 },
		{ 0x3719,	0 },
		{ 0x3817,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0C,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F0D,	0 },
		{ 0x401F,	0 },
		{ 0x410E,	0 },
	},
	{	/* 270 Candela */
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3351,	0 },
		{ 0x346C,	0 },
		{ 0x356F,	0 },
		{ 0x3618,	0 },
		{ 0x3719,	0 },
		{ 0x3817,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0E,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0E,	0 },
		{ 0x3F0D,	0 },
		{ 0x401F,	0 },
		{ 0x410E,	0 },
	},
	{	/* 280 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3352,	0 },
		{ 0x346E,	0 },
		{ 0x3571,	0 },
		{ 0x3618,	0 },
		{ 0x3719,	0 },
		{ 0x3817,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0E,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0E,	0 },
		{ 0x3F0E,	0 },
		{ 0x401F,	0 },
		{ 0x410F,	0 },
	},
	{	/* 290 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3354,	0 },
		{ 0x3470,	0 },
		{ 0x3573,	0 },
		{ 0x3619,	0 },
		{ 0x3719,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0D,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F0E,	0 },
		{ 0x401F,	0 },
		{ 0x410F,	0 },
	},
	{	/* 300 Candela */	
		{ 0x303E,	0 },
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3355,	0 },
		{ 0x3472,	0 },
		{ 0x3575,	0 },
		{ 0x3619,	0 },
		{ 0x3719,	0 },
		{ 0x3818,	0 },
		{ 0x391B,	0 },
		{ 0x3A1F,	0 },
		{ 0x3B1B,	0 },
		{ 0x3C0D,	0 },
		{ 0x3D1B,	0 },
		{ 0x3E0D,	0 },
		{ 0x3F0E,	0 },
		{ 0x401F,	0 },
		{ 0x410F,	0 },
	},
};

static struct setting_table gamma_setting_table_video[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/*  50 Candela */		
		{ 0x303E,   0 },	
		{ 0x3139,   0 },	
		{ 0x323F,   0 },	
		{ 0x3322,   0 },	
		{ 0x3430,   0 },	
		{ 0x3532,   0 },	
		{ 0x361E,   0 },	
		{ 0x3722,   0 },	
		{ 0x381D,   0 },	
		{ 0x3924,   0 },	
		{ 0x3A27,   0 },	
		{ 0x3B25,   0 },	
		{ 0x3C0A,   0 },	
		{ 0x3D24,   0 },	
		{ 0x3E0B,   0 },	
		{ 0x3F17,   0 },	
		{ 0x4029,   0 },	
		{ 0x4115,   0 },       
	},
	{	/* 90 Candela */	
		{ 0x303E,   0 },	
		{ 0x3139,   0 },
		{ 0x323F,   0 },
		{ 0x332F,   0 },
		{ 0x3440,   0 },
		{ 0x3543,   0 },
		{ 0x361E,   0 },
		{ 0x3721,   0 },
		{ 0x381D,   0 },
		{ 0x3926,   0 },
		{ 0x3A28,   0 },
		{ 0x3B26,   0 },
		{ 0x3C14,   0 },
		{ 0x3D2C,   0 },
		{ 0x3E15,   0 },
		{ 0x3F17,   0 },
		{ 0x4029,   0 },
		{ 0x4117,   0 },
	},
	{	/* 100 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3331,	0 },
		{ 0x3443,	0 },
		{ 0x3546,	0 },
		{ 0x3620,	0 },
		{ 0x3722,	0 },
		{ 0x381F,	0 },
		{ 0x3926,	0 },
		{ 0x3A27,	0 },
		{ 0x3B26,	0 },
		{ 0x3C15,	0 },
		{ 0x3D2D,	0 },
		{ 0x3E15,	0 },
		{ 0x3F1C,	0 },
		{ 0x402E,	0 },
		{ 0x411B,	0 },
	},
	{	/* 110 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3334,   0 },
		{ 0x3447,   0 },
		{ 0x3549,   0 },
		{ 0x361E,   0 },
		{ 0x3721,   0 },
		{ 0x381E,   0 },
		{ 0x3926,   0 },
		{ 0x3A27,   0 },
		{ 0x3B27,   0 },
		{ 0x3C17,   0 },
		{ 0x3D2E,   0 },
		{ 0x3E17,   0 },
		{ 0x3F1C,   0 },
		{ 0x402E,   0 },
		{ 0x411C,   0 },
	},
	{	/* 120 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3336,   0 },
		{ 0x3449,   0 },
		{ 0x354C,   0 },
		{ 0x361F,   0 },
		{ 0x3722,   0 },
		{ 0x381F,   0 },
		{ 0x3926,   0 },
		{ 0x3A27,   0 },
		{ 0x3B25,   0 },
		{ 0x3C17,   0 },
		{ 0x3D2E,   0 },
		{ 0x3E19,   0 },
		{ 0x3F20,   0 },
		{ 0x4030,   0 },
		{ 0x411F,   0 },
	},
	{	/* 130 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3339,   0 },
		{ 0x344C,   0 },
		{ 0x354F,   0 },
		{ 0x361D,   0 },
		{ 0x3721,   0 },
		{ 0x381E,   0 },
		{ 0x3928,   0 },
		{ 0x3A27,   0 },
		{ 0x3B27,   0 },
		{ 0x3C18,   0 },
		{ 0x3D30,   0 },
		{ 0x3E19,   0 },
		{ 0x3F24,   0 },
		{ 0x4033,   0 },
		{ 0x4124,   0 },
	},
	{	/* 140 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x333B,   0 },
		{ 0x344F,   0 },
		{ 0x3552,   0 },
		{ 0x361D,   0 },
		{ 0x3721,   0 },
		{ 0x381D,   0 },
		{ 0x3927,   0 },
		{ 0x3A27,   0 },
		{ 0x3B27,   0 },
		{ 0x3C19,   0 },
		{ 0x3D30,   0 },
		{ 0x3E1A,   0 },
		{ 0x3F25,   0 },
		{ 0x4033,   0 },
		{ 0x4125,   0 },
	},
	{	/* 150 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x333D,   0 },
		{ 0x3452,   0 },
		{ 0x3555,   0 },
		{ 0x361D,   0 },
		{ 0x3721,   0 },
		{ 0x381C,   0 },
		{ 0x3927,   0 },
		{ 0x3A26,   0 },
		{ 0x3B27,   0 },
		{ 0x3C1C,   0 },
		{ 0x3D32,   0 },
		{ 0x3E1C,   0 },
		{ 0x3F27,   0 },
		{ 0x4036,   0 },
		{ 0x4128,   0 },
	},
	{	/* 160 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x333F,   0 },
		{ 0x3454,   0 },
		{ 0x3558,   0 },
		{ 0x361E,   0 },
		{ 0x3721,   0 },
		{ 0x381C,   0 },
		{ 0x3927,   0 },
		{ 0x3A27,   0 },
		{ 0x3B27,   0 },
		{ 0x3C1A,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E1A,   0 },
		{ 0x3F27,   0 },
		{ 0x4037,   0 },
		{ 0x4128,   0 },
	},
	{	/* 170 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3341,   0 },
		{ 0x3457,   0 },
		{ 0x355A,   0 },
		{ 0x361D,   0 },
		{ 0x3720,   0 },
		{ 0x381C,   0 },
		{ 0x3927,   0 },
		{ 0x3A27,   0 },
		{ 0x3B28,   0 },
		{ 0x3C1A,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E19,   0 },
		{ 0x3F27,   0 },
		{ 0x4037,   0 },
		{ 0x4128,   0 },
	},
	{	/* 180 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3343,   0 },
		{ 0x3459,   0 },
		{ 0x355D,   0 },
		{ 0x361D,   0 },
		{ 0x3720,   0 },
		{ 0x381B,   0 },
		{ 0x3926,   0 },
		{ 0x3A27,   0 },
		{ 0x3B28,   0 },
		{ 0x3C1B,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E1A,   0 },
		{ 0x3F2A,   0 },
		{ 0x403A,   0 },
		{ 0x412A,   0 },
	},
	{	/* 190 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3344,	0 },
		{ 0x345B,	0 },
		{ 0x355F,	0 },
		{ 0x361E,	0 },
		{ 0x3720,	0 },
		{ 0x381C,	0 },
		{ 0x3927,	0 },
		{ 0x3A27,	0 },
		{ 0x3B28,	0 },
		{ 0x3C1C,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1C,	0 },
		{ 0x3F2B,	0 },
		{ 0x403A,	0 },
		{ 0x412A,	0 },
	},
	{	/* 200 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3346,	0 },
		{ 0x345E,	0 },
		{ 0x3561,	0 },
		{ 0x361E,	0 },
		{ 0x371F,	0 },
		{ 0x381C,	0 },
		{ 0x3926,	0 },
		{ 0x3A27,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1C,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1D,	0 },
		{ 0x3F2D,	0 },
		{ 0x403B,	0 },
		{ 0x412C,	0 },
	},
	{	/* 210 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3348,	0 },
		{ 0x3460,	0 },
		{ 0x3564,	0 },
		{ 0x361D,	0 },
		{ 0x371F,	0 },
		{ 0x381C,	0 },
		{ 0x3926,	0 },
		{ 0x3A27,	0 },
		{ 0x3B26,	0 },
		{ 0x3C1B,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1D,	0 },
		{ 0x3F2D,	0 },
		{ 0x403A,	0 },
		{ 0x412C,	0 },
	},
	{	/* 220 Candela */
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3349,	0 },
		{ 0x3462,	0 },
		{ 0x3565,	0 },
		{ 0x361E,	0 },
		{ 0x371F,	0 },
		{ 0x381D,	0 },
		{ 0x3927,	0 },
		{ 0x3A27,	0 },
		{ 0x3B26,	0 },
		{ 0x3C1A,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1C,	0 },
		{ 0x3F2C,	0 },
		{ 0x403A,	0 },
		{ 0x412C,	0 },
	},
	{	/* 230 Candela */
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334B,	0 },
		{ 0x3464,	0 },
		{ 0x3568,	0 },
		{ 0x361D,	0 },
		{ 0x371F,	0 },
		{ 0x381C,	0 },
		{ 0x3928,	0 },
		{ 0x3A27,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1A,	0 },
		{ 0x3D2F,	0 },
		{ 0x3E1B,	0 },
		{ 0x3F2C,	0 },
		{ 0x403B,	0 },
		{ 0x412C,	0 },
	},
	{	/* 240 Candela */
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334C,	0 },
		{ 0x3466,	0 },
		{ 0x3569,	0 },
		{ 0x361C,	0 },
		{ 0x371F,	0 },
		{ 0x381C,	0 },
		{ 0x3928,	0 },
		{ 0x3A26,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1B,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1B,	0 },
		{ 0x3F2C,	0 },
		{ 0x403B,	0 },
		{ 0x412D,	0 },
	},
	{	/* 250 Candela */
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x334F,	0 },
		{ 0x3468,	0 },
		{ 0x356C,	0 },
		{ 0x361B,	0 },
		{ 0x371F,	0 },
		{ 0x381B,	0 },
		{ 0x3929,	0 },
		{ 0x3A26,	0 },
		{ 0x3B28,	0 },
		{ 0x3C1C,	0 },
		{ 0x3D30,	0 },
		{ 0x3E1C,	0 },
		{ 0x3F2E,	0 },
		{ 0x403D,	0 },
		{ 0x412E,	0 },
	},
	{	/* 260 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3350,	0 },
		{ 0x346A,	0 },
		{ 0x356E,	0 },
		{ 0x361C,	0 },
		{ 0x371F,	0 },
		{ 0x381B,	0 },
		{ 0x3927,	0 },
		{ 0x3A25,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1E,	0 },
		{ 0x3D31,	0 },
		{ 0x3E1E,	0 },
		{ 0x3F2F,	0 },
		{ 0x403D,	0 },
		{ 0x412F,	0 },
	},
	{	/* 270 Candela */
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3351,	0 },
		{ 0x346C,	0 },
		{ 0x356F,	0 },
		{ 0x361C,	0 },
		{ 0x371E,	0 },
		{ 0x381B,	0 },
		{ 0x3928,	0 },
		{ 0x3A27,	0 },
		{ 0x3B28,	0 },
		{ 0x3C1B,	0 },
		{ 0x3D2F,	0 },
		{ 0x3E1C,	0 },
		{ 0x3F30,	0 },
		{ 0x403D,	0 },
		{ 0x4130,	0 },
	},
	{	/* 280 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3353,	0 },
		{ 0x346E,	0 },
		{ 0x3572,	0 },
		{ 0x361C,	0 },
		{ 0x371F,	0 },
		{ 0x381B,	0 },
		{ 0x3927,	0 },
		{ 0x3A25,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1D,	0 },
		{ 0x3D31,	0 },
		{ 0x3E1D,	0 },
		{ 0x3F31,	0 },
		{ 0x403D,	0 },
		{ 0x4130,	0 },
	},
	{	/* 290 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3354,	0 },
		{ 0x3470,	0 },
		{ 0x3573,	0 },
		{ 0x361C,	0 },
		{ 0x371E,	0 },
		{ 0x381B,	0 },
		{ 0x3927,	0 },
		{ 0x3A26,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1C,	0 },
		{ 0x3D2F,	0 },
		{ 0x3E1C,	0 },
		{ 0x3F30,	0 },
		{ 0x403D,	0 },
		{ 0x4130,	0 },
	},
	{	/* 300 Candela */	
		{ 0x303E,	0 },	
		{ 0x3139,	0 },
		{ 0x323F,	0 },
		{ 0x3355,	0 },
		{ 0x3471,	0 },
		{ 0x3575,	0 },
		{ 0x361D,	0 },
		{ 0x371F,	0 },
		{ 0x381B,	0 },
		{ 0x3926,	0 },
		{ 0x3A25,	0 },
		{ 0x3B27,	0 },
		{ 0x3C1E,	0 },
		{ 0x3D31,	0 },
		{ 0x3E1D,	0 },
		{ 0x3F32,	0 },
		{ 0x403E,	0 },
		{ 0x4132,	0 },
	},
};

static struct setting_table gamma_setting_table_cam[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/*  50 Candela */		
		{ 0x303F,   0 },	
		{ 0x313F,   0 },	
		{ 0x323F,   0 },	
		{ 0x3322,   0 },	
		{ 0x3430,   0 },	
		{ 0x3532,   0 },	
		{ 0x3621,   0 },	
		{ 0x3723,   0 },	
		{ 0x3821,   0 },	
		{ 0x3928,   0 },	
		{ 0x3A26,   0 },	
		{ 0x3B2A,   0 },	
		{ 0x3C13,   0 },	
		{ 0x3D21,   0 },	
		{ 0x3E19,   0 },	
		{ 0x3F27,   0 },	
		{ 0x4025,   0 },	
		{ 0x412E,   0 },       
	},
	{	/* 90 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x332F,   0 },
		{ 0x3440,   0 },
		{ 0x3543,   0 },
		{ 0x3621,   0 },
		{ 0x3723,   0 },
		{ 0x3822,   0 },
		{ 0x392A,   0 },
		{ 0x3A27,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C1F,   0 },
		{ 0x3D2B,   0 },
		{ 0x3E22,   0 },
		{ 0x3F29,   0 },
		{ 0x4028,   0 },
		{ 0x412F,   0 },
	},
	{	/* 100 Candela */	
		{ 0x303F,	0 },	
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3332,	0 },
		{ 0x3444,	0 },
		{ 0x3546,	0 },
		{ 0x3621,	0 },
		{ 0x3722,	0 },
		{ 0x3822,	0 },
		{ 0x392A,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C1F,	0 },
		{ 0x3D2B,	0 },
		{ 0x3E23,	0 },
		{ 0x3F2C,	0 },
		{ 0x402A,	0 },
		{ 0x4132,	0 },
	},
	{	/* 110 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x3334,   0 },
		{ 0x3446,   0 },
		{ 0x3549,   0 },
		{ 0x3621,   0 },
		{ 0x3723,   0 },
		{ 0x3822,   0 },
		{ 0x392B,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C1F,   0 },
		{ 0x3D2C,   0 },
		{ 0x3E23,   0 },
		{ 0x3F2C,   0 },
		{ 0x402B,   0 },
		{ 0x4132,   0 },
	},
	{	/* 120 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x3336,   0 },
		{ 0x3449,   0 },
		{ 0x354C,   0 },
		{ 0x3622,   0 },
		{ 0x3723,   0 },
		{ 0x3821,   0 },
		{ 0x392A,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C1E,   0 },
		{ 0x3D2C,   0 },
		{ 0x3E23,   0 },
		{ 0x3F2E,   0 },
		{ 0x402D,   0 },
		{ 0x4134,   0 },
	},
	{	/* 130 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x3338,   0 },
		{ 0x344C,   0 },
		{ 0x354F,   0 },
		{ 0x3622,   0 },
		{ 0x3722,   0 },
		{ 0x3822,   0 },
		{ 0x392A,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2A,   0 },
		{ 0x3C22,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E25,   0 },
		{ 0x3F31,   0 },
		{ 0x4030,   0 },
		{ 0x4137,   0 },
	},
	{	/* 140 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x333A,   0 },
		{ 0x344F,   0 },
		{ 0x3551,   0 },
		{ 0x3621,   0 },
		{ 0x3722,   0 },
		{ 0x3822,   0 },
		{ 0x392B,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C22,   0 },
		{ 0x3D2E,   0 },
		{ 0x3E24,   0 },
		{ 0x3F31,   0 },
		{ 0x4031,   0 },
		{ 0x4137,   0 },
	},
	{	/* 150 Candela */	
		{ 0x303F,   0 },	
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x333D,   0 },
		{ 0x3451,   0 },
		{ 0x3554,   0 },
		{ 0x3620,   0 },
		{ 0x3722,   0 },
		{ 0x3821,   0 },
		{ 0x392B,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2C,   0 },
		{ 0x3C22,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E24,   0 },
		{ 0x3F32,   0 },
		{ 0x4031,   0 },
		{ 0x4137,   0 },
	},
	{	/* 160 Candela */	
		{ 0x303F,   0 },
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x333F,   0 },
		{ 0x3454,   0 },
		{ 0x3557,   0 },
		{ 0x361F,   0 },
		{ 0x3722,   0 },
		{ 0x3820,   0 },
		{ 0x392B,   0 },
		{ 0x3A27,   0 },
		{ 0x3B2C,   0 },
		{ 0x3C23,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E25,   0 },
		{ 0x3F34,   0 },
		{ 0x4034,   0 },
		{ 0x4139,   0 },
	},
	{	/* 170 Candela */	
		{ 0x303F,   0 },
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x3341,   0 },
		{ 0x3456,   0 },
		{ 0x355A,   0 },
		{ 0x3621,   0 },
		{ 0x3723,   0 },
		{ 0x3821,   0 },
		{ 0x392A,   0 },
		{ 0x3A27,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C23,   0 },
		{ 0x3D30,   0 },
		{ 0x3E25,   0 },
		{ 0x3F34,   0 },
		{ 0x4035,   0 },
		{ 0x4139,   0 },
	},
	{	/* 180 Candela */	
		{ 0x303F,   0 },
		{ 0x313F,   0 },
		{ 0x323F,   0 },
		{ 0x3343,   0 },
		{ 0x3459,   0 },
		{ 0x355D,   0 },
		{ 0x3620,   0 },
		{ 0x3721,   0 },
		{ 0x3820,   0 },
		{ 0x392B,   0 },
		{ 0x3A28,   0 },
		{ 0x3B2B,   0 },
		{ 0x3C23,   0 },
		{ 0x3D2F,   0 },
		{ 0x3E25,   0 },
		{ 0x3F34,   0 },
		{ 0x4034,   0 },
		{ 0x4138,   0 },
	},
	{	/* 190 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3345,	0 },
		{ 0x345C,	0 },
		{ 0x355F,	0 },
		{ 0x361F,	0 },
		{ 0x3721,	0 },
		{ 0x3820,	0 },
		{ 0x392B,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C26,	0 },
		{ 0x3D31,	0 },
		{ 0x3E28,	0 },
		{ 0x3F37,	0 },
		{ 0x4036,	0 },
		{ 0x413B,	0 },
	},
	{	/* 200 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3347,	0 },
		{ 0x345D,	0 },
		{ 0x3561,	0 },
		{ 0x361F,	0 },
		{ 0x3722,	0 },
		{ 0x3820,	0 },
		{ 0x392B,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C24,	0 },
		{ 0x3D2F,	0 },
		{ 0x3E27,	0 },
		{ 0x3F38,	0 },
		{ 0x4038,	0 },
		{ 0x413C,	0 },
	},
	{	/* 210 Candela */	
		{ 0x303F,	0 },	
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3348,	0 },
		{ 0x345F,	0 },
		{ 0x3563,	0 },
		{ 0x3620,	0 },
		{ 0x3722,	0 },
		{ 0x3820,	0 },
		{ 0x392A,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C25,	0 },
		{ 0x3D30,	0 },
		{ 0x3E27,	0 },
		{ 0x3F39,	0 },
		{ 0x4038,	0 },
		{ 0x413C,	0 },
	},
	{	/* 220 Candela */
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3349,	0 },
		{ 0x3461,	0 },
		{ 0x3565,	0 },
		{ 0x3621,	0 },
		{ 0x3722,	0 },
		{ 0x3821,	0 },
		{ 0x392B,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C25,	0 },
		{ 0x3D31,	0 },
		{ 0x3E27,	0 },
		{ 0x3F39,	0 },
		{ 0x4039,	0 },
		{ 0x413D,	0 },
	},
	{	/* 230 Candela */
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x334B,	0 },
		{ 0x3464,	0 },
		{ 0x3567,	0 },
		{ 0x3620,	0 },
		{ 0x3720,	0 },
		{ 0x3820,	0 },
		{ 0x392B,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C24,	0 },
		{ 0x3D31,	0 },
		{ 0x3E27,	0 },
		{ 0x3F37,	0 },
		{ 0x4038,	0 },
		{ 0x413C,	0 },
	},
	{	/* 240 Candela */
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x334C,	0 },
		{ 0x3465,	0 },
		{ 0x3569,	0 },
		{ 0x3621,	0 },
		{ 0x3722,	0 },
		{ 0x3821,	0 },
		{ 0x392B,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C25,	0 },
		{ 0x3D31,	0 },
		{ 0x3E27,	0 },
		{ 0x3F37,	0 },
		{ 0x4039,	0 },
		{ 0x413C,	0 },
	},
	{	/* 250 Candela */
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x334E,	0 },
		{ 0x3468,	0 },
		{ 0x356B,	0 },
		{ 0x3620,	0 },
		{ 0x3720,	0 },
		{ 0x3820,	0 },
		{ 0x392B,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C25,	0 },
		{ 0x3D30,	0 },
		{ 0x3E27,	0 },
		{ 0x3F39,	0 },
		{ 0x403A,	0 },
		{ 0x413D,	0 },
	},
	{	/* 260 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3350,	0 },
		{ 0x346A,	0 },
		{ 0x356E,	0 },
		{ 0x361F,	0 },
		{ 0x3720,	0 },
		{ 0x381F,	0 },
		{ 0x392C,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2C,	0 },
		{ 0x3C25,	0 },
		{ 0x3D2F,	0 },
		{ 0x3E27,	0 },
		{ 0x3F3B,	0 },
		{ 0x403B,	0 },
		{ 0x413E,	0 },
	},
	{	/* 270 Candela */
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3351,	0 },
		{ 0x346C,	0 },
		{ 0x356F,	0 },
		{ 0x361F,	0 },
		{ 0x3721,	0 },
		{ 0x381F,	0 },
		{ 0x392B,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2C,	0 },
		{ 0x3C27,	0 },
		{ 0x3D30,	0 },
		{ 0x3E29,	0 },
		{ 0x3F3B,	0 },
		{ 0x403B,	0 },
		{ 0x413D,	0 },
	},
	{	/* 280 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3353,	0 },
		{ 0x346E,	0 },
		{ 0x3572,	0 },
		{ 0x361F,	0 },
		{ 0x3720,	0 },
		{ 0x381E,	0 },
		{ 0x392A,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2B,	0 },
		{ 0x3C28,	0 },
		{ 0x3D32,	0 },
		{ 0x3E2B,	0 },
		{ 0x3F3B,	0 },
		{ 0x403B,	0 },
		{ 0x413E,	0 },
	},
	{	/* 290 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3354,	0 },
		{ 0x3470,	0 },
		{ 0x3573,	0 },
		{ 0x3620,	0 },
		{ 0x3720,	0 },
		{ 0x381F,	0 },
		{ 0x3929,	0 },
		{ 0x3A27,	0 },
		{ 0x3B2A,	0 },
		{ 0x3C26,	0 },
		{ 0x3D31,	0 },
		{ 0x3E2A,	0 },
		{ 0x3F3B,	0 },
		{ 0x403A,	0 },
		{ 0x413E,	0 },
	},
	{	/* 300 Candela */	
		{ 0x303F,	0 },
		{ 0x313F,	0 },
		{ 0x323F,	0 },
		{ 0x3355,	0 },
		{ 0x3471,	0 },
		{ 0x3575,	0 },
		{ 0x3621,	0 },
		{ 0x3720,	0 },
		{ 0x3820,	0 },
		{ 0x392A,	0 },
		{ 0x3A28,	0 },
		{ 0x3B2A,	0 },
		{ 0x3C24,	0 },
		{ 0x3D30,	0 },
		{ 0x3E28,	0 },
		{ 0x3F3B,	0 },
		{ 0x403B,	0 },
		{ 0x413E,	0 },
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
static s32 old_level = 0;

typedef enum {
	LCD_IDLE = 0,
	LCD_VIDEO,
	LCD_CAMERA
} lcd_gamma_status;

int lcd_gamma_present = 0;

void lcd_gamma_change(int gamma_status)
{
	if(old_level < 1)
		return;
		
	printk("[S3C LCD] %s : level %d, gamma_status %d\n", __FUNCTION__, (old_level-1), gamma_status);
	int i;

	if(gamma_status == LCD_IDLE)
	{
		lcd_gamma_present = LCD_IDLE;
		for (i = 0; i < GAMMA_SETTINGS; i++)
			setting_table_write(&gamma_setting_table[(old_level - 1)][i]);
	}
	else if(gamma_status == LCD_VIDEO)
	{
		lcd_gamma_present = LCD_VIDEO;
		for (i = 0; i < GAMMA_SETTINGS; i++)
			setting_table_write(&gamma_setting_table_video[(old_level - 1)][i]);
	}
	else if(gamma_status == LCD_CAMERA)
	{
		lcd_gamma_present = LCD_CAMERA;
		for (i = 0; i < GAMMA_SETTINGS; i++)
			setting_table_write(&gamma_setting_table_cam[(old_level - 1)][i]);
	}
	else
		return;
}

EXPORT_SYMBOL(lcd_gamma_change);

void lcd_power_ctrl(s32 value)
{
		s32 i;	
		u8 data;
		
		if (value) {
			printk("Lcd power on sequence start\n");
			/* Power On Sequence */
		
			/* Reset Asseert */
			gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
	
			/* Power Enable */
			pmic_read(MAX8698_ID, ONOFF2, &data, 1); 
			data |= (ONOFF2_ELDO6 | ONOFF2_ELDO7);
			pmic_write(MAX8698_ID, ONOFF2, &data, 1); 
	
			msleep(20); 
	
			/* Reset Deasseert */
			gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_HIGH);
	
			msleep(20); 
	
			for (i = 0; i < POWER_ON_SETTINGS; i++)
				setting_table_write(&power_on_setting_table[i]);	

			switch(lcd_gamma_present)
			{
				printk("[S3C LCD] %s : level dimming, lcd_gamma_present %d\n", __FUNCTION__, lcd_gamma_present);

				case LCD_IDLE:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table[1][i]);
					break;
				case LCD_VIDEO:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table_video[1][i]);
					break;
				case LCD_CAMERA:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table_cam[1][i]);
					break;
				default:
					break;
			}
			
			for (i = 0; i < DISPLAY_ON_SETTINGS; i++)
				setting_table_write(&display_on_setting_table[i]);	
			printk("Lcd power on sequence end\n");

}
		else {
			printk("Lcd power off sequence start\n");	
			/* Power Off Sequence */
			for (i = 0; i < DISPLAY_OFF_SETTINGS; i++)
				setting_table_write(&display_off_setting_table[i]);	
			
			for (i = 0; i < POWER_OFF_SETTINGS; i++)
				setting_table_write(&power_off_setting_table[i]);	
	
			/* Reset Assert */
			gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
			
			/* Power Disable */
			pmic_read(MAX8698_ID, ONOFF2, &data, 1); 
			data &= ~(ONOFF2_ELDO6 | ONOFF2_ELDO7);
			pmic_write(MAX8698_ID, ONOFF2, &data, 1); 
			printk("Lcd power off sequence end\n");	
		}
	
		lcd_power = value;
	}



void backlight_ctrl(s32 value)
{
	s32 i, level;
	u8 data;
	int param_lcd_level = value;

	value &= BACKLIGHT_LEVEL_VALUE;

		if (value == 0)
			level = 0;
		else if ((value > 0) && (value < 30))
			level = 1;
		else	
			level = (((value - 30) / 11) + 2); 

	if (level) {	
		
		if (level != old_level) {
			old_level = level;
#if 0
		/* set the new lcd brightness level value to parameter block */
			if (sec_get_param_value)
				sec_get_param_value(__LCD_LEVEL, &param_lcd_level);

			if((param_lcd_level != value) && (value > 30))
			{
				if (sec_set_param_value)
				{
					param_lcd_level = value;
					sec_set_param_value(__LCD_LEVEL, &param_lcd_level);
					printk("[LCD] PARAM new LCD_LEVEL %d\n", value);
					sec_get_param_value(__LCD_LEVEL, &param_lcd_level);
					printk("[LCD] PARAM LCD_LEVEL %d\n", param_lcd_level);
				}
			}
#endif
		/* Power & Backlight On Sequence */
			if (lcd_power == OFF)
			{
				if(!s3cfb_is_clock_on())
				{
					s3cfb_enable_clock_power();
				}

				lcd_power_ctrl(ON);
			}

			printk("LCD Backlight level setting value ==> %d  , level ==> %d \n",value,level);
			
			switch(lcd_gamma_present)
			{
				printk("[S3C LCD] %s : level %d, lcd_gamma_present %d\n", __FUNCTION__, (level-1), lcd_gamma_present);

				case LCD_IDLE:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table[(level - 1)][i]);
					break;
				case LCD_VIDEO:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table_video[(level - 1)][i]);
					break;
				case LCD_CAMERA:
					for (i = 0; i < GAMMA_SETTINGS; i++)
						setting_table_write(&gamma_setting_table_cam[(level - 1)][i]);
					break;
				default:
					break;
			}

		}
	}
	else {
		old_level = level;
		lcd_power_ctrl(OFF);			
	}
}

void backlight_level_ctrl(s32 value)
{
	if ((value < BACKLIGHT_LEVEL_MIN) ||	/* Invalid Value */
		(value > BACKLIGHT_LEVEL_MAX) ||
		(value == backlight_level))	/* Same Value */
		return;

	printk("%s %d\n", __FUNCTION__, __LINE__);

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

static int ams320fs01_open (struct inode *inode, struct file *filp)
{
    printk(KERN_ERR "[S3C_LCD] ams320fs01_open called\n");

    return nonseekable_open(inode, filp);
}

static int ams320fs01_release (struct inode *inode, struct file *filp)
{
    printk(KERN_ERR "[S3C_LCD] ams320fs01_release called\n");

    return 0;
}

static int ams320fs01_ioctl(struct inode *inode, struct file *filp, 
	                        unsigned int ioctl_cmd,  unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;   

	if( _IOC_TYPE(ioctl_cmd) != AMS320FS01_IOC_MAGIC )
	{
		printk("[S3C_LCD] Inappropriate ioctl 1 0x%x\n",ioctl_cmd);
		return -ENOTTY;
	}
	if( _IOC_NR(ioctl_cmd) > AMS320FS01_IOC_NR_MAX )
	{
		printk("[S3C_LCD] Inappropriate ioctl 2 0x%x\n",ioctl_cmd);	
		return -ENOTTY;
	}

	switch (ioctl_cmd)
	{
		case AMS320FS01_IOC_GAMMA22:
			printk(KERN_ERR "[S3C_LCD] Changing gamma to 2.2\n");
			lcd_gamma_change(LCD_IDLE);
			ret = 0;
			break;

		case AMS320FS01_IOC_GAMMA19:
			printk(KERN_ERR "[S3C_LCD] Changing gamma to 1.9\n");
			lcd_gamma_change(LCD_VIDEO);
			ret = 0;
			break;

		case AMS320FS01_IOC_GAMMA17:
			printk(KERN_ERR "[S3C_LCD] Changing gamma to 1.7\n");
			lcd_gamma_change(LCD_CAMERA);
			ret = 0;
			break;

        default:
            printk(KERN_ERR "[S3C_LCD] ioctl default\n");
            ret = -ENOTTY;
            break;
    }

    return ret;
}


static struct file_operations ams320fs01_fops = {
	.owner = THIS_MODULE,
	.open = ams320fs01_open,
	.release = ams320fs01_release,
	.ioctl = ams320fs01_ioctl,
};

static struct miscdevice ams320fs01_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ams320fs01",
	.fops = &ams320fs01_fops,
};

static int ams320fs01_backlight_probe(struct platform_device *pdev)
{
	int ret = 0;
	
	ret = misc_register(&ams320fs01_device);
	
	if (ret) {
		printk(KERN_ERR "[S3C_LCD] misc_register err %d\n", ret);
		return ret;
	}
	
	ret = led_classdev_register(&pdev->dev, &ams320fs01_backlight_led);
	if (ret < 0)
		printk("%s fail\n", __func__);
	return ret;
}

static int ams320fs01_backlight_remove(struct platform_device *pdev)
{
	misc_deregister(&ams320fs01_device);

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
