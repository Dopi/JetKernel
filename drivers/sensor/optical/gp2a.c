/* 
 *  Title : Optical Sensor(light sensor + proximity sensor) driver for GP2AP002A00F   
 *  Date  : 2009.02.27
 *  Name  : ms17.kim
 *
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

#include <linux/input.h>
#include <linux/workqueue.h>



#include "gp2a.h"

/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/





/* global var */
static struct i2c_client *opt_i2c_client = NULL;

struct class *lightsensor_class;

struct device *switch_cmd_dev;

static bool light_enable = OFF;
static bool proximity_enable = OFF;
static state_type cur_state = STATE_0;

static short proximity_value = 0;






static struct i2c_driver opt_i2c_driver = {
	.driver = {
		.name = "gp2a",
	},
	//.id = GP2A_ID,
	.attach_adapter = &opt_attach_adapter,
};


static struct file_operations proximity_fops = {
	.owner  = THIS_MODULE,
	.open   = proximity_open,
    .release = proximity_release,
    .unlocked_ioctl = proximity_ioctl,
};
                 
static struct miscdevice proximity_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "proximity",
    .fops   = &proximity_fops,
};

static unsigned short opt_normal_i2c[] = {(GP2A_ADDR>>1),I2C_CLIENT_END};
static unsigned short opt_ignore[] = {1,(GP2A_ADDR>>1),I2C_CLIENT_END};
static unsigned short opt_probe[] = {I2C_CLIENT_END};




static struct i2c_client_address_data opt_addr_data = {
	.normal_i2c = opt_normal_i2c,
	.ignore		= opt_ignore,
	.probe		= opt_probe,	
};

short gp2a_get_proximity_value()
{
	  return proximity_value;

}
EXPORT_SYMBOL(gp2a_get_proximity_value);



/*****************************************************************************************
 *  
 *  function    : gp2a_work_func_light 
 *  description : This function is for light sensor. It has a few state.
 *                "STATE_0" means that circumstance in which you are now is clear as day.
 *                The more value is increased, the more circumstance is dark. 
 *                 
 */

void gp2a_work_func_light(struct work_struct *work)
{
	//struct gp2a_data *gp2a = container_of(work, struct gp2a_data, work_light);
	int adc=0;
	int i;

	bool top = false;
	bool bottom = false; 

	/* read adc data from s3c64xx */
	adc = s3c_adc_get_adc_data(ADC_CHANNEL);
	gprintk("adc = %d \n",adc);
	gprintk("cur_state = %d\n",cur_state);
	
	/* decision to check value whether it is suitable for current state */
	if(adc < light_state[cur_state].adc_bottom_limit)
	{
		/* value of ADC is not suitable for current state */
		/* ask to move state downward */
		bottom = true;
		gprintk("bottom flag is set \n");
	}
	

	/* decision to check value whether it is suitable for current state */
	if(adc > light_state[cur_state].adc_top_limit)
	{
		/* value of ADC is not suitable for current state */
		/* ask to move state upward */

		top = true;
		gprintk("top flag is set \n");
	}

	/* process to move state downward  */
	for(i=cur_state;i<STATE_NUM,bottom;i++)
	{
		gprintk("for i = %d \n",i);

		/* decison to change state more */
		/* if condition is true, it is unavailable to move state any more */
		if(adc > light_state[i+1].adc_bottom_limit)
		{
			cur_state = i +1;
			gprintk("state is changed. cur_state is %d \n",cur_state);
			break;
		}
	}

	/* process to move state upward */
	for(i=cur_state;i>0,top;i--)
	{
		gprintk("for i = %d \n",i);
		/* decision to change state more */
		/* if condition is true, it is unavailable to move state any more */
		if(adc < light_state[i-1].adc_top_limit)
		{
			cur_state = i - 1;
			gprintk("state is changed. cur_state is %d \n",cur_state);
			break;
		}

	}
		
	/* if state is changed, adjust brightness of lcd */
	if(bottom || top)
	{

		backlight_level_ctrl(light_state[cur_state].brightness);
	}


	
	


}


/*****************************************************************************************
 *  
 *  function    : gp2a_timer_func 
 *  description : This function is for light sensor. 
 *                it operates every a few seconds. 
 *                 
 */

static enum hrtimer_restart gp2a_timer_func(struct hrtimer *timer)
{
	struct gp2a_data *gp2a = container_of(timer, struct gp2a_data, timer);
				
	
	queue_work(gp2a_wq, &gp2a->work_light);
	hrtimer_start(&gp2a->timer,ktime_set(LIGHT_PERIOD,0),HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

/*****************************************************************************************
 *  
 *  function    : gp2a_work_func_prox 
 *  description : This function is for proximity sensor (using B-1 Mode ). 
 *                when INT signal is occured , it gets value from VO register.   
 *
 *                 
 */

void gp2a_work_func_prox(struct work_struct *work)
{
	struct gp2a_data *gp2a = container_of(work, struct gp2a_data, work_prox);
	
	unsigned char value;
	unsigned char int_val=REGS_PROX;
	unsigned char vout=0;

	/* Read VO & INT Clear */

	if(INT_CLEAR)
	{
		int_val = REGS_PROX | (1 <<7);
	}
	opt_i2c_read((u8)(int_val),&value,1);
	vout = value & 0x01;
	gprintk("value = %d \n",vout);


	/* Report proximity information */
	/* not fixed. */
	proximity_value = vout;

	if(USE_INPUT_DEVICE)
	{
    	input_report_abs(gp2a->input_dev,ABS_DISTANCE,(int)vout);
    	input_sync(gp2a->input_dev);
	
	
		mdelay(1);
	}
	
	/* Write HYS Register */

	if(!vout)
	{
		value = 0x40;

	}
	else
	{
		value = 0x20;

	}

	opt_i2c_write((u8)(REGS_HYS),&value);

	/* Forcing vout terminal to go high */

	value = 0x18;
	opt_i2c_write((u8)(REGS_CON),&value);


	/* enable INT */

	enable_irq(gp2a->irq);

	/* enabling VOUT terminal in nomal operation */

	value = 0x00;

	opt_i2c_write((u8)(REGS_CON),&value);

}



irqreturn_t gp2a_irq_handler(int irq, void *dev_id)
{

	struct gp2a_data *gp2a = dev_id;

	gprintk("gp2a->irq = %d\n",gp2a->irq);

	if(gp2a->irq !=-1)
	{
		disable_irq(gp2a->irq);

		queue_work(gp2a_wq, &gp2a->work_prox);

	}

	return IRQ_HANDLED;


}


static int opt_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret,err=0;

	
	gprintk("\n");
	if ( !i2c_check_functionality(adap,I2C_FUNC_SMBUS_BYTE_DATA) ) {
		printk(KERN_INFO "byte op is not permited.\n");
		return err;
	}
	c = kzalloc(sizeof(struct i2c_client),GFP_KERNEL);
	if (!c)
	{
		printk("kzalloc error \n");
		return -ENOMEM;

	}

	memset(c, 0, sizeof(struct i2c_client));	
	strncpy(c->name,"gp2a",I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &opt_i2c_driver;
	c->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;

	if ((ret = i2c_attach_client(c)) < 0)
	{
		printk("i2c_attach_client error\n");
		goto error;
	}
	opt_i2c_client = c;

	gprintk("\n");
	return ret;

error:
	printk("in %s , ret = %d \n",__func__,ret);
	kfree(c);
	return err;
}

static int opt_attach_adapter(struct i2c_adapter *adap)
{
	int ret;
	gprintk("\n");
	ret =  i2c_probe(adap, &opt_addr_data, opt_attach);
	return ret;
}


static int opt_i2c_init(void) 
{
	
	
	if( i2c_add_driver(&opt_i2c_driver))
	{
		printk("i2c_add_driver failed \n");
		return -ENODEV;
	}

	return 0;
}


int opt_i2c_read(u8 reg, u8 *val, unsigned int len )
{

	int err;
	u8 buf[1];
	struct i2c_msg msg[2];


	buf[0] = reg; 

	msg[0].addr = opt_i2c_client->addr;
	msg[0].flags = 1;
	
	msg[0].len = 2;
	msg[0].buf = buf;
	err = i2c_transfer(opt_i2c_client->adapter, msg, 1);

	
	*val = buf[1];
	
    if (err >= 0) return 0;

    printk("%s %d i2c transfer error\n", __func__, __LINE__);
    return err;
}

int opt_i2c_write( u8 reg, u8 *val )
{
    int err;
    struct i2c_msg msg[1];
    unsigned char data[2];

    if( (opt_i2c_client == NULL) || (!opt_i2c_client->adapter) ){
        return -ENODEV;
    }


    data[0] = reg;
    data[1] = *val;

    msg->addr = opt_i2c_client->addr;
    msg->flags = I2C_M_WR;
    msg->len = 2;
    msg->buf = data;

    err = i2c_transfer(opt_i2c_client->adapter, msg, 1);

    if (err >= 0) return 0;

    printk("%s %d i2c transfer error\n", __func__, __LINE__);
    return err;
}



void gp2a_chip_init(void)
{
	gprintk("\n");
	
	/* Power On */
	if (gpio_is_valid(GPIO_LUM_PWM))
	{
		if (gpio_request(GPIO_LUM_PWM, S3C_GPIO_LAVEL(GPIO_LUM_PWM)))
			printk(KERN_ERR "Filed to request GPIO_LUM_PWM!\n");
		gpio_direction_output(GPIO_LUM_PWM, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_LUM_PWM, S3C_GPIO_PULL_NONE); 

	mdelay(5);
	
	/* set INT 	*/
	s3c_gpio_cfgpin(GPIO_PS_VOUT, S3C_GPIO_SFN(GPIO_PS_VOUT_AF));
	s3c_gpio_setpull(GPIO_PS_VOUT, S3C_GPIO_PULL_NONE);

	set_irq_type(IRQ_GP2A_INT, IRQ_TYPE_EDGE_FALLING);
		
	
}


/*****************************************************************************************
 *  
 *  function    : gp2a_on 
 *  description : This function is power-on function for optical sensor.
 *
 *  int type    : Sensor type. Two values is available (PROXIMITY,LIGHT).
 *                it support power-on function separately.
 *                
 *                 
 */

void gp2a_on(struct gp2a_data *gp2a, int type)
{
	u8 value;
	gprintk("gp2a_on(%d)\n",type);
	if((type ==LIGHT && proximity_enable==OFF) || (type==PROXIMITY && light_enable==OFF)) 
	{
		gprintk("gp2a power on \n");
		value = 0x18;
		opt_i2c_write((u8)(REGS_CON),&value);

		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);

		value = 0x03;
		opt_i2c_write((u8)(REGS_OPMOD),&value);

		if(type == PROXIMITY)
		{
			gprintk("enable irq for proximity\n");
			enable_irq(gp2a ->irq);
		}

		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);

	}

	if(type ==PROXIMITY && light_enable ==ON)
	{
		gprintk("enable irq for proximity\n");
		enable_irq(gp2a->irq);

	}


}

/*****************************************************************************************
 *  
 *  function    : gp2a_off 
 *  description : This function is power-off function for optical sensor.
 *
 *  int type    : Sensor type. Three values is available (PROXIMITY,LIGHT,ALL).
 *                it support power-on function separately.
 *                
 *                 
 */

void gp2a_off(struct gp2a_data *gp2a, int type)
{
	u8 value;

	gprintk("gp2a_off(%d)\n",type);
	if((type == LIGHT && proximity_enable ==OFF)|| (type==PROXIMITY && light_enable==OFF)|| type==ALL)
	{
	
		gprintk("gp2a power off \n");
		if(type == PROXIMITY || type==ALL)
		{
			gprintk("disable irq for proximity \n");
			disable_irq(gp2a ->irq);
		}
		value = 0x02;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
		
	}

	if(type==PROXIMITY && light_enable ==ON)
	{

		gprintk("disable irq for proximity \n");
		disable_irq(gp2a->irq);

	}



	
}



static ssize_t lightsensor_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{


	return sprintf(buf,"%u\n",light_enable);
}
static ssize_t lightsensor_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	struct gp2a_data *gp2a = dev_get_drvdata(dev);
	int value;
    sscanf(buf, "%d", &value);

	gprintk("in lightsensor_file_cmd_store, input value = %d \n",value);

	if(value==1 && light_enable == OFF)
	{
		gp2a_on(gp2a,LIGHT);
		value = ON;
		light_enable = ON;
		gprintk("timer start for light sensor\n");
	    hrtimer_start(&gp2a->timer,ktime_set(LIGHT_PERIOD,0),HRTIMER_MODE_REL);
	}
	else if(value==0 && light_enable ==ON) 
	{
		gp2a_off(gp2a,LIGHT);
		light_enable = OFF;
		gprintk("timer cancel for light sensor\n");
		hrtimer_cancel(&gp2a->timer);
	}

	/* temporary test code for proximity sensor */
	else if(value==3 && proximity_enable == OFF)
	{
		gp2a_on(gp2a,PROXIMITY);
		proximity_enable = ON;


	}
	else if(value==2 && proximity_enable == ON)
	{
		gp2a_off(gp2a,PROXIMITY);
		proximity_enable = OFF;

	}

	return size;
}

static DEVICE_ATTR(lightsensor_file_cmd,0644, lightsensor_file_cmd_show, lightsensor_file_cmd_store);

static int gp2a_opt_probe( struct platform_device* pdev )
{
	
	struct gp2a_data *gp2a;
	int irq;
	int i;
	int ret;

	/* allocate driver_data */
	gp2a = kzalloc(sizeof(struct gp2a_data),GFP_KERNEL);
	if(!gp2a)
	{
		pr_err("kzalloc error\n");
		return -ENOMEM;

	}


	gprintk("in %s \n",__func__);
	
	/* init i2c */
	opt_i2c_init();

	if(opt_i2c_client == NULL)
	{
		pr_err("opt_probe failed : i2c_client is NULL\n"); 
		return -ENODEV;
	}

	/* hrtimer Settings */

	hrtimer_init(&gp2a->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	gp2a->timer.function = gp2a_timer_func;

	/* Input device Settings */
	if(USE_INPUT_DEVICE)
	{
		gp2a->input_dev = input_allocate_device();
		if (gp2a->input_dev == NULL) 
		{
			pr_err("Failed to allocate input device\n");
			return -ENOMEM;
		}
		gp2a->input_dev->name = "proximity";
	
		set_bit(EV_SYN,gp2a->input_dev->evbit);
		set_bit(EV_ABS,gp2a->input_dev->evbit);
		
   	 input_set_abs_params(gp2a->input_dev, ABS_DISTANCE, 0, 1, 0, 0);
		
	
		ret = input_register_device(gp2a->input_dev);
		if (ret) 
		{
			pr_err("Unable to register %s input device\n", gp2a->input_dev->name);
			input_free_device(gp2a->input_dev);
			kfree(gp2a);
			return -1;
		}

	}
	/* WORK QUEUE Settings */


    gp2a_wq = create_singlethread_workqueue("gp2a_wq");
    if (!gp2a_wq)
	    return -ENOMEM;
    INIT_WORK(&gp2a->work_prox, gp2a_work_func_prox);
    INIT_WORK(&gp2a->work_light, gp2a_work_func_light);
	gprintk("Workqueue Settings complete\n");

	/* misc device Settings */
	ret = misc_register(&proximity_device);
	if(ret) {
		pr_err(KERN_ERR "misc_register failed \n");
	}


	/* INT Settings */	

	irq = IRQ_GP2A_INT;
	gp2a->irq = -1;
	ret = request_irq(irq, gp2a_irq_handler, 0, "gp2a_int", gp2a);
	if (ret) {
		pr_err("unable to request irq %d\n", irq);
		return ret;
	}       
	gp2a->irq = irq;

	gprintk("INT Settings complete\n");

	/* set platdata */
	platform_set_drvdata(pdev, gp2a);

	/* set sysfs for light sensor */

	lightsensor_class = class_create(THIS_MODULE, "lightsensor");
	if (IS_ERR(lightsensor_class))
		pr_err("Failed to create class(lightsensor)!\n");

	switch_cmd_dev = device_create(lightsensor_class, NULL, 0, NULL, "switch_cmd");
	if (IS_ERR(switch_cmd_dev))
		pr_err("Failed to create device(switch_cmd_dev)!\n");

	if (device_create_file(switch_cmd_dev, &dev_attr_lightsensor_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_lightsensor_file_cmd.attr.name);

	dev_set_drvdata(switch_cmd_dev,gp2a);
	


	/* GP2A Regs INIT SETTINGS */
	

	for(i=1;i<5;i++)
	{
		opt_i2c_write((u8)(i),&gp2a_original_image[i]);
	}

	mdelay(2);




	/* maintain power-down mode before using sensor */

	gp2a_off(gp2a,ALL);
	
	return 0;
}

static int gp2a_opt_suspend( struct platform_device* pdev, pm_message_t state )
{
	struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	u8 value;
	gprintk("gp2a = %x,pdev->dev %x\n",gp2a,pdev->dev);

	if(proximity_enable)
		disable_irq(gp2a ->irq);

	if(proximity_enable || light_enable)
	{
		value = 0x02;
		opt_i2c_write((u8)(REGS_OPMOD),&value);
	}




	return 0;
}

static int gp2a_opt_resume( struct platform_device* pdev )
{

	struct gp2a_data *gp2a = platform_get_drvdata(pdev);
	u8 value;

	gprintk("gp2a = %x, pdev->dev.driver_data = %x\n",gp2a,pdev->dev.driver_data);

	if(proximity_enable || light_enable)
	{

		value = 0x18;
		opt_i2c_write((u8)(REGS_CON),&value);

		value = 0x40;
		opt_i2c_write((u8)(REGS_HYS),&value);

		value = 0x03;
		opt_i2c_write((u8)(REGS_OPMOD),&value);

		if(proximity_enable)
			enable_irq(gp2a ->irq);

		value = 0x00;
		opt_i2c_write((u8)(REGS_CON),&value);

	}
	



	return 0;
}

static int proximity_open(struct inode *ip, struct file *fp)
{
	return 0;

}

static int proximity_release(struct inode *ip, struct file *fp)
{
	return 0;

}

static long proximity_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	struct gp2a_data *gp2a = dev_get_drvdata(switch_cmd_dev);
	int ret=0;
	switch(cmd) {

		case SHARP_GP2AP_OPEN:
			{
				printk(KERN_INFO "[PROXIMITY] %s : case OPEN\n", __FUNCTION__);
				gp2a_on(gp2a,PROXIMITY);
				proximity_enable =1;
				
			}
			break;

		case SHARP_GP2AP_CLOSE:
			{
				printk(KERN_INFO "[PROXIMITY] %s : case CLOSE\n", __FUNCTION__);
				gp2a_off(gp2a,PROXIMITY);
				proximity_enable=0;
			}
			break;

		default:
			printk(KERN_INFO "[PROXIMITY] unknown ioctl %d\n", cmd);
			ret = -1;
			break;
	}
	return ret;
}





static struct platform_driver gp2a_opt_driver = {
	.probe 	 = gp2a_opt_probe,
	.suspend = gp2a_opt_suspend,
	.resume  = gp2a_opt_resume,
	.driver  = {
		.name = "gp2a-opt",
		.owner = THIS_MODULE,
	},
};

static int __init gp2a_opt_init(void)
{
	int ret;
	
	gp2a_chip_init();
	ret = platform_driver_register(&gp2a_opt_driver);
	return ret;
	
	
}
static void __exit gp2a_opt_exit(void)
{
	struct gp2a_data *gp2a = dev_get_drvdata(switch_cmd_dev);
    if (gp2a_wq)
		destroy_workqueue(gp2a_wq);

	free_irq(IRQ_GP2A_INT,gp2a);
	
	
	gpio_direction_output(GPIO_LUM_PWM,GPIO_LEVEL_LOW);

	
	if(USE_INPUT_DEVICE)
		input_unregister_device(gp2a->input_dev);
	kfree(gp2a);

	gpio_free(GPIO_LUM_PWM);
	platform_driver_unregister(&gp2a_opt_driver);
}


module_init( gp2a_opt_init );
module_exit( gp2a_opt_exit );

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("Optical Sensor driver for gp2ap002a00f");
MODULE_LICENSE("GPL");
