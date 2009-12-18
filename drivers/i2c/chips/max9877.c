// Maxim 8906 Command Module: Interface Window
// Firmware Group
// 1/2/2008   initialize
// (C) 2004 Maxim Integrated Products
//---------------------------------------------------------------------------

//#include <linux/i2c/maximi2c.h>  // dgahn
#include <linux/i2c/max9877.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <asm/io.h>

#include <linux/i2c.h>


#include <linux/i2c/max9877.h> /* define ioctls */


#define SUBJECT "max9877.c"
#define P(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#define FI \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - IN" "\n", __func__, __LINE__, __func__);
#define FO \
	printk ("[ "SUBJECT " (%s,%d) ] " "%s - OUT" "\n", __func__, __LINE__, __func__);


/*===========================================================================

      D E F I N E S

===========================================================================*/
#define ALLOW_USPACE_RW		1

#define MAX9877_INPUTMODE_CTRL 0x00
#define MAX9877_SPKVOL_CTRL 0x01
#define MAX9877_LEFT_HPHVOL_CTRL 0x02
#define MAX9877_RIGHT_HPHVOL_CTRL 0x03
#define MAX9877_OUTPUTMODE_CTRL 0x04

#ifdef CONFIG_HAS_EARLYSUSPEND
#undef CONFIG_HAS_EARLYSUSPEND
#endif

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif	/* CONFIG_HAS_EARLYSUSPEND */

static struct wake_lock	max9877_wake_lock;

static inline void init_suspend(void)
{
	wake_lock_init(&max9877_wake_lock, WAKE_LOCK_SUSPEND, "max9877");
}

static inline void deinit_suspend(void)
{
	wake_lock_destroy(&max9877_wake_lock);
}

static inline void prevent_suspend(void)
{
	wake_lock(&max9877_wake_lock);
}

static inline void allow_suspend(void)
{
	wake_unlock(&max9877_wake_lock);
}
#else
static inline void init_suspend(void) {}
static inline void deinit_suspend(void) {}
static inline void prevent_suspend(void) {}
static inline void allow_suspend(void) {}
#endif	/* CONFIG_HAS_WAKELOCK */

#ifdef CONFIG_HAS_EARLYSUSPEND
static void max9877_early_suspend(struct early_suspend *h);
static void max9877_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

DECLARE_MUTEX(audio_sem);

struct max9877_data {
	struct work_struct work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif	/* CONFIG_HAS_EARLYSUSPEND */
};



/*===================================================================================================================*/
/* MAX9877 I2C Interface                                                                                             */
/*===================================================================================================================*/


#define MAX9877_ID	0x9A	/* Read Time Clock */

static struct i2c_driver max9877_driver;

static struct i2c_client *max9877_i2c_client = NULL;

static unsigned short max9877_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max9877_ignore[] = { I2C_CLIENT_END };
static unsigned short max9877_probe[] = { 2, (MAX9877_ID >> 1),  I2C_CLIENT_END };

static struct i2c_client_address_data max9877_addr_data = {
	.normal_i2c = max9877_normal_i2c,
	.ignore		= max9877_ignore,
	.probe		= max9877_probe,
};

#define I2C_WRITE(reg,data) if (!max9877_i2c_write(reg, data) < 0) return -EIO
#define I2C_READ(reg,data) if (max9877_i2c_read(reg,data) < 0 ) return -EIO

char max9877_outmod_reg;

int audio_i2c_tx_data(char* txData, int length)
{
	int rc; 

	struct i2c_msg msg[] = {
		{
			.addr = max9877_i2c_client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,		
		},
	};
    
	rc = i2c_transfer(max9877_i2c_client->adapter, msg, 1);
	if (rc < 0) {
		printk(KERN_ERR "max9877: audio_i2c_tx_data error %d\n", rc);
		return rc;
	}

#if 0
	else {
		int i;
		/* printk(KERN_INFO "mt_i2c_lens_tx_data: af i2c client addr = %x,"
		   " register addr = 0x%02x%02x:\n", slave_addr, txData[0], txData[1]); 
		   */
		for (i = 0; i < length; i++)
			printk("\tdata[%d]: 0x%02x\n", i, txData[i]);
	}
#endif
	return 0;
}


static int max9877_i2c_write(unsigned char u_addr, unsigned char u_data)
{
	int rc;
	unsigned char buf[2];

	buf[0] = u_addr;
	buf[1] = u_data;
   
        printk("--- max9877 i2c --- write : reg - 0x%02x, val - 0x%02x\n",u_addr,u_data);
 
	rc = audio_i2c_tx_data(buf, 2);
	if(rc < 0)
		printk(KERN_ERR "max9877: txdata error %d add:0x%02x data:0x%02x\n",
			rc, u_addr, u_data);
	return rc;	
}

static int audio_i2c_rx_data(char* rxData, int length)
{
	int rc;

	struct i2c_msg msgs[] = {
		{
			.addr = max9877_i2c_client->addr,
			.flags = 0,      
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = max9877_i2c_client->addr,
			.flags = I2C_M_RD|I2C_M_NO_RD_ACK,
			.len = length,
			.buf = rxData,
		},
	};

	rc = i2c_transfer(max9877_i2c_client->adapter, msgs, 2);
      
	if (rc < 0) {
		printk(KERN_ERR "max9877: audio_i2c_rx_data error %d\n", rc);
		return rc;
	}
      
#if 0
	else {
		int i;
		for (i = 0; i < length; i++)
			printk(KERN_INFO "\tdata[%d]: 0x%02x\n", i, rxData[i]);
	}
#endif

	return 0;
}

static int max9877_i2c_read(unsigned char u_addr, unsigned char *pu_data)
{
	int rc;
	unsigned char buf;

	buf = u_addr;
	rc = audio_i2c_rx_data(&buf, 1);
	if (!rc)
		*pu_data = buf;
	else printk(KERN_ERR "max9877: i2c read failed\n");
	return rc;	
}

static void max9877_chip_init(void)
{
//	int ret;
//
	unsigned char reg_value;

	printk(KERN_INFO "max9877: init\n");
	if (!max9877_i2c_client) 
	{
        printk(KERN_INFO "max9877: max9877_i2c_client = NULL\n");
        return;
    }


    /* max9877 init sequence */
    I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,0x04); // output all
    
    I2C_WRITE(MAX9877_INPUTMODE_CTRL,0x50); // pre-gain 0dB

    I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,0x84); // SDHN ONLCR

    mdelay(10);

    I2C_WRITE(MAX9877_SPKVOL_CTRL,0x1F); // MAX 0x1F : 0dB

    I2C_WRITE(MAX9877_LEFT_HPHVOL_CTRL,0x1F); // MAX 0x1F : 0dB

    I2C_WRITE(MAX9877_RIGHT_HPHVOL_CTRL,0x1F); // MAX 0x1F : 0dB


    I2C_READ(MAX9877_INPUTMODE_CTRL, &reg_value);
    P(" outputmode reg(0x00) = 0x%02x\n",reg_value);  

    I2C_READ(MAX9877_OUTPUTMODE_CTRL, &reg_value);
    P(" inputmode reg(0x04) = 0x%02x\n",reg_value);  

	printk(KERN_INFO "max9877: max9877 sensor init sequence done\n");
}

int max9877_i2c_hph_gain(uint8_t gain)
{
	static const uint8_t max_legal_gain  = 0x1F;
	
	if (gain > max_legal_gain) gain = max_legal_gain;

	I2C_WRITE(MAX9877_LEFT_HPHVOL_CTRL, gain);
	I2C_WRITE(MAX9877_RIGHT_HPHVOL_CTRL, gain);
	return 0;
}

int max9877_i2c_spk_gain(uint8_t gain)
{
	static const uint8_t max_legal_gain  = 0x1F;
	
	if (gain > max_legal_gain) gain = max_legal_gain;

	I2C_WRITE(MAX9877_SPKVOL_CTRL, gain);
	return 0;
}

int max9877_i2c_speaker_headset_onoff(int nOnOff)	// dh0421.hwang_081218 : for ??
{
        unsigned char reg_value;

        if( nOnOff )
        {
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x09;

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL, reg_value);

        }
        else
        {
#if 1		// return to isr last value
            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL, max9877_outmod_reg); 
#endif
        }

	return 0;
}

int max9877_i2c_speaker_onoff(int nOnOff)
{
        unsigned char reg_value;

        if( nOnOff )
        {
            P("max9877 control Speaker On");
            
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x04;

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL, reg_value);

        }
        else
        {
            P("max9877 control Speaker Off");
            
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

            reg_value &= 0xB0;
			reg_value |= 0x08;

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,reg_value); 
        }
		max9877_outmod_reg = reg_value;
	return 0;
}

int max9877_i2c_receiver_onoff(int nOnOff)
{
        unsigned char reg_value;

        if( nOnOff )
        {
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value); 

            reg_value |= 0x40; // BYPASS_ON
            
            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,reg_value); 
        }
        else
        {
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

            reg_value &= 0xBF; // BYPASS_OFF

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,reg_value); 
        }
		max9877_outmod_reg = reg_value;

	return 0;
}

int max9877_i2c_headset_onoff(int nOnOff)
{
        unsigned char reg_value;

        if( nOnOff )
        {
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

            reg_value &= 0xB0;
	    reg_value |= 0x02;

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL,reg_value); 
        }
        else
        {
            I2C_READ(MAX9877_OUTPUTMODE_CTRL,&reg_value);

			reg_value &= 0xB0;
			reg_value |= 0x04;

            I2C_WRITE(MAX9877_OUTPUTMODE_CTRL, reg_value);
        }
		max9877_outmod_reg = reg_value;

	return 0;
}



static int max9877_attach(struct i2c_adapter *adap, int addr, int kind)
#if 1
{
	struct i2c_client *c;
	int ret,err=0;

	printk("in %s \n",__func__);
  
	if ( !i2c_check_functionality(adap,I2C_FUNC_I2C) ) {
		printk("byte op is not permited.\n");
		return err;
	}

	c = kzalloc(sizeof(struct i2c_client),GFP_KERNEL);
	if (!c)
	{
		printk("kzalloc error \n");
		return -ENOMEM;

	}

	memset(c, 0, sizeof(struct i2c_client));	
	strncpy(c->name,"max9877_i2c",I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &max9877_driver;
	c->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;

	if ((ret = i2c_attach_client(c)) < 0)
	{
		printk("i2c_attach_client error\n");
		goto error;
	}
	max9877_i2c_client = c;

    printk("max9877 is attached..\n");

	return ret;

error:
	printk("in %s , ret = %d \n",__func__,ret);
	kfree(c);
	return err;
}

#else
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, max9877_driver.driver.name);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &max9877_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	if ((addr << 1) == MAX9877_ID)
		max9877_i2c_client = c;

error:
	return ret;
}
#endif

static int max9877_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &max9877_addr_data, max9877_attach);
}

static int max9877_detach_client(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int max9877_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

static struct i2c_driver max9877_driver = {
	.driver = {
		.name = "max9877_i2c",
	},
	.attach_adapter = max9877_attach_adapter,
	.detach_client = max9877_detach_client,
	.command = max9877_command
};

int max9877_i2c_init(void) 
{
	printk("in %s\n",__func__);
	
	
	if( i2c_add_driver(&max9877_driver))
	{
		printk("i2c_add_driver failed \n");
		return -ENODEV;
	}
    
	printk("max9877_i2c_init success... \n");

	return 0;
}


static int max9877_amp_probe( struct platform_device* pdev )
{
	struct max9877_data *mt;
	int err = 0;
	printk(KERN_INFO "max9877: probe\n");

	max9877_chip_init();


#ifdef CONFIG_HAS_EARLYSUSPEND
	mt->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	mt->early_suspend.suspend = max9877_early_suspend;
	mt->early_suspend.resume = max9877_late_resume;
	register_early_suspend(&mt->early_suspend);
#else
	init_suspend();
#endif	/* CONFIG_HAS_EARLYSUSPEND */    

	return 0;
	
exit_misc_device_register_failed:
exit_alloc_data_failed:
exit_check_functionality_failed:
	
	return err;
}

	
static int max9877_amp_remove(struct i2c_client *client)
{
	struct max9877_data *mt = i2c_get_clientdata(client);
	// free_irq(client->irq, mt);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mt->early_suspend);
#else
	deinit_suspend();
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#if 0
	i2c_detach_client(client);
	max9877_i2c_client = NULL;
	misc_deregister(&max9877_device);
	kfree(mt);
#endif

	return 0;
}

int max9877_suspend(void)
{
    int ret; 
    unsigned char reg_value;

//	printk("[%s/%d] audio_enabled : %d\n", __FUNCTION__, __LINE__, audio_enabled);
//	if(!audio_enabled) {

		ret = max9877_i2c_read(MAX9877_OUTPUTMODE_CTRL,&reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "max9877_suspend: max9877_i2c_read failed\n");
			return -EIO;
		}

		reg_value = reg_value & 0x7f; // SHDN = 0 max9877 shut down

		ret = max9877_i2c_write(MAX9877_OUTPUTMODE_CTRL, reg_value);
		if (ret < 0 )
		{
			printk(KERN_ERR "max9877_suspend: max9877_i2c_write failed\n");
			return -EIO;
		}

		printk("max9877_suspend: success\n");
//	}

    return 0;
}

int max9877_resume(void)
{
    int ret; 
    unsigned char reg_value;
    
    ret = max9877_i2c_read(MAX9877_OUTPUTMODE_CTRL,&reg_value);
    if (ret < 0 )
    {
        printk(KERN_ERR "max9877_resume: max9877_i2c_read failed\n");
        return -EIO;
    }

    reg_value = reg_value | 0x80; // SHDN = 1 max9877 wakeup
    
    ret = max9877_i2c_write(MAX9877_OUTPUTMODE_CTRL, reg_value);
    if (ret < 0 )
    {
        printk(KERN_ERR "max9877_resume: max9877_i2c_write failed\n");
        return -EIO;
    }

    msleep(10); // 10m startup time delay
    
    printk("max9877_resume: success\n");

    return 0;
}
//#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void max9877_early_suspend(struct early_suspend *h)
{
	struct max9877_data *mt;
	mt = container_of(h, struct max9877_data, early_suspend);
	max9877_suspend();
}

static void max9877_late_resume(struct early_suspend *h)
{
	struct max9877_data *mt;
	mt = container_of(h, struct max9877_data, early_suspend);
	max9877_resume();
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

static struct platform_driver max9877_amp_driver = {
	.probe = max9877_amp_probe,
	.remove = max9877_amp_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= max9877_suspend,
	.resume	= max9877_resume,
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	.driver = {		
		.name   = "max9877",
        .owner = THIS_MODULE,
	},
};

static int __init max9877_init(void)
{
    int ret;

    max9877_i2c_init();
	
	ret = platform_driver_register(&max9877_amp_driver);
	printk("after %s :return value = %d \n",__func__,ret);
	return ret;
}

static void __exit max9877_exit(void)
{
	i2c_del_driver(&max9877_driver);
}

EXPORT_SYMBOL(max9877_i2c_speaker_onoff);
EXPORT_SYMBOL(max9877_i2c_headset_onoff);
EXPORT_SYMBOL(max9877_i2c_spk_gain);
EXPORT_SYMBOL(max9877_i2c_hph_gain);
EXPORT_SYMBOL(max9877_suspend);
EXPORT_SYMBOL(max9877_resume);    

MODULE_AUTHOR("@samsung.com>");
MODULE_DESCRIPTION("MAX9877 Driver");
MODULE_LICENSE("GPL");

module_init(max9877_init);
module_exit(max9877_exit);
