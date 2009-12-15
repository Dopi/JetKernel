/*****************************************************************************/
/*                                                                           */
/* NAME    : Samsung Secondary BootLoader									 */
/* FILE    : s3c_i2c.c														 */
/* PURPOSE : This file implements for Nand Bootloader                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*              COPYRIGHT 2008 SAMSUNG ELECTRONICS CO., LTD.                 */
/*                      ALL RIGHTS RESERVED                                  */
/*                                                                           */
/*   Permission is hereby granted to licensees of Samsung Electronics        */
/*   Co., Ltd. products to use or abstract this computer program for the     */
/*   sole purpose of implementing a product based on Samsung                 */
/*   Electronics Co., Ltd. products. No other rights to reproduce, use,      */
/*   or disseminate this computer program, whether in part or in whole,      */
/*   are granted.                                                            */
/*                                                                           */
/*   Samsung Electronics Co., Ltd. makes no representation or warranties     */
/*   with respect to the performance of this computer program, and           */
/*   specifically disclaims any responsibility for any damages,              */
/*   special or consequential, connected with the use of this program.       */
/*                                                                           */
/*****************************************************************************/

#if 0
#include <asm/arch.h>
#include <asm/command.h>
#include <asm/util.h>
#include <asm/error.h>
#include <asm/main.h>
#endif

#include <asm/delay.h>
#include <linux/kernel.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/gpio.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-h.h>
#include <plat/regs-clock.h>


/*
 * GPIO External I2C Emulation
 */

#define FM_I2C_FREQ		100

#define FM_I2C_SCL_HIGH	gpio_set_value(GPIO_FM_I2C_SCL, GPIO_LEVEL_HIGH);
#define FM_I2C_SCL_LOW		gpio_set_value(GPIO_FM_I2C_SCL, GPIO_LEVEL_LOW);
#define FM_I2C_SDA_HIGH	gpio_set_value(GPIO_FM_I2C_SDA, GPIO_LEVEL_HIGH);
#define FM_I2C_SDA_LOW		gpio_set_value(GPIO_FM_I2C_SDA, GPIO_LEVEL_LOW);

static void SCLH_SDAH(u32 delay)
{
	FM_I2C_SCL_HIGH
	FM_I2C_SDA_HIGH
	udelay(delay);
}

static void SCLH_SDAL(u32 delay)
{
	FM_I2C_SCL_HIGH
	FM_I2C_SDA_LOW
	udelay(delay);
}

static void SCLL_SDAH(u32 delay)
{
	FM_I2C_SCL_LOW
	FM_I2C_SDA_HIGH
	udelay(delay);
}

static void SCLL_SDAL(u32 delay)
{
	FM_I2C_SCL_LOW
	FM_I2C_SDA_LOW
	udelay(delay);
}

static void FM_I2C_LOW(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	SCLH_SDAL(delay);
	SCLL_SDAL(delay);
}

static void FM_I2C_HIGH(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAH(delay);
	SCLL_SDAH(delay);
}

static void FM_I2C_START(u32 delay)
{
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void FM_I2C_RESTART(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void FM_I2C_END(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLH_SDAH(delay);
}

static void FM_I2C_ACK(u32 delay)
{
	u32 ack;

	FM_I2C_SCL_LOW
	udelay(delay);

	gpio_direction_input(GPIO_FM_I2C_SDA);

	FM_I2C_SCL_HIGH
	udelay(delay);

	ack = gpio_get_value(GPIO_FM_I2C_SDA);

	FM_I2C_SCL_HIGH
	udelay(delay);
	
	gpio_direction_output(GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF);

	FM_I2C_SCL_LOW
	udelay(delay);

	if(ack)
		printk("FM_I2C -> No ACK\n");
}

void fm_i2c_read(u32 SlaveAddr, u8 WordAddr, u8 *pData)
{
	u32 i;
	*pData = 0;

	/* Start Conditions*/
	FM_I2C_START(FM_I2C_FREQ);

	/* Write Slave address */
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_LOW(FM_I2C_FREQ);

	FM_I2C_ACK(FM_I2C_FREQ);

	/* Write Command address */
	for (i = 8; i > 0; i--) {
		if ((WordAddr >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_ACK(FM_I2C_FREQ);

	/* Start Conditions*/
	FM_I2C_RESTART(FM_I2C_FREQ);

	/* Write Slave address + Read bit*/
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_HIGH(FM_I2C_FREQ);

	FM_I2C_ACK(FM_I2C_FREQ);

	/* Read Data */
	for (i = 8; i > 0; i--) { 
		FM_I2C_SCL_LOW
		udelay(FM_I2C_FREQ);     
		if (i == 8)
			gpio_direction_input(GPIO_FM_I2C_SDA);
		FM_I2C_SCL_HIGH
		udelay(FM_I2C_FREQ);     

		*pData |= (!!gpio_get_value(GPIO_FM_I2C_SDA)) << (i - 1);

		FM_I2C_SCL_HIGH
		udelay(FM_I2C_FREQ);     
		if (i == 1) {	
			FM_I2C_SDA_HIGH
			gpio_direction_output(GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF);
		}
		FM_I2C_SCL_LOW
		udelay(FM_I2C_FREQ);     
	}

	FM_I2C_HIGH(FM_I2C_FREQ);

	/* End Conditions */
	FM_I2C_END(FM_I2C_FREQ);
}

void fm_i2c_write(u32 SlaveAddr, u8 WordAddr, u8 Data)
{
	u32 i;

	printk("[I2C EMUL] write transfer - Addr : 0x%02x, Data = 0x%02x \n", WordAddr, Data);

	FM_I2C_START(FM_I2C_FREQ);

	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_LOW(FM_I2C_FREQ);

	FM_I2C_ACK(FM_I2C_FREQ);
	
	for (i = 8; i > 0; i--) {
		if ((WordAddr >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_ACK(FM_I2C_FREQ);
	
	for (i = 8; i > 0; i--) {
		if ((Data >> (i - 1)) & 0x1)
			FM_I2C_HIGH(FM_I2C_FREQ);
		else
			FM_I2C_LOW(FM_I2C_FREQ);
	}

	FM_I2C_ACK(FM_I2C_FREQ);

	FM_I2C_END(FM_I2C_FREQ);
}

void fm_i2c_init(void)
{
	/* FM_I2C_SCL */
	if (gpio_is_valid(GPIO_FM_I2C_SCL)) {
		if (gpio_request(GPIO_FM_I2C_SCL, S3C_GPIO_LAVEL(GPIO_FM_I2C_SCL))) 
			printk(KERN_ERR "Failed to request GPIO_FM_I2C_SCL!\n");
		gpio_direction_output(GPIO_FM_I2C_SCL, GPIO_FM_I2C_SCL_AF);
		gpio_set_value(GPIO_FM_I2C_SCL, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_FM_I2C_SCL, S3C_GPIO_PULL_NONE);

	/* FM_I2C_SDA */
	if (gpio_is_valid(GPIO_FM_I2C_SDA)) {
		if (gpio_request(GPIO_FM_I2C_SDA, S3C_GPIO_LAVEL(GPIO_FM_I2C_SDA))) 
			printk(KERN_ERR "Failed to request GPIO_FM_I2C_SDA!\n");
		gpio_direction_output(GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF);
		gpio_set_value(GPIO_FM_I2C_SDA, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_FM_I2C_SDA, S3C_GPIO_PULL_NONE);
}
