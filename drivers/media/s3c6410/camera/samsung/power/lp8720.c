/*
 *  lp8720.c - driver for LP8720
 *
 *  Copyright (C) 2009 Jeonghwan Min <jh78.min@samsung.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */

#include <linux/i2c.h>
#include <linux/delay.h>

#include <mach/hardware.h>

#include <plat/gpio-cfg.h>

#define LP8720_ID	0xFA

static struct i2c_driver lp8720_driver;

static struct i2c_client *lp8720_i2c_client = NULL;

static unsigned short lp8720_normal_i2c[] = { (LP8720_ID >> 1), I2C_CLIENT_END };
static unsigned short lp8720_ignore[] = { 1, (LP8720_ID >> 1), I2C_CLIENT_END };	/* To Avoid HW Problem */
static unsigned short lp8720_probe[] = { I2C_CLIENT_END };

static struct i2c_client_address_data lp8720_addr_data = {
	.normal_i2c = lp8720_normal_i2c,
	.ignore		= lp8720_ignore,
	.probe		= lp8720_probe,
};

static int lp8720_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int lp8720_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

static int lp8720_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;
	
	printk("%s %d\n", __FUNCTION__, __LINE__);
	
	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, lp8720_driver.driver.name);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &lp8720_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	lp8720_i2c_client = c;

error:
	return ret;
}

static int lp8720_attach_adapter(struct i2c_adapter *adap)
{
	printk("%s %d nr %d\n", __FUNCTION__, __LINE__, adap->nr);
	return i2c_probe(adap, &lp8720_addr_data, lp8720_attach);
}

static int lp8720_detach_client(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver lp8720_driver = {
	.driver = {
		.name = "lp8720",
	},
	.attach_adapter = lp8720_attach_adapter,
	.detach_client = lp8720_detach_client,
};

static int lp8720_init(void)
{
	if (lp8720_i2c_client == NULL)
		return i2c_add_driver(&lp8720_driver);

	return 0;
}

static void lp8720_exit(void)
{
	i2c_del_driver(&lp8720_driver);
}

#define GENERAL_SETTINGS_REG	0x00
#define LDO1_SETTINGS_REG		0x01
#define LDO2_SETTINGS_REG		0x02
#define LDO3_SETTINGS_REG		0x03
#define LDO4_SETTINGS_REG		0x04
#define LDO5_SETTINGS_REG		0x05
#define BUCK_SETTINGS1_REG		0x06
#define BUCK_SETTINGS2_REG		0x07
#define ENABLE_BITS_REG			0x08
#define PULLDOWN_BITS_REG		0x09
#define STATUS_BITS_REG			0x0A
#define INTERRUPT_BITS_REG		0x0B
#define INTERRUPT_MASK_REG		0x0C

void s5k4ca_sensor_power_init(void)
{
	u8 data;

	if (!lp8720_init()) {
		lp8720_read(lp8720_i2c_client, LDO1_SETTINGS_REG, &data);	
		data &= ~(0x1F << 0);	
		data |= (0x19 << 0);	/* AF 2.8V */
		lp8720_write(lp8720_i2c_client, LDO1_SETTINGS_REG, data);	
		printk("LDO1_SETTINGS_REG 0x%02x, DATA 0x%02x\n", LDO1_SETTINGS_REG, data);

		lp8720_read(lp8720_i2c_client, LDO2_SETTINGS_REG, &data);	
		data &= ~(0x1F << 0);	
		data |= (0x19 << 0);	/* AVDD 2.8V */
		lp8720_write(lp8720_i2c_client, LDO2_SETTINGS_REG, data);	
		printk("LDO2_SETTINGS_REG 0x%02x, DATA 0x%02x\n", LDO2_SETTINGS_REG, data);

		lp8720_read(lp8720_i2c_client, LDO3_SETTINGS_REG, &data);	
		data &= ~(0x1F << 0);	
		data |= (0x0C << 0);	/* DVDD 1.8V */
		lp8720_write(lp8720_i2c_client, LDO3_SETTINGS_REG, data);	
		printk("LDO3_SETTINGS_REG 0x%02x, DATA 0x%02x\n", LDO3_SETTINGS_REG, data);
		
		lp8720_read(lp8720_i2c_client, LDO5_SETTINGS_REG, &data);	
		data &= ~(0x1F << 0);	
		data |= (0x19 << 0);	/* IO 2.8V */
		lp8720_write(lp8720_i2c_client, LDO5_SETTINGS_REG, data);	
		printk("LDO5_SETTINGS_REG 0x%02x, DATA 0x%02x\n", LDO5_SETTINGS_REG, data);
		
		lp8720_read(lp8720_i2c_client, ENABLE_BITS_REG, &data);	
		data &= ~(0x1F << 0);	
		data |= (0x17 << 0);	/* LDO5, LDO3, LDO2, LDO1 */
		lp8720_write(lp8720_i2c_client, ENABLE_BITS_REG, data);	
		printk("ENABLE_BITS_REG 0x%02x, DATA 0x%02x\n", ENABLE_BITS_REG, data);
	}
}
