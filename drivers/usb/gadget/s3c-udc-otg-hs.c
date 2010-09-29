/*
 * drivers/usb/gadget/s3c-udc-otg-hs.c
 * Samsung S3C on-chip full/high speed USB OTG 2.0 device controllers
 *
 * Copyright (C) 2009 Samsung Electronics, Seung-Soo Yang
 * Copyright (C) 2008 Samsung Electronics, Kyu-Hyeok Jang, Seung-Soo Yang
 * Copyright (C) 2004 Mikko Lahteenmaki, Nordic ID
 * Copyright (C) 2004 Bo Henriksen, Nordic ID
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

/*
 *	The changes of udc_state of struct s3c_udc in functions
 *	
 *	  s3c_udc_probe			s3c_udc->udc_state = USB_STATE_NOTATTACHED;
 *	  s3c_udc_enable		s3c_udc->udc_state = USB_STATE_POWERED;
 *	  handle_reset_intr		s3c_udc->udc_state = USB_STATE_DEFAULT;
 *	  s3c_udc_set_address	s3c_udc->udc_state = USB_STATE_ADDRESS;
 *	  s3c_ep0_setup			s3c_udc->udc_state = USB_STATE_CONFIGURED;
 *	  handle_suspend_intr	s3c_udc->udc_state = USB_STATE_SUSPENDED;
 *	  s3c_udc_disable		s3c_udc->udc_state = USB_STATE_NOTATTACHED;
 *
 */ 

#include "s3c-udc.h"

//---------------------------------------------------------------------------------------

/* enalbing many debug message could make the USB enumeration process failed */
#define OTG_DBG_ENABLE	0

/*
 * set USBCV_CH9_REMOTE_WAKE_UP_TEST 1 ONLY for testing USBCV ch9
 * RemoteWakeupTestEnabled & RemoteWakeupTestDisabled
 * Confirm USBCV_CH9_REMOTE_WAKE_UP_TEST 
 * in s3c-udc-otg-hs.c and f_adb.h 
 */
#define	USBCV_CH9_REMOTE_WAKE_UP_TEST 0

/*
 * setting TESTING_SOFT_CONNCTION 1 make
 * /sys/class/switch/S3C_UDC_SOFT_SWITCH file
 * this is only for testing UDC soft connect / disconnect logic 
 * to find any side-effect related with battery to identify 
 * a device connected (USB, AC, TA)
 */
#define TESTING_SOFT_CONNCTION 0

#if OTG_DBG_ENABLE
#define DEBUG_S3C_UDC_SETUP
#define DEBUG_S3C_UDC_EP0
#define DEBUG_S3C_UDC_ISR
#define DEBUG_S3C_UDC_OUT_EP
#define DEBUG_S3C_UDC_IN_EP
#define DEBUG_S3C_UDC
#define DEBUG_S3C_UDC_PM
#define DEBUG_S3C_UDC_SETUP_FEATURE
#define DEBUG_S3C_UDC_ERROR

#else
#undef DEBUG_S3C_UDC_SETUP
#undef DEBUG_S3C_UDC_EP0
#undef DEBUG_S3C_UDC_ISR
#undef DEBUG_S3C_UDC_OUT_EP
#undef DEBUG_S3C_UDC_IN_EP
#undef DEBUG_S3C_UDC
#undef DEBUG_S3C_UDC_PM
#undef DEBUG_S3C_UDC_SETUP_FEATURE
#undef DEBUG_S3C_UDC_ERROR
#endif

//temp
#define DEBUG_S3C_UDC_ERROR
//#define DEBUG_S3C_UDC_SETUP
//#define DEBUG_S3C_UDC_EP0
#define DEBUG_S3C_UDC_PM
//#define DEBUG_S3C_UDC_SETUP_FEATURE
	
#if defined(DEBUG_S3C_UDC_ISR) || defined(DEBUG_S3C_UDC_EP0)
static char *state_names[] = {
	"WAIT_FOR_SETUP",
	"DATA_STATE_XMIT",
	"DATA_STATE_NEED_ZLP",
	"WAIT_FOR_OUT_STATUS",
	"DATA_STATE_RECV",
	"RegReadErr"
	};
#endif

#ifdef DEBUG_S3C_UDC_ERROR
#define DEBUG_ERROR(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_ERROR(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_SETUP
#define DEBUG_SETUP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_SETUP(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_PM
#define DEBUG_PM(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_PM(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_EP0
#define DEBUG_EP0(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_EP0(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC
#define DEBUG(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_ISR
#define DEBUG_ISR(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_ISR(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_OUT_EP
#define DEBUG_OUT_EP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_OUT_EP(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_IN_EP
#define DEBUG_IN_EP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_IN_EP(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_SETUP_FEATURE
#define DEBUG_FEATURE(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_FEATURE(fmt,args...) do {} while(0)
#endif
//---------------------------------------------------------------------------------------

#define	DRIVER_DESC		"Samsung Dual-speed USB 2.0 OTG Device Controller"
#define	DRIVER_VERSION	__DATE__

static	struct s3c_udc	*the_controller;
static struct clk	*otg_clock = NULL;

static const char driver_name[] = "s3c-udc";
static const char driver_desc[] = DRIVER_DESC;
static const char ep0name[] = "ep0-control";
//---------------------------------------------------------------------------------------

/*
 *	Samsung S3C on-chip full/high speed USB OTG 2.0 device operates
 *	both internal DMA mode and slave mode.
 *	The implementation of the slave mode is not stable at the time of writing(Feb. 18 2009)
 *  Make sure CONFIG_USB_GADGET_S3C_OTGD_HS_DMA_MODE is defineds
 */

#if defined(CONFIG_USB_GADGET_S3C_OTGD_HS_DMA_MODE) 
#define GINTMSK_INIT	(INT_ENUMDONE|INT_RESET|INT_SUSPEND)
#define GINTMSK_RESET	(INT_OUT_EP|INT_IN_EP|INT_RESUME|INT_ENUMDONE|INT_RESET\
							|INT_SUSPEND)
#define DOEPMSK_INIT	(CTRL_OUT_EP_SETUP_PHASE_DONE|TRANSFER_DONE)
#define DIEPMSK_INIT	(NON_ISO_IN_EP_TIMEOUT|TRANSFER_DONE)
#define GAHBCFG_INIT	(PTXFE_HALF|NPTXFE_HALF|MODE_DMA|BURST_SINGLE|GBL_INT_UNMASK)
#else
#define GINTMSK_INIT	(INT_RESUME|INT_ENUMDONE|INT_RESET|INT_SUSPEND|	INT_RX_FIFO_NOT_EMPTY\
						|INT_GOUTNakEff|INT_GINNakEff)
#define DOEPMSK_INIT	(CTRL_OUT_EP_SETUP_PHASE_DONE|AHB_ERROR|TRANSFER_DONE)
#define DIEPMSK_INIT	(INTKN_TXFEMP|NON_ISO_IN_EP_TIMEOUT|TRANSFER_DONE|AHB_ERROR)
#define GAHBCFG_INIT	(PTXFE_HALF|NPTXFE_HALF|MODE_SLAVE|BURST_INCR16|GBL_INT_UNMASK)

#endif
//---------------------------------------------------------------------------------------

/*
 * Behavior of Power Management
 * 
 * 1. USB Bus PM
 *		The suspend interrupt happens when USB Host(PC) falls into sleep mode.
 *		Then handler of suspend calls suspend callback function of gadget 
 *		and call s3c_udc_stop_activity() if it already enumerated
 *		Then, s3c_udc_power_down will be invoked by fsa9480 to power down s3c-udc and 
 *		disable sourcing otg clock
 *		
 *		When USB host wakes up, s3c_udc_power_up will be invoked by fsa9480 to 
 *		power up s3c-udc and enable sourcing otg clock
 *		And, USB host enumerates S3C-UDC again without resume interrupt
 *
 * 2. System PM
 *		When the target system falls into sleep mode, it calls s3c_udc_suspend().
 *		Then, s3c_udc_power_down will be invoked by fsa9480 to power down s3c-udc and 
 *		disable sourcing otg clock
 * 
 *		When the target resumes, it calls s3c_udc_resume().
 *		Then fsa9480_check_usb_connection() will be invoked to check USB connections 
 * 		if connected s3c_udc_power_up() power up s3c-udc and enable sourcing otg clock
 * 		or s3c_udc_power_down() power down s3c-udc and disable sourcing otg clock
 * 
 *		In case of Android system there is a lock for not falling into the sleep mode when
 *		USB connected. [settings->Application->Development->Stay awake]
 *		Although usb connected the target system will fall into sleep mode 
 *		if [Stay awake] unchecked
 */

/*
 * Local declarations.
 */
static void s3c_req_done(struct s3c_ep *ep, struct s3c_request *req, int status);
static void s3c_udc_stop_activity(struct s3c_udc *dev, struct usb_gadget_driver *driver);
static int	s3c_udc_enable(struct s3c_udc *dev);
static void s3c_udc_set_address(struct s3c_udc *dev, unsigned char address);
static void s3c_udc_initialize(struct s3c_udc *dev);

static int  s3c_ep_enable(struct usb_ep *ep, const struct usb_endpoint_descriptor *);
static int  s3c_ep_disable(struct usb_ep *ep);
static void s3c_ep_free_request(struct usb_ep *ep, struct usb_request *);
static int  s3c_ep_queue(struct usb_ep *ep, struct usb_request *, gfp_t gfp_flags);
static int  s3c_ep_dequeue(struct usb_ep *ep, struct usb_request *);
static int  s3c_ep_set_halt(struct usb_ep *ep, int);
static struct usb_request *s3c_ep_alloc_request(struct usb_ep *ep, gfp_t gfp_flags);
static inline void s3c_ep_send_zlp(u32 ep_num);

static void s3c_ep0_read(struct s3c_udc *dev);
static void s3c_ep0_handle(struct s3c_udc *dev);
static int  s3c_ep0_write(struct s3c_udc *dev);
static int  s3c_ep0_write_fifo(struct s3c_ep *ep, struct s3c_request *req);

//---------------------------------------------------------------------------------------

/**
 * global usb_ctrlrequest struct to store 
 * Setup data of Control Request Host sent
 */
struct usb_ctrlrequest g_ctrl __attribute__((aligned(8)));
//---------------------------------------------------------------------------------------

/**
 * usb_ep_ops s3c_ep_ops
 * 
 */
static struct usb_ep_ops s3c_ep_ops = {
	.enable = s3c_ep_enable,
	.disable = s3c_ep_disable,

	.alloc_request = s3c_ep_alloc_request,
	.free_request = s3c_ep_free_request,

	.queue = s3c_ep_queue,
	.dequeue = s3c_ep_dequeue,

	.set_halt = s3c_ep_set_halt,
};
//---------------------------------------------------------------------------------------

/*
 * make s3c-udc being connected
 */ 
void s3c_udc_soft_connect(void)
{
	u32 reg_val;
	
	DEBUG("[%s]\n", __func__);
	reg_val = readl(S3C_UDC_OTG_DCTL);
	reg_val = reg_val & ~SOFT_DISCONNECT;
	writel(reg_val, S3C_UDC_OTG_DCTL);
}
//---------------------------------------------------------------------------------------

/*
 * make s3c-udc being disconnected
 */ 
void s3c_udc_soft_disconnect(void)
{
	u32 reg_val;
//	struct s3c_udc *dev = the_controller;

	DEBUG("[%s]\n", __func__);	
	reg_val = readl(S3C_UDC_OTG_DCTL);
	reg_val |= SOFT_DISCONNECT;
	writel(reg_val, S3C_UDC_OTG_DCTL);

//	s3c_udc_stop_activity(dev, dev->driver);
//	s3c_udc_set_disconnect_state(dev);
}
//---------------------------------------------------------------------------------------

#if TESTING_SOFT_CONNCTION
#include <linux/switch.h>
struct switch_dev soft_switch;
#define	SOFT_SWITCH_NAME "S3C_UDC_SOFT_SWITCH"

static ssize_t soft_switch_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", SOFT_SWITCH_NAME);
}

static ssize_t soft_switch_state(struct switch_dev *sdev, char *buf)
{
	if (soft_switch.state == 0) {
		s3c_udc_soft_disconnect();
		udelay(20);
		s3c_udc_soft_connect();
		soft_switch.state = 1;
	}
	else {
		s3c_udc_soft_disconnect();
		udelay(20);
		s3c_udc_soft_connect();
		soft_switch.state = 0;
	}
	return sprintf(buf, "%s\n", (soft_switch.state ? "1" : "0"));
}
#endif

/**
 * Proc related
 * 
 */
#ifdef CONFIG_USB_GADGET_DEBUG_FILES

static const char proc_node_name[] = "driver/udc";

static int
s3c_udc_proc_read(char *page, char **start, off_t off, int count, int *eof, void *_dev)
{
	char *buf = page;
	struct s3c_udc *dev = _dev;
	char *next = buf;
	unsigned size = count;
	unsigned long flags;
	int t;

	if (off != 0)
		return 0;

	local_irq_save(flags);

	/* basic device status */
	t = scnprintf(next, size,
		      DRIVER_DESC "\n"
		      "%s version: %s\n"
		      "Gadget driver: %s\n"
		      "\n",
		      driver_name, DRIVER_VERSION,
		      dev->driver ? dev->driver->driver.name : "(none)");
	size -= t;
	next += t;

	local_irq_restore(flags);
	*eof = 1;
	return count - size;
}

#define create_proc_files() \
	create_proc_read_entry(proc_node_name, 0, NULL, s3c_udc_proc_read, dev)
#define remove_proc_files() \
	remove_proc_entry(proc_node_name, NULL)

#else	/* !CONFIG_USB_GADGET_DEBUG_FILES */

#define create_proc_files() do {} while (0)
#define remove_proc_files() do {} while (0)

#endif	/* CONFIG_USB_GADGET_DEBUG_FILES */
//---------------------------------------------------------------------------------------

/* 
 * until it becomes enabled, this UDC should be completely invisible
 * to any USB host.
 */
static int s3c_udc_enable(struct s3c_udc *dev)
{
	u32 reg_val;
	DEBUG_SETUP("%s: %p\n", __func__, dev);

	reg_val = readl(S3C_OTHERS);
	reg_val |= (1<<16);   // USB_SIG_MASK
	writel(reg_val, S3C_OTHERS);
	
	// 1. Initializes OTG Phy.
	writel(0x0, S3C_USBOTG_PHYPWR);
	writel(0x20, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);
	// confirm delay time with thinking pm logic
	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
	
	dev->udc_state = USB_STATE_POWERED;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	
	/* 14. Initialize OTG Link Core. */
	writel(GAHBCFG_INIT, S3C_UDC_OTG_GAHBCFG);
	
	writel( 0<<15		// PHY Low Power Clock sel
		|1<<14		// Non-Periodic TxFIFO Rewind Enable
		|0x5<<10	// Turnaround time
		|0<<9|0<<8	// [0:HNP disable, 1:HNP enable][ 0:SRP disable, 1:SRP enable] H1= 1,1
		|0<<7		// Ulpi DDR sel
		|0<<6		// 0: high speed utmi+, 1: full speed serial
		|0<<4		// 0: utmi+, 1:ulpi
		|1<<3		// phy i/f	0:8bit, 1:16bit
		|0x7<<0,	// HS/FS Timeout*
		S3C_UDC_OTG_GUSBCFG);

	s3c_udc_initialize(dev);

	//change proper register instead of S3C_UDC_OTG_GINTMSK
	reg_val = readl(S3C_UDC_OTG_GINTMSK);
	if(!reg_val)
	{
		DEBUG_ERROR("[%s] Fail to set GINTMSK 0x%x\n", __func__, reg_val);
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------------------

/*
 * 	s3c_udc_disable - disable USB device controller
 */
static void s3c_udc_disable(struct s3c_udc *dev)
{
	DEBUG_PM("%s: %p\n", __func__, dev);

	s3c_udc_set_address(dev, 0);

	dev->ep0state = WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	dev->udc_state = USB_STATE_NOTATTACHED;

	writel(readl(S3C_USBOTG_PHYPWR)|(1<<OTG_DISABLE)|(1<<ANALOG_POWERDOWN), S3C_USBOTG_PHYPWR);
}
//---------------------------------------------------------------------------------------

/*
 * 	s3c_ep_list_reinit - initialize software state
 */
static void s3c_ep_list_reinit(struct s3c_udc *dev)
{
	u8 i;

	DEBUG_SETUP("%s: %p\n", __func__, dev);

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0state = WAIT_FOR_SETUP;

	/* basic endpoint records init */
	for (i = 0; i < S3C_MAX_ENDPOINTS; i++) {
		struct s3c_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->desc = 0;
		ep->stopped = 0;
		ep->pio_irqs = 0;
		INIT_LIST_HEAD(&ep->queue);
	}

	/* the rest was statically initialized, and is read-only */
}
//---------------------------------------------------------------------------------------


/*
 * usb_gadget_register_driver
 * Register entry point for the peripheral controller driver.
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *dev = the_controller;
	int retval;

	DEBUG_SETUP("%s: %s\n", __func__, driver->driver.name);
#if 1
/*
	adb composite fail to !driver->unbind in composite.c as below
	static struct usb_gadget_driver composite_driver = {
		.speed		= USB_SPEED_HIGH,

		.bind		= composite_bind,
		.unbind		= __exit_p(composite_unbind),
*/
	if (!driver
	    || (driver->speed < USB_SPEED_FULL)
	    || !driver->bind
	    || !driver->disconnect || !driver->setup)
		return -EINVAL;		
#else	
if (!driver
	|| (driver->speed != USB_SPEED_FULL && driver->speed != USB_SPEED_HIGH)
	|| !driver->bind
	|| !driver->unbind || !driver->disconnect || !driver->setup)
	return -EINVAL;
#endif
	
	if (!dev)
		return -ENODEV;

	if (dev->driver)
		return -EBUSY;

	/* first hook up the driver ... */
	dev->devstatus = 1 << USB_DEVICE_SELF_POWERED;
	dev->driver = driver;
	dev->gadget.dev.driver = &driver->driver;
	retval = device_add(&dev->gadget.dev);

	if(retval) { /* TODO */
		DEBUG_ERROR("target device_add failed, error %d\n", retval);
		return retval;
	}
	
	retval = driver->bind(&dev->gadget);
	if (retval) {
		DEBUG_ERROR("%s: bind to driver %s --> error %d\n", dev->gadget.name,
		       driver->driver.name, retval);
		device_del(&dev->gadget.dev);

		dev->driver = 0;
		dev->gadget.dev.driver = 0;
		return retval;
	}
	enable_irq(IRQ_OTG);
	
	DEBUG_SETUP("Registered gadget driver '%s'\n", driver->driver.name);

#ifndef CONFIG_PM
	s3c_udc_enable(dev);
#endif

/* 	in case of rndis, will be chaned at the time of SET_CONFIGURATION */
	if (strcmp(driver->driver.name, "g_ether") == 0)
		dev->config_gadget_driver = ETHER_CDC;	
	
	else if (strcmp(driver->driver.name, "android_adb") == 0)
		dev->config_gadget_driver = ANDROID_ADB;
	
	else if (strcmp(driver->driver.name, "android_usb") == 0)
		dev->config_gadget_driver = ANDROID_ADB_UMS;
	
	else if (strcmp(driver->driver.name, "android_adb_ums_acm") == 0)
		dev->config_gadget_driver = ANDROID_ADB_UMS_ACM;
	
	else if (strcmp(driver->driver.name, "g_serial") == 0)
		dev->config_gadget_driver = SERIAL;
	
	else if (strcmp(driver->driver.name, "g_cdc") == 0)
		dev->config_gadget_driver = CDC2;
	
	else if (strcmp(driver->driver.name, "g_file_storage") == 0)
		dev->config_gadget_driver = FILE_STORAGE;

	else
		DEBUG_ERROR("Not recognized driver's name '%s'\n", driver->driver.name);
		
	return 0;
}

EXPORT_SYMBOL(usb_gadget_register_driver);
//---------------------------------------------------------------------------------------

/*
 * Unregister entry point for the peripheral controller driver.
 */ 
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *dev = the_controller;
	unsigned long flags;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;
	
	disable_irq(IRQ_OTG);

	spin_lock_irqsave(&dev->lock, flags);
	s3c_udc_stop_activity(dev, driver);
	spin_unlock_irqrestore(&dev->lock, flags);

	driver->unbind(&dev->gadget);
	device_del(&dev->gadget.dev);

	DEBUG_SETUP("Unregistered gadget driver '%s'\n", driver->driver.name);

#ifndef CONFIG_PM
	s3c_udc_disable(dev);
#endif	
	dev->gadget.dev.driver = NULL;
	dev->driver = NULL;
	dev->config_gadget_driver = NO_GADGET_DRIVER;

	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);
//---------------------------------------------------------------------------------------

#if defined(CONFIG_USB_GADGET_S3C_OTGD_HS_DMA_MODE) 
	/* DMA Mode */
	#include "s3c-udc-otg-hs_dma.c"
#else 
	/* Slave Mode */
	#error Unsupporting slave mode
#endif

#include "fsa9480_i2c.c"
//---------------------------------------------------------------------------------------

/*
 * power up s3c-udc power & enable otg clock
 */ 
void s3c_udc_power_up(void)
{
	struct s3c_udc *dev = the_controller;
	if(dev->udc_state == USB_STATE_NOTATTACHED) {		
		DEBUG_PM("[%s] \n", __func__);

S3C_UDC_POWER_UP:	
		fsa9480_s3c_udc_on();
			
		if(!dev->clocked) {
			if(clk_enable(otg_clock) != 0) {
				DEBUG_ERROR("\n[%s] clk_enable(otg_clock) failed.\n", __func__);
			}
			else {
				dev->clocked = 1;
				DEBUG_PM("\n[%s] clk_enable(otg_clock) OK.\n", __func__);
			}
		}
		else
			DEBUG_PM("\n[%s] already clk_enabled.\n", __func__);
		
		if(s3c_udc_enable(dev) != 0 || !dev->clocked) {
		/* 	Just in case */
			DEBUG_ERROR("\n[%s] FAIL TO s3c_udc_enable()\n", __func__);
			DEBUG_ERROR("\n[%s] Power UP again.\n", __func__);
			goto S3C_UDC_POWER_UP;
		}
		else
			DEBUG_PM("\n[%s] POWER-UP OK!.\n", __func__);
	}
	else {
		DEBUG_PM("[%s] skipped , already powered up\n", __func__);			
		
		if(!readl(S3C_UDC_OTG_GINTMSK) || !dev->clocked) {
		/* 	Just in case */
			DEBUG_ERROR("\n[%s] !readl(S3C_UDC_OTG_GINTMSK) || !dev->clocked\n", __func__);
			DEBUG_ERROR("\n[%s] Power UP again.\n", __func__);
			goto S3C_UDC_POWER_UP;
		}
	}		
	dev->powered = 1;
}
//---------------------------------------------------------------------------------------

/*
 * power down s3c-udc power & disable otg clock
 */ 
void s3c_udc_power_down(void)
{
	struct s3c_udc *dev = the_controller;

	/* Confirm 	 */
	if (dev->udc_state != USB_STATE_SUSPENDED) {
		//s3c_udc_set_disconnect_state(dev);
		s3c_udc_stop_activity(dev, dev->driver);
	}
	
	if(dev->udc_state > USB_STATE_NOTATTACHED) {		
		/* s3c_udc_disable() set dev->udc_state = USB_STATE_NOTATTACHED; */
		s3c_udc_disable(dev);

S3C_UDC_POWER_OFF:		
		if(dev->clocked) {
			if (!IS_ERR(otg_clock) && otg_clock != NULL) {
				clk_disable(otg_clock);
				dev->clocked = 0;
				DEBUG_PM("[%s] clk_disable() OK.\n", __func__);
			}
			else
				DEBUG_ERROR("[%s] otg_clock error\n", __func__);
		}
		else 
			DEBUG_PM("[%s] already clk_disabled\n", __func__);
		
		fsa9480_s3c_udc_off();

		if(dev->clocked) {
			/*	Just in case */
				DEBUG_ERROR("\n[%s] readl(S3C_UDC_OTG_GINTMSK) || dev->clocked\n", __func__);
				DEBUG_ERROR("\n[%s] Power OFF again.\n", __func__);
				goto S3C_UDC_POWER_OFF;
		}
		
		DEBUG_PM("[%s] \n", __func__);
	}
	else {		
		DEBUG_PM("[%s] skipped , already powered off\n", __func__);

		if(dev->clocked) {
			/*	Just in case */
			DEBUG_ERROR("\n[%s] readl(S3C_UDC_OTG_GINTMSK) || dev->clocked\n", __func__);
			DEBUG_ERROR("\n[%s] Power OFF again.\n", __func__);
			goto S3C_UDC_POWER_OFF;
		}
	}
	dev->powered = 0;
}
//---------------------------------------------------------------------------------------

/*
 * get frame number 
 */ 
static int s3c_udc_get_frame(struct usb_gadget *_gadget)
{
	/*fram count number [21:8]*/
	u32 frame = readl(S3C_UDC_OTG_DSTS);

	DEBUG("[%s]: %s\n", __func__, _gadget->name);
	return (frame & 0x3ff00);
}
//---------------------------------------------------------------------------------------

/*
 * This function is called when the SRP timer expires.	The SRP should
 * complete within 6 seconds. 
 */
static void s3c_udc_srp_timeout(unsigned long _ptr )
{
	struct s3c_udc *dev = the_controller;
	struct timer_list *srp_timer = &dev->srp_timer;
	u32 usb_otgctl;	
	
	DEBUG_SETUP("[%s] SRP Timeout \n", __func__);		

	/*
		Currently (2009.6.22) OTG not supported
		Originally GOTGINT.SesReqSucStsChng interrupt handler should read 
		GOTGCTL.SesReqScs
		ref. Synopsis Spec. 5.3.2.2 OTG Interrupt Register(GOTGINT)
	*/
	usb_otgctl = readl(S3C_UDC_OTG_GOTGCTL);
	if (usb_otgctl & B_SESSION_VALID &&
		usb_otgctl & SESSION_REQ_SUCCESS)
	{
		DEBUG_SETUP("[%s] SRP Success \n", __func__);		
		spin_unlock(&dev->lock);
		dev->driver->resume(&dev->gadget);
		spin_lock(&dev->lock);
	
		del_timer(srp_timer);		
	}
	else
	{
		DEBUG_SETUP("[%s] Device not connected/responding \n", __func__);		
	}
	
	usb_otgctl &= ~SESSION_REQ;
	writel(usb_otgctl, S3C_UDC_OTG_GOTGCTL);
}

/*
 * usb_gadget_ops wake-up 
 * This function starts the Protocol if no session is in progress. If
 * a session is already in progress, but the device is suspended,
 * remote wakeup signaling is started.
 */ 
static int s3c_udc_wakeup(struct usb_gadget *_gadget)
{
	struct s3c_udc *dev = the_controller;
	struct timer_list *srp_timer = &dev->srp_timer;
	
	u32 usb_otgctl, usb_dctl, usb_status;	
	unsigned long flags;
	int ret = -EINVAL;
	if (!_gadget)
	{
		return -ENODEV;
	} 
	DEBUG("[%s]: %s\n", __func__, _gadget->name);
 
	spin_lock_irqsave(&dev->lock, flags);
	if (!(dev->devstatus & (1 << USB_DEVICE_REMOTE_WAKEUP))) 
	{
		DEBUG_ERROR("[%s]Not set USB_DEVICE_REMOTE_WAKEUP \n", __func__);		
		goto s3c_udc_wakeup_exit;
	}
	
	/* Check if valid session */
	usb_otgctl = readl(S3C_UDC_OTG_GOTGCTL);
	if (usb_otgctl & B_SESSION_VALID)
	{
		/* Check if suspend state */
		usb_status = readl(S3C_UDC_OTG_DSTS);
		if (usb_status & (1<<SUSPEND_STS)) 
		{
			//sending remote wake up signaling			
			DEBUG("[%s]Set Remote Wakeup \n", __func__);		
			usb_dctl = readl(S3C_UDC_OTG_DCTL);
			usb_dctl |= REMOTE_WAKEUP_SIG;
			writel(usb_dctl, S3C_UDC_OTG_DCTL);
			mdelay(1);
			
			DEBUG_SETUP("[%s]Clear Remote Wakeup \n", __func__);		
			usb_dctl = readl(S3C_UDC_OTG_DCTL);
			usb_dctl = usb_dctl & ~REMOTE_WAKEUP_SIG;
			writel(usb_dctl, S3C_UDC_OTG_DCTL);
		}
		else
			DEBUG_SETUP("[%s] Already woked up \n", __func__); 				
	}
	else if (usb_otgctl & SESSION_REQ)
	{
		DEBUG_SETUP("[%s] Session Request Already active! \n", __func__);		
		goto s3c_udc_wakeup_exit;	
	}
	else
	{
		DEBUG_SETUP("[%s] Initiate Session Request  \n", __func__);		
	//The core clears this bit when the HstNegSucStsChng bit is cleared.
		usb_otgctl |= SESSION_REQ;
		writel(usb_otgctl, S3C_UDC_OTG_GOTGCTL);
		
		/* Start the SRP timer */
		init_timer(srp_timer);
		srp_timer->function = (void*)s3c_udc_srp_timeout;
		srp_timer->expires = jiffies + (HZ*6);
		add_timer(srp_timer);
	}
	ret = 0;

s3c_udc_wakeup_exit:	
	spin_unlock_irqrestore(&dev->lock, flags);
	return ret;
}
//---------------------------------------------------------------------------------------

/*
 * usb_gadget_ops pullup
 */ 
static int s3c_udc_pullup(struct usb_gadget *gadget, int is_on)
{
#if 0
	//logical 
	if (is_on)
	{
		s3c_udc_soft_connect();
	}
	else
	{
		s3c_udc_soft_disconnect();
	}
#else
	struct s3c_udc *dev = the_controller;
	unsigned long flags;

	//UDC power on/off
	if (is_on)
	{
	/*
	 * if early s3c_udc_power_up make 
	 * fsa9480_check_usb_connection failed to detect USB connection
	 */ 
	//	s3c_udc_power_up();

		fsa9480_check_usb_connection();
	}
	else
	{
		/*
		 * Any transactions on the AHB Master are terminated. 
		 * And all the transmit FIFOs are flushed.
		 */
		writel(CORE_SOFT_RESET, S3C_UDC_OTG_GRSTCTL);
		udelay(10);
		if (!(readl(S3C_UDC_OTG_GRSTCTL) & AHB_MASTER_IDLE))
			printk("OTG Core Reset is not done.\n");
	
		spin_lock_irqsave(&dev->lock, flags);
		s3c_udc_stop_activity(dev, dev->driver);
		s3c_udc_power_down();
		spin_unlock_irqrestore(&dev->lock, flags);
	}
#endif
	return 0;
}
//---------------------------------------------------------------------------------------

static int s3c_udc_set_selfpowered(struct usb_gadget *gadget, int is_selfpowered)
{
	struct s3c_udc *dev = the_controller;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (is_selfpowered)
		dev->devstatus |= 1 << USB_DEVICE_SELF_POWERED;
	else
		dev->devstatus &= ~(1 << USB_DEVICE_SELF_POWERED);
	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/**
 * struct usb_gadget_ops s3c_udc_ops
 * 
 */
static const struct usb_gadget_ops s3c_udc_ops = {
	.get_frame = s3c_udc_get_frame,
	.wakeup = s3c_udc_wakeup,
	.set_selfpowered = s3c_udc_set_selfpowered,
	.pullup = s3c_udc_pullup,
};
//---------------------------------------------------------------------------------------

/*
 * dev release
 */ 
static void nop_release(struct device *dev)
{
	DEBUG("%s %s\n", __func__, dev->bus_id);
}
//---------------------------------------------------------------------------------------

/**
 * struct s3c_udc memory
 * 
 */
static struct s3c_udc memory = {
	.usb_address = 0,

	.gadget = {
		   .ops = &s3c_udc_ops,
		   .ep0 = &memory.ep[0].ep,
		   .name = driver_name,
		   .dev = {
			   //.bus_id = "gadget",	for 2.6.27
		   	   .init_name = "gadget",
	   /*
		* release function releases the Gadget device.
		* required by device_unregister().
		* currently no need
		*/
			   .release = nop_release,
			   },
		   },
	/*
	 * usb_ep.maxpacket will be refered by ep_matches() 
	 * of epautoconf.c
	 */
	
	/* control endpoint */
	.ep[0] = {
		  .ep = {
			 .name = ep0name,
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP0_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 0,
		  .bmAttributes = USB_ENDPOINT_XFER_CONTROL,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP0_FIFO,
		  },

	/* first group of endpoints */
	.ep[1] = {
		  .ep = {
			 .name = "ep1-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 1,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP1_FIFO,
		  },

	.ep[2] = {
		  .ep = {
			 .name = "ep2-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 2,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP2_FIFO,
		  },

	.ep[3] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep3-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 3,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP3_FIFO,
		  },
		  
	.ep[4] = {
		  .ep = {
			 .name = "ep4-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 4,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP4_FIFO,
		  },

	.ep[5] = {
		  .ep = {
			 .name = "ep5-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 5,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP5_FIFO,
		  },

	.ep[6] = {				 
		  .ep = {
			 .name = "ep6-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 6,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP6_FIFO,
		  },
		  
	.ep[7] = {				 
		  .ep = {
			 .name = "ep7-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 7,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP7_FIFO,
		  },
		  
	.ep[8] = {				 
		  .ep = {
			 .name = "ep8-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = BULK_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
		  
	.ep[9] = {				 
		  .ep = {
			 .name = "ep9-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
//following eps are not used		  
	.ep[10] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep10-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
	.ep[11] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep11-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
	.ep[12] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep12-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
	.ep[13] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep13-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
	.ep[14] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep14-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
	.ep[15] = {				// Though NOT USED XXX
		  .ep = {
			 .name = "ep15-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = INT_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
};
//---------------------------------------------------------------------------------------

/*
 * 	probe - binds to the platform device
 */
static int s3c_udc_probe(struct platform_device *pdev)
{
	struct s3c_udc *dev = &memory;
	int retval;

	DEBUG("%s: %p\n", __func__, pdev);

	retval = i2c_add_driver(&fsa9480_i2c_driver);
	if (retval != 0)
		DEBUG_ERROR("[USB Switch] can't add i2c driver");

	spin_lock_init(&dev->lock);
	dev->dev = pdev;

	device_initialize(&dev->gadget.dev);
	dev->gadget.dev.parent = &pdev->dev;

	dev->gadget.is_dualspeed = 1;	// Hack only
	dev->gadget.is_otg = 0;
	dev->gadget.is_a_peripheral = 0;
	dev->gadget.b_hnp_enable = 0;
	dev->gadget.a_hnp_support = 0;
	dev->gadget.a_alt_hnp_support = 0;
	dev->udc_state = USB_STATE_NOTATTACHED;
	dev->config_gadget_driver = NO_GADGET_DRIVER;
	dev->clocked = 0;
	dev->powered = 0;
	
	the_controller = dev;
	platform_set_drvdata(pdev, dev);

	otg_clock = clk_get(&pdev->dev, "otg");

	if (IS_ERR(otg_clock)) {
		DEBUG_ERROR(KERN_INFO "failed to find otg clock source\n");
		return -ENOENT;
	}

#ifndef CONFIG_PM
	clk_enable(otg_clock);
#endif

	s3c_ep_list_reinit(dev);

	local_irq_disable();

	/* irq setup after old hardware state is cleaned up */
	retval =  request_irq(IRQ_OTG, s3c_udc_irq, 0, driver_name, dev);

	if (retval != 0) {
		DEBUG(KERN_ERR "%s: can't get irq %i, err %d\n", driver_name,
		      IRQ_OTG, retval);
		return -EBUSY;
	}

	disable_irq(IRQ_OTG);
	local_irq_enable();
	create_proc_files();

	//it just prints which file included regarding whether DMA mode or SLAVE mode
	s3c_show_mode();

	
#if TESTING_SOFT_CONNCTION
	soft_switch.name = SOFT_SWITCH_NAME;
	soft_switch.print_name = soft_switch_name;
	soft_switch.print_state = soft_switch_state;
	soft_switch.state = 0;
	if (switch_dev_register(&soft_switch) < 0)
		switch_dev_unregister(&soft_switch);
#endif

	return retval;
}
//---------------------------------------------------------------------------------------

/*
 * 	remove - unbinds to the platform device
 */
static int s3c_udc_remove(struct platform_device *pdev)
{
	struct s3c_udc *dev = platform_get_drvdata(pdev);

	DEBUG("%s: %p\n", __func__, pdev);

	if (otg_clock != NULL) {
		clk_disable(otg_clock);
		clk_put(otg_clock);
		otg_clock = NULL;
	}

	remove_proc_files();
	usb_gadget_unregister_driver(dev->driver);

	free_irq(IRQ_OTG, dev);

	platform_set_drvdata(pdev, 0);

	the_controller = 0;

	return 0;
}
//---------------------------------------------------------------------------------------

/*
 * 	AHB clock gating for suspend
 */
int s3c_udc_suspend_clock_gating(void)
{
	u32	uReg;
	DEBUG_PM("[%s]\n", __func__);
	uReg = readl(S3C_UDC_OTG_PCGCCTL);

	writel(uReg|1<<STOP_PCLK_BIT|1<<GATE_HCLK_BIT, S3C_UDC_OTG_PCGCCTL); 
	DEBUG_PM("[%s] : S3C_UDC_OTG_PCGCCTL 0x%x \n", __func__, uReg);	
	return 0;
}
//---------------------------------------------------------------------------------------

/*
 * 	AHB clock gating for resume
 */
int s3c_udc_resume_clock_gating(void)
{
	u32	uReg;
	DEBUG_PM("[%s]\n", __func__);
	
	uReg = readl(S3C_UDC_OTG_PCGCCTL);
	uReg &= ~(1<<STOP_PCLK_BIT|1<<GATE_HCLK_BIT);
	writel(uReg, S3C_UDC_OTG_PCGCCTL); 
	DEBUG_PM("[%s] : S3C_UDC_OTG_PCGCCTL 0x%x \n", __func__, uReg);	
	return 0;
}
//---------------------------------------------------------------------------------------

#ifdef CONFIG_PM

/*
 *	Synopsys OTG PM supports Partial Power-Down and AHB Clock Gating.
 *	OTG PM just turns on or off OTG PHYPWR 
 *	because S3C6410 can only use the clock gating method.		 
 */
enum OTG_PM
{
	ALL_POWER_DOWN,    		//turn off OTG module

	//the followings are not tested yet
	CLOCK_GATING,			//using AHB clock gating,
	OPHYPWR_FORCE_SUSPEND,	//using OPHYPWR.force_suspend
}; 
//---------------------------------------------------------------------------------------

//specifying pm_policy
static enum	OTG_PM pm_policy = ALL_POWER_DOWN;

static int s3c_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_udc *dev = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);

	DEBUG_PM("[%s]: System Suspend \n", __func__);

	//save the state of connection to udc_resume_state
	dev->udc_resume_state = dev->udc_state;
	dev->udc_resume_speed = dev->gadget.speed;

	if (dev->gadget.speed != USB_SPEED_UNKNOWN) {
		DEBUG_PM("[%s]: USB enumerated at suspend\n", __func__);
		//s3c_udc_soft_disconnect();		
		s3c_udc_stop_activity(dev, dev->driver);
	}
	else {
		DEBUG_PM("[%s] USB not enumerated at suspend\n", __func__);
		DEBUG_PM("[%s] dev->powered [%d] at suspend\n", __func__, dev->powered);
		if (dev->powered == 0) {
			DEBUG_PM("[%s] skip~~ s3c_udc_power_down() at suspend\n", __func__);
			spin_unlock_irqrestore(&dev->lock, flags);
		return 0;
	}
	}

	switch(pm_policy)
	{
		case ALL_POWER_DOWN:			
//			s3c_udc_stop_activity(dev, dev->driver);
			//confirm clk disable
			s3c_udc_power_down();
			break;
		case CLOCK_GATING: 		
			s3c_udc_suspend_clock_gating();
			break;
		case OPHYPWR_FORCE_SUSPEND:			
			writel((readl(S3C_USBOTG_PHYPWR)|(1<<FORCE_SUSPEND)), S3C_USBOTG_PHYPWR);
			break;
		default:
			DEBUG_ERROR("[%s]: not proper pm_policy\n", __func__);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return 0;
}
//---------------------------------------------------------------------------------------

static int s3c_udc_resume(struct platform_device *pdev)
{
#if 1
	//first check status of connection
	DEBUG_PM("[%s]: fsa9480_check_usb_connection\n", __func__);
	fsa9480_check_usb_connection();
	return 0;

#else
//	chekc udc_resume_speed & udc_resume_state
	struct s3c_udc *dev = platform_get_drvdata(pdev);
	u32 tmp;
	
	DEBUG_PM("[%s]: System Resume \n", __func__);
	//if not suspended as connected
	if (dev->udc_resume_state == USB_STATE_CONFIGURED)
	{
		DEBUG_PM("[%s]: USB connected before suspend \n", __func__);
	}
	else
	{	
		DEBUG_PM("[%s]: USB not connected before suspend\n", __func__);
		return 0;
	}
	
	switch(pm_policy)
	{
		case ALL_POWER_DOWN:			
			s3c_udc_power_up();
			break;
		case CLOCK_GATING:		
			s3c_udc_resume_clock_gating();
			break;
		case OPHYPWR_FORCE_SUSPEND: 		
			tmp = readl(S3C_USBOTG_PHYPWR);
			tmp &= ~(1<<FORCE_SUSPEND);
			writel(tmp, S3C_USBOTG_PHYPWR);
			break;
		default:
			DEBUG_ERROR("[%s]: not proper pm_policy\n", __func__);
	}
	return 0;
#endif
}

#endif /* CONFIG_PM */

/**
 * struct platform_driver s3c_udc_driver
 */
static struct platform_driver s3c_udc_driver = {
	.probe		= s3c_udc_probe,
	.remove 	= s3c_udc_remove,
	.driver 	= {
		.owner	= THIS_MODULE,
		.name	= "s3c6410-usbgadget",
	},
#ifdef CONFIG_PM
	.suspend	= s3c_udc_suspend,
	.resume 	= s3c_udc_resume,
#endif /* CONFIG_PM */
};
//---------------------------------------------------------------------------------------

static int __init s3c_udc_init(void)
{
	int ret;

	ret = platform_driver_register(&s3c_udc_driver);

	return ret;
}
//---------------------------------------------------------------------------------------

static void __exit s3c_udc_exit(void)
{
	platform_driver_unregister(&s3c_udc_driver);
	DEBUG("Unloaded %s version %s\n", driver_name, DRIVER_VERSION);
}
//---------------------------------------------------------------------------------------

module_init(s3c_udc_init);
module_exit(s3c_udc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("SeungSoo Yange");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

