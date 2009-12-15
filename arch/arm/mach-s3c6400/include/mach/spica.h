/*
 *  linux/include/asm-arm/arch-s3c2410/spica.h
 *
 *  Author:		Samsung Electronics
 *  Created:	05, Jul, 2007
 *  Copyright:	Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef ASM_MACH_SPICA_H

#define ASM_MACH_SPICA_H

/* 
 * Board Configuration
 */

#define CONFIG_SPICA_REV00			0x80	/* REV00 */
#define CONFIG_SPICA_REV01			0x81	/* REV01 */
#define CONFIG_SPICA_REV02			0x82	/* REV02 */
#define CONFIG_SPICA_REV03			0x83	/* REV03 */
#define CONFIG_SPICA_REV04			0x84	/* REV04 */
#define CONFIG_SPICA_REV05			0x85	/* REV05 */
#define CONFIG_SPICA_REV06			0x86	/* REV06 */
#define CONFIG_SPICA_REV07			0x87	/* REV07 */
#define CONFIG_SPICA_REV08			0x88	/* REV08 */
#define CONFIG_SPICA_REV09			0x89	/* REV09 */

#define CONFIG_SPICA_TEST_REV00	0x00	
#define CONFIG_SPICA_TEST_REV01	0x01	
#define CONFIG_SPICA_TEST_REV02	0x02	
#define CONFIG_SPICA_TEST_REV03	0x03	

#ifdef CONFIG_BOARD_REVISION
#define CONFIG_SPICA_REV			CONFIG_BOARD_REVISION
#else
#error	"Board revision is not defined!"
#endif

#if (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV00)
#include "spica_rev00.h"
#elif (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV01)
#include "spica_rev01.h"
#elif (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
#include "spica_rev02.h"
#else
#error	"Board revision is not valid!"
#endif

/*
 * Partition Information
 */

#define BOOT_PART_ID			0x0
#define CSA_PART_ID				0xF		/* Not Used */
#define SBL_PART_ID				0x1
#define PARAM_PART_ID			0x2
#define KERNEL_PART_ID			0x3
#define RAMDISK_PART_ID			0xF		/* Not Used */
#define FILESYSTEM_PART_ID		0x4
#define FILESYSTEM1_PART_ID		0x5
#define FILESYSTEM2_PART_ID		0x6
#define TEMP_PART_ID			0x7		/* Temp Area for FOTA */
#define MODEM_IMG_PART_ID		0x8
#define MODEM_EFS_PART_ID		0x9		/* Modem EFS Area for OneDRAM */

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
	gpio_set_value(GPIO_CAM_3M_STBY_N, GPIO_LEVEL_HIGH);	\
} while (0)	

#define	MCAM_STB_DIS do {	\
	/* CAM_3M STB Low */	\
	if (gpio_is_valid(GPIO_CAM_3M_STBY_N)) {	\
		gpio_direction_output(GPIO_CAM_3M_STBY_N, GPIO_LEVEL_LOW);	\
	}	\
	s3c_gpio_setpull(GPIO_CAM_3M_STBY_N, S3C_GPIO_PULL_NONE);	\
} while (0)

#define	VCAM_STB_DIS do { } while (0)

#define CAM_MEM_SIZE	0x0D000000

#if (CONFIG_SPICA_REV == CONFIG_SPICA_TEST_REV02)
#define S5K4CA_ID	0x78
#else
#define S5K4CA_ID	0x5A
#endif

#define	LCD_18V_OFF do {  \
		if (Get_MAX8698_PM_REG(ELDO6, &onoff_lcd_18)) {  \
			pr_info("%s: LCD 1.8V off(%d)\n", __func__, onoff_lcd_18);  \
			if (onoff_lcd_18) \
				Set_MAX8698_PM_REG(ELDO6, 0);  \
		}  \
}	while (0)
#define	LCD_28V_OFF do {} while (0)
#define	LCD_30V_OFF do {  \
		if (Get_MAX8698_PM_REG(ELDO7, &onoff_lcd_30)) {  \
			pr_info("%s: LCD 3.0V off(%d)\n", __func__, onoff_lcd_30);  \
			if (onoff_lcd_30) \
				Set_MAX8698_PM_REG(ELDO7, 0);  \
		}  \
	}	while (0)

#define	LCD_18V_ON do {  \
		if (!onoff_lcd_18) {  \
			pr_info("%s: LCD 1.8V On\n", __func__);  \
			Set_MAX8698_PM_REG(ELDO6, 1);  \
		}  \
}	while (0)
#define	LCD_28V_ON do {} while (0)

#define	LCD_30V_ON do {  \
		if (!onoff_lcd_30) {  \
		pr_info("%s: LCD 3.0V On\n", __func__);  \
		Set_MAX8698_PM_REG(ELDO7, 1);  \
	}  \
}	while (0)

#endif	/* ASM_MACH_SPICA_H */
