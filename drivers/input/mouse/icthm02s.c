/*
 * 
 *  Title : Hall Mouse driver for ICTHM02S   
 *  Date  : 2009.03.17
 *  Name  : ms17.kim
 *
 */

 
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>

#include "icthm02s.h"


/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/









#if 0
static int hm_spi_write_reg(struct hm_icthm02s_data *hm, u8 reg, u8 val)
{
	struct spi_message msg;
	struct spi_transfer msg_xfer = {
		.len        = 2,
		.cs_change  = 0,
		.delay_usecs = 30,
	};
	int retval;

	spi_message_init(&msg);

	hm->spi_wbuffer[0] = reg;
	hm->spi_wbuffer[1] = val;

	msg_xfer.tx_buf = hm->spi_wbuffer;
	msg_xfer.rx_buf = hm->spi_rbuffer;
	spi_message_add_tail(&msg_xfer, &msg);

	retval = spi_sync(hm->spi, &msg);

	/* write success */
	if (!retval)
		hm->reg_image[reg] = val;

	return retval;
}
#endif


static enum hrtimer_restart hm_timer_func(struct hrtimer *timer)
{
	struct hm_icthm02s_data *hm = container_of(timer, struct hm_icthm02s_data, timer);

	gprintk("hm->time_tick = %d\n",hm->time_tick);
	input_sync(hm->input_dev);



	if(hm->time_tick<3)
	{
		hm->time_tick = hm->time_tick + 1;

	}

	

	

	enable_irq(hm ->irq);
	
	return HRTIMER_NORESTART;
}

/*  cs low -> write 8bit -> delay -> write 8bit -> cs high  */ 
static int hm_spi_write_reg_type1(struct hm_icthm02s_data *hm, u8 reg, u8 val,int inverse)
{
	u8 w_reg[2]={0};
	u8 r_reg[2]={0};
	int ret;


	w_reg[0] = reg;
	w_reg[1] = val;


	if(inverse)
	{
			w_reg[0] = inverseBit(reg);
			w_reg[1] = inverseBit(val);
	}

	ret = spi_write_then_read(hm->spi,w_reg,2,r_reg,0);

	if(ret)
	{
		printk(KERN_ERR "write register failed \n");

	}
		


	return ret;



}

static int two_power(int val)
{
	int result=1;
	int i;
	for(i=0;i<val;i++)
	{
		result *= 2;

	}
	return result;

}

/* send LSB first for SPI */
static int inverseBit(u8 value_ori)
{

	int i;
	u8 temp[8]={0};
	u8 value_out=0;


	for(i=0;i<8;i++)
	{
		temp[i] = ((value_ori & (0x1 << i))>>i) * two_power(7-i);
		value_out += temp[i];

	}
	
	return value_out;

}

/* cs low -> write 8bit -> cs high -> delay -> cs low -> write 8bit -> cs high -> delay  */
#if 0
static int hm_spi_write_reg_type2(struct hm_icthm02s_data *hm,u8 reg,u8 val,int inverse)
{
	u8 r_reg=0;
	u8 w_reg_single,w_val_single;
	int ret;
	
	if(!inverse)
	{
		w_reg_single = reg;
		w_val_single = val;
	}
	else
	{
			w_reg_single = inverseBit(reg);
			w_val_single = inverseBit(val);
	}

	ret = spi_write_then_read(hm->spi,&w_reg_single,1,&r_reg,0);
	if(ret)
		return ret;
	udelay(30);

	ret = spi_write_then_read(hm->spi,&w_val_single,1,&r_reg,0);
	if(ret)
		return ret;
	udelay(30);


	return 0;

}
#endif

#if 0
static int hm_spi_read_reg(struct hm_icthm02s_data *hm, u8 reg, u8 *val,int inverse)
{
	u8 w_reg_single;

	int retval;

	w_reg_single= reg | (0x1 <<6);
	if(inverse)
	{
		w_reg_single = inverseBit(w_reg_single);
	}


	retval = spi_write_then_read(hm->spi,&w_reg_single,1,&val,1);


	return retval;



}
#endif


static int hm_spi_read_reg_type2(struct hm_icthm02s_data *hm,u8 reg,u8 *val,int inverse)
{
	u8 w_reg = reg | (0x1 <<6);
	
	u8 r_reg_dummy=0;
	u8 w_reg_dummy=0;

	int ret;
	
	if(inverse)
	{
	
		w_reg = inverseBit(w_reg);
	
	}

	ret = spi_write_then_read(hm->spi,&w_reg,1,&r_reg_dummy,0);
	if(ret)
		return ret;
	udelay(30);
	
	
	ret = spi_write_then_read(hm->spi,&w_reg_dummy,0,val,1);
	if(ret)
		return ret;
	udelay(30);


	return 0;
	


}


static int hm_spi_read_movement(struct hm_icthm02s_data *hm,int *data, int inverse)
{
	u8 w_reg;
	u8 r_data[3] = {0};
	
	
	u8 r_reg_dummy=0;
	u8 w_reg_dummy=0;

	int ret;
	
	
	if(!inverse)
	{
		w_reg = 0xc0;
		
	}
	else
	{
		w_reg = 0x03;
		

	}

	ret = spi_write_then_read(hm->spi,&w_reg,1,&r_reg_dummy,0);
	if(ret)
		return ret;
	 
	udelay(30);
	
	ret = spi_write_then_read(hm->spi,&w_reg_dummy,0,&r_data[0],1);
	if(ret)
		return ret;
	 
	
	udelay(30);
	
	ret = spi_write_then_read(hm->spi,&w_reg_dummy,0,&r_data[1],1);
	if(ret)
		return ret;
	 
	udelay(30);
	
	ret = spi_write_then_read(hm->spi,&w_reg_dummy,0,&r_data[2],1);
	if(ret)
		return ret;
	
	udelay(30);

	if(inverse)
	{
		r_data[0] = inverseBit(r_data[0]);
		r_data[1] = inverseBit(r_data[1]);
		r_data[2] = inverseBit(r_data[2]);

	}

	data[0] =(int)( r_data[0]);
	data[1] =(int)( r_data[1]);
	data[2] =(int)( r_data[2]);
	if(r_data[0] & (0x1 <<4))
		data[1] = data[1] * -1;

	if(r_data[0] & (0x1 <<5))
		data[2] = data[2] * -1;

	
	
	if(data[1]>7 || data[1] <-7 || data[2]>7 || data[2]<-7)
	{
		printk(KERN_ERR "read data is invalid\n");
		return -1;
	}
	
	return 0;


}

static int hm_process_data(struct hm_icthm02s_data *hm, int x, int y)
{

	bool sign_x=0,sign_y=0;
	int abs_x=abs(x);
	int abs_y=abs(y);
	int value;

	int delay_time [4][8] = {{ 515, 515, 515,  515,  515,  515,  515,  515},
					   		{600,800,700,600,500,400,300,200},
	                        {600,800,700,500,400,300,200,100},
							{600,800,700,400,300,200,150,50}
							};
	
	int tk = hm->time_tick;
	int move_type[8][8] = {{0,2,2,2,2,2,2,2},
						   {1,0,0,0,0,2,2,2},
						   {1,0,0,0,0,0,2,2},
						   {1,0,0,0,0,0,0,2},
						   {1,0,0,0,0,0,0,0},
						   {1,1,0,0,0,0,0,0},
						   {1,1,1,0,0,0,0,0},
						   {1,1,1,1,0,0,0,0}};

	


	if(!x && !y)
	{
		hm->time_tick = 0;
		enable_irq(hm->irq);
		return 0;

	}

	if(x<0)
		sign_x = 1;
	if(y<0)
		sign_y = 1;
	


	switch(move_type[abs_x][abs_y])
	{
		case 0 :  
				 if(sign_x)
				 {
				 	input_report_key(hm->input_dev,KEY_RIGHT,1);
					input_report_key(hm->input_dev,KEY_RIGHT,0);
			     }

				 else
				 {
				 	input_report_key(hm->input_dev,KEY_LEFT,1);
					input_report_key(hm->input_dev,KEY_LEFT,0);
				 	

				 }

				 if(sign_y)
				 {
				 	input_report_key(hm->input_dev,KEY_DOWN,1);
					input_report_key(hm->input_dev,KEY_DOWN,0);
			     }

				 else
				 {
				 	input_report_key(hm->input_dev,KEY_UP,1);
					input_report_key(hm->input_dev,KEY_UP,0);
				 	

				 }
				 value = (abs_x > abs_y) ? delay_time[tk][abs_x] : delay_time[tk][abs_y];
				 hrtimer_start(&hm->timer,ktime_set(value/1000,(value % 1000) *1000000),HRTIMER_MODE_REL);
				 break;



		case 1 :  
				 if(sign_x)
				 {
				 	input_report_key(hm->input_dev,KEY_RIGHT,1);
					input_report_key(hm->input_dev,KEY_RIGHT,0);
			     }

				 else
				 {
				 	input_report_key(hm->input_dev,KEY_LEFT,1);
					input_report_key(hm->input_dev,KEY_LEFT,0);
				 	

				 }
				 value = delay_time[tk][abs_x];

				 hrtimer_start(&hm->timer,ktime_set(value/1000,(value % 1000) *1000000),HRTIMER_MODE_REL);
				 
				 break;

		case 2 :  
				 if(sign_y)
				 {
				 	input_report_key(hm->input_dev,KEY_DOWN,1);
					input_report_key(hm->input_dev,KEY_DOWN,0);
			     }

				 else
				 {
				 	input_report_key(hm->input_dev,KEY_UP,1);
					input_report_key(hm->input_dev,KEY_UP,0);
				 	

				 }
				 
				 value = delay_time[tk][abs_y];
				 hrtimer_start(&hm->timer,ktime_set(value/1000,(value % 1000) *1000000),HRTIMER_MODE_REL);
				 break;

		default: break;


	}

	return 0;

}






void hm_work_func(struct work_struct *work)
{
	struct hm_icthm02s_data *hm = container_of(work, struct hm_icthm02s_data, work);
	int data[3]={0};
	int ret;
	event_type type= hm->type;


	/* read data from spi */

	ret = hm_spi_read_movement(hm,data,SPI_TYPE);
	if(ret)
		printk(KERN_ERR "Fail to read movement from spi\n");

	
	gprintk("x = %d , y = %d  \n",data[1],data[2]);

	
	/* report data */

	if(type ==KEY_TYPE || type==ALL_TYPE)
	{
		
		hm_process_data(hm,data[1],data[2]);
		
	}
	if(type== MOUSE_TYPE || type ==ALL_TYPE)
	{

		gprintk("..\n");
		input_report_rel(hm->input_dev, REL_X, data[1]);
		input_report_rel(hm->input_dev, REL_Y, data[2]);
		input_sync(hm->input_dev);

	}




}

irqreturn_t hm_irq_handler(int irq, void *dev_id)
{
	struct hm_icthm02s_data *hm = dev_id;
	disable_irq(hm->irq);

	queue_work(hm_wq, &hm->work);

	return IRQ_HANDLED;


}





static int hm_spi_suspend(struct spi_device *spi)
{
    struct hm_icthm02s_data *hm = spi_get_drvdata(spi);

	gprintk("..\n");
	disable_irq(hm ->irq);
	gpio_direction_output(GPIO_HM_EN,GPIO_LEVEL_LOW);

	return 0;

}
static int hm_spi_resume(struct spi_device *spi)
{
	int i,ret;
    struct hm_icthm02s_data *hm = spi_get_drvdata(spi);
	event_type type;
	type = HM_TYPE;

	gprintk("..\n");
	gpio_direction_output(GPIO_HM_EN,GPIO_LEVEL_HIGH);

	mdelay(300);


		for(i=0x00;i<0x08;i++)
		{
			ret = hm_spi_write_reg_type1(hm,i,hm_icthm02s_original_image[i],SPI_TYPE);
			if(ret)
				return ret;

	
		}
		
	if(type == MOUSE_TYPE)
	{
		ret = hm_spi_write_reg_type1(hm,REG_SRATE,0x10,SPI_TYPE);
		if(ret)
			return ret;
	}
		
		mdelay(20);

	

	

	enable_irq(hm ->irq);
	return 0;

}

static int __devinit hm_spi_probe(struct spi_device *spi)
{
	struct hm_icthm02s_data *hm;
	int irq;
	int i,ret;	
	event_type type;
	u8 test_read;
	u8 result;
	
	/* alloc driver data */
    hm = kzalloc(sizeof(struct hm_icthm02s_data), GFP_KERNEL);
    if (!hm) 
	{
        gprintk("kzalloc error\n");
    	return -ENOMEM;
	}
	
	hm->spi = spi;
	

	if(HM_TYPE < 0 ||  HM_TYPE>MAX_TYPE_NUMBER)
	{	
		return -1;
	}
	
	type = HM_TYPE;
	hm->type = type;
	hm->time_tick = 0;
	hrtimer_init(&hm->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hm->timer.function = hm_timer_func;

	spi_set_drvdata(spi,hm);

	/* Input device Settings */
	
	hm->input_dev = input_allocate_device();
	if (hm->input_dev == NULL) 
	{
		gprintk("Failed to allocate input device\n");
		return -ENOMEM;
	}
	hm->input_dev->name = "hallmouse";

	set_bit(EV_SYN, hm->input_dev->evbit);
	
	if(type==MOUSE_TYPE || type ==ALL_TYPE)
	{

	
		gprintk("..\n");
		set_bit(EV_REL   , hm->input_dev->evbit);

		set_bit(REL_X    , hm->input_dev->relbit);
		set_bit(REL_Y    , hm->input_dev->relbit);
	
	}

	if(type==KEY_TYPE || type==ALL_TYPE)
	{

		gprintk("..\n");
		set_bit(EV_KEY,    hm->input_dev->evbit);

		set_bit(KEY_LEFT,  hm->input_dev->keybit);
		set_bit(KEY_RIGHT, hm->input_dev->keybit);
		set_bit(KEY_UP,    hm->input_dev->keybit);
		set_bit(KEY_DOWN,  hm->input_dev->keybit);
	}

	ret = input_register_device(hm->input_dev);
	if (ret) 
	{
		gprintk("Unable to register %s input device\n", hm->input_dev->name);
		input_free_device(hm->input_dev);
		kfree(hm);
		return -1;
	}


	/* INT SETTINGS */	
	irq = IRQ_HM_INT;
	hm->irq = -1;
	ret = request_irq(irq, hm_irq_handler, 0, "icthm02s", hm);
	if (ret) {
		gprintk("unable to request irq %d\n", irq);
		return ret;
	}       
	hm->irq = irq;

	gprintk("INT Settings complete\n");
	
	/* WORK QUEUE SETTING */
    INIT_WORK(&hm->work, hm_work_func);


	/* init hm register */

	//while(1)
	{
		for(i=0x00;i<MAX_REG_NUMBER;i++)
		{
			ret = hm_spi_write_reg_type1(hm,i,hm_icthm02s_original_image[i],SPI_TYPE);
			if(ret)
			{

				free_irq(IRQ_HM_INT,hm);
				gpio_direction_output(GPIO_HM_EN,GPIO_LEVEL_LOW);

				input_unregister_device(hm->input_dev);
				kfree(hm);

				gpio_free(GPIO_HM_EN);
				gpio_free(GPIO_HMLDO_EN);
				return -1;
			}
	
		}
		
	
		
		mdelay(20);
	
		for(i=0x00;i<MAX_REG_NUMBER;i++)
		{
			test_read = 0;
			hm_spi_read_reg_type2(hm,i,&test_read,SPI_TYPE);
		
			
			if(SPI_TYPE==INVERSE_TYPE)
			{
				result = inverseBit(test_read);
	
				gprintk("i= %x, test_read = %x , result = %x \n",i,test_read, result);
		
			}
			else
			{
				gprintk("i= %x, test_read = %x \n",i,test_read);

			}
		}

		
	
		mdelay(500);

	}


	
	

	if(type==MOUSE_TYPE)
	{

		gprintk("..\n");
		hm_spi_write_reg_type1(hm,REG_SRATE,0x10,SPI_TYPE);
		mdelay(20);
	}


	printk("Hall Mouse Settings complete\n");

	

	
	return 0;

}


static int hm_hw_init(void)
{

	/* VDD On */
	if (gpio_is_valid(GPIO_HMLDO_EN))
	{
		if (gpio_request(GPIO_HMLDO_EN, S3C_GPIO_LAVEL(GPIO_HMLDO_EN)))
			printk(KERN_ERR "Filed to request GPIO_HMLDO_EN!\n");
		gpio_direction_output(GPIO_HMLDO_EN, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_HMLDO_EN, S3C_GPIO_PULL_NONE); 

	/* INT Settings */
	s3c_gpio_cfgpin(GPIO_HM_INT, S3C_GPIO_SFN(GPIO_HM_INT_AF));
	s3c_gpio_setpull(GPIO_HM_INT, S3C_GPIO_PULL_NONE);
	set_irq_type(IRQ_HM_INT, IRQ_TYPE_EDGE_FALLING);

	/* Power On */
	if (gpio_is_valid(GPIO_HM_EN))
	{
		if (gpio_request(GPIO_HM_EN, S3C_GPIO_LAVEL(GPIO_HM_EN)))
			printk(KERN_ERR "Filed to request GPIO_HM_EN!\n");
		gpio_direction_output(GPIO_HM_EN, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_HM_EN, S3C_GPIO_PULL_NONE); 

	mdelay(5);
	gpio_direction_output(GPIO_HM_EN,GPIO_LEVEL_HIGH);
	return 0;
}



static struct spi_driver hm_spi_driver = {
	.driver = {
		.name =     "hm_spi",
	},
	.probe   =   hm_spi_probe,
	.suspend =   hm_spi_suspend,
	.resume  =   hm_spi_resume,
	.remove  =   __devexit_p(hm_spi_remove),
};


static int __devexit hm_spi_remove(struct spi_device *spi)
{
    struct hm_icthm02s_data *hm = spi_get_drvdata(spi);



    if (hm_wq)
		destroy_workqueue(hm_wq);

	free_irq(IRQ_HM_INT,hm);
	

	
	gpio_direction_output(GPIO_HM_EN,GPIO_LEVEL_LOW);


	input_unregister_device(hm->input_dev);
	kfree(hm);

	gpio_free(GPIO_HM_EN);
	gpio_free(GPIO_HMLDO_EN);

	

	return 0;

}

static int __init hm_init(void)
{

	gprintk("debug start\n");
	
	hm_hw_init();

	mdelay(300);

    hm_wq = create_singlethread_workqueue("hm_wq");
    if (!hm_wq)
	    return -ENOMEM;
					 

	return spi_register_driver(&hm_spi_driver);
}
module_init(hm_init);


static void __exit hm_exit(void)
{
	spi_unregister_driver(&hm_spi_driver);

	printk("hall mouse module is removed\n");
}
module_exit(hm_exit);



MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Hall Mouse driver for icthm02s");
MODULE_LICENSE("GPL");

