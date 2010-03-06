#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

//#include <asm/hardware.h>
//#include <asm/arch/gpio.h>
//add by inter.park
#include <mach/hardware.h>
#include <linux/gpio.h>

#include <linux/irq.h>
#include <linux/i2c.h>

#include "bma020_i2c.h"

static int i2c_acc_bma020_attach_adapter(struct i2c_adapter *adapter);
static int i2c_acc_bma020_probe_client(struct i2c_adapter *, int,  int);
static int i2c_acc_bma020_detach_client(struct i2c_client *client);


#define	ACC_SENSOR_ADDRESS		0x38

#define I2C_M_WR				0x00
#define I2C_DF_NOTIFY			0x01

struct i2c_driver acc_bma020_i2c_driver =
{
	.driver = {
		.name = "bma020_accelerometer_driver",
	},
	.attach_adapter	= &i2c_acc_bma020_attach_adapter,
	.detach_client	= &i2c_acc_bma020_detach_client,
};

#if 0
static struct i2c_client *g_client;
static unsigned short ignore[] = { I2C_CLIENT_END };

static unsigned short normal_addr[] = {
	ACC_SENSOR_ADDRESS, 
	I2C_CLIENT_END 
};
#endif

static struct i2c_client *g_client;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { I2C_CLIENT_END };
static unsigned short probe_addr[] = { 0, ACC_SENSOR_ADDRESS, I2C_CLIENT_END };


static struct i2c_client_address_data addr_data = {
	.normal_i2c		= normal_addr,
	.probe			= probe_addr,
	.ignore			= ignore,
};

int i2c_acc_bma020_init(void)
{
	int ret;

	if ( (ret = i2c_add_driver(&acc_bma020_i2c_driver)) ) 
	{
		printk("Driver registration failed, module not inserted.\n");
		return ret;
	}

	return 0;
}

void i2c_acc_bma020_exit(void)
{
	i2c_del_driver(&acc_bma020_i2c_driver); 
}


char i2c_acc_bma020_read(u8 reg, u8 *val, unsigned int len )
{
	int 	 err;
	struct 	 i2c_msg msg[1];
	
	unsigned char data[1];
	if( (g_client == NULL) || (!g_client->adapter) )
	{
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
		return 0;
	}
	printk("%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */

	return err;

}
char i2c_acc_bma020_write( u8 reg, u8 *val )
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

	printk("%s %d i2c transfer error\n", __func__, __LINE__);/* add by inter.park */
	return err;
}

static int i2c_acc_bma020_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, &i2c_acc_bma020_probe_client);
}

static int i2c_acc_bma020_probe_client(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;
	int err = 0;
   	
	if ( !i2c_check_functionality(adapter,I2C_FUNC_SMBUS_BYTE_DATA) ) {
		printk(KERN_INFO "byte op is not permited.\n");
		goto ERROR0;
	}

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL );

	if ( !new_client )  {
		err = -ENOMEM;
		goto ERROR0;
	}

	new_client->addr = address;	
 	new_client->adapter = adapter;
	new_client->driver = &acc_bma020_i2c_driver;
	new_client->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;


	g_client = new_client;

	strlcpy(new_client->name, "bma020", I2C_NAME_SIZE);

	if ((err = i2c_attach_client(new_client)))
		goto ERROR1;

		return 0;

	ERROR1:
		printk("i2c_acc_bma020_probe_client() ERROR1\n");/* add by inter.park */
		kfree(new_client);
	ERROR0:
		printk("i2c_acc_bma020_probe_client() ERROR0\n");/* add by inter.park */
    	return err;
}

static int i2c_acc_bma020_detach_client(struct i2c_client *client)
{
	int err;

  	/* Try to detach the client from i2c space */
	if ((err = i2c_detach_client(client))) {
        return err;
	}
	

	kfree(client); /* Frees client data too, if allocated at the same time */
	g_client = NULL;
	return 0;
}

