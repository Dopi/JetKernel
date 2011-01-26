#include <mach/infobowlq.h>
#include "kr3dm_i2c.h"

#define I2C_READ_FAIL_PROTECTION

#ifdef I2C_READ_FAIL_PROTECTION
#define I2C_READ_RETRY_CNT_DEF 3
#endif

kr3dm_t *p_kr3dm;
kr3dm_t kr3dm;
kr3dmregs_t kr3dmregs;

static struct i2c_client *g_client;
static struct platform_device *kr3dm_accelerometer_device;
struct class *kr3dm_acc_class;

static int __devinit i2c_acc_kr3dm_probe(struct i2c_client *, const struct i2c_device_id *);
static int __devexit i2c_acc_kr3dm_remove(struct i2c_client *);
static int i2c_acc_kr3dm_detect(struct i2c_client *, int kind, struct i2c_board_info *);

static unsigned short ignore[] = {I2C_CLIENT_END };
static unsigned short normal_addr[] = {I2C_CLIENT_END };
static unsigned short probe_addr[] = { 5, SENS_ADD >> 1, I2C_CLIENT_END };


static struct i2c_client_address_data addr_data = {
	.normal_i2c		= normal_addr,
	.probe			= probe_addr,
	.ignore			= ignore,
};

static struct platform_driver kr3dm_accelerometer_driver = {
	.probe 	 = kr3dm_accelerometer_probe,
	.suspend = kr3dm_accelerometer_suspend,
	.resume  = kr3dm_accelerometer_resume,
	.driver  = {
		.name = "kr3dm-accelerometer",
	}
};

struct file_operations kr3dm_acc_fops =
{
	.owner   = THIS_MODULE,
	.read    = kr3dm_read,
	.write   = kr3dm_write,
	.open    = kr3dm_open,
	.ioctl   = kr3dm_ioctl,
	.release = kr3dm_release,
};


static struct i2c_device_id kr3dm_acc_id[] = {
	{ "kr3dm_i2c_driver", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, kr3dm_acc_id);

struct i2c_driver acc_kr3dm_i2c_driver =
{
	.driver = {
		.name = "kr3dm_i2c_driver",
	},
	.class		= I2C_CLASS_HWMON,
	.probe		= i2c_acc_kr3dm_probe,
	.remove		= __devexit_p(i2c_acc_kr3dm_remove),
	.detect		= i2c_acc_kr3dm_detect,
	.id_table	= kr3dm_acc_id,
	.address_data 	= &addr_data
};

char i2c_acc_kr3dm_read(u8 reg, u8 *val, unsigned int len )
{
	int 	 err;
	struct 	 i2c_msg msg[1];
	unsigned char data[1];

	if( (g_client == NULL) || (g_client->adapter==NULL) )
	{
#ifdef DEBUG
	printk("[KR3DM] i2c_acc_kr3dm_read >>>>>>>>>>>>>>>>>>>>>>>error1<<<<<<<<<<<<<<<<<<\n");
#endif
		return -ENODEV;
	}

	msg->addr 	= g_client->addr;
	msg->flags 	= I2C_M_WR;
	msg->len 	= 1;
	msg->buf 	= data;
	*data       = reg;

	err = i2c_transfer(g_client->adapter, msg, 1);


	if (err >= 0)
	{
		msg->flags = I2C_M_RD;
		msg->len   = len;
		msg->buf   = val;
		err = i2c_transfer(g_client->adapter, msg, 1);
	}

	if (err >= 0)
	{
#ifdef DEBUG
	    //printk("[KR3DM] %s %d >>>>>>>>>>>>>>>>>>>>>>>error2(%d)<<<<<<<<<<<<<<<<<<\n", __func__, __LINE__, err);
#endif
		return 0;
	}
#if DEBUG
	printk(KERN_ERR "%s %d i2c transfer error\n", __func__, __LINE__);
#endif
#ifdef DEBUG
	if(msg->len >3)
	    printk("[KR3DM] i2c_acc_kr3dm_read  %x %x %x %x len %d \n", msg->buf[0] , msg->buf[1], msg->buf[2], msg->buf[3],msg->len );
#endif

	return err;;

}
char i2c_acc_kr3dm_write( u8 reg, u8 *val )
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if( (g_client == NULL) || (!g_client->adapter) ){
		return -ENODEV;
	}

	data[0] = reg;
	data[1] = *val;

	msg->addr = g_client->addr;
	msg->flags = I2C_M_WR;
	msg->len = 2;
	msg->buf = data;

	err = i2c_transfer(g_client->adapter, msg, 1);

	if (err >= 0) return 0;
#if DEBUG
	printk(KERN_ERR "%s %d i2c transfer error[%d]\n", __func__, __LINE__, err);
#endif
	return err;
}


static int __devinit i2c_acc_kr3dm_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;


	if ( !i2c_check_functionality(client->adapter,I2C_FUNC_SMBUS_BYTE_DATA) ) {
		printk(KERN_INFO "byte op is not permited.\n");
		goto ERROR0;
	}

	client->addr = SENS_ADD >> 1;
	client->driver = &acc_kr3dm_i2c_driver;
	client->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;

	g_client = client;

#if DEBUG
	printk("i2c_acc_kr3dm_probe_client() completed!!!!!!!!!!!!!!!!!!\n");
#endif
	return 0;

ERROR0:
	printk(KERN_ERR "[KR3DM][%s] probe failed!\n", __func__);
	return err;
}


static int __devexit i2c_acc_kr3dm_remove(struct i2c_client *client)
{
	return 0;
}


int i2c_acc_kr3dm_init(void)
{
	int ret;
#ifdef DEBUG
	printk("[KR3DM] ********** i2c_acc_kr3dm_init =====================\n");
#endif
	if ( (ret = i2c_add_driver(&acc_kr3dm_i2c_driver)) )
	{
		printk(KERN_ERR "Driver registration failed, module not inserted.\n");
		return ret;
	}

	return 0;
}


void i2c_acc_kr3dm_exit(void)
{
	i2c_del_driver(&acc_kr3dm_i2c_driver);
}

static int i2c_acc_kr3dm_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strlcpy(info->type, "kr3dm_i2c_driver", I2C_NAME_SIZE);
	return 0;
}

int kr3dm_set_range(char range)
{
   int comres = 0;
   unsigned char data;
#ifdef DEBUG
	printk("[KR3DM] ********** kr3dm_set_range =====================\n");
#endif
   if (p_kr3dm==0)
	    return E_KR3DM_NULL_PTR;

   if (range<3){
   		comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, CTRL_REG4, &data, 1 );
		data = data | (range << 4);
		comres += p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG4, &data, 1);
   }
   return comres;

}

int kr3dm_set_mode(unsigned char mode)
{

	int comres=0;
	unsigned char normal = 0x27;
	unsigned char sleep = 0x00;

	if (p_kr3dm==0)
		return E_KR3DM_NULL_PTR;

	switch(mode)
	{
		case KR3DM_MODE_NORMAL:
		case KR3DM_MODE_WAKE_UP:
#ifdef DEBUG
			printk("[KR3DM]=====================KR3DM_MODE_WAKE_UP =====================\n");
#endif			
			comres += p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG1, &normal, 1);
			break;
		case KR3DM_MODE_SLEEP:
#ifdef DEBUG
			printk("[KR3DM]=====================KR3DM_MODE_SLEEP =====================\n");
#endif
			comres += p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG1, &sleep, 1);
			break;
		default:
			return E_OUT_OF_RANGE;
	}
	p_kr3dm->mode = mode;

	return comres;

}

unsigned char kr3dm_get_mode(void)
{
    if (p_kr3dm==0)
    	return E_KR3DM_NULL_PTR;

	return p_kr3dm->mode;

}

int kr3dm_set_bandwidth(char bw)
{
	int comres = 0;
	unsigned char data = 0x27;

	if (p_kr3dm==0)
		return E_KR3DM_NULL_PTR;

	if (bw<8)
	{
	  data = data | (3 << bw);
	  comres += p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG1, &data, 1 );
	}

	return comres;
}


int kr3dm_get_bandwidth(unsigned char *bw)
{
	int comres = 1;

	if (p_kr3dm==0)
		return E_KR3DM_NULL_PTR;

	comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, CTRL_REG1, bw, 1 );

	*bw = (*bw & 0x18);

	return comres;
}

int kr3dm_open (struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t kr3dm_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t kr3dm_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int kr3dm_release (struct inode *inode, struct file *filp)
{
	return 0;
}

int kr3dm_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,  unsigned long arg)
{
	
#ifdef DEBUG
			//printk("[KR3DM]<<<<<<kr3dm_ioctl %d %d>>>>>>\n",cmd,arg);
#endif	

	int err = 0;
	unsigned char data[3];
	kr3dmacc_t accels;
	unsigned char val1 = 0x27;

	/* check cmd */
	if(_IOC_TYPE(cmd) != KR3DM_IOC_MAGIC)
	{
		printk("[KR3DM] cmd magic type error\n");
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > KR3DM_IOC_MAXNR)
	{
		printk("[KR3DM] cmd number error\n");
		return -ENOTTY;
	}

	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user*)arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	if(err)
	{
		printk("[KR3DM] cmd access_ok error\n");
		return -EFAULT;
	}

	switch(cmd)
	{
		case KR3DM_READ_ACCEL_XYZ:
			err = kr3dm_read_accel_xyz(&accels);
			if(copy_to_user((kr3dmacc_t*)arg, &accels, sizeof(kr3dmacc_t))!=0)
			{
				printk("[KR3DM] copy_to error\n");
				return -EFAULT;
			}
			return err;

		case KR3DM_SET_RANGE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
				printk("[KR3DM] copy_from_user error\n");
				return -EFAULT;
			}
			err = kr3dm_set_range(*data);
			return err;

		case KR3DM_SET_MODE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
				printk("[KR3DM] copy_from_user error\n");
				return -EFAULT;
			}
			err = kr3dm_set_mode(*data);
			return err;

		case KR3DM_SET_BANDWIDTH:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
				printk("[KR3DM] copy_from_user error\n");
				return -EFAULT;
			}
			err = kr3dm_set_bandwidth(*data);
			return err;

		default:
			return 0;
	}
	return 0;
}


int kr3dm_read_accel_xyz(kr3dmacc_t * acc)
{
	int comres;
	unsigned char data[3];

#ifdef I2C_READ_FAIL_PROTECTION
	int i2c_read_retry_cnt;
#endif
	
	if (p_kr3dm==0)
		return E_KR3DM_NULL_PTR;

#ifdef I2C_READ_FAIL_PROTECTION
	i2c_read_retry_cnt = I2C_READ_RETRY_CNT_DEF;

	while(i2c_read_retry_cnt--)
	{
		comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_X_L, &data[0], 1);
		if(!comres)
			break;
	}

	i2c_read_retry_cnt = I2C_READ_RETRY_CNT_DEF;
	
	while(i2c_read_retry_cnt--)
	{
		comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_Y_L, &data[1], 1);
		if(!comres)
			break;
	}

	i2c_read_retry_cnt = I2C_READ_RETRY_CNT_DEF;
	
	while(i2c_read_retry_cnt--)
	{
		comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_Z_L, &data[2], 1);
		if(!comres)
			break;
	}
#else
	comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_X_L, &data[0], 1);
	comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_Y_L, &data[1], 1);
	comres = p_kr3dm->KR3DM_BUS_READ_FUNC(p_kr3dm->dev_addr, OUT_Z_L, &data[2], 1);
#endif

	data[0] = (~data[0] + 1);
	data[1] = (~data[1] + 1);
	data[2] = (~data[2] + 1);
	    if(data[0] & 0x80)
		    acc->x = (0x100-data[0])*(-1);
    else
		    acc->x = ((data[0]) & 0xFF);
 
    	if(data[1]& 0x80)
    		acc->y = (0x100-data[1]);
    	else
    		acc->y = ((data[1]) & 0xFF)*(-1);
    	if(data[2]& 0x80)
    		acc->z = (0x100-data[2]);
    	else
    		acc->z = ((data[2]) & 0xFF)*(-1);
#if DEBUG
	//printk("[KR3DM] ==> read-xyz2 : x = %d  /  y =  %d  /  z = %d\n", acc->x, acc->y, acc->z);
#endif
	return comres;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kr3dm_early_suspend(struct early_suspend *handler)
{
#ifdef DEBUG
	printk("[KR3DM] ==> kr3dm_early_suspend\n");
#endif
	kr3dm_set_mode( KR3DM_MODE_SLEEP );
}

static void kr3dm_late_resume(struct early_suspend *handler)
{
#ifdef DEBUG
	printk("[KR3DM] ==> kr3dm_late_resume\n");
#endif
	kr3dm_set_mode( KR3DM_MODE_NORMAL );
}
#endif /* CONFIG_HAS_EARLYSUSPEND */


void kr3dm_chip_init(void)
{

	kr3dm.kr3dm_bus_write = i2c_acc_kr3dm_write;
	kr3dm.kr3dm_bus_read  = i2c_acc_kr3dm_read;

#ifdef CONFIG_HAS_EARLYSUSPEND
	kr3dm.early_suspend.suspend = kr3dm_early_suspend;
	kr3dm.early_suspend.resume = kr3dm_late_resume;
	register_early_suspend(&kr3dm.early_suspend);
#endif
	kr3dm_init( &kr3dm );
}


int kr3dm_init(kr3dm_t *kr3dm)
{

	unsigned char val1 = 0x27;
	unsigned char val2 = 0x00;
#ifdef DEBUG
	printk("[KR3DM] ********** kr3dm_init =====================\n");
#endif
	p_kr3dm = kr3dm;
	p_kr3dm->dev_addr = SENS_ADD;										/* preset KR3DM I2C_addr */
	p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG1, &val1, 1 );
	p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG2, &val2, 1 );
	p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG3, &val2, 1 );
	p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG4, &val2, 1 );
	p_kr3dm->KR3DM_BUS_WRITE_FUNC(p_kr3dm->dev_addr, CTRL_REG5, &val2, 1 );

	return 0;
}

int kr3dm_acc_start(void)
{
	int result;
	struct device *dev_t;

	kr3dmacc_t accels; /* only for test */

	result = register_chrdev( KR3DM_MAJOR, "kr3dm", &kr3dm_acc_fops);

	if (result < 0)
	{
		return result;
	}

	kr3dm_acc_class = class_create (THIS_MODULE, "KR3DM-dev");

	if (IS_ERR(kr3dm_acc_class))
	{
		unregister_chrdev( KR3DM_MAJOR, "kr3dm" );
		return PTR_ERR( kr3dm_acc_class );
	}

	dev_t = device_create( kr3dm_acc_class, NULL, MKDEV(KR3DM_MAJOR, 0), "%s", "kr3dm");

	if (IS_ERR(dev_t))
	{
		return PTR_ERR(dev_t);
	}


	result = i2c_acc_kr3dm_init();

	if(result)
	{
		return result;
	}

	kr3dm_chip_init();
#if DEBUG
	printk("[KR3DM] read_xyz ==========================\n");
	kr3dm_read_accel_xyz( &accels );
	printk("[KR3DM] x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
	printk("[KR3DM] ======================kr3dm_acc_start Ready for use !!!!! =============\n");
#else
	kr3dm_read_accel_xyz( &accels );
#endif
	return 0;
}



static int kr3dm_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	return 0;
}


static int kr3dm_accelerometer_resume( struct platform_device* pdev )
{
	return 0;
}

void kr3dm_acc_end(void)
{
	unregister_chrdev( KR3DM_MAJOR, "kr3dm" );

	i2c_acc_kr3dm_exit();

	device_destroy( kr3dm_acc_class, MKDEV(KR3DM_MAJOR, 0) );
	class_destroy( kr3dm_acc_class );
	unregister_early_suspend(&kr3dm.early_suspend);
}

static int kr3dm_accelerometer_probe( struct platform_device* pdev )
{

	#ifdef DEBUG
	printk("[KR3DM] ********** kr3dm_accelerometer_probe =====================\n");
#endif
	return kr3dm_acc_start();

}


static int __init kr3dm_acc_init(void)
{
	int result;

	result = platform_driver_register( &kr3dm_accelerometer_driver);
#if DEBUG
	printk("[KR3DM] ********** kr3dm_acc_init =====================\n");
#endif
	if( result )
	{
		return result;
	}

	kr3dm_accelerometer_device  = platform_device_register_simple( "kr3dm-accelerometer", -1, NULL, 0 );

	if( IS_ERR( kr3dm_accelerometer_device ) )
	{
		return PTR_ERR( kr3dm_accelerometer_device );
	}

	return 0;
}


static void __exit kr3dm_acc_exit(void)
{
	kr3dm_acc_end();
	platform_device_unregister( kr3dm_accelerometer_device );
	platform_driver_unregister( &kr3dm_accelerometer_driver );
}


module_init( kr3dm_acc_init );
module_exit( kr3dm_acc_exit );

MODULE_AUTHOR("vishnu.p");
MODULE_DESCRIPTION("accelerometer driver for KR3DM");
MODULE_LICENSE("GPL");
