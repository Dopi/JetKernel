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

#include "po4010.h"

static struct i2c_driver po4010_driver;

static void po4010_sensor_gpio_init(void);
static void po4010_sensor_enable(void);
static void po4010_sensor_disable(void);

static int po4010_sensor_init(void);
static void po4010_sensor_exit(void);

static int po4010_sensor_change_size(struct i2c_client *client, int size);

static camif_cis_t po4010_data = {
	itu_fmt:       	CAMIF_ITU601,
	order422:      	CAMIF_CBYCRY,
	camclk:        	24000000,		
	source_x:      	352,		
	source_y:      	288,
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

#define PO4010_ID	0xE0

static unsigned short po4010_normal_i2c[] = { (PO4010_ID >> 1), I2C_CLIENT_END };
static unsigned short po4010_ignore[] = { I2C_CLIENT_END };
static unsigned short po4010_probe[] = { I2C_CLIENT_END };

static struct i2c_client_address_data po4010_addr_data = {
	.normal_i2c = po4010_normal_i2c,
	.ignore		= po4010_ignore,
	.probe		= po4010_probe,
};

static int po4010_sensor_read(struct i2c_client *client,
		unsigned char subaddr, unsigned char *data)
{
	int ret;
	unsigned char buf[1];
	struct i2c_msg msg = { client->addr, 0, 1, buf };
	
	buf[0] = subaddr;

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	if (ret == -EIO) 
		goto error;

	*data = buf[0];

error:
	return ret;
}

static int po4010_sensor_write(struct i2c_client *client, 
		unsigned char subaddr, unsigned char val)
{
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };
	
	buf[0] = subaddr;
	buf[1] = val;

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static void po4010_sensor_get_id(struct i2c_client *client)
{
	unsigned char id_h, id_l, rev; 	

	po4010_sensor_write(client, 0x03, 0x00);
	po4010_sensor_read(client, 0x00, &id_h);
	po4010_sensor_read(client, 0x01, &id_l);
	po4010_sensor_read(client, 0x02, &rev);

	printk("Sensor ID(0x%04x) Rev(0x%02x)\n", ((id_h << 8) | (id_l)), rev); 
}

static void po4010_sensor_gpio_init(void)
{
	/* MCAM & VCAM I2C Low */
	I2C_CAM_EN_CLR		
	
	/* VCAM RST Low */
	if (gpio_is_valid(GPIO_VCAM_RST_N)) {
		if (gpio_request(GPIO_VCAM_RST_N, S3C_GPIO_LAVEL(GPIO_VCAM_RST_N_AF))) 
			printk(KERN_ERR "Failed to request GPIO_VCAM_RST_N!\n");
		gpio_direction_output(GPIO_VCAM_RST_N, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_VCAM_RST_N, S3C_GPIO_PULL_NONE);

	/* CAM PWR Low */
	CAM_PWR_EN_CLR
	
	/* MCAM AF PWR Low */
	AF_EN_CLR

	/* MCAM STB Low */
	MCAM_STB_CLR	

	/* VCAM STB High */
	VCAM_STB_SET	
}	

static void po4010_sensor_enable(void)
{
	po4010_sensor_gpio_init();

	/* MCLK Disable */
	clk_disable(cam_clock);

	/* VCAM STB Low */
	VCAM_STB_CLR
	
	/* > 0 ms */
	mdelay(1);
	
	/* CAM PWR High */
	CAM_PWR_EN_SET
	
	/* > 0 ms */
	mdelay(1);

	/* MCLK Set */
	clk_set_rate(cam_clock, po4010_data.camclk);

	/* MCLK Enable */
	clk_enable(cam_clock);

	/* > 0 ms */
	mdelay(10);

	/* VCAM RST High */
	gpio_set_value(GPIO_VCAM_RST_N, GPIO_LEVEL_HIGH);

	/* CAM I2C High */
	I2C_CAM_EN_SET		
}

static void po4010_sensor_disable(void)
{
	/* CAM I2C Low */
	I2C_CAM_EN_CLR		
	
	/* VCAM STB High */
	VCAM_STB_SET

	/* > 0 ms */
	mdelay(1);

	/* MCLK Disable */
	clk_disable(cam_clock);

	/* > 0 ms */
	mdelay(1);

	/* VCAM RST Low */
	gpio_set_value(GPIO_VCAM_RST_N, GPIO_LEVEL_LOW);

	/* > 0 ms */
	mdelay(1);

	/* CAM PWR Low */
	CAM_PWR_EN_CLR
}

static void sensor_init(struct i2c_client *client)
{
	int i, size;

	po4010_sensor_get_id(client);

	size = (sizeof(po4010_reg)/sizeof(po4010_reg[0]));
	for (i = 0; i < size; i++) { 
		po4010_sensor_write(client,
			po4010_reg[i].subaddr, po4010_reg[i].value);
	}	

	mdelay(300);	

	po4010_sensor_change_size(client, SENSOR_CIF);	
}

static int po4010_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strcpy(c->name, "po4010");
	c->addr = addr;
	c->adapter = adap;
	c->driver = &po4010_driver;
	po4010_data.sensor = c;

	return i2c_attach_client(c);
}

static int po4010_sensor_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &po4010_addr_data, po4010_attach);
}

static int po4010_sensor_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int po4010_sensor_change_size(struct i2c_client *client, int size)
{
	int i;

	switch (size) {
		case SENSOR_CIF:
			size = (sizeof(po4010_preview)/sizeof(po4010_preview[0]));
			for (i = 0; i < size; i++) { 
				po4010_sensor_write(client,
						po4010_preview[i].subaddr, po4010_preview[i].value);
			}
			break;

		default:
			printk("Unknown Size! (Only CIF)\n");
			break;
	}

	return 0;
}

static int po4010_sensor_af_control(struct i2c_client *client, int type)
{
	return 0;
}

static int po4010_sensor_change_effect(struct i2c_client *client, int type)
{
	int i, size;	
	
	printk("Effects Mode ");

	switch (type) {
		case 0:
		default:
			printk("-> Mode None\n");
			size = (sizeof(po4010_effect_off)/sizeof(po4010_effect_off[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_off[i].subaddr, po4010_effect_off[i].value);
			break;
		case 1:
			printk("-> Mode Gray\n");
			size = (sizeof(po4010_effect_mono)/sizeof(po4010_effect_mono[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_mono[i].subaddr, po4010_effect_mono[i].value);
			break;
		case 2:
			printk("-> Mode Sepia\n");
			size = (sizeof(po4010_effect_sepia)/sizeof(po4010_effect_sepia[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_sepia[i].subaddr, po4010_effect_sepia[i].value);
			break;
		case 3:
			printk("-> Mode Negative\n");
					size = (sizeof(po4010_effect_negative)/sizeof(po4010_effect_negative[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_negative[i].subaddr, po4010_effect_negative[i].value);
			break;
		case 4:
			printk("-> Mode Aqua\n");
			size = (sizeof(po4010_effect_aqua)/sizeof(po4010_effect_aqua[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_aqua[i].subaddr, po4010_effect_aqua[i].value);
			break;
		case 5:
			printk("-> Mode Green\n");
			size = (sizeof(po4010_effect_green)/sizeof(po4010_effect_green[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_effect_green[i].subaddr, po4010_effect_green[i].value);
			break;
	}

	return 0;
}

static int po4010_sensor_change_br(struct i2c_client *client, int type)
{
	int i, size;

	printk("Brightness Mode \n");

	switch (type) {
		case 0:	
			printk("-> Brightness 1\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 1:
			printk("-> Brightness 2\n");	
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 2:
			printk("-> Brightness 3\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 3:
			printk("-> Brightness 4\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 4:
			printk("-> Brightness 5\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 5:
			printk("-> Brightness 6\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 6:
			printk("-> Brightness 7\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 7:
			printk("-> Brightness 8\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		case 8:
			printk("-> Brightness 9\n");
			size = (sizeof(po4010_br_1)/sizeof(po4010_br_1[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_br_1[i].subaddr, po4010_br_1[i].value);
			break;
		
		default:
			printk("Unknown Brightness!\n");
			break;
	}

	return 0;
}

static int po4010_sensor_change_wb(struct i2c_client *client, int type)
{
	int i, size;
	
	printk("White Balance Mode ");

	switch (type) {
		case 0:
			printk("-> WB auto mode\n");
			size = (sizeof(po4010_wb_auto)/sizeof(po4010_wb_auto[0]));
			for (i = 0; i < size; i++) 
				po4010_sensor_write(client, po4010_wb_auto[i].subaddr, po4010_wb_auto[i].value);
			break;
		case 1:
			printk("-> WB Sunny mode\n");
			size = (sizeof(po4010_wb_daylight)/sizeof(po4010_wb_daylight[0]));
			for (i = 0; i < size; i++)  
				po4010_sensor_write(client, po4010_wb_daylight[i].subaddr, po4010_wb_daylight[i].value);					
			break;
		case 2:
			printk("-> WB Cloudy mode\n");
			size = (sizeof(po4010_wb_cloudy)/sizeof(po4010_wb_cloudy[0]));
			for (i = 0; i < size; i++)  
				po4010_sensor_write(client, po4010_wb_cloudy[i].subaddr, po4010_wb_cloudy[i].value);					
			break;
		case 3:
			printk("-> WB Flourescent mode\n");
			size = (sizeof(po4010_wb_fluorescent)/sizeof(po4010_wb_fluorescent[0]));
			for (i = 0; i < size; i++)  
				po4010_sensor_write(client, po4010_wb_fluorescent[i].subaddr, po4010_wb_fluorescent[i].value);					
			break;
		case 4:
			printk("-> WB Tungsten mode\n");
			size = (sizeof(po4010_wb_incandescent)/sizeof(po4010_wb_incandescent[0]));
			for (i = 0; i < size; i++)  
				po4010_sensor_write(client, po4010_wb_incandescent[i].subaddr, po4010_wb_incandescent[i].value);					
			break;

		default:
			printk("Unknown White Balance!\n");
			break;
	}

	return 0;
}

static int po4010_sensor_user_read(struct i2c_client *client, po4010_t *r_data)
{
	po4010_sensor_write(client, 0x03, r_data->page);
	return po4010_sensor_read(client, r_data->subaddr, &(r_data->value));
}

static int po4010_sensor_user_write(struct i2c_client *client, unsigned char *w_data)
{
	return po4010_sensor_write(client, w_data[0], w_data[1]);
}

static int po4010_sensor_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	struct v4l2_control *ctrl;
	unsigned char *w_data;		/* To support user level i2c */	
	po4010_t *r_data;

	switch (cmd) {
		case SENSOR_INIT:
			sensor_init(client);
			break;

		case USER_ADD:
			break;

		case USER_EXIT:
			po4010_sensor_exit();
			break;

		case SENSOR_EFFECT:
			ctrl = (struct v4l2_control *)arg;
			po4010_sensor_change_effect(client, ctrl->value);
			break;

		case SENSOR_BRIGHTNESS:
			ctrl = (struct v4l2_control *)arg;
			po4010_sensor_change_br(client, ctrl->value);
			break;

		case SENSOR_WB:
			ctrl = (struct v4l2_control *)arg;
			po4010_sensor_change_wb(client, ctrl->value);
			break;

		case SENSOR_AF:
			ctrl = (struct v4l2_control *)arg;
			po4010_sensor_af_control(client, ctrl->value);
			break;

		case SENSOR_XGA:
			po4010_sensor_change_size(client, SENSOR_XGA);	
			break;

		case SENSOR_QSVGA:
			po4010_sensor_change_size(client, SENSOR_QSVGA);
			break;

		case SENSOR_VGA:
			po4010_sensor_change_size(client, SENSOR_VGA);
			break;

		case SENSOR_SVGA:
			po4010_sensor_change_size(client, SENSOR_SVGA);
			break;

		case SENSOR_SXGA:
			po4010_sensor_change_size(client, SENSOR_SXGA);
			break;

		case SENSOR_UXGA:
			po4010_sensor_change_size(client, SENSOR_UXGA);
			break;

		case SENSOR_USER_WRITE:
			w_data = (unsigned char *)arg;
			po4010_sensor_user_write(client, w_data);
			break;

		case SENSOR_USER_READ:
			r_data = (po4010_t *)arg;
			po4010_sensor_user_read(client, r_data);
			break;

		default:
			break;
	}

	return 0;
}

static struct i2c_driver po4010_driver = {
	.driver = {
		.name = "po4010",
	},
	.id = PO4010_ID,
	.attach_adapter = po4010_sensor_attach_adapter,
	.detach_client = po4010_sensor_detach,
	.command = po4010_sensor_command
};

static int po4010_sensor_init(void)
{
	int ret, i = 0;

	po4010_sensor_enable();

	s3c_camif_open_sensor(&po4010_data);
	
retry:	/* Bug Fix 
		   First i2c transfer is failed */
	if (po4010_data.sensor == NULL) {
		if ((ret = i2c_add_driver(&po4010_driver)))
			return ret;
	}	
	
	if (po4010_data.sensor == NULL) {
		i2c_del_driver(&po4010_driver);	
		if (i++ > 3)	/* three times */
			return -ENODEV;
		goto retry;
	}	
	
	s3c_camif_register_sensor(&po4010_data);

	return 0;
}

static void po4010_sensor_exit(void)
{
	po4010_sensor_disable();

	if (po4010_data.sensor != NULL)
		s3c_camif_unregister_sensor(&po4010_data);
}

static struct v4l2_input po4010_input = {
		.index		= 0,
		.name		= "Camera Input (PO4010)",
		.type		= V4L2_INPUT_TYPE_CAMERA,
		.audioset	= 1,
		.tuner		= 0,
		.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
		.status		= 0,
};

static struct v4l2_input_handler po4010_input_handler = {
	po4010_sensor_init,
	po4010_sensor_exit	
};

#ifdef CONFIG_VIDEO_SAMSUNG_MODULE
static int po4010_sensor_add(void)
{
	return s3c_camif_add_sensor(&po4010_input, &po4010_input_handler);
}

static void po4010_sensor_remove(void)
{
	if (po4010_data.sensor != NULL)
		i2c_del_driver(&po4010_driver);
	
	s3c_camif_remove_sensor(&po4010_input, &po4010_input_handler);
}

module_init(po4010_sensor_add)
module_exit(po4010_sensor_remove)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("I2C Client Driver For FIMC V4L2 Driver");
MODULE_LICENSE("GPL");
#else
int po4010_sensor_add(void)
{
	return s3c_camif_add_sensor(&po4010_input, &po4010_input_handler);
}

void po4010_sensor_remove(void)
{
	if (po4010_data.sensor != NULL)
		i2c_del_driver(&po4010_driver);
	
	s3c_camif_remove_sensor(&po4010_input, &po4010_input_handler);
}
#endif
