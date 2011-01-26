/*
 * drivers/usb/gadget/s3c-udc.h
 * Samsung S3C on-chip full/high speed USB device controllers
 *
 * Copyright (C) 2009 Samsung Electronics, Seung-Soo Yang
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
 *
 */
 
#ifndef __S3C_USB_GADGET
#define __S3C_USB_GADGET

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include <asm/byteorder.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include <plat/regs-usb-otg-hs.h>
#include <plat/regs-clock.h> 
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/map.h>
#include <mach/hardware.h>


//S3C UDC send ZLP if req.zero == 1
#define S3C_UDC_ZLP		1

//Set delaying or not for serial usb bulk transfer
#define SERIAL_TRANSFER_DELAY 	1

//Maxium EPs implemented
#define	SUPPORTING_MAX_EP_NUM	9	//0 ~ 9

/**
 * Max packet size OTG IP support 
 */
#define EP0_FIFO_SIZE		64
#define BULK_FIFO_SIZE		512
#define INT_FIFO_SIZE		1024
#define ISO_FIFO_SIZE		1024

#define S3C_MAX_ENDPOINTS	16

/**
 * states of control tranfser
 */
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_RECV         2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_NEED_ZLP     4
#define RegReadErr				5
#define FAIL_TO_SETUP		    6

/**
 * Definitions of EP index
 */
#define EP0_CON		0
#define EP1_OUT		1
#define EP2_IN		2
#define EP3_IN		3
#define EP4_OUT		4
#define EP5_IN		5
#define EP6_IN		6
#define EP7_OUT		7
#define EP8_IN		8
#define EP9_IN		9
#define EP_MASK		0xF

/**
 * struct s3c_ep
 */
struct s3c_ep {
	struct usb_ep ep;
	struct s3c_udc *dev;

	const struct usb_endpoint_descriptor *desc;
	struct list_head queue;
	unsigned long pio_irqs;

	u8 stopped;
	u8 bEndpointAddress;
	u8 bmAttributes;
	u32 fifo;
};
//---------------------------------------------------------------------------------------

/**
 * @struct s3c_request
 */
struct s3c_request {
	struct usb_request req;
	struct list_head queue;	
#if S3C_UDC_ZLP
	bool	zlp;
#endif
};
//---------------------------------------------------------------------------------------


/**
 * gadget_driver
 * USB functional gadget driver registered
 */
enum gadget_driver {
	NO_GADGET_DRIVER,
	ETHER_RNDIS, 
	ETHER_CDC,
	ANDROID_ADB, 
	ANDROID_ADB_UMS,
	ANDROID_ADB_UMS_ACM,
	SERIAL,
	CDC2,
	FILE_STORAGE,
};
//---------------------------------------------------------------------------------------

/**
 * @struct s3c_udc
 * 
 */
struct s3c_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct platform_device *dev;
	struct s3c_ep ep[S3C_MAX_ENDPOINTS];
	
	enum usb_device_state udc_state, udc_resume_state;
	enum gadget_driver config_gadget_driver;
	enum usb_device_speed udc_resume_speed;

	spinlock_t lock;		
	u16 devstatus;

	int clocked;
	int powered;

	int ep0state;
	unsigned char usb_address;
	
	struct timer_list srp_timer;
};
//---------------------------------------------------------------------------------------

/**
 * definition of helper macro
 * 
 */
#define ep_is_in(EP) 		(((EP)->bEndpointAddress&USB_DIR_IN)==USB_DIR_IN)
#define ep_index(EP) 		((EP)->bEndpointAddress&USB_ENDPOINT_NUMBER_MASK)
#define ep_maxpacket(EP) 	((EP)->ep.maxpacket)

#define BYTES2MAXP(x)	(x / 8)
#define MAXP2BYTES(x)	(x * 8)
//---------------------------------------------------------------------------------------

#endif
