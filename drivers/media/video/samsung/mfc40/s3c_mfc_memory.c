/*
 * drivers/media/video/samsung/mfc40/s3c_mfc_memory.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/sizes.h>
#include <asm/memory.h>

#include "s3c_mfc_memory.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_types.h"
#include "s3c_mfc_interface.h"

extern volatile unsigned char	*s3c_mfc_virt_fw_buf;
extern void __iomem		*s3c_mfc_sfr_virt_base;

volatile unsigned char *s3c_mfc_get_fw_buf_virt_addr()
{
	return (volatile unsigned char *)s3c_mfc_virt_fw_buf;      
}

volatile unsigned char *s3c_mfc_get_vsp_buf_virt_addr(int instNo)	
{
	volatile unsigned char *virAddr;

	virAddr = s3c_mfc_virt_fw_buf + MFC_MAX_FW_NUM*FIRMWARE_CODE_SIZE + instNo*VSP_BUF_SIZE;
	return virAddr; 
}

unsigned int s3c_mfc_get_sfr_phys_addr()
{
	return (unsigned int)S5PC1XX_PA_MFC;
}

unsigned int s3c_mfc_get_fw_buf_phys_addr()
{
	return (unsigned int)__virt_to_phys((unsigned int)s3c_mfc_virt_fw_buf); /* IMAGE_MFC_BUFFER_PA_START; */
}

unsigned int s3c_mfc_get_vsp_buf_phys_addr(int instNo)
{
	unsigned int phyAddr;

	phyAddr = s3c_mfc_get_fw_buf_phys_addr() + MFC_MAX_FW_NUM*FIRMWARE_CODE_SIZE + instNo * VSP_BUF_SIZE+DB_STT_SIZE;
	return phyAddr; 
}

unsigned int s3c_mfc_get_data_buffer_size(void)
{
	unsigned int out = 0;
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
	out = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K;
#endif
	return out;
}

