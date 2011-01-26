/* jpeg/JPGMisc.h
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
 
#ifndef __JPG_MISC_H__
#define __JPG_MISC_H__

#include <linux/types.h>

typedef	unsigned char	UCHAR;
typedef unsigned long	ULONG;
typedef	unsigned int	UINT;
typedef struct mutex *	HANDLE;
typedef unsigned long	DWORD;
typedef unsigned int	UINT32;
typedef unsigned char	UINT8;
typedef enum {FALSE, TRUE} BOOL;

HANDLE CreateJPGmutex(void);
DWORD LockJPGMutex(void);
DWORD UnlockJPGMutex(void);
void DeleteJPGMutex(void);
unsigned int get_fb0_addr(void);
void get_lcd_size(int *width, int *height);
void WaitForInterrupt(void);
void WaitForDecInterrupt(void);

#endif
