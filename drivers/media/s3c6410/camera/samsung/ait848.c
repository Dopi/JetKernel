/*
 *  Copyright (C) 2004 Samsung Electronics
 *             SW.LEE <hitchcar@samsung.com>
 *            - based on Russell King : pcf8583.c
 * 	      - added  smdk24a0, smdk2440
 *            - added  poseidon (s3c24a0+wavecom)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Driver for FIMC2.x Camera Decoder
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include <mach/hardware.h>

#include <plat/gpio-cfg.h>

#include "../s3c_camif.h"

#include "ait848.h"

static struct i2c_driver ait848_driver;

static void ait848_sensor_gpio_init(void);
static void ait848_sensor_enable(void);
static void ait848_sensor_disable(void);

static int ait848_sensor_init(void);
static void ait848_sensor_exit(void);

static int ait848_sensor_change_size(struct i2c_client *client, int size);

/* 
 * MCLK: 24MHz, PCLK: 54MHz
 * 
 * In case of PCLK 54MHz
 *
 * Preview Mode (1024 * 768)  
 * 
 * Capture Mode (2048 * 1536)
 * 
 * Camcorder Mode
 */
static camif_cis_t ait848_data = {
	itu_fmt:       	CAMIF_ITU601,
	order422:      	CAMIF_CRYCBY,
	camclk:        	26000000,		
	source_x:      	1024,		
	source_y:      	768,
	win_hor_ofst:  	0,
	win_ver_ofst:  	0,
	win_hor_ofst2: 	0,
	win_ver_ofst2: 	0,
	polarity_pclk: 	0,
	polarity_vsync:	1,
	polarity_href: 	0,
	reset_type:		CAMIF_RESET,
	reset_udelay: 	5000,
};

#define AIT848_ID	0x78

static unsigned short ait848_normal_i2c[] = { (AIT848_ID >> 1), I2C_CLIENT_END };
static unsigned short ait848_ignore[] = { I2C_CLIENT_END };
static unsigned short ait848_probe[] = { I2C_CLIENT_END };

static struct i2c_client_address_data ait848_addr_data = {
	.normal_i2c = ait848_normal_i2c,
	.ignore		= ait848_ignore,
	.probe		= ait848_probe,
};

static int ait848_sensor_read(struct i2c_client *client,
		unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };
	
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}

static int ait848_sensor_write(struct i2c_client *client, 
		unsigned short subaddr, unsigned short val)
{
	unsigned char buf[4];
	struct i2c_msg msg = { client->addr, 0, 4, buf };

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);
	buf[2] = (val >> 8);
	buf[3] = (val & 0xFF);

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static void ait848_sensor_get_id(struct i2c_client *client)
{
	unsigned short id = 0;
	
	ait848_sensor_write(client, 0x002C, 0x7000);
	ait848_sensor_write(client, 0x002E, 0x01FA);
	ait848_sensor_read(client, 0x0F12, &id);

	printk("Sensor ID(0x%04x) is %s!\n", id, (id == 0x4CA4) ? "Valid" : "Invalid"); 
}

static void ait848_sensor_gpio_init(void)
{
}	

static void ait848_sensor_enable(void)
{
	ait848_sensor_gpio_init();
}

static void ait848_sensor_disable(void)
{
}

static void sensor_init(struct i2c_client *client)
{
	int i, size;

	size = (sizeof(ait848_init0)/sizeof(ait848_init0[0]));
	for (i = 0; i < size; i++) { 
		ait848_sensor_write(client,
			ait848_init0[i].subaddr, ait848_init0[i].value);
	}	

	mdelay(100);	

	/* Check Sensor ID */
	ait848_sensor_get_id(client);

	size = (sizeof(ait848_init1)/sizeof(ait848_init1[0]));
	for (i = 0; i < size; i++) { 
		ait848_sensor_write(client,
			ait848_init1[i].subaddr, ait848_init1[i].value);
	}

	ait848_sensor_change_size(client, SENSOR_XGA);	
}

static int ait848gx_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	
	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "ait848");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &ait848_driver;
	ait848_data.sensor = c;

	return i2c_attach_client(c);
}

static int ait848_sensor_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &ait848_addr_data, ait848gx_attach);
}

static int ait848_sensor_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int ait848_sensor_mode_set(struct i2c_client *client, int type)
{
	int i, size;
	unsigned short light;
	int delay = 300;

	printk("Sensor Mode ");

	if (type & SENSOR_PREVIEW) {
		
		printk("-> Preview ");
		
		size = (sizeof(ait848_preview)/sizeof(ait848_preview[0]));
		for (i = 0; i < size; i++) 
			ait848_sensor_write(client, ait848_preview[i].subaddr, ait848_preview[i].value);

		if (type & SENSOR_NIGHTMODE) {
		
			printk("Night\n");
			
			size = (sizeof(ait848_nightmode_on)/sizeof(ait848_nightmode_on[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_nightmode_on[i].subaddr, ait848_nightmode_on[i].value);
		}
		else {
			
			printk("Normal\n");
			
			size = (sizeof(ait848_nightmode_off)/sizeof(ait848_nightmode_off[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_nightmode_off[i].subaddr, ait848_nightmode_off[i].value);
		}
	}
	else if (type & SENSOR_CAPTURE) {
		
		printk("-> Capture ");
		
		ait848_sensor_write(client, 0x002C, 0x7000);	
		ait848_sensor_write(client, 0x002E, 0x12FE);
		ait848_sensor_read(client, 0x0F12, &light);

		if (light <= 0x20) {	/* Low light */
			if (type & SENSOR_NIGHTMODE) {	
			
				printk("Night Low Light\n");
				
				delay = 1500;
				size = (sizeof(ait848_snapshot_nightmode)/sizeof(ait848_snapshot_nightmode[0]));
				for (i = 0; i < size; i++)  
					ait848_sensor_write(client, ait848_snapshot_nightmode[i].subaddr, ait848_snapshot_nightmode[i].value);	
			}
			else {
				
				printk("Normal Low Light\n");
				
				delay = 1200;
				size = (sizeof(ait848_snapshot_low)/sizeof(ait848_snapshot_low[0]));
				for (i = 0; i < size; i++)  
					ait848_sensor_write(client, ait848_snapshot_low[i].subaddr, ait848_snapshot_low[i].value);
			}
		}
		else {
			if (light <= 0x40) {
				if (type & SENSOR_NIGHTMODE) {
				
					printk("Night Middle Lignt\n");
					
					delay = 1000;
				}
				else {
					
					printk("Normal Middle Lignt\n");
					
					delay = 600;
				}
			}
			else {
				
				printk("Normal Normal Light\n");
				
				delay = 300;
			}

			size = (sizeof(ait848_capture)/sizeof(ait848_capture[0]));
			for (i = 0; i < size; i++)  
				ait848_sensor_write(client, ait848_capture[i].subaddr, ait848_capture[i].value);	
		}
	}
	else {

		printk("Record\n");
		
		delay = 300;
		size = (sizeof(ait848_fps_15fix)/sizeof(ait848_fps_15fix[0]));
		for (i = 0; i < size; i++)  
			ait848_sensor_write(client, ait848_fps_15fix[i].subaddr, ait848_fps_15fix[i].value);	
	}

	mdelay(delay);
	
	return 0;
}

static int ait848_sensor_change_size(struct i2c_client *client, int size)
{
	switch (size) {
		case SENSOR_XGA:
			ait848_sensor_mode_set(client, SENSOR_PREVIEW);
			break;

		case SENSOR_QXGA:
			ait848_sensor_mode_set(client, SENSOR_CAPTURE);
			break;		
	
		default:
			printk("Unknown Size! (Only XGA & QXGA)\n");
			break;
	}

	return 0;
}

static int ait848_sensor_af_control(struct i2c_client *client, int type)
{
	switch (type) {
		case 1:
		default:
			printk("Focus Mode -> Auto\n");
			ait848_sensor_write(client, 0x002C, 0x7000);	
			ait848_sensor_write(client, 0x002E, 0x030C);
			ait848_sensor_write(client, 0x0F12, 0x0002);
			break;
		case 0:
			printk("Focus Mode -> Manual\n");
			ait848_sensor_write(client, 0x002C, 0x7000);
			ait848_sensor_write(client, 0x002E, 0x030C);
			ait848_sensor_write(client, 0x0F12, 0x0000);
			break;
	}

	return 0;
}

static int ait848_sensor_change_effect(struct i2c_client *client, int type)
{
	int i, size;	
	
	printk("Effects Mode ");

	switch (type) {
		case 0:
		default:
			printk("-> Mode None\n");
			size = (sizeof(ait848_effect_off)/sizeof(ait848_effect_off[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_off[i].subaddr, ait848_effect_off[i].value);
			break;
		case 1:
			printk("-> Mode Gray\n");
			size = (sizeof(ait848_effect_gray)/sizeof(ait848_effect_gray[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_gray[i].subaddr, ait848_effect_gray[i].value);
			break;
		case 2:
			printk("-> Mode Sepia\n");
			size = (sizeof(ait848_effect_sepia)/sizeof(ait848_effect_sepia[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_sepia[i].subaddr, ait848_effect_sepia[i].value);
			break;
		case 3:
			printk("-> Mode Negative\n");
			size = (sizeof(ait848_effect_negative)/sizeof(ait848_effect_negative[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_negative[i].subaddr, ait848_effect_negative[i].value);
			break;
		case 4:
			printk("-> Mode Aqua\n");
			size = (sizeof(ait848_effect_aqua)/sizeof(ait848_effect_aqua[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_aqua[i].subaddr, ait848_effect_aqua[i].value);
			break;
		case 5:
			printk("-> Mode Sketch\n");
			size = (sizeof(ait848_effect_sketch)/sizeof(ait848_effect_sketch[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_effect_sketch[i].subaddr, ait848_effect_sketch[i].value);
			break;
	}

	return 0;
}

static int ait848_sensor_change_br(struct i2c_client *client, int type)
{
	int i, size;

	printk("Brightness Mode \n");

	switch (type) {
		case 0:	
		default :
			printk("-> Brightness Minus 4\n");
			size = (sizeof(ait848_br_minus4)/sizeof(ait848_br_minus4[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_minus4[i].subaddr, ait848_br_minus4[i].value);
			break;
		case 1:
			printk("-> Brightness Minus 3\n");	
			size = (sizeof(ait848_br_minus3)/sizeof(ait848_br_minus3[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_minus3[i].subaddr, ait848_br_minus3[i].value);
			break;
		case 2:
			printk("-> Brightness Minus 2\n");
			size = (sizeof(ait848_br_minus2)/sizeof(ait848_br_minus2[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_minus2[i].subaddr, ait848_br_minus2[i].value);
			break;
		case 3:
			printk("-> Brightness Minus 1\n");
			size = (sizeof(ait848_br_minus1)/sizeof(ait848_br_minus1[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_minus1[i].subaddr, ait848_br_minus1[i].value);
			break;
		case 4:
			printk("-> Brightness Zero\n");
			size = (sizeof(ait848_br_zero)/sizeof(ait848_br_zero[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_zero[i].subaddr, ait848_br_zero[i].value);
			break;
		case 5:
			printk("-> Brightness Plus 1\n");
			size = (sizeof(ait848_br_plus1)/sizeof(ait848_br_plus1[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_plus1[i].subaddr, ait848_br_plus1[i].value);
			break;
		case 6:
			printk("-> Brightness Plus 2\n");
			size = (sizeof(ait848_br_plus2)/sizeof(ait848_br_plus2[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_plus2[i].subaddr, ait848_br_plus2[i].value);
			break;
		case 7:
			printk("-> Brightness Plus 3\n");
			size = (sizeof(ait848_br_plus3)/sizeof(ait848_br_plus3[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_plus3[i].subaddr, ait848_br_plus3[i].value);
			break;
		case 8:
			printk("-> Brightness Plus 4\n");
			size = (sizeof(ait848_br_plus4)/sizeof(ait848_br_plus4[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_br_plus4[i].subaddr, ait848_br_plus4[i].value);
			break;
	}

	return 0;
}

static int ait848_sensor_change_wb(struct i2c_client *client, int type)
{
	int i, size;
	
	printk("White Balance Mode ");

	switch (type) {
		case 0:
		default :
			printk("-> WB auto mode\n");
			size = (sizeof(ait848_wb_auto)/sizeof(ait848_wb_auto[0]));
			for (i = 0; i < size; i++) 
				ait848_sensor_write(client, ait848_wb_auto[i].subaddr, ait848_wb_auto[i].value);
			break;
		case 1:
			printk("-> WB Sunny mode\n");
			size = (sizeof(ait848_wb_sunny)/sizeof(ait848_wb_sunny[0]));
			for (i = 0; i < size; i++)  
				ait848_sensor_write(client, ait848_wb_sunny[i].subaddr, ait848_wb_sunny[i].value);					
			break;
		case 2:
			printk("-> WB Cloudy mode\n");
			size = (sizeof(ait848_wb_cloudy)/sizeof(ait848_wb_cloudy[0]));
			for (i = 0; i < size; i++)  
				ait848_sensor_write(client, ait848_wb_cloudy[i].subaddr, ait848_wb_cloudy[i].value);					
			break;
		case 3:
			printk("-> WB Flourescent mode\n");
			size = (sizeof(ait848_wb_fluorescent)/sizeof(ait848_wb_fluorescent[0]));
			for (i = 0; i < size; i++)  
				ait848_sensor_write(client, ait848_wb_fluorescent[i].subaddr, ait848_wb_fluorescent[i].value);					
			break;
		case 4:
			printk("-> WB Tungsten mode\n");
			size = (sizeof(ait848_wb_tungsten)/sizeof(ait848_wb_tungsten[0]));
			for (i = 0; i < size; i++)  
				ait848_sensor_write(client, ait848_wb_tungsten[i].subaddr, ait848_wb_tungsten[i].value);					
			break;

	}

	return 0;
}

static int ait848_sensor_user_read(struct i2c_client *client, ait848_short_t *r_data)
{
	ait848_sensor_write(client, 0x002C, r_data->page);
	ait848_sensor_write(client, 0x002E, r_data->subaddr);
	return ait848_sensor_read(client, 0x0F12, &(r_data->value));
}

static int ait848_sensor_user_write(struct i2c_client *client, unsigned short *w_data)
{
	return ait848_sensor_write(client, w_data[0], w_data[1]);
}

static int ait848_sensor_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	struct v4l2_control *ctrl;
	unsigned short *w_data;		/* To support user level i2c */	
	ait848_short_t *r_data;

	switch (cmd) {
		case SENSOR_INIT:
			sensor_init(client);
			break;

		case USER_ADD:
			break;

		case USER_EXIT:
			ait848_sensor_exit();
			break;

		case SENSOR_EFFECT:
			ctrl = (struct v4l2_control *)arg;
			ait848_sensor_change_effect(client, ctrl->value);
			break;

		case SENSOR_BRIGHTNESS:
			ctrl = (struct v4l2_control *)arg;
			ait848_sensor_change_br(client, ctrl->value);
			break;

		case SENSOR_WB:
			ctrl = (struct v4l2_control *)arg;
			ait848_sensor_change_wb(client, ctrl->value);
			break;

		case SENSOR_AF:
			ctrl = (struct v4l2_control *)arg;
			ait848_sensor_af_control(client, ctrl->value);
			break;

		case SENSOR_MODE_SET:
			ctrl = (struct v4l2_control *)arg;
			ait848_sensor_mode_set(client, ctrl->value);
			break;

		case SENSOR_XGA:
			ait848_sensor_change_size(client, SENSOR_XGA);	
			break;

		case SENSOR_QSVGA:
			ait848_sensor_change_size(client, SENSOR_QSVGA);
			break;

		case SENSOR_VGA:
			ait848_sensor_change_size(client, SENSOR_VGA);
			break;

		case SENSOR_SVGA:
			ait848_sensor_change_size(client, SENSOR_SVGA);
			break;

		case SENSOR_SXGA:
			ait848_sensor_change_size(client, SENSOR_SXGA);
			break;

		case SENSOR_UXGA:
			ait848_sensor_change_size(client, SENSOR_UXGA);
			break;

		case SENSOR_USER_WRITE:
			w_data = (unsigned short *)arg;
			ait848_sensor_user_write(client, w_data);
			break;

		case SENSOR_USER_READ:
			r_data = (ait848_short_t *)arg;
			ait848_sensor_user_read(client, r_data);
			break;

		default:
			break;
	}

	return 0;
}

static struct i2c_driver ait848_driver = {
	.driver = {
		.name = "ait848",
	},
	.id = AIT848_ID,
	.attach_adapter = ait848_sensor_attach_adapter,
	.detach_client = ait848_sensor_detach,
	.command = ait848_sensor_command
};

static int ait848_sensor_init(void)
{
	int ret;

	ait848_sensor_enable();
	
	s3c_camif_open_sensor(&ait848_data);

	if (ait848_data.sensor == NULL)
		if ((ret = i2c_add_driver(&ait848_driver)))
			return ret;

	if (ait848_data.sensor == NULL) {
		i2c_del_driver(&ait848_driver);	
		return -ENODEV;
	}

	s3c_camif_register_sensor(&ait848_data);
	
	return 0;
}

static void ait848_sensor_exit(void)
{
	ait848_sensor_disable();
	
	if (ait848_data.sensor != NULL)
		s3c_camif_unregister_sensor(&ait848_data);
}

static struct v4l2_input ait848_input = {
	.index		= 0,
	.name		= "Camera Input (AIT848)",
	.type		= V4L2_INPUT_TYPE_CAMERA,
	.audioset	= 1,
	.tuner		= 0,
	.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
	.status		= 0,
};

static struct v4l2_input_handler ait848_input_handler = {
	ait848_sensor_init,
	ait848_sensor_exit	
};

#ifdef CONFIG_VIDEO_SAMSUNG_MODULE
static int ait848_sensor_add(void)
{
	return s3c_camif_add_sensor(&ait848_input, &ait848_input_handler);
}

static void ait848_sensor_remove(void)
{
	if (ait848_data.sensor != NULL)
		i2c_del_driver(&ait848_driver);

	s3c_camif_remove_sensor(&ait848_input, &ait848_input_handler);
}

module_init(ait848_sensor_add)
module_exit(ait848_sensor_remove)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("I2C Client Driver For FIMC V4L2 Driver");
MODULE_LICENSE("GPL");
#else
int ait848_sensor_add(void)
{
	return s3c_camif_add_sensor(&ait848_input, &ait848_input_handler);
}

void ait848_sensor_remove(void)
{
	if (ait848_data.sensor != NULL)
		i2c_del_driver(&ait848_driver);

	s3c_camif_remove_sensor(&ait848_input, &ait848_input_handler);
}
#endif
