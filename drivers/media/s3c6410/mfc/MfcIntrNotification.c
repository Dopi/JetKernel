/* mfc/MfcIntrNotification.c
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

#include "s3c-mfc.h"
#include "MfcIntrNotification.h"
#include "MfcSfr.h"


extern wait_queue_head_t	WaitQueue_MFC;
static unsigned int  		gIntrType = 0;

int SendInterruptNotification(int intr_type)
{
	gIntrType = intr_type;
	wake_up_interruptible(&WaitQueue_MFC);
	
	return 0;
}

int WaitInterruptNotification(void)
{
	if(interruptible_sleep_on_timeout(&WaitQueue_MFC, 500) == 0)
	{
		MfcStreamEnd();
		return WAIT_INT_NOTI_TIMEOUT; 
	}
	
	return gIntrType;
}
