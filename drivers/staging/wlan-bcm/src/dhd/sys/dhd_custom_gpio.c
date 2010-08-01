/*
* Customer code to add GPIO control during WLAN start/stop
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
* $Id: dhd_custom_gpio.c,v 1.1.2.2 2009/04/13 07:39:23 Exp $
*/

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <bcmutils.h>


#include <wlioctl.h>
#include <wl_iw.h>

#define WL_ERROR(x) printf x

/* Customer specific function to insert/remove wlan reset gpio pin */
void
dhd_customer_gpio_wlan_reset(bool onoff)
{
	if (onoff == G_WLAN_SET_OFF) {
		WL_ERROR(("%s: call customer specific GPIO to insert WLAN RESET\n", __FUNCTION__));
		WL_ERROR(("=========== WLAN placed in RESET ========\n"));
	}
	else {
		WL_ERROR(("%s: callc customer specific GPIO to remove WLAN RESET\n", __FUNCTION__));
		WL_ERROR(("=========== WLAN goin back to live  ========\n"));
	}
}
