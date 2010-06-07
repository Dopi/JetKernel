/*****************************************************************************
 *
 * COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
 *
 *****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>

#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

#include "opt_joy.h"

struct optjoy_drv_data {
	struct input_dev *input_dev;
	struct hrtimer timer;
	/* in order to lock touch key during sending event for user's convenience*/
	struct hrtimer timer_touchlock; 
	struct work_struct  work;
	int use_irq;
	struct early_suspend early_suspend;
};

static struct workqueue_struct *optjoy_workqueue;

static int16_t sum_x = 0;
static int16_t sum_y = 0;

static int sending_oj_event = INACTIVE;
static int lock_oj_event = INACTIVE;


#ifdef CONFIG_HAS_EARLYSUSPEND
static void optjoy_spi_early_suspend(struct early_suspend *h);
static void optjoy_spi_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#define OJT_SPI_SET_ADDR_WRITE(addr)  (addr | 0x80)
#define OJT_SPI_SET_ADDR_READ(addr)   (addr & 0x7F)



int get_sending_oj_event(void)
{
	return sending_oj_event;

}

EXPORT_SYMBOL(get_sending_oj_event);

void set_lock_oj_event(int num)
{
	lock_oj_event = num;

}
EXPORT_SYMBOL(set_lock_oj_event);



static void optjoy_spi_write_data(uint8_t data)
{
    uint8_t i;

	for( i = 0; i < 8; i++ ) {

		if( data & 0x80)
			gpio_set_value(GPIO_OJ_SPI_MOSI, OJT_HI);  // MOSI			
		else
			gpio_set_value(GPIO_OJ_SPI_MOSI, OJT_LO);	

		gpio_set_value(GPIO_OJ_SPI_CLK, OJT_LO);	  // SCLK

		data <<= 1;

		gpio_set_value(GPIO_OJ_SPI_CLK, OJT_HI);	  
	}	
	gpio_set_value(GPIO_OJ_SPI_MOSI, OJT_HI);  // MOSI	
}

static void optjoy_spi_write_byte(uint8_t address,uint8_t data)
{
	gpio_set_value(GPIO_OJ_CS, OJT_HI);    // NCS = High
	gpio_set_value(GPIO_OJ_CS, OJT_LO);     // NCS = Low

	udelay(10);	

	gpio_set_value(GPIO_OJ_SPI_CLK, OJT_HI);      // SCLK
	gpio_set_value(GPIO_OJ_SPI_MOSI, OJT_HI);  // MOSI	

	optjoy_spi_write_data(OJT_SPI_SET_ADDR_WRITE(address));
	optjoy_spi_write_data(data);

	udelay(10);	
	
	gpio_set_value(GPIO_OJ_CS, OJT_HI);    // NCS = High

}

static uint8_t optjoy_spi_read_data(void)
{
	uint8_t i;
	uint8_t ret = 0x00;


	for( i = 0; i < 8; i++ ) {
		gpio_set_value(GPIO_OJ_SPI_CLK, OJT_LO);	// SCLK  

		udelay(1);	

		ret = ret << 1;        

		if(gpio_get_value(GPIO_OJ_SPI_MISO))
			ret |= 0x01;

		gpio_set_value(GPIO_OJ_SPI_CLK, OJT_HI);      // SCLK
	}	

	return ret;
}

static uint8_t optjoy_spi_read_byte(uint8_t address)
{
	 uint8_t ret;

	 gpio_set_value(GPIO_OJ_CS, OJT_HI);    // NCS = High
	 gpio_set_value(GPIO_OJ_CS, OJT_LO);     // NCS = Low

	 udelay(10);	
	 
	 gpio_set_value(GPIO_OJ_SPI_CLK, OJT_HI);      // SCLK
	 gpio_set_value(GPIO_OJ_SPI_MOSI, OJT_HI);  // MOSI	

	 optjoy_spi_write_data(OJT_SPI_SET_ADDR_READ(address));

#if defined(CONFIG_MACH_VINSQ)
         udelay(200); //vinsq.boot temp.. needed for LJ
#endif
	 ret = optjoy_spi_read_data();

	 udelay(10);	
	 
	 gpio_set_value(GPIO_OJ_CS, OJT_HI);    // NCS = High

	 return ret;
}

static void optjoy_spi_work_func(struct work_struct *work)
{
	struct optjoy_drv_data *optjoy_data = container_of(work, struct optjoy_drv_data, work);
	uint8_t nx, ny, mot_val;
	u8 sq,pxsum, shu_up,shu_dn;
	u16 shutter;
	int8_t dx, dy;
	int i;
	unsigned int keycode = 0;	
	bool check_env = false;

	u16 oj_sht_tbl[12]= {0,500,750,1000,1250,1500,1750,2000,2250,2500,2750,3000};
	u8  oj_pxsum_tbl[12] = {0,0,40,50,60,70,80,80,80,90,90,90};
	u8  oj_sq_tbl[12] = {0,0,35,35,35,35,35,35,35,35,35,35};


	/* reading motion */
   	mot_val = optjoy_spi_read_byte(OJT_MOT);
	if(!(mot_val & 0x80) || lock_oj_event || mot_val==0xFF)
		return;


#ifndef CONFIG_MACH_VINSQ
   	sq = optjoy_spi_read_byte(OJT_SQ);
   	shu_up = optjoy_spi_read_byte(OJT_SHUTTER_UP);
   	shu_dn = optjoy_spi_read_byte(OJT_SHUTTER_DOWN);
	shutter = (shu_up << 8) | shu_dn;
   	pxsum = optjoy_spi_read_byte(OJT_PIXEL_SUM);
#endif
	nx = optjoy_spi_read_byte(OJT_DELT_Y);
	ny = optjoy_spi_read_byte(OJT_DELT_X);

#ifndef CONFIG_MACH_VINSQ
	for(i=1;i<12;i++)
	{
		if( ((oj_sht_tbl[i-1] <= shutter) && (shutter < oj_sht_tbl[i])) && 
		((oj_pxsum_tbl[i]<=pxsum) && (oj_sq_tbl[i] <=sq)) )
		{
			gprintk("[OJ_KEY] valid environment \n");
			check_env = true;
			
			break;
		}

	}

	if(!check_env)
	{
		gprintk("[OJ_KEY] invalid environment \n");
		return;
	}
#endif


	dx = (int8_t)nx;
	dy = ((int8_t)ny) * (-1);


	sum_x = sum_x + dx;
	sum_y = sum_y + dy;

	gprintk("dx=%d, dy=%d , sum_x = %d , sum_y = %d mot_val=0x%2X \n",dx, dy,sum_x ,sum_y, mot_val);

#if defined(CONFIG_MACH_VINSQ)
	if(sum_x > SUM_X_THRESHOLD) keycode = SEC_KEY_LEFT;
	else if(sum_x < -SUM_X_THRESHOLD) keycode = SEC_KEY_RIGHT;
	else if(sum_y > SUM_Y_THRESHOLD) keycode = SEC_KEY_DOWN;
	else if(sum_y < -SUM_Y_THRESHOLD) keycode = SEC_KEY_UP;
	else keycode = 0;
#else
	if(sum_x > SUM_X_THRESHOLD) keycode = SEC_KEY_DOWN;
	else if(sum_x < -SUM_X_THRESHOLD) keycode = SEC_KEY_UP;
	else if(sum_y > SUM_Y_THRESHOLD) keycode = SEC_KEY_LEFT;
	else if(sum_y < -SUM_Y_THRESHOLD) keycode = SEC_KEY_RIGHT;
	else keycode = 0;
#endif
	if (keycode) {

		input_report_key(optjoy_data->input_dev, keycode, 1);
		input_report_key(optjoy_data->input_dev, keycode, 0);
		input_sync(optjoy_data->input_dev);

		hrtimer_cancel(&optjoy_data->timer_touchlock); 

		gprintk("[opt_joy] key code: %d (sum_x: %d, sum_y: %d)\n",keycode,sum_x,sum_y);
		sum_x = sum_y = 0;
		
		sending_oj_event = ACTIVE;

		hrtimer_start(&optjoy_data->timer_touchlock, ktime_set(0,500000000), HRTIMER_MODE_REL);
	}

	if (optjoy_data->use_irq)
		enable_irq(IRQ_OJT_INT);
}

static enum hrtimer_restart optjoy_spi_timer_func_touchlock(struct hrtimer *timer)
{
	//struct optjoy_drv_data *optjoy_data = container_of(timer, struct optjoy_drv_data, timer_touchlock);
	gprintk("\n");
	
	sending_oj_event = INACTIVE;

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart optjoy_spi_timer_func(struct hrtimer *timer)
{
	struct optjoy_drv_data *optjoy_data = container_of(timer, struct optjoy_drv_data, timer);

	// puts a job into the workqueue
	queue_work(optjoy_workqueue, &optjoy_data->work);
	
	hrtimer_start(&optjoy_data->timer, ktime_set(0, OJT_POLLING_PERIOD), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

static irqreturn_t optjoy_spi_irq_handler(int irq, void *dev_id)
{
	struct optjoy_drv_data *optjoy_data = dev_id;
	gprintk(" -> work_func \n");
	
	disable_irq(irq);

#if 0
	schedule_work(&optjoy_data->work);
#else
	// puts a job into the workqueue
	queue_work(optjoy_workqueue, &optjoy_data->work);
#endif

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void optjoy_spi_early_suspend(struct early_suspend *h)
{
	struct optjoy_drv_data *optjoy_data;
	optjoy_data = container_of(h, struct optjoy_drv_data, early_suspend);

	hrtimer_cancel(&optjoy_data->timer);
	cancel_work_sync(&optjoy_data->work);

	printk(KERN_INFO "Optical Joystick : suspend - gpio\n");
	
	gpio_set_value(GPIO_OJ_SHUTDOWN, OJT_HI); 

	mdelay(1);
	
	s3c_gpio_cfgpin(GPIO_OJ_SPI_MISO, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OJ_SPI_MISO, S3C_GPIO_PULL_DOWN);
	
	s3c_gpio_cfgpin(GPIO_OJ_SPI_CLK, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OJ_SPI_CLK, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_OJ_SPI_MOSI, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OJ_SPI_MOSI, S3C_GPIO_PULL_DOWN);

	s3c_gpio_cfgpin(GPIO_OJ_CS, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OJ_CS, S3C_GPIO_PULL_DOWN);

	

	

	gpio_set_value(GPIO_OJ_SHUTDOWN, OJT_LO); 


	printk(KERN_INFO "Optical Joystick : suspend.\n");
}

static void optjoy_spi_late_resume(struct early_suspend *h)
{
	struct optjoy_drv_data *optjoy_data;
	optjoy_data = container_of(h, struct optjoy_drv_data, early_suspend);

	// initialize....
	gpio_set_value(GPIO_OJ_SHUTDOWN, OJT_HI); 
	msleep(1);
	s3c_gpio_cfgpin(GPIO_OJ_SPI_MISO, S3C_GPIO_SFN(OJT_GPIO_IN));
	s3c_gpio_setpull(GPIO_OJ_SPI_MISO, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_OJ_SPI_CLK, S3C_GPIO_SFN(OJT_GPIO_OUT));
	s3c_gpio_setpull(GPIO_OJ_SPI_CLK, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_OJ_SPI_MOSI, S3C_GPIO_SFN(OJT_GPIO_OUT));
	s3c_gpio_setpull(GPIO_OJ_SPI_MOSI, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_OJ_CS, S3C_GPIO_SFN(OJT_GPIO_OUT));
	s3c_gpio_setpull(GPIO_OJ_CS, S3C_GPIO_PULL_NONE);

	
	msleep(50);

	gpio_set_value(GPIO_OJ_CS, OJT_HI); 
	gpio_set_value(GPIO_OJ_SHUTDOWN, OJT_LO); 
	msleep(200);
	gpio_set_value(GPIO_OJ_CS, OJT_LO); 
	optjoy_spi_write_byte(OJT_POWER_UP_RESET,0x5A);
	msleep(20);

	// OJ Register Clear
	optjoy_spi_read_byte(OJT_MOT);
	optjoy_spi_read_byte(OJT_DELT_Y);
	optjoy_spi_read_byte(OJT_DELT_X);

	hrtimer_start(&optjoy_data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	printk(KERN_INFO "Optical Joystick : resume.\n");
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */  

static int optjoy_hw_init(void)
{
	// initialize....
	gpio_set_value(GPIO_OJ_CS, OJT_HI); 
	gpio_set_value(GPIO_OJ_SHUTDOWN, OJT_LO); 
	gpio_set_value(GPIO_OJ_CS, OJT_LO); 
	optjoy_spi_write_byte(OJT_POWER_UP_RESET,0x5A);
	mdelay(100);

	// OJ Register Clear
	optjoy_spi_read_byte(OJT_MOT);
	optjoy_spi_read_byte(OJT_DELT_Y);
	optjoy_spi_read_byte(OJT_DELT_X);

	return 0;
}	

static int optjoy_gpio_init(void)
{
	/* SPI Pin Settings */
	s3c_gpio_cfgpin(GPIO_OJ_SPI_MISO, S3C_GPIO_SFN(OJT_GPIO_IN));
	s3c_gpio_setpull(GPIO_OJ_SPI_MISO, S3C_GPIO_PULL_NONE);
	
	s3c_gpio_cfgpin(GPIO_OJ_SPI_CLK, S3C_GPIO_SFN(OJT_GPIO_OUT));
	s3c_gpio_cfgpin(GPIO_OJ_SPI_MOSI, S3C_GPIO_SFN(OJT_GPIO_OUT));
	s3c_gpio_cfgpin(GPIO_OJ_CS, S3C_GPIO_SFN(OJT_GPIO_OUT));

	/* Power mode Settings */
	/* HIGH -> SHUTDOWN MODE   ;  LOW -> ACTIVE MODE */  
	s3c_gpio_cfgpin(GPIO_OJ_SHUTDOWN, S3C_GPIO_SFN(OJT_GPIO_OUT));

	/* INT Settings */
	s3c_gpio_cfgpin(GPIO_OJ_MOTION, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_OJ_MOTION, S3C_GPIO_PULL_DOWN);

#if 0
	set_irq_type(IRQ_OJT_INT, IRQ_TYPE_EDGE_FALLING); 
#endif

	return 0;
}

static int __devinit optjoy_spi_probe(struct platform_device *pdev)
{
	struct optjoy_drv_data *optjoy_data;
	int ret = 0;

	gprintk("start.\n");

	optjoy_gpio_init();  

	optjoy_workqueue = create_singlethread_workqueue("optjoy_workqueue");  
	if (optjoy_workqueue == NULL){
		printk(KERN_ERR "[optjoy_spi_probe] create_singlethread_workqueue failed.\n");
		ret = -ENOMEM;
		goto err_create_workqueue_failed;
	}

	/* alloc driver data */
	optjoy_data = kzalloc(sizeof(struct optjoy_drv_data), GFP_KERNEL);
	if (!optjoy_data) {
		printk(KERN_ERR "[optjoy_spi_probe] kzalloc error\n");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
  	
	optjoy_data->input_dev = input_allocate_device();
	if (optjoy_data->input_dev == NULL) {
		printk(KERN_ERR "[optjoy_spi_probe] Failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}


	/* workqueue initialize */
	INIT_WORK(&optjoy_data->work, optjoy_spi_work_func);

	optjoy_hw_init();

	optjoy_data->input_dev->name = "optjoy_device";
	//optjoy_data->input_dev->phys = "optjoy_device/input2";

	set_bit(EV_KEY, optjoy_data->input_dev->evbit);

	set_bit(SEC_KEY_LEFT, optjoy_data->input_dev->keybit);
	set_bit(SEC_KEY_RIGHT, optjoy_data->input_dev->keybit);
	set_bit(SEC_KEY_UP, optjoy_data->input_dev->keybit);	
	set_bit(SEC_KEY_DOWN, optjoy_data->input_dev->keybit);

	optjoy_data->input_dev->keycode = optjoy_keycode;

	ret = input_register_device(optjoy_data->input_dev);
	if (ret) {
		printk(KERN_ERR "[optjoy_spi_probe] Unable to register %s input device\n", optjoy_data->input_dev->name);
		goto err_input_register_device_failed;
	}

#if 0  //TEMP
	/* IRQ setting */
	ret = request_irq(IRQ_OJT_INT, optjoy_spi_irq_handler, 0, "optjoy_device", optjoy_data);
	if (!ret) {
		optjoy_data->use_irq = 1;
		gprintk("Start INTERRUPT mode!\n");
	}
	else {
		gprintk(KERN_ERR "[optjoy_spi_probe] unable to request_irq\n");
		optjoy_data->use_irq = 0;
	}       
#else
	optjoy_data->use_irq = 0;
#endif

	/* timer init & start (if not INTR mode...) */
	if (!optjoy_data->use_irq) {
		hrtimer_init(&optjoy_data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		optjoy_data->timer.function = optjoy_spi_timer_func;
		hrtimer_start(&optjoy_data->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	hrtimer_init(&optjoy_data->timer_touchlock, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	optjoy_data->timer_touchlock.function = optjoy_spi_timer_func_touchlock;
	

#ifdef CONFIG_HAS_EARLYSUSPEND
	optjoy_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	optjoy_data->early_suspend.suspend = optjoy_spi_early_suspend;
	optjoy_data->early_suspend.resume = optjoy_spi_late_resume;
	register_early_suspend(&optjoy_data->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	return 0;

err_input_register_device_failed:
	input_free_device(optjoy_data->input_dev);

err_input_dev_alloc_failed:
	kfree(optjoy_data);

err_create_workqueue_failed:
err_alloc_data_failed:
	return ret;
}

static int __devexit optjoy_spi_remove(struct platform_device *pdev)
{
	struct optjoy_drv_data *optjoy_data = platform_get_drvdata(pdev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&optjoy_data->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	gprintk("...\n");

	if (optjoy_workqueue)
		destroy_workqueue(optjoy_workqueue);  // moved to here
	
	free_irq(IRQ_OJT_INT, optjoy_data);
	input_unregister_device(optjoy_data->input_dev);

	kfree(optjoy_data);
	platform_set_drvdata(pdev, NULL);
	
	return 0;
}

static struct platform_driver optjoy_spi_driver = {
	.probe		= optjoy_spi_probe,
	.remove		= __devexit_p(optjoy_spi_remove),
	.driver		= {
		.name	= "optjoy_device",
		.owner    = THIS_MODULE,
	},
};

static int __init optjoy_spi_init(void)
{
	gprintk("start!\n");
	return platform_driver_register(&optjoy_spi_driver);
}

static void __exit optjoy_spi_exit(void)
{
	gprintk("...\n");
	platform_driver_unregister(&optjoy_spi_driver);
}

module_init(optjoy_spi_init);
module_exit(optjoy_spi_exit);

MODULE_AUTHOR("Hyung-Gyun Kim <hyunggyun.kim@samsung.com>");
MODULE_DESCRIPTION("Crucialtec Otptical Joystick Driver");
MODULE_LICENSE("GPL");
