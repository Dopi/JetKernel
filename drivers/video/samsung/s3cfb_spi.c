/*
 * drivers/video/s3c/s3c24xxfb_spi.c
 *
 * $Id: s3cfb_spi.c,v 1.1 2008/11/17 11:12:08 jsgood Exp $
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

#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-clock.h>
#include <plat/regs-lcd.h>

#if 0 //defined(CONFIG_PLAT_S3C24XX)

#define S3CFB_SPI_CLK(x)	(S3C2443_GPL10 + (ch * 0))
#define S3CFB_SPI_MOSI(x)	(S3C2443_GPL11 + (ch * 0))
#define S3CFB_SPI_CS(x)	(S3C2443_GPL14 + (ch * 0))

static inline void s3cfb_spi_lcd_dclk(int ch, int value)
{
	s3c2410_gpio_setpin(S3CFB_SPI_CLK(ch), value);
}

static inline void s3cfb_spi_lcd_dseri(int ch, int value)
{
	s3c2410_gpio_setpin(S3CFB_SPI_MOSI(ch), value);
}

static inline void s3cfb_spi_lcd_den(int ch, int value)
{
	s3c2410_gpio_setpin(S3CFB_SPI_CS(ch), value);
}

static inline void s3cfb_spi_set_lcd_data(int ch)
{
	s3c2410_gpio_cfgpin(S3CFB_SPI_CLK(ch), 1);
	s3c2410_gpio_cfgpin(S3CFB_SPI_MOSI(ch), 1);
	s3c2410_gpio_cfgpin(S3CFB_SPI_CS(ch), 1);

	s3c2410_gpio_pullup(S3CFB_SPI_CLK(ch), 2);
	s3c2410_gpio_pullup(S3CFB_SPI_MOSI(ch), 2);
	s3c2410_gpio_pullup(S3CFB_SPI_CS(ch), 2);
}

#elif defined(CONFIG_PLAT_S3C64XX) || defined(CONFIG_PLAT_S5P64XX)

#if defined(CONFIG_PLAT_S5P64XX)
#define S3CFB_SPI_CLK(x)	(S5P64XX_GPN(2 + (x * 4)))
#define S3CFB_SPI_MOSI(x)	(S5P64XX_GPN(3 + (x * 4)))
#define S3CFB_SPI_CS(x)		(S5P64XX_GPN(1 + (x * 4)))

int s3cfb_spi_gpio_request(int ch)
{
	int err = 0;

	if (gpio_is_valid(S3CFB_SPI_CLK(ch))) {
		err = gpio_request(S3CFB_SPI_CLK(ch), "GPN");

		if (err)
			goto err_clk;
	} else {
		err = 1;
		goto err_clk;
	}

	if (gpio_is_valid(S3CFB_SPI_MOSI(ch))) {
		err = gpio_request(S3CFB_SPI_MOSI(ch), "GPN");

		if (err)
			goto err_mosi;
	} else {
		err = 1;
		goto err_mosi;
	}

	if (gpio_is_valid(S3CFB_SPI_CS(ch))) {
		err = gpio_request(S3CFB_SPI_CS(ch), "GPN");

		if (err)
			goto err_cs;
	} else {
		err = 1;
		goto err_cs;
	}

err_cs:
	gpio_free(S3CFB_SPI_MOSI(ch));

err_mosi:
	gpio_free(S3CFB_SPI_CLK(ch));

err_clk:
	return err;

}

#elif defined(CONFIG_PLAT_S3C64XX)
#define S3CFB_SPI_CLK(x)	(S3C64XX_GPC(1 + (x * 4)))
#define S3CFB_SPI_MOSI(x)	(S3C64XX_GPC(2 + (x * 4)))
#define S3CFB_SPI_CS(x)		(S3C64XX_GPC(3 + (x * 4)))

int s3cfb_spi_gpio_request(int ch)
{
	int err = 0;

	if (gpio_is_valid(S3CFB_SPI_CLK(ch))) {
		err = gpio_request(S3CFB_SPI_CLK(ch), "GPC");

		if (err)
			goto err_clk;
	} else {
		err = 1;
		goto err_clk;
	}

	if (gpio_is_valid(S3CFB_SPI_MOSI(ch))) {
		err = gpio_request(S3CFB_SPI_MOSI(ch), "GPC");

		if (err)
			goto err_mosi;
	} else {
		err = 1;
		goto err_mosi;
	}

	if (gpio_is_valid(S3CFB_SPI_CS(ch))) {
		err = gpio_request(S3CFB_SPI_CS(ch), "GPC");

		if (err)
			goto err_cs;
	} else {
		err = 1;
		goto err_cs;
	}

err_cs:
	gpio_free(S3CFB_SPI_MOSI(ch));

err_mosi:
	gpio_free(S3CFB_SPI_CLK(ch));

err_clk:
	return err;

}
#endif

inline void s3cfb_spi_lcd_dclk(int ch, int value)
{
	gpio_set_value(S3CFB_SPI_CLK(ch), value);
}

inline void s3cfb_spi_lcd_dseri(int ch, int value)
{
	gpio_set_value(S3CFB_SPI_MOSI(ch), value);
}

inline void s3cfb_spi_lcd_den(int ch, int value)
{
	gpio_set_value(S3CFB_SPI_CS(ch), value);
}

inline void s3cfb_spi_set_lcd_data(int ch)
{
	gpio_direction_output(S3CFB_SPI_CLK(ch), 1);
	gpio_direction_output(S3CFB_SPI_MOSI(ch), 1);
	gpio_direction_output(S3CFB_SPI_CS(ch), 1);

	s3c_gpio_setpull(S3CFB_SPI_CLK(ch), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S3CFB_SPI_MOSI(ch), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S3CFB_SPI_CS(ch), S3C_GPIO_PULL_NONE);
}

void s3cfb_spi_gpio_free(int ch)
{
	gpio_free(S3CFB_SPI_CLK(ch));
	gpio_free(S3CFB_SPI_MOSI(ch));
	gpio_free(S3CFB_SPI_CS(ch));
}

#elif defined(CONFIG_PLAT_S5PC1XX)

#define S5P_FB_SPI_CLK(x)	(S5PC1XX_GPB(1 + (x * 4)))
#define S5P_FB_SPI_MOSI(x)	(S5PC1XX_GPB(2 + (x * 4)))
#define S5P_FB_SPI_CS(x)	(S5PC1XX_GPB(3 + (x * 4)))

int s3cfb_spi_gpio_request(int ch)
{
        int err = 0;

        if (gpio_is_valid(S5P_FB_SPI_CLK(ch))) {
                err = gpio_request(S5P_FB_SPI_CLK(ch), "GPB");

                if (err)
                        goto err_clk;
        } else {
                err = 1;
                goto err_clk;
        }

        if (gpio_is_valid(S5P_FB_SPI_MOSI(ch))) {
                err = gpio_request(S5P_FB_SPI_MOSI(ch), "GPB");

                if (err)
                        goto err_mosi;
        } else {
                err = 1;
                goto err_mosi;
        }

        if (gpio_is_valid(S5P_FB_SPI_CS(ch))) {
                err = gpio_request(S5P_FB_SPI_CS(ch), "GPB");

                if (err)
                        goto err_cs;
        } else {
                err = 1;
                goto err_cs;
        }

err_cs:
        gpio_free(S5P_FB_SPI_MOSI(ch));

err_mosi:
        gpio_free(S5P_FB_SPI_CLK(ch));

err_clk:
        return err;

}

inline void s3cfb_spi_lcd_dclk(int ch, int value)
{
	gpio_set_value(S5P_FB_SPI_CLK(ch), value);
}

inline void s3cfb_spi_lcd_dseri(int ch, int value)
{
	gpio_set_value(S5P_FB_SPI_MOSI(ch), value);
}

inline void s3cfb_spi_lcd_den(int ch, int value)
{
	gpio_set_value(S5P_FB_SPI_CS(ch), value);
}

inline void s3cfb_spi_set_lcd_data(int ch)
{
	gpio_direction_output(S5P_FB_SPI_CLK(ch), 1);
	gpio_direction_output(S5P_FB_SPI_MOSI(ch), 1);
	gpio_direction_output(S5P_FB_SPI_CS(ch), 1);

	s3c_gpio_setpull(S5P_FB_SPI_CLK(ch), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P_FB_SPI_MOSI(ch), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P_FB_SPI_CS(ch), S3C_GPIO_PULL_NONE);
}

void s3cfb_spi_gpio_free(int ch)
{
        gpio_free(S5P_FB_SPI_CLK(ch));
        gpio_free(S5P_FB_SPI_MOSI(ch));
        gpio_free(S5P_FB_SPI_CS(ch));
}

#endif

