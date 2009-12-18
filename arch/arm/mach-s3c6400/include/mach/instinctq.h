/*
 *  linux/include/asm-arm/arch-s3c2410/instinctq.h
 *
 *  Author:		Samsung Electronics
 *  Created:	05, Jul, 2007
 *  Copyright:	Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef ASM_MACH_INSTINCTQ_H

#define ASM_MACH_INSTINCTQ_H

#include <mach/gpio.h>

/* 
 * Board Configuration
 */

#define CONFIG_INSTINCTQ_REV00			0x00	/* REV00 */
#define CONFIG_INSTINCTQ_REV01			0x01	/* REV01 */

#ifdef CONFIG_BOARD_REVISION
#define CONFIG_INSTINCTQ_REV			CONFIG_BOARD_REVISION
#else
#error	"Board revision is not defined!"
#endif

#include "instinctq_gpio.h"

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

#define	LCD_18V_OFF do {  \
		if (Get_MAX8698_PM_REG(ELDO6, &onoff_lcd_18)) {  \
			pr_info("%s: LCD 1.8V off(%d)\n", __func__, onoff_lcd_18);  \
			if (onoff_lcd_18) \
				Set_MAX8698_PM_REG(ELDO6, 0);  \
		}  \
}	while (0)
#define	LCD_28V_OFF do {  \
		if (Get_MAX8698_PM_REG(ELDO7, &onoff_lcd_28)) {  \
			pr_info("%s: LCD 2.8V off(%d)\n", __func__, onoff_lcd_28);  \
			if (onoff_lcd_28) \
				Set_MAX8698_PM_REG(ELDO7, 0);  \
		}  \
}	while (0)
#define	LCD_30V_OFF do {}	while (0)

#define	LCD_18V_ON do {  \
		if (onoff_lcd_18) {  \
			pr_info("%s: LCD 1.8V On\n", __func__);  \
			Set_MAX8698_PM_REG(ELDO6, 1);  \
		}  \
}	while (0)
#define	LCD_28V_ON do {  \
		if (onoff_lcd_28) {  \
		pr_info("%s: LCD 2.8V On\n", __func__);  \
		Set_MAX8698_PM_REG(ELDO7, 1);  \
	}  \
}while (0)

#define	LCD_30V_ON do {}	while (0)

#endif	/* ASM_MACH_INSTINCTQ_H */

