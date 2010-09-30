/*
 * drivers/video/s3c/s3cfb_a070vw04.c
 *
 * $Id: s3cfb_lte480wv.c,v 1.12 2008/06/05 02:13:24 jsgood Exp $
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

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-clock.h>
#include <plat/regs-lcd.h>
#include <plat/regs-gpio.h>

#include <mach/map.h>
#include "s3cfb.h"

#define S3C_FB_HFP   40              /* front porch */
#define S3C_FB_HSW   1		      /* Hsync width */
#define S3C_FB_HBP   216              /* Back porch */

#define S3C_FB_VFP   10               /* front porch */
#define S3C_FB_VSW   1               /* Vsync width */
#define S3C_FB_VBP   35		     /* Back porch */

#define S3C_FB_HRES		800	/* horizon pixel  x resolition */
#define S3C_FB_VRES		480	/* line cnt       y resolution */

#define S3C_FB_HRES_VIRTUAL	800	/* horizon pixel  x resolition */
#define S3C_FB_VRES_VIRTUAL	960	/* line cnt       y resolution */

#define S3C_FB_HRES_OSD		800	/* horizon pixel  x resolition */
#define S3C_FB_VRES_OSD		480	/* line cnt       y resolution */

#define S3C_FB_VFRAME_FREQ     	60	/* frame rate freq */

#define S3C_FB_PIXEL_CLOCK	(S3C_FB_VFRAME_FREQ * (S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * (S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void set_lcd_power(int val)
{
    if(val > 0) {
	/* Display On */
        s3cfb_start_lcd();
    } else {
	/* Direct Off */
	s3cfb_stop_lcd();
    }
}

#define WAITTIME    (10 * HZ / 1000)    // 10ms
static int old_display_brightness = S3CFB_DEFAULT_BACKLIGHT_LEVEL;

static void __set_brightness(int val)
{
    int channel = 1;  // must use channel-1
    int usec = 0;       // don't care value
    unsigned long tcnt=1000;
    unsigned long tcmp=0;
	printk("%s@%d:: val = %d\n", __func__, __LINE__, val);

    if(val == S3CFB_MAX_BRIGHTNESS)
	gpio_direction_output(S3C64XX_GPF(15), 1);
    else if(val == 0)
	gpio_direction_output(S3C64XX_GPF(15), 0);
    else {
	tcmp = val * 10;
#if defined(CONFIG_S3C6410_PWM) && defined(CONFIG_PWM)
	s3c6410_timer_setup (channel, usec, tcnt, tcmp);
#else
	gpio_direction_output(S3C64XX_GPF(15), 1);
#endif
    }
}

static void set_brightness(int val)
{
	int old_val = old_display_brightness;

	if(val < 0) val = 0;
	if(val > S3CFB_MAX_BRIGHTNESS) val = S3CFB_MAX_BRIGHTNESS;

	if(val > old_val) {
	    while((++old_val) < val) {
		__set_brightness(old_val);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(WAITTIME);
	    }
	} else {
	    while((--old_val) > val) {
		__set_brightness(old_val);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(WAITTIME);
	    }
	}

	__set_brightness(val);
	old_display_brightness = val;
}

static void set_backlight_power(int val)
{
	printk("%s@%d:: val = %d\n", __func__, __LINE__, val);
    if(val > 0)
	gpio_direction_output(S3C64XX_GPM(3), 1);	
    else
	gpio_direction_output(S3C64XX_GPM(3), 0);
}

static void s3cfb_set_fimd_info(void)
{
	s3cfb_fimd.vidcon1 = S3C_VIDCON1_IHSYNC_INVERT | S3C_VIDCON1_IVSYNC_INVERT |S3C_VIDCON1_IVDEN_INVERT;
	s3cfb_fimd.vidtcon0 = S3C_VIDTCON0_VBPD(S3C_FB_VBP - 1) | S3C_VIDTCON0_VFPD(S3C_FB_VFP - 1) | S3C_VIDTCON0_VSPW(S3C_FB_VSW - 1);
	s3cfb_fimd.vidtcon1 = S3C_VIDTCON1_HBPD(S3C_FB_HBP - 1) | S3C_VIDTCON1_HFPD(S3C_FB_HFP - 1) | S3C_VIDTCON1_HSPW(S3C_FB_HSW - 1);
	s3cfb_fimd.vidtcon2 = S3C_VIDTCON2_LINEVAL(S3C_FB_VRES - 1) | S3C_VIDTCON2_HOZVAL(S3C_FB_HRES - 1);

	s3cfb_fimd.vidosd0a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0);
	s3cfb_fimd.vidosd0b = S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES - 1) | S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES - 1);

	s3cfb_fimd.vidosd1a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0);
	s3cfb_fimd.vidosd1b = S3C_VIDOSDxB_OSD_RBX_F(S3C_FB_HRES_OSD - 1) | S3C_VIDOSDxB_OSD_RBY_F(S3C_FB_VRES_OSD - 1);

	s3cfb_fimd.width = S3C_FB_HRES;
	s3cfb_fimd.height = S3C_FB_VRES;
	s3cfb_fimd.xres = S3C_FB_HRES;
	s3cfb_fimd.yres = S3C_FB_VRES;

#if defined(CONFIG_FB_S3C_VIRTUAL_SCREEN)
	s3cfb_fimd.xres_virtual = S3C_FB_HRES_VIRTUAL;
	s3cfb_fimd.yres_virtual = S3C_FB_VRES_VIRTUAL;
#else
	s3cfb_fimd.xres_virtual = S3C_FB_HRES;
	s3cfb_fimd.yres_virtual = S3C_FB_VRES;
#endif

	s3cfb_fimd.osd_width = S3C_FB_HRES_OSD;
	s3cfb_fimd.osd_height = S3C_FB_VRES_OSD;
	s3cfb_fimd.osd_xres = S3C_FB_HRES_OSD;
	s3cfb_fimd.osd_yres = S3C_FB_VRES_OSD;

	s3cfb_fimd.osd_xres_virtual = S3C_FB_HRES_OSD;
	s3cfb_fimd.osd_yres_virtual = S3C_FB_VRES_OSD;

	s3cfb_fimd.pixclock = S3C_FB_PIXEL_CLOCK;

	s3cfb_fimd.hsync_len = S3C_FB_HSW;
	s3cfb_fimd.vsync_len = S3C_FB_VSW;
	s3cfb_fimd.left_margin = S3C_FB_HFP;
	s3cfb_fimd.upper_margin = S3C_FB_VFP;
	s3cfb_fimd.right_margin = S3C_FB_HBP;
	s3cfb_fimd.lower_margin = S3C_FB_VBP;

	s3cfb_fimd.set_lcd_power = set_lcd_power;
	s3cfb_fimd.set_backlight_power = set_backlight_power;
	s3cfb_fimd.set_brightness = set_brightness;
	s3cfb_fimd.backlight_min = S3CFB_DEFAULT_BRIGHTNESS;
	s3cfb_fimd.backlight_max = S3CFB_MAX_BRIGHTNESS;
}

#define LCD_SCEN S3C64XX_GPM(0)
#define LCD_SCL S3C64XX_GPM(1)
#define LCD_SDA S3C64XX_GPM(2)
#define S3C_GPIO_OUTP 1
#define S3C_GPIO_INP  0

static int lcd_write(unsigned char,unsigned char);
static void lcd_spi_stop(void);
static void lcd_spi_start(void);
static void lcd_spi_start(void)
{
	//set scen output 0
	s3c_gpio_cfgpin(LCD_SCEN,S3C_GPIO_OUTP);
	s3c_gpio_setpin(LCD_SCEN,0);
}

static void lcd_spi_stop(void)
{
	//set scen output 1
	s3c_gpio_cfgpin(LCD_SCEN,S3C_GPIO_OUTP);
	s3c_gpio_setpin(LCD_SCEN,1);
	s3c_gpio_setpin(LCD_SCL,0);
	s3c_gpio_setpin(LCD_SDA,0);
	s3c_gpio_cfgpin(LCD_SCL,S3C_GPIO_INP);
	s3c_gpio_cfgpin(LCD_SDA,S3C_GPIO_INP);
}
static int lcd_write(unsigned char addr,unsigned char data)
{
	unsigned char myaddr,mydata,i;
	myaddr = (addr&0x3f)<<1 ;
	myaddr <<= 1;
	myaddr |= 0x1;
	lcd_spi_start();

	s3c_gpio_cfgpin(LCD_SCL,S3C_GPIO_OUTP);
	s3c_gpio_setpin(LCD_SCL,0);
	s3c_gpio_cfgpin(LCD_SDA,S3C_GPIO_OUTP);
	for(i=0;i<8;i++){
	    s3c_gpio_setpin(LCD_SCL,0);
	    udelay(1);
	    s3c_gpio_setpin(LCD_SDA,(myaddr&0x80)>>7);
	    myaddr <<= 1 ;
	    udelay(1);
	    s3c_gpio_setpin(LCD_SCL,1);
	    udelay(1);
	} //8nd is null turn
	mydata = data;
	for(i=0;i<8;i++){
	    s3c_gpio_setpin(LCD_SCL,0);
	    udelay(1);
	    s3c_gpio_setpin(LCD_SDA,(mydata&0x80)>>7);
	    mydata <<= 1;
	    udelay(1);
	    s3c_gpio_setpin(LCD_SCL,1);
	    udelay(1);
	}

	lcd_spi_stop();

	return 0;
}


void lcd_init_hw(void)
{
#if 1
	lcd_spi_stop();

	lcd_write(0x02,0x07);
	lcd_write(0x03,0x5f);
	lcd_write(0x04,0x17);

	lcd_write(0x05,0x20);
	lcd_write(0x06,0x08);

	lcd_write(0x07,0x20);
	lcd_write(0x08,0x20);
	lcd_write(0x09,0x20);
	lcd_write(0x0a,0x20);

	lcd_write(0x0b,0x20);
	lcd_write(0x0c,0x20);
	lcd_write(0x0d,0x22);

	lcd_write(0x0e,0x10);
	lcd_write(0x0f,0x10);
	lcd_write(0x10,0x10);

	lcd_write(0x11,0x15);
	lcd_write(0x12,0xaa);
	lcd_write(0x13,0xff);
	lcd_write(0x14,0x86);
	lcd_write(0x15,0x89);
	lcd_write(0x16,0xc6);
	lcd_write(0x17,0xea);
	lcd_write(0x18,0x0c);
	lcd_write(0x19,0x33);
	lcd_write(0x1a,0x5e);
	lcd_write(0x1b,0xd0);
	lcd_write(0x1c,0x33);
	lcd_write(0x1d,0x7e);
	lcd_write(0x1e,0xb3);
	lcd_write(0x1f,0xff);
	lcd_write(0x20,0xf0);
	lcd_write(0x21,0xf0);
	lcd_write(0x22,0x08);
#endif
}

void s3cfb_init_hw(void)
{
	printk(KERN_INFO "LCD TYPE :: TD043MTEX will be initialized\n");

	s3cfb_set_fimd_info();
	s3cfb_set_gpio();
	lcd_init_hw();
}

