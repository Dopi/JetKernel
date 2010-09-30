/**************************************************************************** 
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 * @file   s3c-otg-hcdi-driver.c
 * @brief  It provides functions related with module for OTGHCD driver. \n
 * @version 
 *  -# Jun 9,2008 v1.0 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Creating the initial version of this code \n
 *  -# Jul 15,2008 v1.2 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Optimizing for performance \n
 *  -# Aug 18,2008 v1.3 by SeungSoo Yang (ss1.yang@samsung.com) \n
 *	  : Modifying for successful rmmod & disconnecting \n
 * @see None
 * 
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

#include "s3c-otg-hcdi-driver.h"

/**
 * static int s3c6410_otg_drv_probe (struct platform_device *pdev)
 * 
 * @brief probe function of OTG hcd platform_driver
 * 
 * @param [in] pdev : pointer of platform_device of otg hcd platform_driver
 * 
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If fail \n
 * @remark 
 * it allocates resources of it and call other modules' init function.
 * then call usb_create_hcd, usb_add_hcd, s3c6410_otghcd_start functions
 */

static struct clk	*otg_clock = NULL;

static int s3c6410_otg_drv_probe (struct platform_device *pdev)
{

	int ret_val = 0;
	u32 reg_val = 0;
	
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s3c_otg_drv_probe \n");
	
	otg_clock = clk_get(&pdev->dev, "otg");
	if (otg_clock == NULL) {
		printk(KERN_INFO "failed to find otg clock source\n");
		return -ENOENT;
	}
	clk_enable(otg_clock);

///init for host mode
/** 
	Allocate memory for the base HCD &	Initialize the base HCD.
*/
	g_pUsbHcd = usb_create_hcd(&s3c6410_otg_hc_driver, &pdev->dev, dev_name(&pdev->dev));
	if (g_pUsbHcd == NULL) 
	{
		ret_val = -ENOMEM;
		otg_err(OTG_DBG_OTGHCDI_DRIVER, "failed to usb_create_hcd\n");
		goto err_out_clk;
	}


//	mapping hcd resource & device resource
	
	g_pUsbHcd->rsrc_start = pdev->resource[0].start;
	g_pUsbHcd->rsrc_len   = pdev->resource[0].end - pdev->resource[0].start + 1;

	if (!request_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len, gHcdName)) 
	{
		otg_err(OTG_DBG_OTGHCDI_DRIVER, "failed to request_mem_region\n");
		reg_val = -EBUSY;
		goto err_out_create_hcd;
	}


//Physical address => Virtual address
	g_pUsbHcd->regs = S3C_VA_OTG; 
	g_pUsbHcd->self.otg_port = 1;
	
	g_pUDCBase = (u8 *)g_pUsbHcd->regs;

	/// call others' init()
	reg_val = otg_hcd_init_modules();
	if( reg_val != USB_ERR_SUCCESS)
	{		
		otg_err(OTG_DBG_OTGHCDI_DRIVER, "failed to otg_hcd_init_modules\n");
		reg_val = USB_ERR_FAIL;
		goto err_out_create_hcd;
	}

	/**
	 * Attempt to ensure this device is really a s3c6410 USB-OTG Controller.
	 * Read and verify the SNPSID register contents. The value should be
	 * 0x45F42XXX, which corresponds to "OT2", as in "OTG version 2.XX".
	 */
	//reg_val = read_reg_32((unsigned int *)((u8 *)g_pUsbHcd->regs + 0x40)); 
	
	reg_val = read_reg_32(0x40); 
	if ((reg_val & 0xFFFFF000) != 0x4F542000) 
	{
		otg_err(OTG_DBG_OTGHCDI_DRIVER, "Bad value for SNPSID: 0x%x\n", reg_val);
		ret_val = -EINVAL;
		goto err_out_create_hcd_init;
	}

	/*
	 * Finish generic HCD initialization and start the HCD. This function
	 * allocates the DMA buffer pool, registers the USB bus, requests the
	 * IRQ line, and calls s3c6410_otghcd_start method.
	 */
	ret_val = usb_add_hcd(g_pUsbHcd, pdev->resource[1].start, IRQF_DISABLED);
	if (ret_val < 0) 
	{
		goto err_out_create_hcd_init;
	}

	otg_dbg(OTG_DBG_OTGHCDI_DRIVER,"OTG HCD Initialized HCD, bus=%s, usbbus=%d\n", 
		    "EMSP OTG Controller", g_pUsbHcd->self.busnum);
	return USB_ERR_SUCCESS;

err_out_create_hcd_init:	
	otg_hcd_deinit_modules();
	release_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len);

err_out_create_hcd: 
	usb_put_hcd(g_pUsbHcd);
	
err_out_clk:
	
	return ret_val;
}
//-------------------------------------------------------------------------------

/**
 * static int s3c6410_otg_drv_remove (struct platform_device *dev)
 * 
 * @brief remove function of OTG hcd platform_driver
 * 
 * @param [in] pdev : pointer of platform_device of otg hcd platform_driver
 * 
 * @return USB_ERR_SUCCESS : If success \n
 *         USB_ERR_FAIL : If fail \n
 * @remark 
 * This function is called when the otg device unregistered with the
 * s3c6410_otg_driver. This happens, for example, when the rmmod command is
 * executed. The device may or may not be electrically present. If it is
 * present, the driver stops device processing. Any resources used on behalf
 * of this device are freed.
 */
static int s3c6410_otg_drv_remove (struct platform_device *dev) 
{     
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s3c6410_otg_drv_remove \n");		

	otg_hcd_deinit_modules();

	usb_remove_hcd(g_pUsbHcd);
	
	release_mem_region(g_pUsbHcd->rsrc_start, g_pUsbHcd->rsrc_len);

	usb_put_hcd(g_pUsbHcd);

	if (otg_clock != NULL) {
		clk_disable(otg_clock);
		clk_put(otg_clock);
		otg_clock = NULL;
	}

	
	return USB_ERR_SUCCESS;
} 
//-------------------------------------------------------------------------------

/**
 * @struct s3c6410_otg_driver
 * 
 * @brief 
 * This structure defines the methods to be called by a bus driver
 * during the lifecycle of a device on that bus. Both drivers and
 * devices are registered with a bus driver. The bus driver matches
 * devices to drivers based on information in the device and driver
 * structures.
 *
 * The probe function is called when the bus driver matches a device
 * to this driver. The remove function is called when a device is
 * unregistered with the bus driver. 
 */
static struct platform_driver s3c6410_otg_driver = {
	.probe = s3c6410_otg_drv_probe,
	.remove = s3c6410_otg_drv_remove,
	.shutdown = usb_hcd_platform_shutdown,
	.driver = {
		.name = "s3c_otghcd",
		.owner = THIS_MODULE,
	},
};
//-------------------------------------------------------------------------------

/**
 * static int __init s3c6410_otg_module_init(void)
 * 
 * @brief module_init function
 * 
 * @return it returns result of platform_driver_register
 * @remark 
 * This function is called when the s3c6410_otg_driver is installed with the
 * insmod command. It registers the s3c6410_otg_driver structure with the
 * appropriate bus driver. This will cause the s3c6410_otg_driver_probe function
 * to be called. In addition, the bus driver will automatically expose
 * attributes defined for the device and driver in the special sysfs file
 * system.
 */
static int __init s3c6410_otg_module_init(void) 
{ 
	int	ret_val = 0;
	
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s3c_otg_module_init \n");		

	ret_val = platform_driver_register(&s3c6410_otg_driver);
	if (ret_val < 0) 
	{		
		otg_err(OTG_DBG_OTGHCDI_DRIVER, "platform_driver_register \n");		
	}
	return ret_val;    
} 
//-------------------------------------------------------------------------------

/**
 * static void __exit s3c6410_otg_module_exit(void)
 * 
 * @brief module_exit function
 * 
 * @remark 
 * This function is called when the driver is removed from the kernel
 * with the rmmod command. The driver unregisters itself with its bus
 * driver. 
 */
static void __exit s3c6410_otg_module_exit(void) 
{     
	otg_dbg(OTG_DBG_OTGHCDI_DRIVER, "s3c_otg_module_exit \n");	
	platform_driver_unregister(&s3c6410_otg_driver);
} 
//-------------------------------------------------------------------------------

module_init(s3c6410_otg_module_init);
module_exit(s3c6410_otg_module_exit);

MODULE_DESCRIPTION("OTG USB HOST controller driver");
MODULE_AUTHOR("SAMSUNG / System LSI / EMSP");
MODULE_LICENSE("GPL");
