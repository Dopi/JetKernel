/*****************************************************************************/
/*                                                                           */
/* NAME    : Samsung Secondary BootLoader									 */
/* FILE    : lcd_capela.c													 */
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

#include <asm/arch.h>
#include <asm/util.h>
#include <asm/error.h>
#include <asm/main.h>

#define PWR_I2C_FREQ		1		

#define PWR_I2C_SCL_HIGH	s3c_gpio_setpin(GPIO_PWR_I2C_SCL, GPIO_LEVEL_HIGH);
#define PWR_I2C_SCL_LOW		s3c_gpio_setpin(GPIO_PWR_I2C_SCL, GPIO_LEVEL_LOW);
#define PWR_I2C_SDA_HIGH	s3c_gpio_setpin(GPIO_PWR_I2C_SDA, GPIO_LEVEL_HIGH);
#define PWR_I2C_SDA_LOW		s3c_gpio_setpin(GPIO_PWR_I2C_SDA, GPIO_LEVEL_LOW);

static void SCLH_SDAH(u32 delay)
{
	PWR_I2C_SCL_HIGH
	PWR_I2C_SDA_HIGH
	usleep(delay);
}

static void SCLH_SDAL(u32 delay)
{
	PWR_I2C_SCL_HIGH
	PWR_I2C_SDA_LOW
	usleep(delay);
}

static void SCLL_SDAH(u32 delay)
{
	PWR_I2C_SCL_LOW
	PWR_I2C_SDA_HIGH
	usleep(delay);
}

static void SCLL_SDAL(u32 delay)
{
	PWR_I2C_SCL_LOW
	PWR_I2C_SDA_LOW
	usleep(delay);
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
	usleep(delay);
	SCLL_SDAL(delay);
}

static void PWR_I2C_RESTART(u32 delay)
{
	SCLL_SDAH(delay);
	SCLH_SDAH(delay);
	SCLH_SDAL(delay);
	usleep(delay);
	SCLL_SDAL(delay);
}

static void PWR_I2C_END(u32 delay)
{
	SCLL_SDAL(delay);
	SCLH_SDAL(delay);
	usleep(delay);
	SCLH_SDAH(delay);
}

static void PWR_I2C_ACK(u32 delay)
{
	u32 ack;

	PWR_I2C_SCL_LOW
	usleep(delay);

	s3c_gpio_cfgpin(GPIO_PWR_I2C_SDA, 0);
	
	PWR_I2C_SCL_HIGH
	usleep(delay);
	ack = s3c_gpio_getpin(GPIO_PWR_I2C_SDA);
	PWR_I2C_SCL_HIGH
	usleep(delay);

	s3c_gpio_cfgpin(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);
	
	PWR_I2C_SCL_LOW
	usleep(delay);

	while (ack)
		printf("PWR_I2C -> No ACK\n");
}

void pwr_i2c_read(u32 SlaveAddr, u8 WordAddr, u8 *pData)
{
	u32 i;

	*pData = 0;

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
	
	PWR_I2C_RESTART(PWR_I2C_FREQ);
	
	for (i = 8; i > 1; i--) {
		if ((SlaveAddr >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_HIGH(PWR_I2C_FREQ);
	
	PWR_I2C_ACK(PWR_I2C_FREQ);
	
	for (i = 8; i > 0; i--) {
		PWR_I2C_SCL_LOW
		usleep(PWR_I2C_FREQ);
		if (i == 8)
			s3c_gpio_cfgpin(GPIO_PWR_I2C_SDA, 0);
		PWR_I2C_SCL_HIGH
		usleep(PWR_I2C_FREQ);
		*pData |= (!!(s3c_gpio_getpin(GPIO_PWR_I2C_SDA)) << (i - 1));
		PWR_I2C_SCL_HIGH
		usleep(PWR_I2C_FREQ);
		if (i == 1) {	
			PWR_I2C_SDA_HIGH
			s3c_gpio_cfgpin(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);
		}
		PWR_I2C_SCL_LOW
		usleep(PWR_I2C_FREQ);
	}	

	PWR_I2C_HIGH(PWR_I2C_FREQ);

	PWR_I2C_END(PWR_I2C_FREQ);
}

void pwr_i2c_write(u32 SlaveAddr, u8 WordAddr, u8 Data)
{
	u32 i;

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
		if((Data >> (i - 1)) & 0x1)
			PWR_I2C_HIGH(PWR_I2C_FREQ);
		else
			PWR_I2C_LOW(PWR_I2C_FREQ);
	}

	PWR_I2C_ACK(PWR_I2C_FREQ);

	PWR_I2C_END(PWR_I2C_FREQ);
}

void pwr_i2c_init(void)
{
	s3c_gpio_setpin(GPIO_PWR_I2C_SCL, GPIO_LEVEL_HIGH);
	s3c_gpio_cfgpin(GPIO_PWR_I2C_SCL, GPIO_PWR_I2C_SCL_AF);
	s3c_gpio_pullup(GPIO_PWR_I2C_SCL, GPIO_PULL_DISABLE);

	s3c_gpio_setpin(GPIO_PWR_I2C_SDA, GPIO_LEVEL_HIGH);
	s3c_gpio_cfgpin(GPIO_PWR_I2C_SDA, GPIO_PWR_I2C_SDA_AF);
	s3c_gpio_pullup(GPIO_PWR_I2C_SDA, GPIO_PULL_DISABLE);
}
