/**************************************************************************** 
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-hcd.h
 * @brief  header of s3c-otg-hcdi-hcd \n
 * @version 
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 *  -# Aug 18,2008 v1.3 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Modifying for successful rmmod & disconnecting \n
 * @see None
 ****************************************************************************/
/****************************************************************************
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
 ****************************************************************************/
 
#ifndef _S3C_OTG_HCDI_HCD_H_
#define _S3C_OTG_HCDI_HCD_H_

#ifdef __cplusplus
extern "C"
{
#endif

//for IRQ_NONE (0) IRQ_HANDLED (1) IRQ_RETVAL(x)	((x) != 0)
#include <linux/interrupt.h>	

#include <linux/usb.h>
//#include <../drivers/usb/core/hcd.h>

#include "s3c-otg-hcdi-debug.h"
#include "s3c-otg-hcdi-kal.h"

#include "s3c-otg-common-common.h"
#include "s3c-otg-common-datastruct.h"
#include "s3c-otg-common-const.h"

#include "s3c-otg-transfer-transfer.h"
#include "s3c-otg-oci.h"
#include "s3c-otg-roothub.h"

//placed in ISR
//extern void otg_handle_interrupt(void);

int     otg_hcd_init_modules(void);
void    otg_hcd_deinit_modules(void);


irqreturn_t	s3c6410_otghcd_irq(struct usb_hcd *hcd);

int	s3c6410_otghcd_start(struct usb_hcd *hcd);
void	s3c6410_otghcd_stop(struct usb_hcd *hcd);
void	s3c6410_otghcd_shutdown(struct usb_hcd *hcd);

int	s3c6410_otghcd_get_frame_number(struct usb_hcd *hcd);

int	s3c6410_otghcd_urb_enqueue(
			struct usb_hcd *hcd,
			struct urb *urb,
			gfp_t mem_flags);

int	s3c6410_otghcd_urb_dequeue(
			struct usb_hcd *_hcd, 
			struct urb *_urb,
			int status);

void	s3c6410_otghcd_endpoint_disable(
			struct usb_hcd *hcd, 
			struct usb_host_endpoint *ep);

int	s3c6410_otghcd_hub_status_data(
			struct usb_hcd *_hcd, 
			char *_buf);

int	s3c6410_otghcd_hub_control(
			struct usb_hcd *hcd, 
			u16 	type_req, 
			u16 	value,
			u16 	index, 
			char *	buf, 
			u16 	length);

int	s3c6410_otghcd_bus_suspend(struct usb_hcd *hcd);
int	s3c6410_otghcd_bus_resume(struct usb_hcd *hcd);
int	s3c6410_otghcd_start_port_reset(struct usb_hcd *hcd, unsigned port);

/**
 * @struct hc_driver s3c6410_otg_hc_driver
 * 
 * @brief implementation of hc_driver for OTG HCD
 * 
 * describe in detail
 */
static const struct hc_driver s3c6410_otg_hc_driver = {
	.description 		=	"EMSP_OTGHCD",
	.product_desc		=	"S3C OTGHCD",

	.irq 			= 	s3c6410_otghcd_irq,
	.flags			=	HCD_MEMORY | HCD_USB2,

	/** basic lifecycle operations	 */
	//.reset = 
	.start			=	s3c6410_otghcd_start,
	//.suspend 		= 	, 
	//.resume			= 	,
	.stop			=	s3c6410_otghcd_stop,
	.shutdown		=	s3c6410_otghcd_shutdown,

	/** managing i/o requests and associated device resources	 */
	.urb_enqueue		=	s3c6410_otghcd_urb_enqueue,
	.urb_dequeue 		=	s3c6410_otghcd_urb_dequeue,
	.endpoint_disable		=	s3c6410_otghcd_endpoint_disable,

	/** scheduling support	 */
	.get_frame_number	=	s3c6410_otghcd_get_frame_number,

	/** root hub support	 */
	.hub_status_data		=	s3c6410_otghcd_hub_status_data,
	.hub_control		=	s3c6410_otghcd_hub_control,
	//.hub_irq_enable = 
	.bus_suspend		=	s3c6410_otghcd_bus_suspend,
	.bus_resume 		=	s3c6410_otghcd_bus_resume,
	.start_port_reset 		=	s3c6410_otghcd_start_port_reset,
};

#ifdef __cplusplus 
} 
#endif 
#endif /* _S3C_OTG_HCDI_HCD_H_ */

