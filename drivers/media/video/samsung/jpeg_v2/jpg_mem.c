/* linux/drivers/media/video/samsung/jpeg_v2/jpg_mem.c
 *
 * Driver file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh, Hyunmin kwak, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/types.h>

#include "jpg_mem.h"
#include "jpg_misc.h"
#include "log_msg.h"
#include "jpg_opr.h"

/*----------------------------------------------------------------------------
*Function: phy_to_vir_addr

*Parameters: 		dwContext		:
*Return Value:		True/False
*Implementation Notes: memory mapping from physical addr to virtual addr
-----------------------------------------------------------------------------*/
void *phy_to_vir_addr(UINT32 phy_addr, int mem_size)
{
	void	*reserved_mem;

	reserved_mem = (void *)ioremap((unsigned long)phy_addr, (int)mem_size);

	if (reserved_mem == NULL) {
		log_msg(LOG_ERROR, "phy_to_vir_addr", "DD::Phyical to virtual memory mapping was failed!\r\n");
		return NULL;
	}

	return reserved_mem;
}

void *mem_move(void *dst, const void *src, unsigned int size)
{
	return memmove(dst, src, size);
}

void *mem_alloc(unsigned int size)
{
	void	*alloc_mem;

	alloc_mem = (void *)kmalloc((int)size, GFP_KERNEL);

	if (alloc_mem == NULL) {
		log_msg(LOG_ERROR, "Mem_Alloc", "memory allocation failed!\r\n");
		return NULL;
	}

	return alloc_mem;
}
