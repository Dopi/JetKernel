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
#include <mach/gpio.h>

#define BACKLIGHT_STATUS_ALC	0x100
#define BACKLIGHT_LEVEL_VALUE	0x0FF	/* 0 ~ 255 */

#define BACKLIGHT_LEVEL_MIN		0
#define BACKLIGHT_LEVEL_MAX		BACKLIGHT_LEVEL_VALUE

#define BACKLIGHT_LEVEL_DEFAULT	100		/* Default Setting */
//#ifdef CONFIG_FB_S3C_LCD_INIT
//#define CONFIG_FB_S3C_LCD_INIT
//#endif

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

#define S3C_FB_HFP			8 		/* Front Porch */
#define S3C_FB_HSW			1 		/* Hsync Width */
#define S3C_FB_HBP			7 		/* Back Porch */

#define S3C_FB_VFP			8 		/* Front Porch */
#define S3C_FB_VSW			1 		/* Vsync Width */
#define S3C_FB_VBP			7 		/* Back Porch */

#define S3C_FB_HRES             480     /* Horizon pixel Resolition */
#define S3C_FB_VRES             800     /* Vertical pixel Resolution */
#define S3C_FB_HRES_VIRTUAL     S3C_FB_HRES     /* Horizon pixel Resolition */
#define S3C_FB_VRES_VIRTUAL     S3C_FB_VRES * 2 /* Vertial pixel Resolution */

#define S3C_FB_HRES_OSD         480     /* Horizon pixel Resolition */
#define S3C_FB_VRES_OSD         800     /* Vertial pixel Resolution */
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
	/* LCD_SDO */ //????? 
	if (gpio_is_valid(GPIO_LCD_SDO)) {
		if (gpio_request(GPIO_LCD_SDO, S3C_GPIO_LAVEL(GPIO_LCD_SDO))) 
			printk(KERN_ERR "Failed to request GPIO_LCD_SDO!\n");
		gpio_direction_output(GPIO_LCD_SDO, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LCD_SDO, S3C_GPIO_PULL_NONE);
	// LCD_SDI /
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

#define LCD_CS_N_HIGH	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_HIGH); //CSB
#define LCD_CS_N_LOW	gpio_set_value(GPIO_LCD_CS_N, GPIO_LEVEL_LOW);

#define LCD_SCLK_HIGH	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_HIGH); //SCL
#define LCD_SCLK_LOW	gpio_set_value(GPIO_LCD_SCLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_HIGH); //SDI
#define LCD_SDI_LOW	    gpio_set_value(GPIO_LCD_SDI, GPIO_LEVEL_LOW);

#define DEFAULT_UDELAY	5	




static void spi_write(u16 reg_data)
{	
	s32 i;
	u8 ID, ID2,reg_data1, reg_data2;
	reg_data1 = (reg_data >> 8); // last byte
	reg_data2 = reg_data; //firt byte
	ID=0x70;
	ID2=0x72;
/*	LCD_SCLK_HIGH
	udelay(DEFAULT_UDELAY);

	 LCD_CS_N_HIGH
	 udelay(DEFAULT_UDELAY);
	
	LCD_CS_N_LOW
	udelay(DEFAULT_UDELAY);

	 LCD_SCLK_HIGH
	 udelay(DEFAULT_UDELAY);
	
*/
	
	LCD_SCLK_HIGH
	udelay(DEFAULT_UDELAY);
	LCD_CS_N_HIGH
	udelay(DEFAULT_UDELAY);
	LCD_CS_N_LOW
	udelay(DEFAULT_UDELAY);
	 LCD_SCLK_HIGH
	 udelay(DEFAULT_UDELAY);

	for (i = 7; i >= 0; i--) { 
		LCD_SCLK_LOW
		udelay(DEFAULT_UDELAY);

		if ((ID >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_UDELAY);	
		LCD_SCLK_HIGH
		udelay(DEFAULT_UDELAY);
	}
	for (i = 7; i >= 0; i--) { 
		LCD_SCLK_LOW
		udelay(DEFAULT_UDELAY);

		if ((reg_data1 >> i) & 0x1) //only the first byte, L->R
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

	LCD_SCLK_HIGH
	udelay(DEFAULT_UDELAY);
	LCD_CS_N_HIGH
	udelay(DEFAULT_UDELAY);
	LCD_CS_N_LOW
	udelay(DEFAULT_UDELAY);


	for (i = 7; i >= 0; i--) { 
		LCD_SCLK_LOW
		udelay(DEFAULT_UDELAY);

		if ((ID2 >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_UDELAY);
		LCD_SCLK_HIGH
		udelay(DEFAULT_UDELAY);	
	}
	for (i = 7; i >= 0; i--) { 
		LCD_SCLK_LOW
		udelay(DEFAULT_UDELAY);

		if ((reg_data2 >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_UDELAY);

		LCD_SCLK_HIGH
		udelay(DEFAULT_UDELAY);	
	}
	

/*	for (i = 15; i >= 0; i--) { 
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
*/
	
/*	 LCD_SCLK_HIGH
	 udelay(DEFAULT_UDELAY);
	
	 LCD_SDI_HIGH
	 udelay(DEFAULT_UDELAY); 
	
	LCD_CS_N_HIGH
	udelay(DEFAULT_UDELAY);
*/	 

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
   	//{ 0x0300 ,  0 }, original
   	{ 0x1DA0 , 0},
   	{ 0x1403 ,  0 },
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
   	{ 0x1DA0 ,  100 },
   	{ 0x1403 ,  0 },
//   	{ 0x0405 ,  20 },
//    { 0x0407 ,  15 },
#if 0
	{ 0x0405 ,  25 },
	{ 0x0407 ,  15 },
	{ 0x0405 ,  25 },
	{ 0x0407 ,   0 },
#endif
};

#define DISPLAY_ON_SETTINGS	(int)(sizeof(display_on_setting_table)/sizeof(struct setting_table))

static struct setting_table display_off_setting_table[] = {
    { 0x1400, 0 },
    { 0x1DA1, 0 },
//    { 0x0400, 10 },
};
//    { 0x0403, 100 },
//    { 0x0401,  20 },
//    { 0x0400, 10 },
//};


#define DISPLAY_OFF_SETTINGS	(int)(sizeof(display_off_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
//    { 0x0500,   0 },
//	{ 0x0302,	10 },
    { 0x1400, 0 },
    { 0x1DA1,  10 },
};


#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))



#define CAMMA_LEVELS	5//#define CAMMA_LEVELS	23

#define GAMMA_SETTINGS	21 //18

static struct setting_table gamma_setting_table[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/* 50 Candela */
		{ 0x6631,	0 },
		{ 0x6524,	0 },
		{ 0x642B,	0 },
		{ 0x632C,	0 },
		{ 0x623B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5623,	0 },
		{ 0x5527,	0 },
		{ 0x542A,	0 },
		{ 0x5322,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4624,	0 },
		{ 0x4527,	0 },
		{ 0x442D,	0 },
		{ 0x432C,	0 },
		{ 0x423C,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 100 Candela */
		{ 0x663F,	0 },
		{ 0x6521,	0 },
		{ 0x6429,	0 },
		{ 0x6329,	0 },
		{ 0x622F,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x562E,	0 },
		{ 0x5524,	0 },
		{ 0x5429,	0 },
		{ 0x5325,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x462F,	0 },
		{ 0x4524,	0 },
		{ 0x442B,	0 },
		{ 0x432A,	0 },
		{ 0x4230,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 150 Candela */
		{ 0x664a,	0 },
		{ 0x651F,	0 },
		{ 0x6426,	0 },
		{ 0x6328,	0 },
		{ 0x622B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5636,	0 },
		{ 0x5522,	0 },
		{ 0x5428,	0 },
		{ 0x5325,	0 },
		{ 0x520B,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4637,	0 },
		{ 0x4523,	0 },
		{ 0x4428,	0 },
		{ 0x4329,	0 },
		{ 0x422D,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 200 Candela */
		{ 0x6653,	0 },
		{ 0x651d,	0 },
		{ 0x6425,	0 },
		{ 0x6327,	0 },
		{ 0x6228,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x563D,	0 },
		{ 0x5520,	0 },
		{ 0x5427,	0 },
		{ 0x5325,	0 },
		{ 0x5210,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x463E,	0 },
		{ 0x4521,	0 },
		{ 0x4427,	0 },
		{ 0x4329,	0 },
		{ 0x4228,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 250 Candela */
		{ 0x665c,	0 },
		{ 0x651b,	0 },
		{ 0x6424,	0 },
		{ 0x6325,	0 },
		{ 0x622a,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5643,	0 },
		{ 0x551f,	0 },
		{ 0x5426,	0 },
		{ 0x5324,	0 },
		{ 0x5217,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4644,	0 },
		{ 0x451f,	0 },
		{ 0x4427,	0 },
		{ 0x4327,	0 },
		{ 0x422a,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
};


static struct setting_table gamma_setting_table_video[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/* 50 Candela */
		{ 0x6631,	0 },
		{ 0x6524,	0 },
		{ 0x642B,	0 },
		{ 0x632C,	0 },
		{ 0x623B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5623,	0 },
		{ 0x5527,	0 },
		{ 0x542A,	0 },
		{ 0x5322,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4624,	0 },
		{ 0x4527,	0 },
		{ 0x442D,	0 },
		{ 0x432C,	0 },
		{ 0x423C,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 100 Candela */
		{ 0x663F,	0 },
		{ 0x6521,	0 },
		{ 0x6429,	0 },
		{ 0x6329,	0 },
		{ 0x622F,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x562E,	0 },
		{ 0x5524,	0 },
		{ 0x5429,	0 },
		{ 0x5325,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x462F,	0 },
		{ 0x4524,	0 },
		{ 0x442B,	0 },
		{ 0x432A,	0 },
		{ 0x4230,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 150 Candela */
		{ 0x664a,	0 },
		{ 0x651F,	0 },
		{ 0x6426,	0 },
		{ 0x6328,	0 },
		{ 0x622B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5636,	0 },
		{ 0x5522,	0 },
		{ 0x5428,	0 },
		{ 0x5325,	0 },
		{ 0x520B,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4637,	0 },
		{ 0x4523,	0 },
		{ 0x4428,	0 },
		{ 0x4329,	0 },
		{ 0x422D,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 200 Candela */
		{ 0x6653,	0 },
		{ 0x651d,	0 },
		{ 0x6425,	0 },
		{ 0x6327,	0 },
		{ 0x6228,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x563D,	0 },
		{ 0x5520,	0 },
		{ 0x5427,	0 },
		{ 0x5325,	0 },
		{ 0x5210,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x463E,	0 },
		{ 0x4521,	0 },
		{ 0x4427,	0 },
		{ 0x4329,	0 },
		{ 0x4228,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 250 Candela */
		{ 0x665c,	0 },
		{ 0x651b,	0 },
		{ 0x6424,	0 },
		{ 0x6325,	0 },
		{ 0x622a,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5643,	0 },
		{ 0x551f,	0 },
		{ 0x5426,	0 },
		{ 0x5324,	0 },
		{ 0x5217,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4644,	0 },
		{ 0x451f,	0 },
		{ 0x4427,	0 },
		{ 0x4327,	0 },
		{ 0x422a,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
};


static struct setting_table gamma_setting_table_cam[CAMMA_LEVELS][GAMMA_SETTINGS] = {
	{	/* 50 Candela */
		{ 0x6631,	0 },
		{ 0x6524,	0 },
		{ 0x642B,	0 },
		{ 0x632C,	0 },
		{ 0x623B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5623,	0 },
		{ 0x5527,	0 },
		{ 0x542A,	0 },
		{ 0x5322,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4624,	0 },
		{ 0x4527,	0 },
		{ 0x442D,	0 },
		{ 0x432C,	0 },
		{ 0x423C,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 100 Candela */
		{ 0x663F,	0 },
		{ 0x6521,	0 },
		{ 0x6429,	0 },
		{ 0x6329,	0 },
		{ 0x622F,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x562E,	0 },
		{ 0x5524,	0 },
		{ 0x5429,	0 },
		{ 0x5325,	0 },
		{ 0x5200,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x462F,	0 },
		{ 0x4524,	0 },
		{ 0x442B,	0 },
		{ 0x432A,	0 },
		{ 0x4230,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 150 Candela */
		{ 0x664a,	0 },
		{ 0x651F,	0 },
		{ 0x6426,	0 },
		{ 0x6328,	0 },
		{ 0x622B,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5636,	0 },
		{ 0x5522,	0 },
		{ 0x5428,	0 },
		{ 0x5325,	0 },
		{ 0x520B,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4637,	0 },
		{ 0x4523,	0 },
		{ 0x4428,	0 },
		{ 0x4329,	0 },
		{ 0x422D,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 200 Candela */
		{ 0x6653,	0 },
		{ 0x651d,	0 },
		{ 0x6425,	0 },
		{ 0x6327,	0 },
		{ 0x6228,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x563D,	0 },
		{ 0x5520,	0 },
		{ 0x5427,	0 },
		{ 0x5325,	0 },
		{ 0x5210,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x463E,	0 },
		{ 0x4521,	0 },
		{ 0x4427,	0 },
		{ 0x4329,	0 },
		{ 0x4228,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
	{	/* 250 Candela */
		{ 0x665c,	0 },
		{ 0x651b,	0 },
		{ 0x6424,	0 },
		{ 0x6325,	0 },
		{ 0x622a,	0 },
		{ 0x613f,	0 },
		{ 0x6000,	0 },
		{ 0x5643,	0 },
		{ 0x551f,	0 },
		{ 0x5426,	0 },
		{ 0x5324,	0 },
		{ 0x5217,	0 },
		{ 0x5100,	0 },
		{ 0x5000,	0 },
		{ 0x4644,	0 },
		{ 0x451f,	0 },
		{ 0x4427,	0 },
		{ 0x4327,	0 },
		{ 0x422a,	0 },
		{ 0x413f,	0 },
		{ 0x4000,	0 },
	},
};



static void setting_table_write(struct setting_table *table)
{
//	printk("setting table write! \n");
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
	if(old_level < 1){
		printk("OLD level <1: %d \n", old_level);
		return;
	}
		
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
		printk(" LCD power ctrl called \n" );
		if (value) {
			printk("Lcd power on sequence start\n");
			/* Power On Sequence */
		
			/* Reset Asseert */
			gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);
	
			/* Power Enable */
			pmic_read(MAX8698_ID, ONOFF2, &data, 1); 
			data |= (ONOFF2_ELDO6 | ONOFF2_ELDO7);
			printk("Lcd power on writing data: %x\n", data);	
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
			
			//for (i = 0; i < POWER_OFF_SETTINGS; i++)
			//	setting_table_write(&power_off_setting_table[i]);	
		
			/* Reset Assert */
			gpio_set_value(GPIO_LCD_RST_N, GPIO_LEVEL_LOW);

			/* Power Disable */
			pmic_read(MAX8698_ID, ONOFF2, &data, 1); 
			data &= ~(ONOFF2_ELDO6 | ONOFF2_ELDO7);	
			pmic_write(MAX8698_ID, ONOFF2, &data, 1); 

		}
	
		lcd_power = value;
	}





void backlight_ctrl(s32 value)
{
	printk("backlight _ctrl is called !! \n");
	s32 i, level;
	u8 data;
	int param_lcd_level = value;

	value &= BACKLIGHT_LEVEL_VALUE;

		if (value == 0)
			level = 0;
		else if ((value > 0) && (value < 51)) //30
			level = 1;
		else if ((value > 51) && (value < 102)) //30
			level = 2;
		else if ((value > 102) && (value < 153)) //30
			level = 3;
		else if ((value > 153) && (value < 204)) //30
			level = 4;
		else if ((value > 204) && (value < 255)) //30
			level = 5;
		//else	
		//	level = (((value - 51) / 50) );  //level = (((value - 30) / 11) + 2); 

	if (level) {	
		printk(" backlight_ctrl : level:%x, old_level: %x \n", level, old_level); 
		if (level != old_level) {
			old_level = level;

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
	printk("backlight level ctrl called ! \n");
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
	printk("ams320fs01_set_backlight_level");
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
			int i;
			for (i = 0; i < DISPLAY_OFF_SETTINGS; i++)
				setting_table_write(&display_off_setting_table[i]);
			ret = 0;
			break;

		case AMS320FS01_IOC_GAMMA19:
			printk(KERN_ERR "[S3C_LCD] Changing gamma to 1.9\n");
			//lcd_power_ctrl(ON); //TODO ALMAR FIXXXX
			//backlight_level_ctrl(BACKLIGHT_LEVEL_DEFAULT);
			//backlight_power_ctrl(ON); 
			lcd_gamma_change(LCD_VIDEO);
			ret = 0;
			break;

		case AMS320FS01_IOC_GAMMA17:
			printk(KERN_ERR "[S3C_LCD] Changing gamma to 1.7\n");
			lcd_gamma_change(LCD_CAMERA);
			//lcd_power_ctrl(OFF); //TODO ALMAR FIXXXX
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
	printk("s3cfb_init_hw!! \n");
	s3cfb_set_fimd_info();

/*	s3cfb_set_gpio();
#ifdef CONFIG_FB_S3C_LCD_INIT	
	lcd_gpio_init();
	
	backlight_gpio_init();

	lcd_power_ctrl(ON); //-locks up

	backlight_level_ctrl(BACKLIGHT_LEVEL_DEFAULT);

//	backlight_power_ctrl(ON); 
#else */
	//lcd_gpio_init();
	
	//backlight_gpio_init();
	
	lcd_power = ON;

	//backlight_level = BACKLIGHT_LEVEL_DEFAULT;
	backlight_level_ctrl(BACKLIGHT_LEVEL_DEFAULT);

	backlight_power = ON;
//#endif 
}

#define LOGO_MEM_BASE		(0x50000000 + 0x05f00000 - 0x100000)	/* SDRAM_BASE + SRAM_SIZE(208MB) - 1MB */
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
#ifdef CONFIG_FB_S3C_BPP_24
	writel(s3c_fimd.wincon0,    S3C_WINCON0);
#endif
	writel(old_wincon1, S3C_WINCON1);
	
	progress_flag = OFF;
}

void standby_on(void)
{

printk("standby_on \n");
			int i;
			for (i = 0; i < DISPLAY_OFF_SETTINGS; i++)
				setting_table_write(&display_off_setting_table[i]);
};
EXPORT_SYMBOL(standby_on);

void standby_off(void)
{
s32 i;
printk("standby_off \n");
			for (i = 0; i < DISPLAY_ON_SETTINGS; i++)
				setting_table_write(&display_on_setting_table[i]);
};
EXPORT_SYMBOL(standby_off);
