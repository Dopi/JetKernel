/*
 *  linux/include/asm-arm/arch-s3c6400/jet.h
 *
 *  Author:	dopi711@googlemail.com
 *  Created:	29, Jun, 2010
 *  Copyright: 	JetDroid project (http://code.google.com/p/jetdroid)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef ASM_MACH_JET_H

#define ASM_MACH_JET_H

#include <mach/gpio.h>

/* 
 * Board Configuration
 */

#ifdef CONFIG_MACH_INSTINCTQ

#define CONFIG_INSTINCTQ_REV00			0x00	/* REV00 */
#define CONFIG_INSTINCTQ_REV01			0x01	/* REV01 */

#ifdef CONFIG_BOARD_REVISION
#define CONFIG_INSTINCTQ_REV			CONFIG_BOARD_REVISION
#else
#error	"Board revision is not defined!"
#endif /* CONFIG_BOARD_REVISION */

#else /* CONFIG_MACH_INSTINCTQ */

#define CONFIG_JET_REV00			0x00	/* REV00 */
#define CONFIG_JET_REV01			0x01	/* REV01 */

#ifdef CONFIG_BOARD_REVISION
#define CONFIG_JET_REV			CONFIG_BOARD_REVISION
#else
#error	"Board revision is not defined!"
#endif /* CONFIG_BOARD_REVISION */

#endif /* CONFIG_MACH_INSTINCTQ */

#include "jet_gpio.h"

#define I2C_CAM_DIS do {	\
	s3c_gpio_cfgpin(GPIO_I2C1_SCL, S3C_GPIO_INPUT);			\
	s3c_gpio_cfgpin(GPIO_I2C1_SDA, S3C_GPIO_INPUT);			\
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_DOWN);		\
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_DOWN);		\
} while (0)

#define I2C_CAM_EN do {		\
	s3c_gpio_cfgpin(GPIO_I2C1_SCL, S3C_GPIO_SFN(GPIO_I2C1_SCL_AF));	\
	s3c_gpio_cfgpin(GPIO_I2C1_SDA, S3C_GPIO_SFN(GPIO_I2C1_SDA_AF));	\
	s3c_gpio_setpull(GPIO_I2C1_SCL, S3C_GPIO_PULL_NONE);		\
	s3c_gpio_setpull(GPIO_I2C1_SDA, S3C_GPIO_PULL_NONE);		\
} while (0)

#define AF_PWR_DIS do {} while (0)
#define AF_PWR_EN do {} while (0)

#define	MCAM_RST_DIS do {	\
	/* MCAM RST Low */	\
	if (gpio_is_valid(GPIO_MCAM_RST_N)) {	\
		gpio_direction_output(GPIO_MCAM_RST_N, GPIO_LEVEL_LOW);	\
	}	\
	s3c_gpio_setpull(GPIO_MCAM_RST_N, S3C_GPIO_PULL_NONE);	\
} while (0)

#define	MCAM_RST_EN do {	\
	gpio_direction_output(GPIO_MCAM_RST_N, GPIO_LEVEL_HIGH);	\
} while (0)

#define	VCAM_RST_DIS do { } while (0)
#define	VCAM_RST_EN do { } while (0)

#define	CAM_PWR_DIS do {	\
	/* CAM PWR Low */	\
	if (gpio_is_valid(GPIO_CAM_EN)) {	\
		gpio_direction_output(GPIO_CAM_EN, GPIO_LEVEL_LOW);	\
	}	\
	s3c_gpio_setpull(GPIO_CAM_EN, S3C_GPIO_PULL_NONE);	\
} while (0)

#define	CAM_PWR_EN do {	\
	gpio_direction_output(GPIO_CAM_EN, GPIO_LEVEL_HIGH);	\
} while (0)

#define	MCAM_STB_EN do {	\
	/* MCAM STB High */	\
	gpio_set_value(GPIO_CAM_STANDBY, GPIO_LEVEL_HIGH);	\
} while (0)

#define	MCAM_STB_DIS do {	\
	/* MCAM STB Low */	\
	if (gpio_is_valid(GPIO_CAM_STANDBY)) {	\
		gpio_direction_output(GPIO_CAM_STANDBY, GPIO_LEVEL_LOW);	\
	}	\
	s3c_gpio_setpull(GPIO_CAM_STANDBY, S3C_GPIO_PULL_NONE);	\
} while (0)

#define	VCAM_STB_DIS do { } while (0)

#define CAM_MEM_SIZE	0x0D000000
#define S5K4CA_ID	0x78 

#endif	/* ASM_MACH_JET_H */

