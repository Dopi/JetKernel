/* linux/arch/arm/plat-s3c/include/plat/devs.h
 *
 * Copyright (c) 2004 Simtec Electronics
 * Ben Dooks <ben@simtec.co.uk>
 *
 * Header file for s3c2410 standard platform devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/platform_device.h>

struct s3c_uart_resources {
	struct resource		*resources;
	unsigned long		 nr_resources;
};

extern struct s3c_uart_resources s3c64xx_uart_resources[];

extern struct platform_device *s3c_uart_devs[];
extern struct platform_device *s3c_uart_src[];

extern struct platform_device s3c_device_timer[];

#if defined(CONFIG_S3C_DMA_PL080_SOL)
extern struct platform_device s3c_device_dma0;
extern struct platform_device s3c_device_dma1;
extern struct platform_device s3c_device_dma2;
extern struct platform_device s3c_device_dma3;
#endif
extern struct platform_device s3c_device_usb;
extern struct platform_device s3c_device_lcd;
extern struct platform_device s3c_device_wdt;
extern struct platform_device s3c_device_i2c0;
extern struct platform_device s3c_device_i2c1;
extern struct platform_device s3c_device_iis;
extern struct platform_device s3c_device_rtc;
extern struct platform_device s3c_device_adcts;
extern struct platform_device s3c_device_adc;
extern struct platform_device s3c_device_sdi;
extern struct platform_device s3c_device_hsmmc0;
extern struct platform_device s3c_device_hsmmc1;
extern struct platform_device s3c_device_hsmmc2;
extern struct platform_device s3c_device_spi0;
extern struct platform_device s3c_device_spi1;
extern struct platform_device s3c_device_nand;
extern struct platform_device s3c_device_keypad;
extern struct platform_device s3c_device_usbgadget;
extern struct platform_device s3c_device_ts;
extern struct platform_device s3c_device_smc911x;
extern struct platform_device s3c_device_2d;
extern struct platform_device s3c_device_camif;
extern struct platform_device s3c_device_mfc;
extern struct platform_device s3c_device_g3d;
extern struct platform_device s3c_device_rotator;
extern struct platform_device s3c_device_jpeg;
extern struct platform_device s3c_device_vpp;

#ifdef CONFIG_S3C6410_TVOUT
extern struct platform_device s3c_device_tvenc;
extern struct platform_device s3c_device_tvscaler;
#endif

