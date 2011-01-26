/* linux/arch/arm/plat-s3c/dev-usb-hsotg.c
 *
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for USB high-speed UDC/OtG block
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>

#include <plat/devs.h>

/* Android Gadget */
#include <linux/usb/android_composite.h>

#define S3C_VENDOR_ID		0x04e8	/* Samsung */
#define S3C_PRODUCT_ID		0x681D
#define S3C_ADB_PRODUCT_ID	0x681C	/* S3C6410 Swallowtail*/
#define MAX_USB_SERIAL_NUM	17


static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

/*
static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};
*/
static char *usb_functions_ums_adb_acm[] = {
	"adb",	
	"usb_mass_storage",
	"acm",	//interface 0
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb_acm),
		.functions	= usb_functions_ums_adb_acm,
	},
	/*
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
	*/
};
// serial number should be changed as real device for commercial release
static char device_serial[MAX_USB_SERIAL_NUM]="0123456789ABCDEF";
/* standard android USB platform data */

// Information should be changed as real product for commercial release
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
	.manufacturer_name	= "Android",//"Samsung",
	.product_name		= "Android",//"Samsung SMDKV210",
	.serial_number		= device_serial,
	.num_products 		= ARRAY_SIZE(usb_products),
	.products 		= usb_products,
	.num_functions 		= ARRAY_SIZE(usb_functions_all),
	.functions 		= usb_functions_all,
};

struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_android_usb);

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android   ",//"Samsung",
	.product		= "UMS Composite",//"SMDKV210",
	.release		= 1,
};
struct platform_device s3c_device_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_usb_mass_storage);

/* USB Device (Gadget)*/

static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start = S3C_PA_OTG,
		.end   = S3C_PA_OTG+S3C_SZ_OTG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		  = "s3c6410-usbgadget",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	  = s3c_usbgadget_resource,
};

EXPORT_SYMBOL(s3c_device_usbgadget);

