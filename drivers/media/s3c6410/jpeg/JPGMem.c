/* jpeg/JPGMem.c
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
 
#include <asm/io.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/types.h>

#include "JPGMem.h"
#include "JPGMisc.h"
#include "LogMsg.h"


/*----------------------------------------------------------------------------
*Function: Phy2VirAddr

*Parameters: 		dwContext		:
*Return Value:		True/False
*Implementation Notes: memory mapping from physical addr to virtual addr 
-----------------------------------------------------------------------------*/
void *Phy2VirAddr(UINT32 phy_addr, int mem_size)
{
	void	*reserved_mem;

	reserved_mem = (void *)ioremap( (unsigned long)phy_addr, (int)mem_size );		

	if (reserved_mem == NULL) {
		JPEG_LOG_MSG(LOG_ERROR, "Phy2VirAddr", "DD::Phyical to virtual memory mapping was failed!\r\n");
		return NULL;
	}

	return reserved_mem;
}

/*----------------------------------------------------------------------------
*Function: JPGMemMapping

*Parameters: 		dwContext		:
*Return Value:		True/False
*Implementation Notes: JPG register mapping from physical addr to virtual addr 
-----------------------------------------------------------------------------*/
BOOL JPGMemMapping(S3C6400_JPG_CTX *base)
{
	// JPG HOST Register
	base->v_pJPG_REG = (volatile S3C6400_JPG_HOSTIF_REG *)Phy2VirAddr(JPG_REG_BASE_ADDR, sizeof(S3C6400_JPG_HOSTIF_REG));
	if (base->v_pJPG_REG == NULL)
	{
		JPEG_LOG_MSG(LOG_ERROR, "JPGMemMapping", "DD::v_pJPG_REG: VirtualAlloc failed!\r\n");
		return FALSE;
	}
	
	return TRUE;
}


void JPGMemFree(S3C6400_JPG_CTX *base)
{
	iounmap((void *)base->v_pJPG_REG);
	base->v_pJPG_REG = NULL;
}

/*----------------------------------------------------------------------------
*Function: JPGBuffMapping

*Parameters: 		dwContext		:
*Return Value:		True/False
*Implementation Notes: JPG Buffer mapping from physical addr to virtual addr 
-----------------------------------------------------------------------------*/
/*
BOOL JPGBuffMapping(S3C6400_JPG_CTX *base)
{
	// JPG Data Buffer
	base->v_pJPGData_Buff = (UINT8 *)Phy2VirAddr(JPG_DATA_BASE_ADDR, JPG_TOTAL_BUF_SIZE);

	if (base->v_pJPGData_Buff == NULL)
	{
		JPEG_LOG_MSG(LOG_ERROR, "JPGBuffMapping", "DD::v_pJPGData_Buff: VirtualAlloc failed!\r\n");
		return FALSE;
	}

	return TRUE;
}

void JPGBuffFree(S3C6400_JPG_CTX *base)
{
	iounmap( (void *)base->v_pJPGData_Buff );
	base->v_pJPGData_Buff = NULL;
}
*/

void *MemMove(void *dst, const void *src, unsigned int size)
{
	return memmove(dst, src, size);
}

void *MemAlloc(unsigned int size)
{
	void	*alloc_mem;

	alloc_mem = (void *)kmalloc((int)size, GFP_KERNEL);
	if (alloc_mem == NULL) {
		JPEG_LOG_MSG(LOG_ERROR, "Mem_Alloc", "memory allocation failed!\r\n");
		return NULL;
	}

	return alloc_mem;
}

int JPEG_Copy_From_User(void *to, const void *from, unsigned long n)
{
	return copy_from_user(to, from, n);
}

int JPEG_Copy_To_User(void *to, const void *from, unsigned long n)
{
	return copy_to_user(to, from, n);
}

