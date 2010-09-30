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

#include <plat/regs-clock.h>
#include <plat/regs-lcd.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <asm/gpio.h>

#include "s3cfb.h"

#define S3C_FB_HFP   40              		/* front porch */
#define S3C_FB_HSW   128             	/* Hsync width */
#define S3C_FB_HBP   88              		/* Back porch */

#define S3C_FB_VFP   1               		/* front porch */
#define S3C_FB_VSW   3              	 	/* Vsync width */
#define S3C_FB_VBP   21			/* Back porch */ 

#define S3C_FB_HRES		800		/* horizon pixel  x resolition */
#define S3C_FB_VRES		480		/* line cnt       y resolution */

#define S3C_FB_HRES_VIRTUAL	800	/* horizon pixel  x resolition */
#define S3C_FB_VRES_VIRTUAL	960	/* line cnt       y resolution */

#define S3C_FB_HRES_OSD		800	/* horizon pixel  x resolition */
#define S3C_FB_VRES_OSD		480	/* line cnt       y resolution */

#define S3C_FB_VFRAME_FREQ     	60	/* frame rate freq */

#define S3C_FB_PIXEL_CLOCK	(S3C_FB_VFRAME_FREQ * (S3C_FB_HFP + S3C_FB_HSW + S3C_FB_HBP + S3C_FB_HRES) * (S3C_FB_VFP + S3C_FB_VSW + S3C_FB_VBP + S3C_FB_VRES))

static void set_lcd_power (int val)
{
	if(val > 0) {
        		s3cfb_start_lcd();
		gpio_direction_output(S3C64XX_GPM(3), 1);

    	} else {
		gpio_direction_output(S3C64XX_GPM(3), 0);
		s3cfb_stop_lcd();
    	}
}

#define WAITTIME    (10 * HZ / 1000) 
static int old_display_brightness = 20;

static void __set_brightness(int val)
{
#if defined(CONFIG_S3C6410_PWM) 
    	int channel = 1;  
    	int usec = 0;
    	unsigned long tcnt=1001;
    	unsigned long tcmp=0;
	tcmp = val * 10;

	s3c6410_timer_setup (channel, usec, tcnt, tcmp);
#elif defined(CONFIG_S3C_HAVE_PWM)
	/* New PWM API */

#else
    	if (val > 0) {
		gpio_direction_output(S3C64XX_GPF(15), 1);
    	} else {
		gpio_direction_output(S3C64XX_GPF(15), 0);
    	}
#endif
}

static void set_brightness(int val)
{
	int old_val = old_display_brightness;

	if (val < S3CFB_MIN_BRIGHTNESS) 
		val = S3CFB_MIN_BRIGHTNESS;
	if (val > S3CFB_MAX_BRIGHTNESS) 
		val = S3CFB_MAX_BRIGHTNESS;

	if (val >= old_val) {
	    while (old_val <= val) {
		__set_brightness(old_val);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(WAITTIME);
		old_val++;
	    }
	} else {
	    while((--old_val) >= val) {
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
    	if (val > 0)
		__set_brightness(old_display_brightness);
    	else
		__set_brightness(S3CFB_MIN_BRIGHTNESS);
}

static void s3cfb_set_fimd_info(void)
{
	s3cfb_fimd.vidcon1 = S3C_VIDCON1_IHSYNC_INVERT | S3C_VIDCON1_IVSYNC_INVERT | S3C_VIDCON1_IVDEN_NORMAL | S3C_VIDCON1_IVCLK_RISE_EDGE;
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
	s3cfb_fimd.backlight_min = S3CFB_MIN_BRIGHTNESS;
	s3cfb_fimd.backlight_max = S3CFB_MAX_BRIGHTNESS; 
}


static void us_delay(unsigned int dly)
{
    	udelay(dly);
}

#define LCD_SCEN 	S3C64XX_GPM(0)
#define LCD_SCL 		S3C64XX_GPM(1)
#define LCD_SDA 		S3C64XX_GPM(2)
#define S3C_GPIO_OUTP 1
#define S3C_GPIO_INP  0

static void write_bit(unsigned char  val)
{
    	gpio_set_value(LCD_SCL, 0);	    // SCL = 0
    	gpio_set_value(LCD_SDA, val);	    // SDA = val
    	us_delay(1000);
    	gpio_set_value(LCD_SCL, 1);	    // SCL = 1
    	us_delay(1000);
}


static void write_reg(unsigned char addr, unsigned short data)
{
	unsigned char  i;
	s3c_gpio_cfgpin(LCD_SCEN, S3C_GPIO_OUTP);
	s3c_gpio_cfgpin(LCD_SCL, S3C_GPIO_OUTP);
	s3c_gpio_cfgpin(LCD_SDA, S3C_GPIO_OUTP);

	gpio_set_value(LCD_SCEN, 0);    // CSB pin of LCD = 0
	us_delay(500);
	/* transfer the register address bits (4 bit) */
	for(i=0; i<4; i++) {
		if (addr & 0x8)
		    	write_bit(1);
		else
			write_bit(0);
		addr <<= 1;
	    }
	    /* transfer the write mode bit (1 bit) */
	    write_bit(0);
	    /* transfer the data bits (11 bits) */
	    write_bit(0);
	    for (i=0; i<10; i++) {
	        if (data & 0x200) 
	            	write_bit(1);
	        else 
	            	write_bit(0);
	        data <<= 1;
	    }
	    us_delay(500);
	    gpio_set_value(LCD_SCEN, 1);    // CSB pin of LCD = 1
	    us_delay(1000);
}

#if 0
static unsigned int read_bit(void)
{
	unsigned int data;
    	gpio_set_value(LCD_SCL, 0);	    				// SCL = 0
   	data = gpio_get_value(LCD_SDA);		// SDA
    	us_delay(1000);
    	gpio_set_value(LCD_SCL, 1);	    				// SCL = 1
    	us_delay(1000);
    	return data;
}

static unsigned int read_reg(unsigned char addr)
{
    	unsigned char  i;
    	unsigned int data = 0x0;

    	s3c_gpio_cfgpin(LCD_SCEN, S3C_GPIO_OUTP);
    	s3c_gpio_cfgpin(LCD_SCL, S3C_GPIO_OUTP);
    	s3c_gpio_cfgpin(LCD_SDA, S3C_GPIO_OUTP);

    	gpio_set_value(LCD_SCEN, 0);    // CSB pin of LCD = 0
    	us_delay(500);
   	/* transfer the register address bits (4 bit) */
    	for (i=0; i<4; i++) {
		if (addr & 0x8)
	    		write_bit(1);
		else
	    		write_bit(0);
		addr <<= 1;
    	}
    	/* transfer the read or read mode (1 bit) */
    	write_bit(1);
    	/* transfer the data (11 bits) */
    	write_bit(0);
    	s3c_gpio_cfgpin(LCD_SDA, S3C_GPIO_INP);

    	for (i = 0; i < 10; i++) {
		data |= (read_bit() << (9 -i));
    	}
    	us_delay(500);
    	gpio_set_value(LCD_SCEN, 1);    // CSB pin of LCD = 1
    	us_delay(1000);

    	return data;
}
#endif

void lcd_init_hw(void)
{
/*
	reg[0] = 0x 	2d3
	reg[1] = 0x 	16f
	reg[2] = 0x 	 80
	reg[3] = 0x 	  8
	reg[6] = 0x 	1ce
	reg[15] = 0x	   0
*/

    	write_reg(0x0, 0x2d3);
	/* write_reg(0x1, 0x181); */
    	write_reg(0x1, 0x16f);
    	write_reg(0x2, 0x80);
}

void s3cfb_init_hw(void)
{
	printk(KERN_INFO "LCD TYPE :: A070VW04 will be initialized\n");

	s3cfb_set_fimd_info();
	s3cfb_set_gpio();
	lcd_init_hw();
	set_backlight_power(1);

	mdelay(5);
	gpio_direction_output(S3C64XX_GPM(3), 1);

	set_brightness(S3CFB_DEFAULT_BRIGHTNESS);
}

