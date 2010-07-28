/* linux/drivers/media/video/samsung/jpeg_v2/jpg_misc.h
 *
 * Driver header file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh,Hyunmin kwak, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#define INT_TIMEOUT			1000

HANDLE create_jpg_mutex(void);
DWORD lock_jpg_mutex(void);
DWORD unlock_jpg_mutex(void);
void delete_jpg_mutex(void);

#ifdef CONFIG_CPU_S5PC100
unsigned int get_fb0_addr(void);
void get_lcd_size(int *width, int *height);
#endif
#endif
