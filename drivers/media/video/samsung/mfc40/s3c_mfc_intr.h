/* 
 * drivers/media/video/samsung/mfc40/s3c_mfc_intr.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_INTR_H_
#define _S3C_MFC_INTR_H_

#include "s3c_mfc_common.h"

int s3c_mfc_wait_for_done(s3c_mfc_wait_done_type command);

#endif /* _S3C_MFC_INTR_H_ */
