/*
 * drivers/video/s3c/s3cfb_lte480wv.c
 *
 * $Id: s3cfb_lts222qv.c,v 1.2 2008/11/18 01:50:23 jsgood Exp $
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
#include <linux/gpio.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-lcd.h>

#include "s3cfb.h"

#define S3CFB_SPI_CH		0	/* spi channel for module init */

#if defined(CONFIG_CPU_S5P6440)
#define S3CFB_HFP		6	/* front porch */
#define S3CFB_HSW		3	/* hsync width */
#define S3CFB_HBP		1	/* back porch */

#define S3CFB_VFP		10	/* front porch */
#define S3CFB_VSW		3	/* vsync width */
#define S3CFB_VBP		9	/* back porch */

#else
#define S3CFB_HFP		7	/* front porch */
#define S3CFB_HSW		4	/* hsync width */
#define S3CFB_HBP		2	/* back porch */

#define S3CFB_VFP		11	/* front porch */
#define S3CFB_VSW		4	/* vsync width */
#define S3CFB_VBP		10	/* back porch */
#endif

#define S3CFB_HRES		240	/* horizon pixel  x resolition */
#define S3CFB_VRES		320	/* line cnt       y resolution */

#define S3CFB_HRES_VIRTUAL	240	/* horizon pixel  x resolition */
#define S3CFB_VRES_VIRTUAL	640	/* line cnt       y resolution */

#define S3CFB_HRES_OSD		240	/* horizon pixel  x resolition */
#define S3CFB_VRES_OSD		320	/* line cnt       y resolution */

#if defined(CONFIG_PLAT_S3C24XX)
#define S3CFB_VFRAME_FREQ     	75	/* frame rate freq */
#else
#define S3CFB_VFRAME_FREQ     	60	/* frame rate freq */
#endif

#define S3CFB_PIXEL_CLOCK	(S3CFB_VFRAME_FREQ * (S3CFB_HFP + S3CFB_HSW + S3CFB_HBP + S3CFB_HRES) * (S3CFB_VFP + S3CFB_VSW + S3CFB_VBP + S3CFB_VRES))

static void s3cfb_set_fimd_info(void)
{
	s3cfb_fimd.vidcon1 = S3C_VIDCON1_IHSYNC_INVERT | S3C_VIDCON1_IVSYNC_INVERT | S3C_VIDCON1_IVDEN_NORMAL;
	s3cfb_fimd.vidtcon0 = S3C_VIDTCON0_VBPD(S3CFB_VBP - 1) | S3C_VIDTCON0_VFPD(S3CFB_VFP - 1) | S3C_VIDTCON0_VSPW(S3CFB_VSW - 1);
	s3cfb_fimd.vidtcon1 = S3C_VIDTCON1_HBPD(S3CFB_HBP - 1) | S3C_VIDTCON1_HFPD(S3CFB_HFP - 1) | S3C_VIDTCON1_HSPW(S3CFB_HSW - 1);
	s3cfb_fimd.vidtcon2 = S3C_VIDTCON2_LINEVAL(S3CFB_VRES - 1) | S3C_VIDTCON2_HOZVAL(S3CFB_HRES - 1);

	s3cfb_fimd.vidosd0a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0);
	s3cfb_fimd.vidosd0b = S3C_VIDOSDxB_OSD_RBX_F(S3CFB_HRES - 1) | S3C_VIDOSDxB_OSD_RBY_F(S3CFB_VRES - 1);

	s3cfb_fimd.vidosd1a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0);
	s3cfb_fimd.vidosd1b = S3C_VIDOSDxB_OSD_RBX_F(S3CFB_HRES_OSD - 1) | S3C_VIDOSDxB_OSD_RBY_F(S3CFB_VRES_OSD - 1);

	s3cfb_fimd.width = S3CFB_HRES;
	s3cfb_fimd.height = S3CFB_VRES;
	s3cfb_fimd.xres = S3CFB_HRES;
	s3cfb_fimd.yres = S3CFB_VRES;

#if defined(CONFIG_FB_S3C_VIRTUAL_SCREEN)
	s3cfb_fimd.xres_virtual = S3CFB_HRES_VIRTUAL;
	s3cfb_fimd.yres_virtual = S3CFB_VRES_VIRTUAL;
#else
	s3cfb_fimd.xres_virtual = S3CFB_HRES;
	s3cfb_fimd.yres_virtual = S3CFB_VRES;
#endif

	s3cfb_fimd.osd_width = S3CFB_HRES_OSD;
	s3cfb_fimd.osd_height = S3CFB_VRES_OSD;
	s3cfb_fimd.osd_xres = S3CFB_HRES_OSD;
	s3cfb_fimd.osd_yres = S3CFB_VRES_OSD;

	s3cfb_fimd.osd_xres_virtual = S3CFB_HRES_OSD;
	s3cfb_fimd.osd_yres_virtual = S3CFB_VRES_OSD;

     	s3cfb_fimd.pixclock = S3CFB_PIXEL_CLOCK;

	s3cfb_fimd.hsync_len = S3CFB_HSW;
	s3cfb_fimd.vsync_len = S3CFB_VSW;
	s3cfb_fimd.left_margin = S3CFB_HFP;
	s3cfb_fimd.upper_margin = S3CFB_VFP;
	s3cfb_fimd.right_margin = S3CFB_HBP;
	s3cfb_fimd.lower_margin = S3CFB_VBP;
}

static void s3cfb_spi_write_byte(int data)
{
	unsigned int delay = 50;
	int i;

	s3cfb_spi_lcd_den(S3CFB_SPI_CH, 1);
	s3cfb_spi_lcd_dclk(S3CFB_SPI_CH, 1);
	s3cfb_spi_lcd_dseri(S3CFB_SPI_CH, 1);
	udelay(delay);

	s3cfb_spi_lcd_den(S3CFB_SPI_CH, 0);
	udelay(delay);

	for (i = 7; i >= 0; i--) {
		s3cfb_spi_lcd_dclk(S3CFB_SPI_CH, 0);

		if ((data >> i) & 0x1)
			s3cfb_spi_lcd_dseri(S3CFB_SPI_CH, 1);
		else
			s3cfb_spi_lcd_dseri(S3CFB_SPI_CH, 0);

		udelay(delay);

		s3cfb_spi_lcd_dclk(S3CFB_SPI_CH, 1);
		udelay(delay);
	}

	s3cfb_spi_lcd_den(S3CFB_SPI_CH, 1);
	udelay(delay);
}

static void s3cfb_spi_write(int address, int data)
{
	unsigned int mode = 0x8;

	writel(mode | 0x01, S3C_SIFCCON0);
	writel(mode | 0x03, S3C_SIFCCON0);

	s3cfb_spi_write_byte(address);

	writel(mode | 0x01, S3C_SIFCCON0);
	writel(mode | 0x00, S3C_SIFCCON0);

	udelay(100);

	writel(mode | 0x01, S3C_SIFCCON0);
	writel(mode | 0x03, S3C_SIFCCON0);

	s3cfb_spi_write_byte(data);

	writel(mode | 0x01, S3C_SIFCCON0);
	writel(mode | 0x00, S3C_SIFCCON0);
}

static void s3cfb_init_ldi(void)
{
	unsigned long long endtime;

	s3cfb_spi_write(0x22, 0x01);
	s3cfb_spi_write(0x03, 0x01);

	s3cfb_spi_write(0x00, 0xa0); udelay(5);
	s3cfb_spi_write(0x01, 0x10); udelay(5);
	s3cfb_spi_write(0x02, 0x00); udelay(5);
	s3cfb_spi_write(0x05, 0x00); udelay(5);

	s3cfb_spi_write(0x0d, 0x00);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x0e, 0x00); udelay(5);
	s3cfb_spi_write(0x0f, 0x00); udelay(5);
	s3cfb_spi_write(0x10, 0x00); udelay(5);
	s3cfb_spi_write(0x11, 0x00); udelay(5);
	s3cfb_spi_write(0x12, 0x00); udelay(5);
	s3cfb_spi_write(0x13, 0x00); udelay(5);
	s3cfb_spi_write(0x14, 0x00); udelay(5);
	s3cfb_spi_write(0x15, 0x00); udelay(5);
	s3cfb_spi_write(0x16, 0x00); udelay(5);
	s3cfb_spi_write(0x17, 0x00); udelay(5);
	s3cfb_spi_write(0x34, 0x01); udelay(5);

	s3cfb_spi_write(0x35, 0x00);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x8d, 0x01); udelay(5);
	s3cfb_spi_write(0x8b, 0x28); udelay(5);
	s3cfb_spi_write(0x4b, 0x00); udelay(5);
	s3cfb_spi_write(0x4e, 0x00); udelay(5);
	s3cfb_spi_write(0x4d, 0x00); udelay(5);
	s3cfb_spi_write(0x4e, 0x00); udelay(5);
	s3cfb_spi_write(0x4f, 0x00); udelay(5);

	s3cfb_spi_write(0x50, 0x00);
	endtime = get_jiffies_64() + 5; while(jiffies < endtime);

	s3cfb_spi_write(0x86, 0x00); udelay(5);
	s3cfb_spi_write(0x87, 0x26); udelay(5);
	s3cfb_spi_write(0x88, 0x02); udelay(5);
	s3cfb_spi_write(0x89, 0x05); udelay(5);
	s3cfb_spi_write(0x33, 0x01); udelay(5);

	s3cfb_spi_write(0x37, 0x06);
	endtime = get_jiffies_64() + 5; while(jiffies < endtime);

	s3cfb_spi_write(0x76, 0x00);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x42, 0x00); udelay(5);
	s3cfb_spi_write(0x43, 0x00); udelay(5);
	s3cfb_spi_write(0x44, 0x00); udelay(5);
	s3cfb_spi_write(0x45, 0x00); udelay(5);
	s3cfb_spi_write(0x46, 0xef); udelay(5);
	s3cfb_spi_write(0x47, 0x00); udelay(5);
	s3cfb_spi_write(0x48, 0x00); udelay(5);

	s3cfb_spi_write(0x49, 0x01);
	endtime = get_jiffies_64() + 5; while(jiffies < endtime);

	s3cfb_spi_write(0x4a, 0x3f); udelay(5);
	s3cfb_spi_write(0x3c, 0x00); udelay(5);
	s3cfb_spi_write(0x3d, 0x00); udelay(5);
	s3cfb_spi_write(0x3e, 0x01); udelay(5);
	s3cfb_spi_write(0x3f, 0x3f); udelay(5);
	s3cfb_spi_write(0x40, 0x01); udelay(5);
	s3cfb_spi_write(0x41, 0x0a); udelay(5);

	s3cfb_spi_write(0x8f, 0x3f);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x90, 0x3f); udelay(5);
	s3cfb_spi_write(0x91, 0x33); udelay(5);
	s3cfb_spi_write(0x92, 0x77); udelay(5);
	s3cfb_spi_write(0x93, 0x77); udelay(5);
	s3cfb_spi_write(0x94, 0x17); udelay(5);
	s3cfb_spi_write(0x95, 0x3f); udelay(5);
	s3cfb_spi_write(0x96, 0x00); udelay(5);
	s3cfb_spi_write(0x97, 0x33); udelay(5);
	s3cfb_spi_write(0x98, 0x77); udelay(5);
	s3cfb_spi_write(0x99, 0x77); udelay(5);
	s3cfb_spi_write(0x9a, 0x17); udelay(5);
	s3cfb_spi_write(0x9b, 0x07); udelay(5);
	s3cfb_spi_write(0x9c, 0x07); udelay(5);

	s3cfb_spi_write(0x9d, 0x80);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x1d, 0x08);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x23, 0x00);
	endtime = get_jiffies_64() + 5; while(jiffies < endtime);

	s3cfb_spi_write(0x24, 0x94);
	endtime = get_jiffies_64() + 5; while(jiffies < endtime);

	s3cfb_spi_write(0x25, 0x6f);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x28, 0x1e);
	s3cfb_spi_write(0x1a, 0x00);
	s3cfb_spi_write(0x21, 0x10);
	s3cfb_spi_write(0x18, 0x25);

	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x19, 0x48);
	s3cfb_spi_write(0x18, 0xe5);

	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x18, 0xF7);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x1b, 0x07);
	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x1f, 0x68);
	s3cfb_spi_write(0x20, 0x45);
	s3cfb_spi_write(0x1e, 0xc1);

	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x21, 0x00);
	s3cfb_spi_write(0x3b, 0x01);

	endtime = get_jiffies_64() + 4; while(jiffies < endtime);

	s3cfb_spi_write(0x00, 0x20);
	s3cfb_spi_write(0x02, 0x01);

	endtime = get_jiffies_64() + 4; while(jiffies < endtime);
}

#if defined(CONFIG_CPU_S5P6440)
static void InitStartPosOnLcd(void)
{
	// start addr setting
	s3cfb_spi_write(0x44, 0x00);            // y addr 2
	s3cfb_spi_write(0x42, 0x00);            // x addr
	s3cfb_spi_write(0x43, 0x00);            // y addr 1
}
#endif

static void s3cfb_set_gpio_lts222qv(void)
{
#if defined(CONFIG_CPU_S5P6440) 
	gpio_request(S5P64XX_GPN(1), "GPN");
	gpio_direction_output(S5P64XX_GPN(1), 1);
	gpio_request(S5P64XX_GPN(2), "GPN");
	gpio_direction_output(S5P64XX_GPN(2), 1);
	gpio_request(S5P64XX_GPN(3), "GPN");
	gpio_direction_output(S5P64XX_GPN(3), 1);

	s3c_gpio_setpull(S5P64XX_GPN(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(3), S3C_GPIO_PULL_NONE);
#elif defined(CONFIG_PLAT_S3C64XX)
	gpio_direction_output(S3C64XX_GPC(1), 1);
	gpio_direction_output(S3C64XX_GPC(2), 1);
	gpio_direction_output(S3C64XX_GPC(3), 1);

	s3c_gpio_setpull(S3C64XX_GPC(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S3C64XX_GPC(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S3C64XX_GPC(3), S3C_GPIO_PULL_NONE);
#elif defined(CONFIG_CPU_S5PC100)
	gpio_request(S5PC1XX_GPB(1), "GPB");
	gpio_direction_output(S5PC1XX_GPB(1), 1);
	gpio_request(S5PC1XX_GPB(2), "GPB");
	gpio_direction_output(S5PC1XX_GPB(2), 1);
	gpio_request(S5PC1XX_GPB(3), "GPB");
	gpio_direction_output(S5PC1XX_GPB(3), 1);

	s3c_gpio_setpull(S5PC1XX_GPB(1), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC1XX_GPB(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5PC1XX_GPB(3), S3C_GPIO_PULL_NONE);
#endif
}

void s3cfb_init_hw(void)
{
	printk(KERN_INFO "LCD TYPE :: LTV222QV will be initialized\n");

	s3cfb_set_fimd_info();
	s3cfb_set_gpio();

	if (s3cfb_spi_gpio_request(S3CFB_SPI_CH))
		printk(KERN_ERR "failed to request GPIO for spi-lcd\n");
	else {
		s3cfb_set_gpio_lts222qv();
		s3cfb_init_ldi();
		s3cfb_spi_gpio_free(S3CFB_SPI_CH);
	}
	#if defined(CONFIG_CPU_S5P6440)
	InitStartPosOnLcd();
	#endif
}

