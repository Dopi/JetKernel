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

#define PWR_I2C_FREQ		100

#define PWR_I2C_SCL_HIGH		gpio_set_value(GPIO_PWR_I2C_SCL, GPIO_LEVEL_HIGH);
#define PWR_I2C_SCL_LOW		gpio_set_value(GPIO_PWR_I2C_SCL, GPIO_LEVEL_LOW);
#define PWR_I2C_SDA_HIGH		gpio_set_value(GPIO_PWR_I2C_SDA, GPIO_LEVEL_HIGH);
#define PWR_I2C_SDA_LOW		gpio_set_value(GPIO_PWR_I2C_SDA, GPIO_LEVEL_LOW);

static void SCLH_SDAH(u32 delay)
{
	PWR_I2C_SCL_HIGH
	PWR_I2C_SDA_HIGH
	udelay(delay);
}

static void SCLH_SDAL(u32 delay)
{
	PWR_I2C_SCL_HIGH
	PWR_I2C_SDA_LOW
	udelay(delay);
}

static void SCLL_SDAH(u32 delay)
{
	PWR_I2C_SCL_LOW
	PWR_I2C_SDA_HIGH
	udelay(delay);
}

static void SCLL_SDAL(u32 delay)
{
	PWR_I2C_SCL_LOW
	PWR_I2C_SDA_LOW
	udelay(delay);
}

static void PWR_I2C_LOW(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	SCLH_SDAL(delay);
	SCLL_SDAL(delay);
}

static void PWR_I2C_HIGH(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAH(delay);
	SCLL_SDAH(delay);
}

static void PWR_I2C_START(u32 delay)
{
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void PWR_I2C_RESTART(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLL_SDAL(delay);
}

static void PWR_I2C_END(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	udelay(delay);
	SCLH_SDAH(delay);
}

static void PWR_I2C_ACK(u32 delay)
{
	u32 ack;

	PWR_I2C_SCL_LOW
	udelay(delay);

	gpio_direction_input(GPIO_PWR_I2C_SDA);

	PWR_I2C_SCL_HIGH
	udelay(delay);

	ack = gpio_get_value(GPIO_PWR_I2C_SDA);

	PWR_I2C_SCL_HIGH
	udelay(delay);
	
	gpio_direction_output(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);

	PWR_I2C_SCL_LOW
	udelay(delay);

	if(ack)
		printk("PWR_I2C -> No ACK\n");

}

void pwr_i2c_read(u32 SlaveAddr, u8 WordAddr, u8 *pData)
{
	u32 i;
	*pData = 0;

	/* Start Conditions*/
	PWR_I2C_START(PWR_I2C_FREQ);

	/* Write Slave address */
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_LOW(PWR_I2C_FREQ);

	PWR_I2C_ACK(PWR_I2C_FREQ);

	/* Write Command address */
	for (i = 8; i > 0; i--) {
		if ((WordAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_ACK(PWR_I2C_FREQ);

	/* Start Conditions*/
	PWR_I2C_RESTART(PWR_I2C_FREQ);

	/* Write Slave address + Read bit*/
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_HIGH(PWR_I2C_FREQ);

	PWR_I2C_ACK(PWR_I2C_FREQ);

	/* Read Data */

	for (i = 8; i > 0; i--) { 
		PWR_I2C_SCL_LOW
		udelay(PWR_I2C_FREQ);     
		if (i == 8)
			gpio_direction_input(GPIO_PWR_I2C_SDA);	
		PWR_I2C_SCL_HIGH
		udelay(PWR_I2C_FREQ);     

		*pData |= (!!gpio_get_value(GPIO_PWR_I2C_SDA)) << (i - 1);

		PWR_I2C_SCL_HIGH
		udelay(PWR_I2C_FREQ);     
		if (i == 1) {	
			PWR_I2C_SDA_HIGH
			gpio_direction_output(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);
		}
		PWR_I2C_SCL_LOW
		udelay(PWR_I2C_FREQ);     
	}

	PWR_I2C_HIGH(PWR_I2C_FREQ);

	/* End Conditions */
	PWR_I2C_END(PWR_I2C_FREQ);
}

void pwr_i2c_write(u32 SlaveAddr, u8 WordAddr, u8 Data)
{
	u32 i;

	printk("[I2C EMUL] write transfer - Addr : 0x%02x, Data = 0x%02x \n", WordAddr, Data);

	PWR_I2C_START(PWR_I2C_FREQ);

	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_LOW(PWR_I2C_FREQ);

	PWR_I2C_ACK(PWR_I2C_FREQ);
	
	for (i = 8; i > 0; i--) {
		if ((WordAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_ACK(PWR_I2C_FREQ);
	
	for (i = 8; i > 0; i--) {
		if ((Data >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_ACK(PWR_I2C_FREQ);

	PWR_I2C_END(PWR_I2C_FREQ);
}

void pwr_i2c_init(void)
{
	/* PWR_I2C_SCL */
	if (gpio_is_valid(GPIO_PWR_I2C_SCL)) {
		if (gpio_request(GPIO_PWR_I2C_SCL, S3C_GPIO_LAVEL(GPIO_PWR_I2C_SCL))) 
			printk(KERN_ERR "Failed to request GPIO_PWR_I2C_SCL!\n");
		gpio_direction_output(GPIO_PWR_I2C_SCL, GPIO_PWR_I2C_SCL_AF);
		gpio_set_value(GPIO_PWR_I2C_SCL, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_PWR_I2C_SCL, S3C_GPIO_PULL_NONE);

	/* PWR_I2C_SDA */
	if (gpio_is_valid(GPIO_PWR_I2C_SDA)) {
		if (gpio_request(GPIO_PWR_I2C_SDA, S3C_GPIO_LAVEL(GPIO_PWR_I2C_SDA))) 
			printk(KERN_ERR "Failed to request GPIO_PWR_I2C_SDA!\n");
		gpio_direction_output(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);
		gpio_set_value(GPIO_PWR_I2C_SDA, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_PWR_I2C_SDA, S3C_GPIO_PULL_NONE);
}
