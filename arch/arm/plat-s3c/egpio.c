/*****************************************************************************/
/*                                                                           */
/* NAME    : GPIO EXPENDER control ROUTINE									 */
/* FILE    : egpio.c														 */
/* PURPOSE : control expand pin levels using i2c with bit-banging algo.      */
/* DEVICE  : TCA6416                                                         */
/*           linux/arch/arm/plat-s3c/gpio-ext.c                              */
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>
#include <plat/egpio.h>
#include <mach/map.h>

int egpio_set_value(unsigned int pin, unsigned int level)
{
	return 0;
}
EXPORT_SYMBOL(egpio_set_value);

int egpio_get_value(unsigned int pin)
{
	return 0;
}
EXPORT_SYMBOL(egpio_get_value);
