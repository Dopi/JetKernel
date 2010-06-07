/*
 *  H2W device detection driver.
 *
 *  Copyright (C) 2009 Samsung Electronics, Inc.
 *
 *  Authors:
 *      Eunki Kim <eunki_kim@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

#include <mach/hardware.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/sec_headset.h>

#define CONFIG_DEBUG_SEC_HEADSET

#ifdef CONFIG_DEBUG_SEC_HEADSET
#define SEC_HEADSET_DBG(fmt, arg...) printk(KERN_INFO "[SEC_HEADSET] %s " fmt "\r\n", __func__, ## arg)
#else
#define SEC_HEADSET_DBG(fmt, arg...) do {} while (0)
#endif

#define KEYCODE_SENDEND 248

//#define USING_ADC_FOR_EAR_DETECTING

/* for short key time limit */
static u64 pressed_jiffies;
static u64 irq_jiffies;
#define SHORTKEY_MS			120
#define SHORTKEY_JIFFIES	((HZ / 10) * (SHORTKEY_MS / 100)) + (HZ / 100) * ((SHORTKEY_MS % 100) / 10)

extern int s3c_adc_get_adc_data(int channel);
extern int mic_ear_enable(int);

struct sec_headset_info {
	struct sec_headset_port port;
	struct input_dev *input;
};

static struct sec_headset_info *hi;

struct switch_dev switch_earjack = {
        .name = "h2w",
};

//SISO Added support for send_end Sysfs node 
struct switch_dev switch_sendend = {
        .name = "send_end",
};

static struct timer_list send_end_enable_timer;
static unsigned int send_end_irq_token = 0;
static unsigned short int headset_status = 0;
static struct wake_lock headset_sendend_wake_lock;

short int get_headset_status()
{
	SEC_HEADSET_DBG(" headset_status %d", headset_status);
	return headset_status;
}
EXPORT_SYMBOL(get_headset_status);

static void send_end_enable_timer_handler(unsigned long arg)
{
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	enable_irq (send_end->eint);
	send_end_irq_token++;
	wake_unlock(&headset_sendend_wake_lock);
	SEC_HEADSET_DBG("enable send_end event, token is %d", send_end_irq_token);
}
static void send_end_press_work_handler(struct work_struct *ignored)
{
	SEC_HEADSET_DBG("SEND/END Button is pressed");
	printk(KERN_ERR "SISO:sendend isr work queue\n");

	switch_set_state(&switch_sendend, 1);
	input_report_key(hi->input, KEYCODE_SENDEND, 1);
	input_sync(hi->input);
}
static void send_end_release_work_handler(struct work_struct *ignored)
{
	switch_set_state(&switch_sendend, 0);
	input_report_key(hi->input, KEYCODE_SENDEND, 0);
	input_sync(hi->input);

	SEC_HEADSET_DBG("SEND/END Button is %s.\n", "released");
}
#ifdef USING_ADC_FOR_EAR_DETECTING
static void ear_adc_caculrator(unsigned long arg)
{
	int adc = 0;
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	int state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if (state)
	{
		adc = s3c_adc_get_adc_data(3);//headset adc port is 3
		if(adc < 3155 && adc > 3140)
		{
			SEC_HEADSET_DBG("4pole ear-mic attached adc is %d, send_end interrupt enable 2sec after", adc);
        		switch_set_state(&switch_earjack, state);
			send_end_enable_timer.expires = get_jiffies_64() + (20*HZ/10);//2sec HZ is 200
			add_timer(&send_end_enable_timer);
		}
		//else if(adc < 50 && adc > 10) //TV Out
		else if(adc < 5)
		{
			SEC_HEADSET_DBG("3pole earphone attached adc is %d", adc);
        		switch_set_state(&switch_earjack, state);
		}
		else
		{
			SEC_HEADSET_DBG("Wrong adc value adc is %d", adc);
        		switch_set_state(&switch_earjack, state);
		}

	}
	else
	{
		SEC_HEADSET_DBG("Error : mic bias enable complete but headset detached!!");
	}
}
#endif

#ifdef USING_ADC_FOR_EAR_DETECTING
static DECLARE_DELAYED_WORK(ear_adc_cal_work, ear_adc_caculrator);
#endif

static void ear_switch_change(struct work_struct *ignored)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state;

	del_timer(&send_end_enable_timer);
#ifdef USING_ADC_FOR_EAR_DETECTING
	cancel_delayed_work_sync(&ear_adc_cal_work);
#endif
	state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if (state && !send_end_irq_token)
	{
#ifdef USING_ADC_FOR_EAR_DETECTING
		SEC_HEADSET_DBG("Headset attached, mic bias enable");
		gpio_set_value(GPIO_MICBIAS_EN, 1);
		schedule_delayed_work(&ear_adc_cal_work, 150);
#else
		SEC_HEADSET_DBG("Headset attached, send end enable 2sec after");
        mic_ear_enable(1);
        	switch_set_state(&switch_earjack, state);
		headset_status = state;
		send_end_enable_timer.expires = get_jiffies_64() + (20*HZ/10);//2sec HZ is 200
		add_timer(&send_end_enable_timer);
		wake_lock(&headset_sendend_wake_lock);
#endif
	}
	else if(!state)
	{
		SEC_HEADSET_DBG("Headset detached %d ", send_end_irq_token);
#ifdef USING_ADC_FOR_EAR_DETECTING
		gpio_set_value(GPIO_MICBIAS_EN, 0);
#endif
        mic_ear_enable(0);
		wake_unlock(&headset_sendend_wake_lock);
        	switch_set_state(&switch_earjack, state);
		headset_status = state;
		if(send_end_irq_token > 0)
		{
			disable_irq (send_end->eint);
			send_end_irq_token--;
		}
	}
	else
		SEC_HEADSET_DBG("Headset state does not valid. or send_end event");

}

static DECLARE_DELAYED_WORK(sendend_press_work, send_end_press_work_handler);
static DECLARE_DELAYED_WORK(sendend_release_work, send_end_release_work_handler);

static void sendend_switch_change(struct work_struct *ignored)
{

	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state, headset_state;

	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && send_end_irq_token)//headset connect && send irq enable
	{
		if(!state)
		{
			printk(KERN_ERR "SISO:sendend isr work queue\n");
			wake_unlock(&headset_sendend_wake_lock);
			/* if keep pressed event for short key, reconize release event */
			schedule_delayed_work(&sendend_release_work, SHORTKEY_MS); 	
		}else{
			/* if keep pressed event for short key, reconize press event */
			schedule_delayed_work(&sendend_press_work, SHORTKEY_MS); 	
			wake_lock(&headset_sendend_wake_lock);
			SEC_HEADSET_DBG("SEND/END Button is %s. timer start\n", "pressed");
		}
	}else{
		SEC_HEADSET_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", state?"pressed":"released");
	}
}

static DECLARE_WORK(ear_switch_work, ear_switch_change);
static DECLARE_WORK(sendend_switch_work, sendend_switch_change);

static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
	schedule_work(&ear_switch_work);
	return IRQ_HANDLED;
}
 
static irqreturn_t send_end_irq_handler(int irq, void *dev_id)
{
	struct sec_gpio_info   *send_end = &hi->port.send_end;

	irq_jiffies = jiffies_64;

	/* Pressed */
	if (gpio_get_value(send_end->gpio) ^ send_end->low_active)
	{
		pressed_jiffies = irq_jiffies;
		schedule_work(&sendend_switch_work);
	}
	/* Released */
	else
	{
		/* ignore shortkey */
		if (irq_jiffies - pressed_jiffies < SHORTKEY_JIFFIES) {
			cancel_delayed_work_sync(&sendend_press_work);
			SEC_HEADSET_DBG("Cancel press work queue");
		}	
		else
			schedule_work(&sendend_switch_work);
	}

	return IRQ_HANDLED;
}

static int sec_headset_probe(struct platform_device *pdev)
{
	int ret;
	struct sec_headset_platform_data *pdata = pdev->dev.platform_data;
	struct sec_gpio_info   *det_headset;
	struct sec_gpio_info   *send_end;
	struct input_dev       *input;

	printk(KERN_INFO "SEC HEADSET: Registering headset driver\n");
	hi = kzalloc(sizeof(struct sec_headset_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	memcpy (&hi->port, pdata->port, sizeof(struct sec_headset_port));

	input = hi->input = input_allocate_device();
	if (!input) {
		ret = -ENOMEM;
		printk(KERN_ERR "SEC HEADSET: Failed to allocate input device.\n");
		goto err_request_input_dev;
	}

	input->name = "sec_headset";
	set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(KEYCODE_SENDEND, input->keybit);

	ret = input_register_device(input);
	if (ret < 0){
		printk(KERN_ERR "SEC HEADSET: Failed to register driver\n");
		goto err_register_input_dev;
	}
	
	init_timer(&send_end_enable_timer);
	send_end_enable_timer.function = send_end_enable_timer_handler;

	ret = switch_dev_register(&switch_earjack);
	if (ret < 0) {
		printk(KERN_ERR "SEC HEADSET: Failed to register switch device\n");
		goto err_switch_dev_register;
	}

    	printk(KERN_ERR "SISO:registering switch_sendend switch_dev");
	ret = switch_dev_register(&switch_sendend);
	if (ret < 0) {
		printk(KERN_ERR "SEC HEADSET: Failed to register switch sendend device\n");
		goto err_switch_dev_register;
        }

	send_end = &hi->port.send_end;
        s3c_gpio_cfgpin(send_end->gpio, S3C_GPIO_SFN(send_end->gpio_af));
        s3c_gpio_setpull(send_end->gpio, S3C_GPIO_PULL_NONE);
        set_irq_type(send_end->eint, IRQ_TYPE_EDGE_BOTH);
	
	ret = request_irq(send_end->eint, send_end_irq_handler,
			  IRQF_DISABLED, "sec_headset_send_end", NULL);
	if (ret < 0) {
		printk(KERN_ERR "SEC HEADSET: Failed to register send/end interrupt.\n");
		goto err_request_send_end_irq;
	}
	disable_irq(send_end->eint);

	det_headset = &hi->port.det_headset;
        s3c_gpio_cfgpin(det_headset->gpio, S3C_GPIO_SFN(det_headset->gpio_af));
        s3c_gpio_setpull(det_headset->gpio, S3C_GPIO_PULL_NONE);
        set_irq_type(det_headset->eint, IRQ_TYPE_EDGE_BOTH);
	
	ret = request_irq(det_headset->eint, detect_irq_handler,
			  IRQF_DISABLED, "sec_headset_detect", NULL);
	if (ret < 0) {
		printk(KERN_ERR "SEC HEADSET: Failed to register detect interrupt.\n");
		goto err_request_detect_irq;
	}
	
	wake_lock_init(&headset_sendend_wake_lock, WAKE_LOCK_SUSPEND, "sec_headset");

	schedule_work(&ear_switch_work);
	
	return 0;

err_request_send_end_irq:
	free_irq(det_headset->eint, 0);
err_request_detect_irq:
	switch_dev_unregister(&switch_earjack);
err_switch_dev_register:
	input_unregister_device(input);
err_register_input_dev:
	input_free_device(input);
err_request_input_dev:
	kfree (hi);

	return ret;
}

//kvpz: this is to add support for earjack, and send end key simulation from sysfs state store for the two
void sec_headset_sendend_report_key(void)
{
	
		input_report_key(hi->input, KEYCODE_SENDEND, 1);				
		input_sync(hi->input);
}

EXPORT_SYMBOL(sec_headset_sendend_report_key);

static int sec_headset_remove(struct platform_device *pdev)
{
	SEC_HEADSET_DBG("");
	input_unregister_device(hi->input);
	free_irq(hi->port.det_headset.eint, 0);
	free_irq(hi->port.send_end.eint, 0);
	switch_dev_unregister(&switch_earjack);
	return 0;
}
#ifdef CONFIG_PM
static int sec_headset_suspend(struct platform_device *pdev,
		pm_message_t state)
{

	SEC_HEADSET_DBG("");
	flush_scheduled_work();
	return 0;
}
static int sec_headset_resume(struct platform_device *pdev)
{
	SEC_HEADSET_DBG("");
	schedule_work(&ear_switch_work);
	schedule_work(&sendend_switch_work);
	return 0;
}
#else
#define s3c_headset_resume 	NULL
#define s3c_headset_suspend	NULL
#endif

static struct platform_driver sec_headset_driver = {
	.probe		= sec_headset_probe,
	.remove		= sec_headset_remove,
	.suspend	= sec_headset_suspend,
	.resume		= sec_headset_resume,
	.driver		= {
		.name		= "sec_headset",
		.owner		= THIS_MODULE,
	},
};

static int __init sec_headset_init(void)
{
	SEC_HEADSET_DBG("");
	return platform_driver_register(&sec_headset_driver);
}

static void __exit sec_headset_exit(void)
{
	platform_driver_unregister(&sec_headset_driver);
}

module_init(sec_headset_init);
module_exit(sec_headset_exit);

MODULE_AUTHOR("Eunki Kim <eunki_kim@samsung.com>");
MODULE_DESCRIPTION("SEC HEADSET detection driver");
MODULE_LICENSE("GPL");
