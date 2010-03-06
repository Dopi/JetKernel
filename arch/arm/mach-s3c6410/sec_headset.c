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

//#define CONFIG_DEBUG_SEC_HEADSET

#ifdef CONFIG_DEBUG_SEC_HEADSET
#define SEC_HEADSET_DBG(fmt, arg...) printk(KERN_INFO "[HEADSET] " fmt "\r\n", ## arg)
#else
#define SEC_HEADSET_DBG(fmt, arg...) 
#endif

#define KEYCODE_SENDEND 248

#define HEADSET_CHECK_COUNT	3
#define	HEADSET_CHECK_TIME	get_jiffies_64() + (HZ/5)// 1000ms / 10 = 100ms
#define	SEND_END_ENABLE_TIME 	get_jiffies_64() + (HZ*2)// 1000ms * 2 = 2sec

#define SEND_END_CHECK_COUNT	3
#define SEND_END_CHECK_TIME get_jiffies_64() + 6 //30ms
//#define SEND_END_CHECK_TIME get_jiffies_64() + 30 /*(HZ/25) //1000ms / 25 = 40ms*/
//#define SEND_END_CHECK_TIME get_jiffies_64() + (HZ/100) //1000ms / 100 = 10ms 

extern int s3c_adc_get_adc_data(int channel);

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

static struct timer_list headset_detect_timer;
static struct timer_list send_end_key_event_timer;

static unsigned int headset_detect_timer_token;
static unsigned int send_end_key_timer_token;
static unsigned int send_end_irq_token;
static unsigned short int headset_status;
static struct wake_lock headset_sendend_wake_lock;

short int get_headset_status()
{
	SEC_HEADSET_DBG(" headset_status %d", headset_status);
	return headset_status;
}

EXPORT_SYMBOL(get_headset_status);
static void release_headset_event(unsigned long arg)
{
	printk("Headset attached\n");
	headset_status = 1;
	switch_set_state(&switch_earjack, 1);
}
static DECLARE_DELAYED_WORK(release_headset_event_work, release_headset_event);

static void ear_adc_caculrator(unsigned long arg)
{
	int adc = 0;
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	
	if (state)
	{
		adc = s3c_adc_get_adc_data(3);
        if((adc > 1700 && adc < 2000) || (adc > 2400 && adc < 2700) || (adc > 2900 && adc < 3400) || (adc > 400 && adc < 700))
		{
			printk("4pole ear-mic adc is %d\n", adc);
			enable_irq (send_end->eint);
			send_end_irq_token++;
		}
		else if(adc < 5)
		{
			printk("3pole earphone adc is %d\n", adc);
			headset_status = 0;
		}
		else
		{
			printk(KERN_ALERT "Wrong adc value!! adc is %d\n", adc);
			headset_status = 0;
		}
	}
	else
	{
		printk(KERN_ALERT "Error : mic bias enable complete but headset detached!!\n");
		gpio_set_value(GPIO_MICBIAS_EN, 0);
	}

	wake_unlock(&headset_sendend_wake_lock);
}

static DECLARE_DELAYED_WORK(ear_adc_cal_work, ear_adc_caculrator);

static void headset_detect_timer_handler(unsigned long arg)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	int state;
	state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if(state)
	{
		SEC_HEADSET_DBG("headset_detect_timer_token is %d\n", headset_detect_timer_token);
		if(headset_detect_timer_token < 3)
		{
			headset_detect_timer.expires = HEADSET_CHECK_TIME;
			add_timer(&headset_detect_timer);
			headset_detect_timer_token++;
		}
		else if(headset_detect_timer_token == 3)
		{
			headset_detect_timer.expires = SEND_END_ENABLE_TIME;
			add_timer(&headset_detect_timer);
			headset_detect_timer_token++;
			schedule_work(&release_headset_event_work);
		}
		else if(headset_detect_timer_token == 4)
		{
			gpio_set_value(GPIO_MICBIAS_EN, 1); 
			schedule_delayed_work(&ear_adc_cal_work, 200);
			SEC_HEADSET_DBG("mic bias enable add work queue \n");
			headset_detect_timer_token = 0;
		}
		else
			printk(KERN_ALERT "wrong headset_detect_timer_token count %d", headset_detect_timer_token);
	}
	else
		printk(KERN_ALERT "headset detach!! %d", headset_detect_timer_token);
}

static void ear_switch_change(struct work_struct *ignored)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state;

	del_timer(&headset_detect_timer);
	cancel_delayed_work_sync(&ear_adc_cal_work);
	state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if (state && !send_end_irq_token)
	{		
		wake_lock(&headset_sendend_wake_lock);
		SEC_HEADSET_DBG("Headset attached timer start\n");
		headset_detect_timer_token = 0;
		headset_detect_timer.expires = HEADSET_CHECK_TIME;
		add_timer(&headset_detect_timer);
	}
	else if(!state)
	{
		switch_set_state(&switch_earjack, state);
		printk("Headset detached %d \n", send_end_irq_token);        	
		headset_status = state;
		if(send_end_irq_token > 0)
		{
			disable_irq (send_end->eint);
			send_end_irq_token--;
		}
		wake_unlock(&headset_sendend_wake_lock);
	}
	else
		SEC_HEADSET_DBG("Headset state does not valid. or send_end event");

}
static void send_end_key_event_timer_handler(unsigned long arg)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int sendend_state, headset_state = 0;
	
	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	sendend_state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && sendend_state)
	{
		if(send_end_key_timer_token < SEND_END_CHECK_COUNT)
		{	
			send_end_key_timer_token++;
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; 
			add_timer(&send_end_key_event_timer);
			SEC_HEADSET_DBG("SendEnd Timer Restart %d", send_end_key_timer_token);
		}
		else if(send_end_key_timer_token == SEND_END_CHECK_COUNT)
		{
			printk("SEND/END is pressed\n");
			input_report_key(hi->input, KEYCODE_SENDEND, 1);
			input_sync(hi->input);
			send_end_key_timer_token = 0;
		}
		else
			printk(KERN_ALERT "[Headset]wrong timer counter %d\n", send_end_key_timer_token);
	}else
			printk(KERN_ALERT "[Headset]GPIO Error\n");
}

static void sendend_switch_change(struct work_struct *ignored)
{

	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state, headset_state;

	del_timer(&send_end_key_event_timer);
	send_end_key_timer_token = 0;
    mdelay(10); // for earjack keyevent delay	
	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && send_end_irq_token)//headset connect && send irq enable
	{
		if(!state)
		{
			SEC_HEADSET_DBG(KERN_ERR "SISO:sendend isr work queue\n");
    			switch_set_state(&switch_sendend, state);
			input_report_key(hi->input, KEYCODE_SENDEND, state);
			input_sync(hi->input);
			printk("SEND/END %s.\n", "released");
			wake_unlock(&headset_sendend_wake_lock);
		}else{
			wake_lock(&headset_sendend_wake_lock);
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; // 10ms ??
			add_timer(&send_end_key_event_timer);
			switch_set_state(&switch_sendend, state);
			SEC_HEADSET_DBG("SEND/END %s.timer start \n", "pressed");
		}

	}else{
		SEC_HEADSET_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", state?"pressed":"released");
	}
}

static DECLARE_WORK(ear_switch_work, ear_switch_change);
static DECLARE_WORK(sendend_switch_work, sendend_switch_change);

static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
	SEC_HEADSET_DBG("headset isr");
	schedule_work(&ear_switch_work);
	return IRQ_HANDLED;
}
 

static irqreturn_t send_end_irq_handler(int irq, void *dev_id)
{
   struct sec_gpio_info   *det_headset = &hi->port.det_headset;
   int headset_state;

   SEC_HEADSET_DBG("sendend isr");
   del_timer(&send_end_key_event_timer);
   headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

   if (headset_state)
   {
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
	
	init_timer(&headset_detect_timer);
	headset_detect_timer.function = headset_detect_timer_handler;

	init_timer(&send_end_key_event_timer);
	send_end_key_event_timer.function = send_end_key_event_timer_handler;

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
