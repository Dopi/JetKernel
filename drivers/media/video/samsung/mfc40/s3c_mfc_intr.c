/*
 * drivers/media/video/samsung/mfc40/s3c_mfc_intr.c
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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/io.h>

#include <plat/regs-mfc.h>

#include "s3c_mfc_intr.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_common.h"
#include "s3c_mfc_types.h"
#include "s3c_mfc_memory.h"

extern wait_queue_head_t	s3c_mfc_wait_queue;
extern unsigned int  		s3c_mfc_int_type;

extern void __iomem		*s3c_mfc_sfr_virt_base;

static int s3c_mfc_wait_polling(unsigned int PollingRegAddress)
{
	int i;
	volatile unsigned int uRegData=0;
	unsigned int waitLoop = 1000; /* 1000msec */


	for (i = 0; (i < waitLoop) && (uRegData == 0) ;i++) {
		mdelay(1);
		uRegData = readl(s3c_mfc_sfr_virt_base + PollingRegAddress);
	}

	if (uRegData == 0) {
		mfc_err("Polling Time Out(Reg : 0x%x)\n", PollingRegAddress);
		return 0;
	}

	return 1;

}

int s3c_mfc_wait_for_done(s3c_mfc_wait_done_type command)
{
	unsigned int retVal = 1; 

	switch(command){
	case MFC_POLLING_DMA_DONE :
		retVal = s3c_mfc_wait_polling(S3C_FIMV_DONE_M);
		break;

	case MFC_POLLING_HEADER_DONE :
		retVal = s3c_mfc_wait_polling(S3C_FIMV_HEADER_DONE);
		break;

	case MFC_POLLING_OPERATION_DONE :
		retVal = s3c_mfc_wait_polling(S3C_FIMV_OPERATION_DONE);
		break;

	case MFC_POLLING_FW_DONE :
		retVal = s3c_mfc_wait_polling(S3C_FIMV_FW_DONE);
		break;

	case MFC_INTR_FRAME_DONE :
	case MFC_INTR_DMA_DONE :
	case MFC_INTR_FW_DONE :
	case MFC_INTR_FRAME_FW_DONE:
		if (interruptible_sleep_on_timeout(&s3c_mfc_wait_queue, 1000) == 0) {
			retVal = 0;
			mfc_err("Interrupt Time Out(%d)\n", command);
			break;
		}

		retVal = s3c_mfc_int_type;
		s3c_mfc_int_type = 0;
		break;		

	default : 
			mfc_err("undefined command\n");
			retVal = 0;
	}

	return retVal;
}

