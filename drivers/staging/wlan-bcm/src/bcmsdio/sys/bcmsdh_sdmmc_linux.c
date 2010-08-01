/*
 * BCMSDH Function Driver for the native SDIO/MMC driver in the Linux Kernel
 *
 * Copyright (C) 1999-2009, Broadcom Corporation
 * 
 *         Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsdh_sdmmc_linux.c,v 1.1.2.5.20.8 2009/07/25 03:50:48 Exp $
 */

#include <typedefs.h>
#include <bcmutils.h>
#include <sdio.h>	/* SDIO Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */

#include <linux/sched.h>	/* request_irq() */

#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <dhd_dbg.h>

#if !defined(SDIO_VENDOR_ID_BROADCOM)
#define SDIO_VENDOR_ID_BROADCOM		0x02d0
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4325) */
#if !defined(SDIO_DEVICE_ID_BROADCOM_4325)
#define SDIO_DEVICE_ID_BROADCOM_4325	0x0000
#endif /* !defined(SDIO_DEVICE_ID_BROADCOM_4325) */

#ifdef CONFIG_BRCM_GPIO_INTR
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/device.h>

#ifdef BCM_HOSTWAKE
int dhd_suspend_context = FALSE;
extern void register_mmc_card_pm(struct mmc_card_pm *);
extern void unregister_mmc_card_pm(void);
int bcmsdh_sdmmc_resume(void);
int bcmsdh_sdmmc_suspend(void);

struct mmc_card_pm wifi_pm = {
	.suspend = bcmsdh_sdmmc_suspend,
	.resume  = bcmsdh_sdmmc_resume,
};

static int dhd_resume (void);
static int dhd_suspend (void);
#endif //BCM_HOSTWAKE
#endif

#include <bcmsdh_sdmmc.h>

extern void sdioh_sdmmc_devintr_off(sdioh_info_t *sd);
extern void sdioh_sdmmc_devintr_on(sdioh_info_t *sd);

extern int  dhdsdio_bussleep_hack(void *, bool sleep);
extern int dhdsdio_bussleep(void *bus, bool sleep);
extern bool dhdsdio_dpc(void *bus);
extern int dhd_os_proto_block(void *pub);
extern int dhd_os_proto_unblock(void * pub);
extern void *dhd_get_dhd_pub ( void );
extern void *dhd_get_dhd_bus_sdh ( void );

int sdio_function_init(void);
void sdio_function_cleanup(void);

#ifdef ANDROID_SPECIFIC

#ifdef CONFIG_MACH_SPICA
#include <mach/spica.h>
#elif CONFIG_MACH_SATURN
#include <mach/saturn.h>
#elif CONFIG_MACH_CYGNUS
#include <mach/cygnus.h>
#elif CONFIG_MACH_INSTINCTQ
#include <mach/instinctq.h>
#elif CONFIG_MACH_BONANZA
#include <mach/bonanza.h>
#endif
  
#include <linux/wakelock.h>
extern int gpio_wlan_poweron(void);
extern int gpio_wlan_poweroff(void);
extern void gpio_regon_lock_init(void);
struct wake_lock wlan_host_wakelock; 
struct wake_lock wlan_host_wakelock_resume;
#endif
#define DESCRIPTION "bcmsdh_sdmmc Driver"
#define AUTHOR "Broadcom Corporation"

/* module param defaults */
static int clockoverride = 0;

module_param(clockoverride, int, 0644);
MODULE_PARM_DESC(clockoverride, "SDIO card clock override");

PBCMSDH_SDMMC_INSTANCE gInstance;

/* Maximum number of bcmsdh_sdmmc devices supported by driver */
#define BCMSDH_SDMMC_MAX_DEVICES 1

extern int bcmsdh_probe(struct device *dev);
extern int bcmsdh_remove(struct device *dev);
struct device sdmmc_dev;
extern struct dhd_bus *dhd_pub_global;

#ifdef BCM_HOSTWAKE
int dhd_register_hwakeup(void);
void dhd_unregister_hwakeup(void);

DECLARE_WAIT_QUEUE_HEAD(bussleep_wake);
typedef struct dhd_mmc_suspend {
	bool drv_loaded;
	int wait_driver_load; /* waiting for driver loaded */
	bool skip;
	bool state;
	unsigned int wifiirq;
} dhd_mmc_suspend_t; 
dhd_mmc_suspend_t dhd_mmc_suspend_ctrl = { 0,0,0,0 };

#endif
static int bcmsdh_sdmmc_probe(struct sdio_func *func,
                              const struct sdio_device_id *id)
{
	int ret = 0;
	sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
	sd_trace(("sdio_bcmsdh: func->class=%x\n", func->class));
	sd_trace(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_trace(("sdio_device: 0x%04x\n", func->device));
	sd_trace(("Function#: 0x%04x\n", func->num));

	if (func->num == 1) {
		/* Keep a copy of F1's 'func' in F0, just in case... */
		gInstance->func[0] = func;
		if(func->device == 0x4) { /* 4318 */
			gInstance->func[2] = NULL;
			sd_trace(("NIC found, calling bcmsdh_probe...\n"));
			bcmsdh_probe(&sdmmc_dev);
		}
	}

	gInstance->func[func->num] = func;

	if (func->num == 2) {
		sd_trace(("F2 found, calling bcmsdh_probe...\n"));
		bcmsdh_probe(&sdmmc_dev);
	}

#ifdef ANDROID_SPECIFIC
	gpio_regon_lock_init();
#endif

	return ret;
}
#ifdef BCM_ARPO
extern int dhd_config_arp_offload(struct dhd_bus *bus, bool flag);
#endif

#if defined(BCM_HOSTWAKE) && defined(BCM_PKTFILTER)
extern int dhdsdio_enable_filters(struct dhd_bus *);
extern int dhdsdio_disable_filters(struct dhd_bus *);
#endif

#ifdef BCM_HOSTWAKE
extern int del_wl_timers(void);
int bcmsdh_sdmmc_suspend(void)
{
	DHD_TRACE((KERN_DEBUG "[WIFI] %s: Enter \n\n", __FUNCTION__));

	dhd_suspend_context = TRUE;

	/* If chip active is done, do put the device to suspend */
	del_wl_timers();

#ifdef BCM_ARPO
	/*Enable ARP Offloading*/
	dhd_config_arp_offload(dhd_pub_global, TRUE);
#endif

#if defined BCM_PKTFILTER
	dhdsdio_enable_filters(dhd_pub_global);
#endif

	if(dhd_suspend() < 0)
		return -1;

#ifdef BCMHOSTWAKE_IRQ
	enable_irq(dhd_mmc_suspend_ctrl.wifiirq);
#endif
	return 0;
		
}

int bcmsdh_sdmmc_resume(void)
{

	DHD_TRACE((KERN_DEBUG "[WIFI] %s: Enter \n\n", __FUNCTION__));

#ifdef BCMHOSTWAKE_IRQ
	disable_irq(dhd_mmc_suspend_ctrl.wifiirq);
#endif	
	/*Do the resume operations*/
        dhd_resume();

#ifdef BCM_ARPO
	/*DiSable ARP Offloading*/
	dhd_config_arp_offload(dhd_pub_global, FALSE);
#endif

#if defined BCM_PKTFILTER
	dhdsdio_disable_filters(dhd_pub_global);
#endif
	return 0;
		
}
#endif //BCM_HOSTWAKE

static void bcmsdh_sdmmc_remove(struct sdio_func *func)
{
	sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));
	sd_info(("sdio_bcmsdh: func->class=%x\n", func->class));
	sd_info(("sdio_vendor: 0x%04x\n", func->vendor));
	sd_info(("sdio_device: 0x%04x\n", func->device));
	sd_info(("Function#: 0x%04x\n", func->num));

	if (func->num == 2) {
		sd_trace(("F2 found, calling bcmsdh_remove...\n"));
		bcmsdh_remove(&sdmmc_dev);
	}
}


/* devices we support, null terminated */
static const struct sdio_device_id bcmsdh_sdmmc_ids[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_BROADCOM, SDIO_DEVICE_ID_BROADCOM_4325) },
	{ SDIO_DEVICE_CLASS(SDIO_CLASS_NONE)		},
	{ /* end: all zeroes */				},
};

MODULE_DEVICE_TABLE(sdio, bcmsdh_sdmmc_ids);

static struct sdio_driver bcmsdh_sdmmc_driver = {
	.probe		= bcmsdh_sdmmc_probe,
	.remove		= bcmsdh_sdmmc_remove,
	.name		= "bcmsdh_sdmmc",
	.id_table	= bcmsdh_sdmmc_ids,
	};

struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};


int
sdioh_sdmmc_osinit(sdioh_info_t *sd)
{
	struct sdos_info *sdos;

	sdos = (struct sdos_info*)MALLOC(sd->osh, sizeof(struct sdos_info));
	sd->sdos_info = (void*)sdos;
	if (sdos == NULL)
		return BCME_NOMEM;

	sdos->sd = sd;
	spin_lock_init(&sdos->lock);
	return BCME_OK;
}

void
sdioh_sdmmc_osfree(sdioh_info_t *sd)
{
	struct sdos_info *sdos;
	ASSERT(sd && sd->sdos_info);

	sdos = (struct sdos_info *)sd->sdos_info;
	MFREE(sd->osh, sdos, sizeof(struct sdos_info));
}

static int
dhd_lock_dhd_bus(void)
{
	dhd_os_proto_block(dhd_get_dhd_pub());

	return 0;
}

int
dhd_unlock_dhd_bus(void)
{
	dhd_os_proto_unblock(dhd_get_dhd_pub());

	return 0;
}

bool
dhd_mmc_suspend_state(void)
{
	return dhd_mmc_suspend_ctrl.state;
}

static int dhd_suspend (void)
{
	int bus_state;
	int max_tries = 3;
	int gpio = 0;
	
    	printk(KERN_DEBUG "[WIFI] %s: SUSPEND Enter\n", __FUNCTION__);

	if ( NULL != dhd_pub_global ) {
		

		dhd_lock_dhd_bus();	
		do {
			bus_state = dhdsdio_bussleep(dhd_pub_global, TRUE);
			if ( bus_state == BCME_BUSY)
			{
#if 1
				/* 250ms timeout */
				wait_event_timeout(bussleep_wake, FALSE, HZ/4);
				printk(KERN_ERR "%s: BUS BUSY so trying again \n",__FUNCTION__);
#else

				/* 50ms timeout */
				wait_event_timeout(bussleep_wake, FALSE, HZ/20);
				printk("%s: BUS BUSY so trying again \n",__FUNCTION__);

#endif
			}
		} while (( bus_state == BCME_BUSY) && (max_tries-- > 0) );

		if(max_tries <= 0)
		{
			printk(KERN_ERR "[WIFI] BUS BUSY!! Couldn't sleep.\n");		
			dhd_unlock_dhd_bus();

			/* This value should be returned to mmc_suspend*/
			return -1;
		}

		/* Mark that we entered suspend state */
		dhd_mmc_suspend_ctrl.state = TRUE;

		gpio = gpio_get_value(GPIO_WLAN_HOST_WAKE);
	
		printk("[WIFI] %s: SUSPEND Done. BusState->%d gpio->%d \n\n", __FUNCTION__, bus_state, gpio);
	} else {

		printk("%s dhd_pub_global is NULL!! \n", __FUNCTION__);
	}


    return 0;
}
static int dhd_resume (void)
{
	 struct dhd_bus *dhd_pub_local = (struct dhd_bus *)dhd_pub_global;
	int gpio = 0;
    
	printk(KERN_DEBUG "[WIFI] %s: Enter\n", __FUNCTION__);

	dhd_suspend_context = FALSE;

	if ( NULL != dhd_pub_local ) {

		dhd_mmc_suspend_ctrl.state = FALSE;

                wake_lock_timeout(&wlan_host_wakelock_resume, 2*HZ);

		dhdsdio_dpc(dhd_pub_local);
			
		dhd_unlock_dhd_bus();

		gpio = gpio_get_value(GPIO_WLAN_HOST_WAKE);

		printk("[WIFI] %s: RESUME Done. gpio->%d \n", __FUNCTION__, gpio);


    		/* dhdsdio_bussleep_hack(dhd_pub_global,0); */
	}
    else {
 		printk("%s dhd_pub_local is NULL!! \n", __FUNCTION__);

	}

	return 0;
}
/* Interrupt enable/disable */
SDIOH_API_RC
sdioh_interrupt_set(sdioh_info_t *sd, bool enable)
{
	ulong flags;
	struct sdos_info *sdos;

	sd_trace(("%s: %s\n", __FUNCTION__, enable ? "Enabling" : "Disabling"));

	sdos = (struct sdos_info *)sd->sdos_info;
	ASSERT(sdos);

	if (enable && !(sd->intr_handler && sd->intr_handler_arg)) {
		sd_err(("%s: no handler registered, will not enable\n", __FUNCTION__));
		return SDIOH_API_RC_FAIL;
	}

	/* Ensure atomicity for enable/disable calls */
	spin_lock_irqsave(&sdos->lock, flags);

	sd->client_intr_enabled = enable;
	if (enable) {
		sdioh_sdmmc_devintr_on(sd);
	} else {
		sdioh_sdmmc_devintr_off(sd);
	}

	spin_unlock_irqrestore(&sdos->lock, flags);

	return SDIOH_API_RC_SUCCESS;
}


#ifdef BCMSDH_MODULE
static int __init
bcmsdh_module_init(void)
{
	int error = 0;
	sdio_function_init();
	return error;
}

static void __exit
bcmsdh_module_cleanup(void)
{
	sdio_function_cleanup();
}

module_init(bcmsdh_module_init);
module_exit(bcmsdh_module_cleanup);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_AUTHOR(AUTHOR);

#endif /* BCMSDH_MODULE */
/*
 * module init
*/
int sdio_function_init(void)
{
	int error = 0;
	sd_trace(("bcmsdh_sdmmc: %s Enter\n", __FUNCTION__));

#ifdef ANDROID_SPECIFIC    
	error = gpio_wlan_poweron();
	if (error)
	    return error;
#endif	

	gInstance = kzalloc(sizeof(BCMSDH_SDMMC_INSTANCE), GFP_KERNEL);
	if (!gInstance)
		return -ENOMEM;

	error = sdio_register_driver(&bcmsdh_sdmmc_driver);


#ifdef BCM_HOSTWAKE
	dhd_mmc_suspend_ctrl.drv_loaded = TRUE;
	dhd_mmc_suspend_ctrl.wait_driver_load = jiffies;

#ifdef CONFIG_BRCM_GPIO_INTR
	/* HostWake up */
	dhd_register_hwakeup();
#endif
	register_mmc_card_pm(&wifi_pm);
#endif
	return error;
}

/*
 * module cleanup
*/
extern int bcmsdh_remove(struct device *dev);
void sdio_function_cleanup(void)
{       
        int error = 0;
	sd_trace(("%s Enter\n", __FUNCTION__));


#ifdef BCM_HOSTWAKE
	/* HostWake up */
	dhd_unregister_hwakeup();

	unregister_mmc_card_pm();

	/* Destroy the wake lock*/
	wake_lock_destroy(&wlan_host_wakelock);
	wake_lock_destroy(&wlan_host_wakelock_resume);

#endif

	sdio_unregister_driver(&bcmsdh_sdmmc_driver);
	
#ifdef ANDROID_SPECIFIC    
	error = gpio_wlan_poweroff();
#endif	
	if (gInstance)
		kfree(gInstance);
}


#ifdef CONFIG_BRCM_GPIO_INTR


//#define GPIO_WLAN_HOST_WAKE 0 // This value is defined in the Kernel header file.


struct dhd_wifisleep_info {
	unsigned host_wake;
	unsigned host_wake_irq;
};

static struct dhd_wifisleep_info *dhd_wifi_sleep;

/**
 * Supposed that Early Suspend/Resume is disable
 */
static int dhd_enable_hwakeup(void)
{
	int ret;

	ret = set_irq_wake(dhd_wifi_sleep->host_wake_irq,1);

	if (ret < 0) {
		DHD_ERROR(("Couldn't enable WLAN_HOST_WAKE as wakeup interrupt"));
		free_irq(dhd_wifi_sleep->host_wake_irq, NULL);
	}

	return ret;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void
dhd_disable_hwakeup(void)
{

	if (set_irq_wake(dhd_wifi_sleep->host_wake_irq, 0))
		DHD_ERROR(("Couldn't disable hostwake IRQ wakeup mode\n"));
}


/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t
dhd_hostwakeup_isr(int irq, void *dev_id)
{
	int gpio = 0;

	gpio = gpio_get_value(GPIO_WLAN_HOST_WAKE);
	
	printk(KERN_INFO "[WIFI] %s: gpio-> %d \n ",__FUNCTION__, gpio);

	/*
	 * WL_HOST_WAKE is configured as Pulse output signal.
	 * so gpio = GPIO_LEVEL_HIGH indicates that host wake is asserted
	 * and GPIO_LEVEL_LOW indicates that it is de-asserted. However, since
	 * asserting happens duing HOST sleep state, the prints within isr won't
	 * come.
	*/

	/* This ISR is just for notification purpose, the resume operation would
         * be done in MMC resume context */

	return IRQ_HANDLED;
}

/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
int
dhd_register_hwakeup(void)
{
	int ret;

	dhd_wifi_sleep = kzalloc(sizeof(struct dhd_wifisleep_info), GFP_KERNEL);
	if (!dhd_wifi_sleep)
		return -ENOMEM;

	dhd_wifi_sleep->host_wake = GPIO_WLAN_HOST_WAKE;

	/* wake lock initialize */
   	wake_lock_init(&wlan_host_wakelock, WAKE_LOCK_SUSPEND, "WLAN_HOST_WAKE");
   	wake_lock_init(&wlan_host_wakelock_resume, WAKE_LOCK_SUSPEND, "WLAN_HOST_WAKE_RESUME");

#if 0 // GPIO config already done in mach-instinctq.c

	ret = gpio_request(dhd_wifi_sleep->host_wake, "wifi_hostwakeup");
	if (ret < 0) {
		DHD_ERROR(("[WiFi] Failed to get gpio_request \n"));
		gpio_free(dhd_wifi_sleep->host_wake);
		return 0;
	}

	ret = gpio_direction_input(dhd_wifi_sleep->host_wake);
	if (ret < 0) {
		DHD_ERROR(("[WiFi] Failed to get direction  \n"));
		return 0;
	}
#endif

	 /*
	dhd_wifi_sleep->host_wake_irq= gpio_to_irq (dhd_wifi_sleep->host_wake);
	*/

	/* External Interrupt Line is 1 for 6410 platform */
	dhd_wifi_sleep->host_wake_irq= IRQ_EINT(1);


	if ( dhd_wifi_sleep->host_wake_irq  < 0 ) {
		DHD_ERROR(("[WiFi] Failed to get irq  \n"));
		return 0;
	}

	set_irq_type(dhd_wifi_sleep->host_wake_irq, IRQ_TYPE_EDGE_BOTH);
	ret= request_irq(dhd_wifi_sleep->host_wake_irq, dhd_hostwakeup_isr, IRQF_DISABLED,
			"wifi_hostwakeup", NULL);
	if (ret) {
		DHD_ERROR(("[WiFi] Failed to get HostWakeUp IRQ \n"));
		free_irq(dhd_wifi_sleep->host_wake_irq, 0);
		return ret;
		/* To do */
	}
	else {
		DHD_INFO(("[WiFi] install HostWakeup IRQ \n"));
	}


#ifdef BCMHOSTWAKE_IRQ
        dhd_mmc_suspend_ctrl.wifiirq = dhd_wifi_sleep->host_wake_irq;

	disable_irq(dhd_wifi_sleep->host_wake_irq);
#endif
	return ret;
}

void
dhd_unregister_hwakeup(void)
{

	//dhd_disable_hwakeup();
    	
	free_irq(dhd_wifi_sleep->host_wake_irq, NULL);
	gpio_free(dhd_wifi_sleep->host_wake);
	kfree(dhd_wifi_sleep);
}
#endif /*  #ifdef CONFIG_BRCM_GPIO_INTR */
