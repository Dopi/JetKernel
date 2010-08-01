/* drivers
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung BCM4325 Bluetooth power sequence through GPIO of S3C6410.
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
 * Revision History
 * ===============
 * 0.0 Initial version WLAN power wakeup
 * 
 */
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>

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

#include <plat/devs.h>
#include <linux/spinlock.h>
#include <linux/mmc/host.h>

#include <linux/i2c/pmic.h>

#if defined(CONFIG_MACH_BONANZA)
#define GPIO_BT_WLAN_REG_ON GPIO_BT_WLAN_EN
#endif

uint flags = 0;
spinlock_t regon_lock = SPIN_LOCK_UNLOCKED;
static void s3c_WLAN_SDIO_on(void)
{
#if defined(CONFIG_MACH_INSTINCTQ)
	s3c_gpio_cfgpin(GPIO_SDIO_CLK, S3C_GPIO_SFN(GPIO_SDIO_CLK_AF));
	s3c_gpio_cfgpin(GPIO_SDIO_CMD, S3C_GPIO_SFN(GPIO_SDIO_CMD_AF));
#else
	s3c_gpio_cfgpin(GPIO_WLAN_CLK, S3C_GPIO_SFN(GPIO_WLAN_CLK_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_CMD, S3C_GPIO_SFN(GPIO_WLAN_CMD_AF));
#endif
	s3c_gpio_cfgpin(GPIO_WLAN_D_0, S3C_GPIO_SFN(GPIO_WLAN_D_0_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_D_1, S3C_GPIO_SFN(GPIO_WLAN_D_1_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_D_2, S3C_GPIO_SFN(GPIO_WLAN_D_2_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_D_3, S3C_GPIO_SFN(GPIO_WLAN_D_3_AF));
#if defined(CONFIG_MACH_INSTINCTQ)
	s3c_gpio_setpull(GPIO_SDIO_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_SDIO_CMD, S3C_GPIO_PULL_NONE);
#else
	s3c_gpio_setpull(GPIO_WLAN_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_CMD, S3C_GPIO_PULL_NONE);
#endif
	s3c_gpio_setpull(GPIO_WLAN_D_0, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_3, S3C_GPIO_PULL_NONE);
}

static void s3c_WLAN_SDIO_off(void)
{
#if defined(CONFIG_MACH_INSTINCTQ)
	s3c_gpio_cfgpin(GPIO_SDIO_CLK, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_SDIO_CMD, S3C_GPIO_INPUT);
#else
	s3c_gpio_cfgpin(GPIO_WLAN_CLK, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_CMD, S3C_GPIO_INPUT);
#endif
	s3c_gpio_cfgpin(GPIO_WLAN_D_0, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_D_1, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_D_2, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_WLAN_D_3, S3C_GPIO_INPUT);
#if defined(CONFIG_MACH_INSTINCTQ)
	s3c_gpio_setpull(GPIO_SDIO_CLK, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_SDIO_CMD, S3C_GPIO_PULL_NONE);
#else
	s3c_gpio_setpull(GPIO_WLAN_CLK, S3C_GPIO_PULL_DOWN);
	s3c_gpio_setpull(GPIO_WLAN_CMD, S3C_GPIO_PULL_NONE);
#endif
	s3c_gpio_setpull(GPIO_WLAN_D_0, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_WLAN_D_3, S3C_GPIO_PULL_NONE);
}
void gpio_regon_lock_init()
{
	spin_lock_init(&regon_lock);
}


int gpio_wlan_poweron (void)
{
	int ret=0;

	printk("[WIFI] Device powering ON\n");

	/* Enable sdio pins and configure it */
	s3c_WLAN_SDIO_on();
	/* Power on WLAN chip */ 
	if (gpio_is_valid(GPIO_WLAN_RST_N)) {
		ret = gpio_request(GPIO_WLAN_RST_N, S3C_GPIO_LAVEL(GPIO_WLAN_RST_N));
		if (ret < 0){
        		printk("%s: failed to request GPIO_WLAN_RST_N!\n", __FUNCTION__);
            		return ret;
        	}	
		gpio_direction_output(GPIO_WLAN_RST_N, GPIO_LEVEL_LOW);
	}

	if (gpio_is_valid(GPIO_BT_WLAN_REG_ON)) {
        ret = gpio_request(GPIO_BT_WLAN_REG_ON, S3C_GPIO_LAVEL(GPIO_BT_WLAN_REG_ON));
		if (ret < 0){
  	    		printk("%s: failed to request GPIO_BT_WLAN_REG_ON!\n", __FUNCTION__);
			gpio_free(GPIO_WLAN_RST_N);
			return ret;
        	}
		gpio_direction_output(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);
	}
        

/* 
 *	PROTECT this check under spinlock.. No other thread should be touching
 *	GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. 
 */
	spin_lock_irqsave(&regon_lock, flags);

	/* Set REG ON as High & configure it for sleep mode */
	gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_HIGH);
	
	s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT1);
	s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);
	
	printk("[WIFI] GPIO_BT_WLAN_REG_ON = %d \n", gpio_get_value(GPIO_BT_WLAN_REG_ON));

	msleep(150);

	/* Active low pin: Make it high for NO RESET & configure it for sleep mode */
	gpio_set_value(GPIO_WLAN_RST_N, GPIO_LEVEL_HIGH);
	
	s3c_gpio_slp_cfgpin(GPIO_WLAN_RST_N, S3C_GPIO_SLP_OUT1);
	s3c_gpio_slp_setpull_updown(GPIO_WLAN_RST_N, S3C_GPIO_PULL_NONE);
	
	printk("[WIFI] GPIO_WLAN_RST_N = %d \n", gpio_get_value(GPIO_WLAN_RST_N));

	spin_unlock_irqrestore(&regon_lock, flags);
        
	gpio_free(GPIO_BT_WLAN_REG_ON);
	gpio_free(GPIO_WLAN_RST_N);
    
	/* mmc_rescan*/    
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc2);

	return 0;
}

int gpio_wlan_poweroff (void)
{
	int ret=0;

	printk("[WIFI] Device powering OFF\n");

/*   
    PROTECT this check under spinlock.. No other thread should be touching
    GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. 
*/
	spin_lock_irqsave(&regon_lock, flags);

	/* Active Low: Assert Reset line unconditionally while turning off WIFI*/
	gpio_set_value(GPIO_WLAN_RST_N, GPIO_LEVEL_LOW);

	s3c_gpio_slp_cfgpin(GPIO_WLAN_RST_N, S3C_GPIO_SLP_OUT0);
	s3c_gpio_slp_setpull_updown(GPIO_WLAN_RST_N, S3C_GPIO_PULL_NONE);

	printk("[WIFI] GPIO_WLAN_RST_N = %d \n", gpio_get_value(GPIO_WLAN_RST_N));	
	
//	msleep(100);	
	
	if(gpio_get_value(GPIO_BT_RST_N) == 0)
	{
		/* Set REG ON as low, only if BT is not operational */
		gpio_set_value(GPIO_BT_WLAN_REG_ON, GPIO_LEVEL_LOW);

		s3c_gpio_slp_cfgpin(GPIO_BT_WLAN_REG_ON, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_BT_WLAN_REG_ON, S3C_GPIO_PULL_NONE);

		printk("[WIFI] GPIO_BT_WLAN_REG_ON = %d \n", gpio_get_value(GPIO_BT_WLAN_REG_ON));
	}	

	spin_unlock_irqrestore(&regon_lock, flags);

	/* Disable SDIO pins */
	s3c_WLAN_SDIO_off();

	gpio_free(GPIO_BT_WLAN_REG_ON);
	gpio_free(GPIO_WLAN_RST_N);
    
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc2);

	return 0;
}
	
EXPORT_SYMBOL_GPL(gpio_regon_lock_init);
EXPORT_SYMBOL_GPL(gpio_wlan_poweroff);
EXPORT_SYMBOL_GPL(gpio_wlan_poweron);
