#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "bma020_acc.h"

/* add by inter.park */
//extern void enable_acc_pins(void);

struct class *acc_class;

/* no use */
//static int bma020_irq_num = NO_IRQ;

/* create bma020 object */
bma020_t bma020;

/* create bma020 registers object */
bma020regs_t bma020regs;

#if 0
static irqreturn_t bma020_acc_isr( int irq, void *unused, struct pt_regs *regs )
{
	printk( "bma020_acc_isr event occur!!!\n" );
	
	return IRQ_HANDLED;
}
#endif


int bma020_open (struct inode *inode, struct file *filp)
{
	gprintk("start\n");
	return 0;
}

ssize_t bma020_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t bma020_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int bma020_release (struct inode *inode, struct file *filp)
{
	return 0;
}

#if 0
int bma020_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num,  unsigned long arg)
{
	bma020acc_t accels;
	unsigned int arg_data; 
	int err = 0;
	
	gprintk("start\n");
	switch( ioctl_num )
	{
		case IOCTL_BMA020_GET_ACC_VALUE :
			{
				bma020_read_accel_xyz( &accels );

				gprintk( "acc data x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
				
				if( copy_to_user( (bma020acc_t*)arg, &accels, sizeof(bma020acc_t) ) )
				{
					err = -EFAULT;
				}   

			}
			break;
		
		case IOC_SET_ACCELEROMETER :  
			{
				if( copy_from_user( (unsigned int*)&arg_data, (unsigned int*)arg, sizeof(unsigned int) ) )
				{
				
				}
				if( arg_data == BMA020_POWER_ON )
				{
					printk( "ioctl : bma020 power on\n" );
					bma020_set_mode( BMA020_MODE_NORMAL );
				}
				else
				{
					printk( "ioctl : bma020 power off\n" );
					bma020_set_mode( BMA020_MODE_SLEEP );
				}
			}
			break;
		default : 
			break;
	}
	return err;
	
}
#endif


int bma020_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,  unsigned long arg)
{
	int err = 0;
	unsigned char data[6];

	/* check cmd */
	if(_IOC_TYPE(cmd) != BMA150_IOC_MAGIC)
	{
#if DEBUG       
		printk("cmd magic type error\n");
#endif
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > BMA150_IOC_MAXNR)
	{
#if DEBUG
		printk("cmd number error\n");
#endif
		return -ENOTTY;
	}

	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user*)arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	if(err)
	{
#if DEBUG
		printk("cmd access_ok error\n");
#endif
		return -EFAULT;
	}
	#if 0
	/* check bam150_client */
	if( bma150_client == NULL)
	{
#if DEBUG
		printk("I2C driver not install\n");
#endif
		return -EFAULT;
	}
	#endif

	switch(cmd)
	{
		case BMA150_READ_ACCEL_XYZ:
			err = bma020_read_accel_xyz((bma020acc_t*)data);
//			printk("#### %s (%d)(%d)(%d)\n", __func__, ((bma020acc_t*)data)->x, ((bma020acc_t*)data)->y, ((bma020acc_t*)data)->z);
			if(copy_to_user((bma020acc_t*)arg,(bma020acc_t*)data,6)!=0)
			{
#if DEBUG
				printk("copy_to error\n");
#endif
				return -EFAULT;
			}
			return err;

		case BMA150_SET_RANGE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_range(*data);
			return err;
		
		case BMA150_SET_MODE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_mode(*data);
			return err;

		case BMA150_SET_BANDWIDTH:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_bandwidth(*data);
			return err;
		case BMA150_CALIBRATE:
			{
			printk("#### BMA150_CALIBRATE\n");
			bma020acc_t data = bma020_calibrate();
			printk("## Calibration finished. (%d) (%d) (%d)\n", data.x, data.y, data.z);
			}
		default:
			return 0;
	}
}

struct file_operations acc_fops =
{
	.owner   = THIS_MODULE,
	.read    = bma020_read,
	.write   = bma020_write,
	.open    = bma020_open,
	.ioctl   = bma020_ioctl,
	.release = bma020_release,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma020_early_suspend(struct early_suspend *handler)
{
	printk("#BMA020:SLEEP\n");
	bma020_set_mode( BMA020_MODE_SLEEP );
}

static void bma020_early_resume(struct early_suspend *handler)
{
	printk("#BMA020:RESUME\n");
	bma020_set_mode( BMA020_MODE_NORMAL );
}
#endif /* CONFIG_HAS_EARLYSUSPEND */ 

void bma020_chip_init(void)
{
	/*assign register memory to bma020 object */
	bma020.image = &bma020regs;

	bma020.bma020_bus_write = i2c_acc_bma020_write;
	bma020.bma020_bus_read  = i2c_acc_bma020_read;

#ifdef CONFIG_HAS_EARLYSUSPEND
	bma020.early_suspend.suspend = bma020_early_suspend;
	bma020.early_suspend.resume = bma020_early_resume;
	register_early_suspend(&bma020.early_suspend);
#endif

	/*call init function to set read write functions, read registers */
	bma020_init( &bma020 );

	/* from this point everything is prepared for sensor communication */


	/* set range to 2G mode, other constants: 
	 * 	   			4G: BMA020_RANGE_4G, 
	 * 	    		8G: BMA020_RANGE_8G */

	bma020_set_range(BMA020_RANGE_2G); 

	/* set bandwidth to 25 HZ */
	bma020_set_bandwidth(BMA020_BW_25HZ);

	/* for interrupt setting */
//	bma020_set_low_g_threshold( BMA020_HG_THRES_IN_G(0.35, 2) );

//	bma020_set_interrupt_mask( BMA020_INT_LG );

}

int bma020_acc_start(void)
{
	int result;

	struct device *dev_t;
	
	bma020acc_t accels; /* only for test */
	
	result = register_chrdev( BMA150_MAJOR, "bma150", &acc_fops);

	if (result < 0) 
	{
		return result;
	}
	
	acc_class = class_create (THIS_MODULE, "BMA-dev");
	
	if (IS_ERR(acc_class)) 
	{
		unregister_chrdev( BMA150_MAJOR, "bma150" );
		return PTR_ERR( acc_class );
	}

	dev_t = device_create( acc_class, NULL, MKDEV(BMA150_MAJOR, 0), "%s", "bma150");

	if (IS_ERR(dev_t)) 
	{
		return PTR_ERR(dev_t);
	}
	
	result = i2c_acc_bma020_init();

	if(result)
	{
		return result;
	}

	bma020_chip_init();

	gprintk("[BMA020] read_xyz ==========================\n");
	bma020_read_accel_xyz( &accels );
	gprintk("[BMA020] x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );

	gprintk("[BMA020] ===================================\n");
	
	/* only for test */
	#if 0
	printk( "before get xyz\n" );
	mdelay(3000);

	while(1)
	{
		bma020_read_accel_xyz( &accels );

		printk( "acc data x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
	
		mdelay(100);
	}
	#endif

	bma020_set_mode(BMA020_MODE_SLEEP);
	gprintk("[BMA020] set_mode BMA020_MODE_SLEEP\n");
	
	return 0;
}

void bma020_acc_end(void)
{
	unregister_chrdev( BMA150_MAJOR, "bma150" );
	
	i2c_acc_bma020_exit();

	device_destroy( acc_class, MKDEV(BMA150_MAJOR, 0) );
	class_destroy( acc_class );
	unregister_early_suspend(&bma020.early_suspend);
}


static int bma020_accelerometer_probe( struct platform_device* pdev )
{
/* not use interrupt */
#if 0	
	int ret;

	//enable_acc_pins();
	/*
	mhn_gpio_set_direction(MFP_ACC_INT, GPIO_DIR_IN);
	mhn_mfp_set_pull(MFP_ACC_INT, MFP_PULL_HIGH);
	*/

	bma020_irq_num = platform_get_irq(pdev, 0);
	ret = request_irq(bma020_irq_num, (void *)bma020_acc_isr, IRQF_DISABLED, pdev->name, NULL);
	if(ret) {
		printk("[BMA020 ACC] isr register error\n");
		return ret;
	}

	//set_irq_type (bma020_irq_num, IRQT_BOTHEDGE);
	
	/* if( request_irq( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), (void *) bma020_acc_isr, 0, "BMA020_ACC_ISR", (void *)0 ) )
	if(
	{
		printk ("[BMA020 ACC] isr register error\n" );
	}
	else
	{
		printk( "[BMA020 ACC] isr register success!!!\n" );
	}*/
	
	// set_irq_type ( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), IRQT_BOTHEDGE );

	/* if interrupt don't register Process don't stop for polling mode */ 

#endif 
	return bma020_acc_start();
}


static int bma020_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	return 0;
}


static int bma020_accelerometer_resume( struct platform_device* pdev )
{
	return 0;
}


static struct platform_device *bma020_accelerometer_device;

static struct platform_driver bma020_accelerometer_driver = {
	.probe 	 = bma020_accelerometer_probe,
	.suspend = bma020_accelerometer_suspend,
	.resume  = bma020_accelerometer_resume,
	.driver  = {
		.name = "bma020-accelerometer", 
	}
};


static int __init bma020_acc_init(void)
{
	int result;

	result = platform_driver_register( &bma020_accelerometer_driver );

	if( result ) 
	{
		return result;
	}

	bma020_accelerometer_device  = platform_device_register_simple( "bma020-accelerometer", -1, NULL, 0 );
	
	if( IS_ERR( bma020_accelerometer_device ) )
	{
		return PTR_ERR( bma020_accelerometer_device );
	}

	return 0;
}


static void __exit bma020_acc_exit(void)
{
	gprintk("start\n");
	bma020_acc_end();

//	free_irq(bma020_irq_num, NULL);
//	free_irq( IRQ_GPIO( MFP2GPIO( MFP_ACC_INT ) ), (void*)0 );

	platform_device_unregister( bma020_accelerometer_device );
	platform_driver_unregister( &bma020_accelerometer_driver );
}


module_init( bma020_acc_init );
module_exit( bma020_acc_exit );

MODULE_AUTHOR("inter.park");
MODULE_DESCRIPTION("accelerometer driver for BMA020");
MODULE_LICENSE("GPL");
