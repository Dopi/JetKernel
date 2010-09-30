/*
 *  S3C64XX GPIO Driver
 *
 *  Copyright (C) 2008 - 2009 HHTECH.
 *
 *  Author: <wk@hhcn.com>
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
 * along with this progSoftwareram; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/gpio_keys.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/gpio.h>

#include <plat/gpio-cfg.h>

#include "smartq_gpio.h"


#if defined(CONFIG_LCD_4)
static unsigned int system_flag = 0;
#else
static unsigned int system_flag = 1;
#endif

static struct delayed_work headp_detect_work;
static int user_disable_speaker = 0;

/*           #################     setting     ##################        */

// HHTECH set system power enable
static void  set_sys_power(int sw)
{
	if (sw)
		gpio_set_value(GPIO_PWR_EN, 0);		/* power on */
	else
		gpio_set_value(GPIO_PWR_EN, 1);		/* pwoer off */
}

// HHTECH set Charging mode
static void set_charge_mode(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_CHARGER_EN, 1);		/* 860ma */	
	else 
		gpio_set_value(GPIO_CHARGER_EN, 0);		/* 200ma */
}

// HHTECH set backlight MP1530  enable
static void set_backlight_en(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_LCD_BLIGHT_EN, 1);	/* enable */	
	else 
		gpio_set_value(GPIO_LCD_BLIGHT_EN, 0);	/* disable */
}


// HHTECH set Vidoe amplifier switch
static void set_video_amp(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_VIDEOAMP_EN, 1);	/* open */
	else 
		gpio_set_value(GPIO_VIDEOAMP_EN, 0);	/* close */
}

// HHTECH set USB system (HOST and OTG)  power enable
static void set_usb_syspwr_en(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_USB_EN, 1);		/* open */
	else 
		gpio_set_value(GPIO_USB_EN, 0);		/* close */
}

// HHTECH set USB HOST  power enable
static void set_usb_hostpwr_en(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_USB_HOSTPWR_EN, 1);	/* open */
	else 
		gpio_set_value(GPIO_USB_HOSTPWR_EN, 0);	/* close */
}

// HHTECH set USB OTG  DRV enable
static void set_usb_otgdrv_en(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_USB_OTGDRV_EN, 1);	/* open */
	else 
		gpio_set_value(GPIO_USB_OTGDRV_EN, 0);	/* close */
}

// HHTECH set power hold enable
static void set_pwr_key(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_PWR_HOLD, 1);
	else 
		gpio_set_value(GPIO_PWR_HOLD, 0);
}

// HHTECH set speaker 
static void set_speaker_en(int sw)
{
	if (sw) 
		gpio_set_value(GPIO_SPEAKER_EN, 1);		/* open */
	else 
		gpio_set_value(GPIO_SPEAKER_EN, 0);		/* close */
}

// HHTECH set wifi 
extern struct mmc_host *g_wifi_host;
extern void mmc_detect_change(struct mmc_host *host, unsigned long delay);
static void set_wifi_en(int sw)
{
	if (sw) {
        		gpio_direction_output(GPIO_WIFI_EN, 1);		/* open */
        		gpio_direction_output(GPIO_WIFI_RESET, 0);
        		mdelay(100);
        		gpio_direction_output(GPIO_WIFI_RESET, 1);
		if (!!g_wifi_host)
			mmc_detect_change(g_wifi_host, 0);
	} else {
	    	gpio_direction_output(GPIO_WIFI_EN, 0);		/* close */
	}
}

// HHTECH set led1 and led2 
void set_led1_en(int sw)
{
	if (sw) 
	    	gpio_set_value(GPIO_LED1_EN, 0);		/* turn on */
	else 
	    	gpio_set_value(GPIO_LED1_EN, 1);		/* turn off */
}
EXPORT_SYMBOL_GPL(set_led1_en);
	
void set_led2_en(int sw)
{
	if (sw) 
	    	gpio_set_value(GPIO_LED2_EN, 0);
	else 
	    	gpio_set_value(GPIO_LED2_EN, 1);
}
EXPORT_SYMBOL_GPL(set_led2_en);

/*@@@@@@@@@@@@@@@@@@@@@@@	sysfs interface   @@@@@@@@@@@*/

#ifdef CONFIG_SYSFS
static ssize_t hhtech_sysfs_show_dc(struct device *dev,
				struct device_attribute *attr, char *buf)
{
#if defined (CONFIG_LCD_4)
	strcpy(buf, gpio_get_value(GPIO_DC_DETE) ? "on" : "off");
#else
	strcpy(buf, gpio_get_value(GPIO_DC_DETE) ? "off" : "on");
#endif
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_sd(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, gpio_get_value(GPIO_SD_WP) ? "off" : "on");
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_system_flag(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, system_flag ? "1" : "0");
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_headp(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, gpio_get_value(GPIO_HEADPHONE_S) ? "on" : "off");
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_blight_s(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, gpio_get_value(GPIO_LCD_BLIGHT_S) ? "on" : "off");
	strcat(buf, "\n");
	return strlen(buf);
}

extern unsigned int get_s3c_adc_value(unsigned int s3c_adc_port);
static ssize_t hhtech_sysfs_show_battery(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int battery_life = 0, reference_value = 0;
	static int old_battery_life = 0, old_reference_value = 0;
	char s[20];

	/* ref voltage:2.4V,battery max :4.2V */
	if ((battery_life = get_s3c_adc_value(0)) == 0) {
	    if(old_battery_life == 0) {
		while (!(battery_life = get_s3c_adc_value(0)));
		old_battery_life = battery_life;
	    } else
		battery_life = old_battery_life;
	}
	
	if ((reference_value = get_s3c_adc_value(1)) == 0) {
	    if(old_reference_value == 0) {
		while (!(reference_value = get_s3c_adc_value(1)));
		old_reference_value = reference_value;
	    } else
		reference_value = old_reference_value;
	}
	
	battery_life = (battery_life * 24000) / (reference_value * 42);
	
	sprintf(s,"%d",battery_life);
	strcpy(buf, s);
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_usbhost(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, gpio_get_value(GPIO_USB_HOST_STATUS) ? "on" : "off");
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_show_usbotg(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	strcpy(buf, gpio_get_value(GPIO_USB_OTG_STATUS) ? "on" : "off");
	strcat(buf, "\n");
	return strlen(buf);
}

static ssize_t hhtech_sysfs_charge_s(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int s=(gpio_get_value(GPIO_CHARG_S1)<<1) + gpio_get_value(GPIO_CHARG_S2);
        switch (s) {
	    case 0:
		strcpy(buf,"0");		
		break;
	    case 1:
		strcpy(buf,"1");        
		break;
	    case 2:
		strcpy(buf,"2");
		break;
	    case 3:
		strcpy(buf,"3");
		break;
	}
	strcat(buf, "\n");
	return strlen(buf);
}

static int hhtech_sysfs_show_power(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_PWR_EN));
}

static ssize_t hhtech_sysfs_store_power(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_sys_power(1);	    		/* power on */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_sys_power(0);	    		/* power off */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_charge(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_CHARGER_EN));
}

static ssize_t hhtech_sysfs_store_charge(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_charge_mode(1);	    		/* 860ma */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_charge_mode(0);	    		/* 200ma */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_blight(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_LCD_BLIGHT_EN));
}

static ssize_t hhtech_sysfs_store_blight(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_backlight_en(1);	    		/* enable MP1530 */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_backlight_en(0);	    		/* disable MP1530 */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_amp(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_VIDEOAMP_EN));
}

static ssize_t hhtech_sysfs_store_amp(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_video_amp(1);	    /* open Vidoe amplifier output */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_video_amp(0);	    /* close */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_usbpwr(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_USB_EN));
}

static ssize_t hhtech_sysfs_store_usbpwr(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_usb_syspwr_en(1);	    /* USB system power enable */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_usb_syspwr_en(0);	    /* disable */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_usbhostpwr(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_USB_HOSTPWR_EN));
}

static ssize_t hhtech_sysfs_store_usbhostpwr(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_usb_hostpwr_en(1);	    		/* USB system power enable */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_usb_hostpwr_en(0);	    		/* disable */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_usbotgdrv(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_USB_OTGDRV_EN));
}

static ssize_t hhtech_sysfs_store_usbotgdrv(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_usb_otgdrv_en(1); 
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_usb_otgdrv_en(0);
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_speaker(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_SPEAKER_EN));
}

static ssize_t hhtech_sysfs_store_speaker(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0) {
	    if(!gpio_get_value(GPIO_HEADPHONE_S))
		set_speaker_en(1);	    		/* speaker enable */
	    user_disable_speaker = 0;
	}
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0) {
	    set_speaker_en(0);	    			/* speaker disable */
	    user_disable_speaker = 1;
	}
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_wifi(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_WIFI_EN));
}

static ssize_t hhtech_sysfs_store_wifi(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_wifi_en(1);	    		/* wifi enable */
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_wifi_en(0);	    		/* wifi disable */
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_led1(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_LED1_EN));
}

static ssize_t hhtech_sysfs_store_led1(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_led1_en(1);
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_led1_en(0);
        else
	    return -EINVAL; 
	return len;
}

static int hhtech_sysfs_show_led2(struct device *dev, struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", gpio_get_value(GPIO_LED2_EN));
}

static ssize_t hhtech_sysfs_store_led2(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
        if (len < 1)
	    return -EINVAL;
   
        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
	    set_led2_en(1);
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	    set_led2_en(0);
        else
	    return -EINVAL; 
	return len;
}


#if defined (CONFIG_LCD_7)  && 0
/*   USB host protect status change Interrupt handle  */
static irqreturn_t usbhost_status_irq(int irq, void *dev_id)
{

	int pin_level = gpio_get_value(GPIO_USB_HOST_STATUS);
	//__raw_writel(__raw_readl(S3C_EINTMASK)&(~(0x1 << 18)), S3C_EINTMASK);	// mask the irq
	printk(KERN_INFO"Usb host status: %d\n", pin_level);
	if (0 == pin_level) {
		gpio_direction_output(GPIO_USB_OTGDRV_EN, 0);
		gpio_direction_output(GPIO_USB_EN, 0);
		//__raw_writel((__raw_readl(S3C_EINTCON1) & ~(0xf << 4)) |
		//	(S3C_EXTINT_FALLEDGE << 4), S3C_EINTCON1);  // enable the irq again
	}

	return IRQ_HANDLED;
}
#endif

/*   USB otg protect status change Interrupt handle  */
static irqreturn_t usbotg_status_irq(int irq, void *dev_id)
{
	int pin_level;
	
	pr_debug("Enter %s: %d\n", __func__, __LINE__);

	pin_level = gpio_get_value(GPIO_USB_OTG_STATUS);
	//__raw_writel(__raw_readl(S3C_EINTMASK)&(~(0x1 << 19)), S3C_EINTMASK);	// mask the irq
	printk(KERN_INFO"Usb otg status: %d\n", pin_level);
	if (0 == pin_level) {
		gpio_direction_output(GPIO_USB_HOSTPWR_EN, 0);
		gpio_direction_output(GPIO_USB_EN, 0);
		//__raw_writel((__raw_readl(S3C_EINTCON1) & ~(0xf << 4)) |
		//	(S3C_EXTINT_FALLEDGE << 4), S3C_EINTCON1);  // enable the irq again
	}

	return IRQ_HANDLED;
}

extern void headp_update_volume(struct work_struct* work);

/*   headphone plug in and out  Interrupt handle  */
static irqreturn_t headp_irq(int irq, void *dev_id)
{
	int pin_level;
	
	pr_debug("Enter %s: %d\n", __func__, __LINE__);

	pin_level = gpio_get_value(GPIO_HEADPHONE_S);
	//__raw_writel(__raw_readl(S3C_EINTMASK)&(~(0x1 << 20)), S3C_EINTMASK);
	if(user_disable_speaker == 0) {
	    	if (pin_level)
			gpio_set_value(GPIO_SPEAKER_EN, 0);
	    	else
			gpio_set_value(GPIO_SPEAKER_EN, 1);
	}

#ifdef CONFIG_SOUND	
	cancel_delayed_work(&headp_detect_work);
        schedule_delayed_work(&headp_detect_work, msecs_to_jiffies(80));
#endif

	return IRQ_HANDLED;
}


static DEVICE_ATTR(system_flag, 0444, hhtech_sysfs_show_system_flag, NULL);
static DEVICE_ATTR(headphone_s, 0444, hhtech_sysfs_show_headp, NULL);
static DEVICE_ATTR(dc_s, 0444, hhtech_sysfs_show_dc, NULL);
static DEVICE_ATTR(sd_s, 0444, hhtech_sysfs_show_sd, NULL);
static DEVICE_ATTR(charge_s, 0444,hhtech_sysfs_charge_s , NULL);
static DEVICE_ATTR(backlight_s, 0444, hhtech_sysfs_show_blight_s, NULL);
static DEVICE_ATTR(usbhost_s, 0444, hhtech_sysfs_show_usbhost, NULL);
static DEVICE_ATTR(usbotg_s, 0444, hhtech_sysfs_show_usbotg, NULL);
static DEVICE_ATTR(battery_s, 0444, hhtech_sysfs_show_battery, NULL);
static DEVICE_ATTR(pwr_en, 0666, hhtech_sysfs_show_power, hhtech_sysfs_store_power);
static DEVICE_ATTR(charge_en, 0666, hhtech_sysfs_show_charge, hhtech_sysfs_store_charge);
static DEVICE_ATTR(backlight_en, 0666,hhtech_sysfs_show_blight, hhtech_sysfs_store_blight);
static DEVICE_ATTR(amp_en, 0666,hhtech_sysfs_show_amp, hhtech_sysfs_store_amp);
static DEVICE_ATTR(usbpwr_en, 0666, hhtech_sysfs_show_usbpwr, hhtech_sysfs_store_usbpwr);
static DEVICE_ATTR(usbhostpwr_en, 0666, hhtech_sysfs_show_usbhostpwr, hhtech_sysfs_store_usbhostpwr);
static DEVICE_ATTR(usbotgdrv_en, 0666, hhtech_sysfs_show_usbotgdrv, hhtech_sysfs_store_usbotgdrv);
static DEVICE_ATTR(speaker_en, 0666, hhtech_sysfs_show_speaker, hhtech_sysfs_store_speaker);
static DEVICE_ATTR(wifi_en, 0666, hhtech_sysfs_show_wifi, hhtech_sysfs_store_wifi);
static DEVICE_ATTR(led1_en, 0666, hhtech_sysfs_show_led1, hhtech_sysfs_store_led1);
static DEVICE_ATTR(led2_en, 0666, hhtech_sysfs_show_led2, hhtech_sysfs_store_led2);

static struct attribute *attrs[] = {
	&dev_attr_system_flag.attr,
	&dev_attr_headphone_s.attr,
	&dev_attr_dc_s.attr,
	&dev_attr_sd_s.attr,
	&dev_attr_charge_s.attr,
	&dev_attr_backlight_s.attr,
	&dev_attr_usbhost_s.attr,
	&dev_attr_usbotg_s.attr,
	&dev_attr_battery_s.attr,
	&dev_attr_pwr_en.attr,
	&dev_attr_charge_en.attr,
	&dev_attr_backlight_en.attr,
	&dev_attr_amp_en.attr,
	&dev_attr_usbpwr_en.attr,
	&dev_attr_usbhostpwr_en.attr,
	&dev_attr_usbotgdrv_en.attr,
	&dev_attr_speaker_en.attr,
	&dev_attr_wifi_en.attr,
	&dev_attr_led1_en.attr,
	&dev_attr_led2_en.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int hhtech_gpio_sysfs_register(struct device* dev)
{
	return sysfs_create_group(&dev->kobj, &attr_group);
}
#else
static int hhtech_gpio_sysfs_register(struct device* dev)
{
	return 0;
}
#endif

static int hhtech_gpio_probe(struct platform_device *pdev)
{
	int retval;

	printk(KERN_INFO"smartQ5/7 gpio driver probe!\n");
	
	retval = hhtech_gpio_sysfs_register(&pdev->dev);
	if (retval < 0) {
		printk(KERN_ERR "Create sys fs fail\n");
		return -1;
	}
	
	gpio_direction_input(GPIO_SD_WP);					/* S3C_GPK0 */
	s3c_gpio_setpull(GPIO_SD_WP, S3C_GPIO_PULL_NONE);
	
#if defined (CONFIG_LCD_7)
	gpio_direction_input(GPIO_USB_HOST_STATUS);		/* S3C_GPL10 */
	s3c_gpio_setpull(GPIO_USB_HOST_STATUS, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_USB_HOST_STATUS, S3C_GPIO_SFN(3));
#endif
	gpio_direction_input(GPIO_USB_OTG_STATUS);		/* S3C_GPL11 */
	s3c_gpio_setpull(GPIO_USB_OTG_STATUS, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_USB_OTG_STATUS, S3C_GPIO_SFN(3));
	gpio_direction_input(GPIO_HEADPHONE_S);			/* S3C_GPL12 */
	s3c_gpio_setpull(GPIO_HEADPHONE_S, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HEADPHONE_S, S3C_GPIO_SFN(3));

	gpio_direction_input(GPIO_CHARG_S1);				/* S3C_GPK4 */
	s3c_gpio_setpull(GPIO_CHARG_S1, S3C_GPIO_PULL_NONE);
	gpio_direction_input(GPIO_CHARG_S2);				/* S3C_GPK5 */
	s3c_gpio_setpull(GPIO_CHARG_S2, S3C_GPIO_PULL_NONE);
#if defined (CONFIG_LCD_4)
	gpio_direction_output(GPIO_USB_EN, 0);				/* S3C_GPL0	: close */
	gpio_set_value(GPIO_USB_EN, 0);
	gpio_direction_output(GPIO_USB_HOSTPWR_EN, 0);	/* S3C_GPL1	: close */
	gpio_set_value(GPIO_USB_HOSTPWR_EN, 0);
#else
	gpio_direction_output(GPIO_USB_EN, 1);				/* S3C_GPL0	: open */
	gpio_set_value(GPIO_USB_EN, 1);
	gpio_direction_output(GPIO_USB_HOSTPWR_EN, 1);	/* S3C_GPL1	: open */
	gpio_set_value(GPIO_USB_HOSTPWR_EN, 1);
#endif

	gpio_direction_output(GPIO_PWR_EN, 0);				/* S3C_GPK15 : open normal it's low */
	gpio_direction_output(GPIO_VIDEOAMP_EN, 0);		/* S3C_GPK13 : close */
	gpio_direction_output(GPIO_SPEAKER_EN, 1);			/* S3C_GPK12 : close */
	gpio_set_value(GPIO_SPEAKER_EN, 1);	
	gpio_direction_output(GPIO_USB_OTGDRV_EN, 0);		/* S3C_GPL8	: close */
	gpio_set_value(GPIO_USB_OTGDRV_EN, 0);
	gpio_direction_output(GPIO_LED1_EN, 1);				/* S3C_GPN8: 1: off, 0: on */
	gpio_direction_output(GPIO_LED2_EN, 1);				/* S3C_GPN9: 1: off, 0: on */
	
	set_wifi_en(0);
	
//#ifdef CONFIG_SOUND	
//	INIT_DELAYED_WORK(&headp_detect_work, headp_update_volume);
//#endif
//
//      retval = request_irq(gpio_to_irq(GPIO_HEADPHONE_S), headp_irq,
//	     IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "Headphone detect", NULL);
//	if (retval) {
//	    printk(KERN_ERR "Request headphone detect fail\n");
//	    goto error1;
//	}
//
	retval = request_irq(gpio_to_irq(GPIO_USB_OTG_STATUS), usbotg_status_irq,
	     IRQF_TRIGGER_FALLING, "Usb otg status detect", NULL);
	if (retval) {
	    printk(KERN_ERR "Request usb otg status detect fail\n");
//	    goto error2;
	}

#if defined (CONFIG_LCD_7)
#if 0
	retval = request_irq(gpio_to_irq(GPIO_USB_HOST_STATUS), usbhost_status_irq,
	     IRQF_TRIGGER_FALLING, "Usb host status detect", NULL);
	if (retval) {
	    printk(KERN_ERR "Request usb host status detect fail\n");
	    goto error3;
	}
#endif	
#endif

	return 0;

error3:
	free_irq(gpio_to_irq(GPIO_USB_OTG_STATUS), NULL);
//error2:
//	free_irq(gpio_to_irq(GPIO_HEADPHONE_S), NULL);
error1:
	return retval;

	return 0;
}

static int hhtech_gpio_remove(struct platform_device *dev)
{

//	free_irq(gpio_to_irq(GPIO_HEADPHONE_S), NULL);
#if defined (CONFIG_LCD_7)
	//free_irq(gpio_to_irq(GPIO_USB_HOST_STATUS), NULL);
#endif
	free_irq(gpio_to_irq(GPIO_USB_OTG_STATUS), NULL);

        return 0;
}

#ifdef CONFIG_PM
static int wifi_old_state = 0;
static int hhtech_gpio_suspend(struct platform_device *dev, pm_message_t state)
{
	wifi_old_state = gpio_get_value(GPIO_WIFI_EN);
	set_wifi_en(0);
        	return 0;
}

static int hhtech_gpio_resume(struct platform_device *dev)
{
        set_wifi_en(wifi_old_state);
        return 0;
}
#else
#define hhtech_gpio_suspend NULL
#define hhtech_gpio_resume NULL
#endif


static struct platform_driver hhtech_gpio = {
        .probe = hhtech_gpio_probe,
        .remove = hhtech_gpio_remove,
        .suspend = hhtech_gpio_suspend,
        .resume = hhtech_gpio_resume,
        .driver =  {
                .name = "smartq_gpio",
                .owner = THIS_MODULE,
        },
};

static int __init hhtech_gpio_init(void)
{
	user_disable_speaker = 0;
        return platform_driver_register(&hhtech_gpio);
}

static void __exit hhtech_gpio_exit(void)
{
        platform_driver_unregister(&hhtech_gpio);
}

module_init(hhtech_gpio_init);
module_exit(hhtech_gpio_exit);


MODULE_AUTHOR("Wangkang");
MODULE_DESCRIPTION("S3C64xx GPIO driver");
MODULE_LICENSE("GPL");
