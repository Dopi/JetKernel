/* jpeg/JPGMisc.c
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C JPEG driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include <asm/io.h>
#include <plat/regs-lcd.h>

#include "JPGMisc.h"
#include "JPGMem.h"

static HANDLE hMutex	= NULL;
extern wait_queue_head_t	WaitQueue_JPEG;

/*----------------------------------------------------------------------------
*Function: CreateJPGmutex
*Implementation Notes: Create Mutex handle 
-----------------------------------------------------------------------------*/
HANDLE CreateJPGmutex(void)
{
	hMutex = (HANDLE)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (hMutex == NULL)
		return NULL;
	
	mutex_init(hMutex);
	
	return hMutex;
}

/*----------------------------------------------------------------------------
*Function: LockJPGMutex
*Implementation Notes: lock mutex 
-----------------------------------------------------------------------------*/
DWORD LockJPGMutex(void)
{
    mutex_lock(hMutex);  
    return 1;
}

/*----------------------------------------------------------------------------
*Function: UnlockJPGMutex
*Implementation Notes: unlock mutex
-----------------------------------------------------------------------------*/
DWORD UnlockJPGMutex(void)
{
	mutex_unlock(hMutex);
	
    return 1;
}

/*----------------------------------------------------------------------------
*Function: DeleteJPGMutex
*Implementation Notes: delete mutex handle 
-----------------------------------------------------------------------------*/
void DeleteJPGMutex(void)
{
	if (hMutex == NULL)
		return;

	mutex_destroy(hMutex);
}

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

void WaitForInterrupt(void)
{
	interruptible_sleep_on_timeout(&WaitQueue_JPEG, 100);
}

