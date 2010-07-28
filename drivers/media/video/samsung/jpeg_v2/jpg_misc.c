/* linux/drivers/media/video/samsung/jpeg_v2/jpg_misc.c
 *
 * Driver file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh,Hyunmin kwak, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/version.h>
#include <plat/regs-lcd.h>

#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "jpg_misc.h"
#include "jpg_mem.h"

static HANDLE h_mutex	= NULL;

/*----------------------------------------------------------------------------
*Function: create_jpg_mutex
*Implementation Notes: Create Mutex handle
-----------------------------------------------------------------------------*/
HANDLE create_jpg_mutex(void)
{
	h_mutex = (HANDLE)kmalloc(sizeof(struct mutex), GFP_KERNEL);

	if (h_mutex == NULL)
		return NULL;

	mutex_init(h_mutex);

	return h_mutex;
}

/*----------------------------------------------------------------------------
*Function: lock_jpg_mutex
*Implementation Notes: lock mutex
-----------------------------------------------------------------------------*/
DWORD lock_jpg_mutex(void)
{
	mutex_lock(h_mutex);
	return 1;
}

/*----------------------------------------------------------------------------
*Function: unlock_jpg_mutex
*Implementation Notes: unlock mutex
-----------------------------------------------------------------------------*/
DWORD unlock_jpg_mutex(void)
{
	mutex_unlock(h_mutex);

	return 1;
}

/*----------------------------------------------------------------------------
*Function: delete_jpg_mutex
*Implementation Notes: delete mutex handle
-----------------------------------------------------------------------------*/
void delete_jpg_mutex(void)
{
	if (h_mutex == NULL)
		return;

	mutex_destroy(h_mutex);
}
#ifdef CONFIG_CPU_S5PC100
unsigned int get_fb0_addr(void)
{
	return readl(S3C_VIDW00ADD0B0);
}

void get_lcd_size(int *width, int *height)
{
	unsigned int	tmp;

	tmp		= readl(S3C_VIDTCON2);
	*height	= ((tmp >> 11) & 0x7FF) + 1;
	*width	= (tmp & 0x7FF) + 1;
}
#endif




