/* mfc/MfcMemory.c
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C MFC driver
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

#include <asm/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/uaccess.h>

#include <linux/version.h>
#include <plat/regs-lcd.h>

#include "MfcConfig.h"
#include "LogMsg.h"


void *Phy2Vir_AddrMapping(unsigned int phy_addr, int mem_size)
{
	void	*reserved_mem;

        // from 2.8.5
	if (phy_addr == S3C6400_BASEADDR_MFC_DATA_BUF) {
		//reserved_mem = (void *)ioremap_cached( (unsigned long)phy_addr, (int)mem_size );
		reserved_mem = (void *)ioremap( (unsigned long)phy_addr, (int)mem_size );
	} else {
		//reserved_mem = (void *)ioremap_nocache( (unsigned long)phy_addr, (int)mem_size );
		reserved_mem = (void *)ioremap( (unsigned long)phy_addr, (int)mem_size );
	}
	
	if (reserved_mem == NULL) {
		LOG_MSG(LOG_ERROR, "Phy2Vir_AddrMapping", "For IOPreg: VirtualAlloc failed!\r\n");
		return NULL;
	}

	
	return reserved_mem;
}


void *Mem_Alloc(unsigned int size)
{
	void	*alloc_mem;

	alloc_mem = (void *)kmalloc(size, GFP_KERNEL);
	if (alloc_mem == NULL) {
		LOG_MSG(LOG_ERROR, "Mem_Alloc", "memory allocation failed!\r\n");
		return NULL;
	}

	return alloc_mem;
}

void Mem_Free(void *addr)
{
	kfree(addr);
}

void *Mem_Cpy(void *dst, const void *src, int size)
{
	return memcpy(dst, src, size);
}

void *Mem_Set(void *target, int val, int size)
{
	return memset(target, val, size);
}

int Copy_From_User(void *to, const void *from, unsigned long n)
{
	return copy_from_user(to, from, n);
}

int Copy_To_User(void *to, const void *from, unsigned long n)
{
	return copy_to_user(to, from, n);
}

