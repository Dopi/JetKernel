/*
 *  aat1271.c - driver for AAT1271 (Support Flash & Movie Mode)
 *
 *  Copyright (C) 2009 Jeonghwan Min <jh78.min@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>


#include <linux/timed_output.h>

#include <mach/hardware.h>

#include <plat/gpio-cfg.h>

static struct hrtimer timer;


static int max_timeout = 5000;
static int flash_value = 0;



/*
extern struct class *timed_gpio_class;

struct timed_gpio_data {
	struct device *dev;
	struct hrtimer timer;
	spinlock_t lock;
};
*/
/*
static struct timed_gpio_data *aat1271_flash_data;
*/

/*
 * Register Address
 */

#define OFF 	0
#define ON		1

#define MOVIE_MODE_CURRENT			17
#define FLASH_SAFETY_TIMER			18
#define MOVIE_MODE_CONFIG			19
#define FLASH_TO_MOVIE_RATIO		20

#define MOVIE_MODE_CURRENT_71		4
#define MOVIE_MODE_CURRENT_63		5
#define MOVIE_MODE_CURRENT_56		6
#define MOVIE_MODE_CURRENT_50       7 //hjkang_DE21

#define OFFSET_CAM_FLASH   (0x1 << 9)

extern void s3c_bat_set_compensation_for_drv(int mode,int offset);

static void aat1271a_flash_write(int addr, int data)
{
	int i;

	for (i = 0; i < addr; i++) {
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		udelay(10);	
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_HIGH);
		udelay(50);
	}

	udelay(500);	

	for (i = 0; i < data; i++) {
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		udelay(10);	
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_HIGH);
		udelay(50);
	}
	
	udelay(500);	
}

/*
 * IFLOUTA = IFLOUTB = 81K / 160K * A = 500mA
 * T = 7.98s / uF * Ct(uF) = 7.98s / uF * 0.1uF = 0.798s 
 */

void aat1271a_flash_camera_control(int ctrl)
{
	if (ctrl) {
		/* Movie Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		udelay(10);
		/* Falsh Mode On */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_HIGH);
		s3c_bat_set_compensation_for_drv(1,OFFSET_CAM_FLASH);

	}
	else {
		/* Movie Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		s3c_bat_set_compensation_for_drv(0,OFFSET_CAM_FLASH);
	}
}	
void aat1271a_flash_torch_camera_control(int ctrl) //hjkang_DE28
{
	if (ctrl) {
		
		/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		/* Movie Mode Current Setting & On */	
		aat1271a_flash_write(MOVIE_MODE_CURRENT, MOVIE_MODE_CURRENT_71); 
		s3c_bat_set_compensation_for_drv(1,OFFSET_CAM_FLASH);
	}
	else {
		/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		/* Movie Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		s3c_bat_set_compensation_for_drv(0,OFFSET_CAM_FLASH);		
	}
}	

/*
 * IMOVIEMODE = IFLOUTA / 7.3 = 500mA / 7.3 = 68mA
 * 45mA / 68mA = 0.66 = 0.7 = 70 %
 */

void aat1271a_flash_movie_control(int ctrl)
{
	if (ctrl) {
		
		/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		/* Movie Mode Current Setting & On */	
		aat1271a_flash_write(MOVIE_MODE_CURRENT, MOVIE_MODE_CURRENT_50); //hjkang_DE21 MOVIE_MODE_CURRENT_63
		s3c_bat_set_compensation_for_drv(1,OFFSET_CAM_FLASH);
	}
	else {
		/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		/* Movie Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
		s3c_bat_set_compensation_for_drv(0,OFFSET_CAM_FLASH);		
	}
}

static enum hrtimer_restart aat1271_flash_timer_func(struct hrtimer *timer)
{
	/* Movie Mode Off */
	gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
	/* Falsh Mode Off */
	gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
	
	return HRTIMER_NORESTART;
}

static int get_time_for_flash(struct timed_output_dev *dev)
{
	int remaining;

	if (hrtimer_active(&timer)) {
		ktime_t r = hrtimer_get_remaining(&timer);
		remaining = r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		remaining = 0;

	if (flash_value ==-1)
		remaining = -1;

	return remaining;

}

static void enable_flash_from_user(struct timed_output_dev *dev,int value)
{
	if (value > 0)
	{
			if (value < 780)	/* Flash Mode */
			aat1271a_flash_camera_control(ON);
			else	/* Movie Mode */
			aat1271a_flash_movie_control(ON);
	}
	else if (value == 0)
	{
		aat1271a_flash_camera_control(OFF);
		aat1271a_flash_movie_control(OFF);
	}
}


#if 0
static ssize_t aat1271_flash_enable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct timed_gpio_data *gpio_data = dev_get_drvdata(dev);
	int value;
	unsigned long flags;

	sscanf(buf, "%d", &value);


	if (value > 0) {
	
		if (value < 780)	/* Flash Mode */
			aat1271a_flash_camera_control(ON);
		else	/* Movie Mode */
			aat1271a_flash_movie_control(ON);
}
else if (value == 0)
	{
		aat1271a_flash_camera_control(OFF);
		aat1271a_flash_movie_control(OFF);
	}
	
	return size;
}

#endif

static struct timed_output_dev timed_output_flash= {
	.name     = "flash",
	.get_time = get_time_for_flash,
	.enable   = enable_flash_from_user,
};




int aat1271a_flash_init(void)
{	

	int ret;

	printk(KERN_ERR "%s\n", __func__);
	
	/* hrtimer settings */
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = aat1271_flash_timer_func;


	if (gpio_is_valid(GPIO_CAM_FLASH_SET)) {
		if (gpio_request(GPIO_CAM_FLASH_SET, S3C_GPIO_LAVEL(GPIO_CAM_FLASH_SET))) 
			printk(KERN_ERR "Failed to request GPIO_CAM_FLASH_SET!\n");
		gpio_direction_output(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_CAM_FLASH_SET, S3C_GPIO_PULL_NONE);

	if (gpio_is_valid(GPIO_CAM_FLASH_EN)) {
		if (gpio_request(GPIO_CAM_FLASH_EN, S3C_GPIO_LAVEL(GPIO_CAM_FLASH_EN))) 
			printk(KERN_ERR "Failed to request GPIO_CAM_FLASH_EN!\n");
		gpio_direction_output(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_CAM_FLASH_EN, S3C_GPIO_PULL_NONE);
	
	/* timed_output_device settings */
	ret = timed_output_dev_register(&timed_output_flash);
	if(ret)
		printk(KERN_ERR "[VIBETONZ] timed_output_dev_register is fail \n");

}

void aat1271a_flash_exit(void)
{
    	/* Falsh Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_EN, GPIO_LEVEL_LOW);
		/* Movie Mode Off */
		gpio_set_value(GPIO_CAM_FLASH_SET, GPIO_LEVEL_LOW);
}
