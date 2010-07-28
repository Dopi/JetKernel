/* linux/drivers/media/video/samsung/s3c_csis.h
 *
 * Header file for Samsung MIPI-CSI2 driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3C_CSIS_H
#define _S3C_CSIS_H

#define S3C_CSIS_NAME		"s3c-csis"
#define S3C_CSIS_NR_LANES	1

#define info(args...)	do { printk(KERN_INFO S3C_CSIS_NAME ": " args); } while (0)
#define err(args...)	do { printk(KERN_ERR  S3C_CSIS_NAME ": " args); } while (0)

enum mipi_format {
	MIPI_CSI_YCBCR422_8BIT	= 0x1e,
	MIPI_CSI_RAW8		= 0x2a,
	MIPI_CSI_RAW10		= 0x2b,
	MIPI_CSI_RAW12		= 0x2c,
};

struct s3c_csis_info {
	char 		name[16];
	struct device	*dev;
	struct clk	*clock;	
	void __iomem	*regs;
	int		irq;
	int		nr_lanes;
};

#endif /* _S3C_CSIS_H */
