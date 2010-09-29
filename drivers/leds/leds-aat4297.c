/*
 * AAT4297 7 LED Driver
 *
 * Copyright Samsung Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-lcd.h>
#include <mach/hardware.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

//#define _DEBUG	
#ifdef _DEBUG
#define dprintk(s, args...) printk("[SATURN LED] %s:%d - " s, __func__, __LINE__,  ##args)
#else
#define dprintk(s, args...)
#endif  /* _DEBUG */

#define LED_BLACK	0x00
#define LED_RED		0x04
#define LED_GREEN	0x02
#define LED_BLUE	0x01
#define LED_YELLOW	0x06
#define LED_MAGENTA	0x05
#define LED_CYAN	0x03
#define LED_WHITE	0x07

int led_count = 0;
int timer_registered = 0;		

int blink_val=0;
int grp_pwm_val = 0;
int grp_freq_val = 0;
int red_val_set = 0;
int green_val_set = 0;
int blue_val_set = 0;
struct timer_list timer;
int blink_off = 0;
int gpio_ldo_en = 0;

void led_blink_on(void);

void register_led_timer(void)
{
	dprintk("\n");	
	init_timer(&timer);
	timer.expires = jiffies + HZ;
	timer.data = NULL;
	timer.function = led_blink_on;
	add_timer(&timer);
	blink_off = ~(blink_off);
}

void aat4297_led_on(void)
{
	int i;	
	int color;
	
	color = ((red_val_set << 2) | (green_val_set << 1) | blue_val_set);
	
	switch (color)
	{
		case LED_BLACK:
			dprintk("LED BLACK\n");
			led_count = 64;
			break;
		case LED_RED:
			dprintk("LED RED\n");
			led_count = 63;
			break;
		case LED_GREEN:
			dprintk("LED GREEN\n");
			led_count = 62;
			break;
		case LED_BLUE:
			dprintk("LED BLUE\n");
			led_count = 48;
			break;
		case LED_YELLOW:
			dprintk("LED YELLOW\n");
			led_count = 61;
			break;
		case LED_MAGENTA:
			dprintk("LED MAGENTA\n");
			led_count = 47;
			break;
		case LED_CYAN:
			dprintk("LED CYAN\n");
			led_count = 46;
			break;
		case LED_WHITE:
			dprintk("LED WHITE\n");
			led_count = 45;
			break;
		default:
			dprintk("Invalid LED Value\n");
			break;
	}

	if (color == LED_BLACK)
	{			
		if (gpio_request(GPIO_7_LED_LDO_EN, S3C_GPIO_LAVEL(GPIO_7_LED_LDO_EN)))
			dprintk(KERN_ERR "Failed to request GPIO_7_LED_LDO_EN!\n");
		gpio_direction_output(GPIO_7_LED_LDO_EN, GPIO_LEVEL_LOW);
		gpio_free(GPIO_7_LED_LDO_EN);
		gpio_ldo_en = 0;
	}
	else
	{	
		if (!gpio_ldo_en)
		{
			if (gpio_request(GPIO_7_LED_LDO_EN, S3C_GPIO_LAVEL(GPIO_7_LED_LDO_EN)))
				dprintk(KERN_ERR "Failed to request GPIO_7_LED_LDO_EN!\n");
			gpio_direction_output(GPIO_7_LED_LDO_EN, GPIO_LEVEL_HIGH);
			gpio_free(GPIO_7_LED_LDO_EN);
			gpio_ldo_en = 1;
			mdelay(50);
		}
		for (i=0; i<led_count; i++)
		{
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_LOW);
			udelay(1);
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_HIGH);
			udelay(1);
		}
	}
}

void led_blink_on(void)
{
	int i;
	
	dprintk("\n");

	if (blink_off)
	{
		for (i=0; i<64; i++)
		{
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_LOW);
			udelay(1);
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_HIGH);
			udelay(1);
		}

	}
	else
	{	
		for (i=0; i<led_count; i++)
		{
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_LOW);
			udelay(1);
			gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_HIGH);
			udelay(1);
		}
	}

	register_led_timer();
}

static void aat4297led_red_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	dprintk("Value : %d\n", value);

	if (value)
		red_val_set = 1;
	else
		red_val_set = 0;	
}

static void aat4297led_green_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	dprintk("Value : %d\n", value);

	if (value)
		green_val_set = 1;
	else
		green_val_set = 0;	
}

static void aat4297led_blue_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	dprintk("Value : %d\n", value);

	if (value)
		blue_val_set = 1;
	else
		blue_val_set = 0;	
}

static ssize_t led_blink_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", blink_val);  
}
   
static ssize_t led_blink_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
                                      
	dprintk("state = %d\n", state);
	blink_val = state;

	aat4297_led_on();
	
	if (blink_val)
	{
		if (timer_registered)
		{
			dprintk("Already Timer is set\n");
			del_timer(&timer);	
		}	
		timer_registered = 1;		
		register_led_timer();
	}
	else
	{
		if (timer_registered)
		{	
			del_timer(&timer);
			timer_registered = 0;
		}
	}	
	
	return size;	
}

static ssize_t led_grp_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", grp_freq_val);  
}
   
static ssize_t led_grp_freq_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
                                      
	printk("[7-LED] %s : state = %d\n", __func__, state);
	grp_freq_val = state;
	return size;	
}

static ssize_t led_grp_pwm_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", grp_pwm_val);  
}
   
static ssize_t led_grp_pwm_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
                                      
	printk("[7-LED] %s : state = %d\n", __func__, state);
	grp_pwm_val = state;
	return size;	
}

static struct led_classdev aat4297_red_led = 
{
	.name			= "red",
	.brightness_set		= aat4297led_red_set,
};

static struct led_classdev aat4297_green_led = 
{
	.name			= "green",
	.brightness_set		= aat4297led_green_set,
};

static struct led_classdev aat4297_blue_led = 
{
	.name			= "blue",
	.brightness_set		= aat4297led_blue_set,
};

static int aat4297led_suspend(struct platform_device *dev, pm_message_t state)
{
	if (gpio_request(GPIO_7_LED_EN, S3C_GPIO_LAVEL(GPIO_7_LED_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_EN!\n");
	gpio_direction_output(GPIO_7_LED_EN, GPIO_LEVEL_LOW);
	gpio_free(GPIO_7_LED_EN);

	if (gpio_request(GPIO_7_LED_LDO_EN, S3C_GPIO_LAVEL(GPIO_7_LED_LDO_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_LDO_EN!\n");
	gpio_direction_output(GPIO_7_LED_LDO_EN, GPIO_LEVEL_LOW);
	gpio_free(GPIO_7_LED_LDO_EN);

		led_classdev_suspend(&aat4297_red_led);
	led_classdev_suspend(&aat4297_green_led);
	led_classdev_suspend(&aat4297_blue_led);
	return 0;
}

static int aat4297led_resume(struct platform_device *dev)
{
	led_classdev_resume(&aat4297_red_led);
	led_classdev_resume(&aat4297_green_led);
	led_classdev_resume(&aat4297_blue_led);
	
	if (gpio_request(GPIO_7_LED_EN, S3C_GPIO_LAVEL(GPIO_7_LED_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_EN!\n");

	if (gpio_request(GPIO_7_LED_LDO_EN, S3C_GPIO_LAVEL(GPIO_7_LED_LDO_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_LDO_EN!\n");
	gpio_direction_output(GPIO_7_LED_LDO_EN, GPIO_LEVEL_HIGH);
	gpio_free(GPIO_7_LED_LDO_EN);

	return 0;
}


static DEVICE_ATTR(blink, 0644, led_blink_show, led_blink_store);
static DEVICE_ATTR(grpfreq, 0644, led_grp_freq_show, led_grp_freq_store);
static DEVICE_ATTR(grppwm, 0644, led_grp_pwm_show, led_grp_pwm_store);

static int aat4297led_probe(struct platform_device *pdev)
{
	int ret, err;
	int i;

	dprintk("start\n");
	ret = led_classdev_register(&pdev->dev, &aat4297_red_led);
	if (ret < 0)
	{
		dprintk("registration fail\n");
		led_classdev_unregister(&aat4297_red_led);
	}

	ret = led_classdev_register(&pdev->dev, &aat4297_green_led);
	if (ret < 0)
	{
		dprintk("registration fail\n");
		led_classdev_unregister(&aat4297_green_led);
	}

	ret = led_classdev_register(&pdev->dev, &aat4297_blue_led);
	if (ret < 0)
	{
		dprintk("registration fail\n");
		led_classdev_unregister(&aat4297_blue_led);
	}
	dprintk("LED CONTROL\n");

	err = device_create_file(&(pdev->dev), &dev_attr_blink);
	if (err < 0)
		printk(KERN_WARNING "aatled: failed to add blink sysfs files\n");

	err = device_create_file(&(pdev->dev), &dev_attr_grpfreq);
	if (err < 0)
		printk(KERN_WARNING "aatled: failed to add grp_freq sysfs files\n");

	err = device_create_file(&(pdev->dev), &dev_attr_grppwm);
	if (err < 0)
		printk(KERN_WARNING "aatled: failed to add grp_pwm sysfs files\n");

	if (gpio_request(GPIO_7_LED_LDO_EN, S3C_GPIO_LAVEL(GPIO_7_LED_LDO_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_LDO_EN!\n");
	gpio_direction_output(GPIO_7_LED_LDO_EN, GPIO_LEVEL_HIGH);
	gpio_free(GPIO_7_LED_LDO_EN);
	gpio_ldo_en = 1;

	if (gpio_request(GPIO_7_LED_EN, S3C_GPIO_LAVEL(GPIO_7_LED_EN)))
		dprintk(KERN_ERR "Failed to request GPIO_7_LED_EN!\n");

	return ret;
}

static int aat4297led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&aat4297_red_led);
	led_classdev_unregister(&aat4297_green_led);
	led_classdev_unregister(&aat4297_blue_led);
	gpio_free(GPIO_7_LED_EN);
	return 0;
}

static struct platform_device aat4297_led = 
{
    .name       = "aat4297-led",
};

static struct platform_driver aat4297led_driver = 
{
	.probe		= aat4297led_probe,
	.remove		= aat4297led_remove,
	.suspend	= aat4297led_suspend,
	.resume		= aat4297led_resume,
	.driver		= 
	{
		.name		= "aat4297-led",
		.owner		= THIS_MODULE,
	},
};

static int __init aat4297led_init(void)
{
	dprintk("start\n");
	platform_device_register(&aat4297_led);
	return platform_driver_register(&aat4297led_driver);
}

static void __exit aat4297led_exit(void)
{
	platform_driver_unregister(&aat4297led_driver);
}

module_init(aat4297led_init);
module_exit(aat4297led_exit);

MODULE_AUTHOR("Hwang sunil <sunil07.hwang@samsung.com>");
MODULE_DESCRIPTION("Saturn LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:saturn-led");
