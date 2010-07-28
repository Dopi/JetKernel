/*
 * Driver for S5KA3DFX (UXGA camera) from Samsung Electronics
 * 
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (C) 2009, Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/s5ka3dfx_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "s5ka3dfx.h"
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/max8998_function.h>

//#define VGA_CAM_DEBUG

#ifdef VGA_CAM_DEBUG
#define dev_dbg	dev_err
#endif

#define S5KA3DFX_DRIVER_NAME	"S5KA3DFX"

/* Default resolution & pixelformat. plz ref s5ka3dfx_platform.h */
#define DEFAULT_RES		WVGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	S5KA3DFX_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10 
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1280 (H) x 1024 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 * Max. pixel clock frequency : 48MHz(upto)
 * Internal PLL (6MHz to 27MHz input frequency)
 */

static int s5ka3dfx_init(struct v4l2_subdev *sd, u32 val);		//for fixing build error	//s1_camera [ Defense process by ESD input ]

/* Camera functional setting values configured by user concept */
struct s5ka3dfx_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct s5ka3dfx_state {
	struct s5ka3dfx_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct s5ka3dfx_userset userset;
	int framesize_index;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
	int vt_mode; /*For VT camera*/
	int check_dataline;
	int check_previewdata;
};

enum {
	S5KA3DFX_PREVIEW_VGA,
} S5KA3DFX_FRAME_SIZE;

struct s5ka3dfx_enum_framesize {
	unsigned int index;
	unsigned int width;
	unsigned int height;	
};

struct s5ka3dfx_enum_framesize s5ka3dfx_framesize_list[] = {
	{S5KA3DFX_PREVIEW_VGA, 640, 480}
};

static inline struct s5ka3dfx_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct s5ka3dfx_state, sd);
}

//s1_camera [ Defense process by ESD input ] _[
static int s5ka3dfx_power_on()
{
	int err;

	printk(KERN_DEBUG "s5ka3dfx_power_on\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}

#if defined CONFIG_ARIES_VER_B1 ||(defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");

		return err;
	}

	// Turn CAM_ISP_SYS_2.8V on
	gpio_direction_output(GPIO_GPB7, 0);
	gpio_set_value(GPIO_GPB7, 1);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_HOST_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_RAM_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);

	mdelay(1);
	
	gpio_free(GPIO_GPB7);	
#elif defined CONFIG_JUPITER_VER_B5 || defined CONFIG_ARIES_VER_B0
	// Turn CAM_ISP_1.2V on
	Set_MAX8998_PM_OUTPUT_Voltage(BUCK4, VCC_1p300);
	Set_MAX8998_PM_REG(EN4, 1);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_ISP_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO15, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO15, 1);

	mdelay(1);

	// Turn CAM_ISP_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);
	
	mdelay(1);

	// Turn CAM_ISP_1.2V off
	Set_MAX8998_PM_REG(EN4, 0);
#else	/* CONFIG_JUPITER_VER_B5 */
	// Turn CAM_A_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO13, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO13, 1);

	mdelay(1);

	// Turn CAM_D_2.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO11, VCC_2p800);
	Set_MAX8998_PM_REG(ELDO11, 1);

	mdelay(1);

	// Turn CAM_IO_1.8V on
	Set_MAX8998_PM_OUTPUT_Voltage(LDO14, VCC_1p800);
	Set_MAX8998_PM_REG(ELDO14, 1);
#endif	/* CONFIG_JUPITER_VER_B5 */

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S5PC11X_GPE1_3_CAM_A_CLKOUT);

	mdelay(1);

	// CAM_VGA_nRST  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);		

	mdelay(4);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);	

	return 0;
}


static int s5ka3dfx_power_off()
{
	int err;

	printk(KERN_DEBUG "s5ka3dfx_power_off\n");

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}


	// CAM_VGA_nRST  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	mdelay(1);

	// CAM_VGA_nSTBY  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

#if defined CONFIG_ARIES_VER_B1 ||(defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");

		return err;
	}

	// Turn CAM_ISP_HOST_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_RAM_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);

	// Turn CAM_ISP_SYS_2.8V off
	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);
	
	gpio_free(GPIO_GPB7);
#elif defined CONFIG_JUPITER_VER_B5 || defined CONFIG_ARIES_VER_B0
	// Turn CAM_ISP_2.8V off
	Set_MAX8998_PM_REG(ELDO15, 0);

	mdelay(1);

	// Turn CAM_SENSOR_A2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_ISP_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);
#else	/* CONFIG_JUPITER_VER_B5 */
	// Turn CAM_D_2.8V off
	Set_MAX8998_PM_REG(ELDO11, 0);

	mdelay(1);

	// Turn CAM_A_2.8V off
	Set_MAX8998_PM_REG(ELDO13, 0);

	// Turn CAM_IO_1.8V off
	Set_MAX8998_PM_REG(ELDO14, 0);
#endif	/* CONFIG_JUPITER_VER_B5 */

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);	

	return 0;
}


static int s5ka3dfx_power_en(int onoff)
{
	if(onoff){
		s5ka3dfx_power_on();
	} else {
		s5ka3dfx_power_off();
	}

	return 0;
}
static int s5ka3dfx_reset(struct v4l2_subdev *sd)
{
	s5ka3dfx_power_en(0);
	mdelay(5);
	s5ka3dfx_power_en(1);
	mdelay(5);
	s5ka3dfx_init(sd, 0);
	return 0;
}
// _]

/*
 * S5KA3DFX register structure : 2bytes address, 2bytes value
 * retry on write failure up-to 5 times
 */
static inline int s5ka3dfx_write(struct v4l2_subdev *sd, u8 addr, u8 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[1];
	unsigned char reg[2];
	int err = 0;
	int retry = 0;


	if (!client->adapter)
		return -ENODEV;

again:
	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = reg;

	reg[0] = addr & 0xff;
	reg[1] = val & 0xff;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return err;	/* Returns here on success */

	/* abnormal case: retry 5 times */
	if (retry < 5) {
		dev_err(&client->dev, "%s: address: 0x%02x%02x, " \
			"value: 0x%02x%02x\n", __func__, \
			reg[0], reg[1], reg[2], reg[3]);
		retry++;
		goto again;
	}

	return err;
}

static int s5ka3dfx_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[],
				unsigned char length)
{
	int ret = -1;
	int retry_count = 1;
	
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = {client->addr, 0, length, buf};

	for (i = 0; i < length; i++) {
		buf[i] = i2c_data[i];
	}
	
#ifdef VGA_CAM_DEBUG
	printk("i2c cmd Length : %d\n", length);
	for (i = 0; i < length; i++) {
		printk("buf[%d] = %x  ", i, buf[i]);
		if(i == length)
			printk("\n");
	}
#endif

	while(retry_count--){
		ret  = i2c_transfer(client->adapter, &msg, 1);
		if(ret == 1)
			break;
		msleep(10);
	}

	return (ret == 1) ? 0 : -EIO;
}

static int s5ka3dfx_write_regs(struct v4l2_subdev *sd, unsigned char regs[], 
				int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;

	for (i = 0; i < size; i++) {
		err = s5ka3dfx_i2c_write(sd, &regs[i], sizeof(regs[i]));
		if (err < 0)
		{
			v4l_info(client, "%s: register set failed\n", \
			__func__);
			
			break;
		}
	}
	if(err < 0)
		return -EIO;	

	return 0;	/* FIXME */
}

#if 0	// temporary delete
static const char *s5ka3dfx_querymenu_wb_preset[] = {
	"WB Tungsten", "WB Fluorescent", "WB sunny", "WB cloudy", NULL
};

static const char *s5ka3dfx_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *s5ka3dfx_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};
#endif

static struct v4l2_queryctrl s5ka3dfx_controls[] = {
#if 0	// temporary delete
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5ka3dfx_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5ka3dfx_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = (ARRAY_SIZE(s5ka3dfx_querymenu_ev_bias_mode) - 2) / 2,	/* 0 EV */
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5ka3dfx_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
#endif	
};

const char **s5ka3dfx_ctrl_get_menu(u32 id)
{
	printk(KERN_DEBUG "s5ka3dfx_ctrl_get_menu is called... id : %d \n", id);

	switch (id) {
#if 0	// temporary delete
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return s5ka3dfx_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return s5ka3dfx_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return s5ka3dfx_querymenu_ev_bias_mode;
#endif
	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5ka3dfx_find_qctrl(int id)
{
	int i;

	printk(KERN_DEBUG "s5ka3dfx_find_qctrl is called...  id : %d \n", id);

	for (i = 0; i < ARRAY_SIZE(s5ka3dfx_controls); i++)
		if (s5ka3dfx_controls[i].id == id)
			return &s5ka3dfx_controls[i];

	return NULL;
}

static int s5ka3dfx_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;

	printk(KERN_DEBUG "s5ka3dfx_queryctrl is called... \n");

	for (i = 0; i < ARRAY_SIZE(s5ka3dfx_controls); i++) {
		if (s5ka3dfx_controls[i].id == qc->id) {
			memcpy(qc, &s5ka3dfx_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int s5ka3dfx_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;

	printk(KERN_DEBUG "s5ka3dfx_querymenu is called... \n");

	qctrl.id = qm->id;
	s5ka3dfx_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5ka3dfx_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int s5ka3dfx_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;

	printk(KERN_DEBUG "s5ka3dfx_s_crystal_freq is called... \n");

	return err;
}

static int s5ka3dfx_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	printk(KERN_DEBUG "s5ka3dfx_g_fmt is called... \n");

	return err;
}

static int s5ka3dfx_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	printk(KERN_DEBUG "s5ka3dfx_s_fmt is called... \n");

	return err;
}
static int s5ka3dfx_enum_framesizes(struct v4l2_subdev *sd, \
					struct v4l2_frmsizeenum *fsize)
{
	struct  s5ka3dfx_state *state = to_state(sd);
	int num_entries = sizeof(s5ka3dfx_framesize_list)/sizeof(struct s5ka3dfx_enum_framesize);	
	struct s5ka3dfx_enum_framesize *elem;	
	int index = 0;
	int i = 0;

	printk(KERN_DEBUG "s5ka3dfx_enum_framesizes is called... \n");

	/* The camera interface should read this value, this is the resolution
 	 * at which the sensor would provide framedata to the camera i/f
 	 *
 	 * In case of image capture, this returns the default camera resolution (WVGA)
 	 */
	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;

	index = state->framesize_index;

	for(i = 0; i < num_entries; i++){
		elem = &s5ka3dfx_framesize_list[i];
		if(elem->index == index){
			fsize->discrete.width = s5ka3dfx_framesize_list[index].width;
			fsize->discrete.height = s5ka3dfx_framesize_list[index].height;
			return 0;
		}
	}

	return -EINVAL;
}


static int s5ka3dfx_enum_frameintervals(struct v4l2_subdev *sd, 
					struct v4l2_frmivalenum *fival)
{
	int err = 0;

	printk(KERN_DEBUG "s5ka3dfx_enum_frameintervals is called... \n");
	
	return err;
}

static int s5ka3dfx_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;

	printk(KERN_DEBUG "s5ka3dfx_enum_fmt is called... \n");

	return err;
}

static int s5ka3dfx_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;

	printk(KERN_DEBUG "s5ka3dfx_enum_fmt is called... \n");

	return err;
}

static int s5ka3dfx_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dev_dbg(&client->dev, "%s\n", __func__);

	return err;
}

static int s5ka3dfx_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;

	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", \
		__func__, param->parm.capture.timeperframe.numerator, \
		param->parm.capture.timeperframe.denominator);

	return err;
}

static int s5ka3dfx_get_framesize_index(struct v4l2_subdev *sd)
{
	int i = 0;
	struct s5ka3dfx_state *state = to_state(sd);
	struct s5ka3dfx_enum_framesize *frmsize;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	/* Check for video/image mode */
	for(i = 0; i < (sizeof(s5ka3dfx_framesize_list)/sizeof(struct s5ka3dfx_enum_framesize)); i++)
	{
		frmsize = &s5ka3dfx_framesize_list[i];
		if(frmsize->width >= state->pix.width && frmsize->height >= state->pix.height){
			return frmsize->index;
		} 
	}
	
	v4l_info(client, "%s: s5ka3dfx_framesize_list[%d].index = %d\n", __func__, i - 1, s5ka3dfx_framesize_list[i].index);
	
	/* FIXME: If it fails, return the last index. */
	return s5ka3dfx_framesize_list[i-1].index;
}

/* This function is called from the s_ctrl api
 * Given the index, it checks if it is a valid index.
 * On success, it returns 0.
 * On Failure, it returns -EINVAL
 */
static int s5ka3dfx_set_framesize_index(struct v4l2_subdev *sd, unsigned int index)
{
	int i = 0;
	struct s5ka3dfx_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	v4l_info(client, "%s: index = %d\n", __func__, index);

	/* Check for video/image mode */
	for(i = 0; i < (sizeof(s5ka3dfx_framesize_list)/sizeof(struct s5ka3dfx_enum_framesize)); i++)
	{
		if(s5ka3dfx_framesize_list[i].index == index){
			state->framesize_index = index; 
			state->pix.width = s5ka3dfx_framesize_list[i].width;
			state->pix.height = s5ka3dfx_framesize_list[i].height;
			return 0;
		} 
	} 
	
	return -EINVAL;
}

/* set sensor register values for adjusting brightness */
static int s5ka3dfx_set_brightness(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	struct s5ka3dfx_userset userset = state->userset;

	int i = 0;
	int err = -EINVAL;
	int ev_value = 0;

	dev_dbg(&client->dev, "%s: value : %d state->vt_mode %d \n", __func__, ctrl->value, state->vt_mode);

	ev_value = ctrl->value;

	printk(KERN_DEBUG "state->vt_mode : %d \n", state->vt_mode);
	if(state->vt_mode == 1)
	{
		switch(ev_value)
		{	
			case EV_MINUS_4:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m4[0], sizeof(s5ka3dfx_ev_vt_m4[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m4[1], sizeof(s5ka3dfx_ev_vt_m4[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m4[2], sizeof(s5ka3dfx_ev_vt_m4[2]));
			break;

			case EV_MINUS_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m3[0], sizeof(s5ka3dfx_ev_vt_m3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m3[1], sizeof(s5ka3dfx_ev_vt_m3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m3[2], sizeof(s5ka3dfx_ev_vt_m3[2]));
			break;

			
			case EV_MINUS_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m2[0], sizeof(s5ka3dfx_ev_vt_m2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m2[1], sizeof(s5ka3dfx_ev_vt_m2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m2[2], sizeof(s5ka3dfx_ev_vt_m2[2]));
			break;
			
			case EV_MINUS_1:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m1[0], sizeof(s5ka3dfx_ev_vt_m1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m1[1], sizeof(s5ka3dfx_ev_vt_m1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_m1[2], sizeof(s5ka3dfx_ev_vt_m1[2]));
			break;

			case EV_DEFAULT:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[0], sizeof(s5ka3dfx_ev_vt_default[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[1], sizeof(s5ka3dfx_ev_vt_default[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[2], sizeof(s5ka3dfx_ev_vt_default[2]));
			break;

			case EV_PLUS_1:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p1[0], sizeof(s5ka3dfx_ev_vt_p1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p1[1], sizeof(s5ka3dfx_ev_vt_p1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p1[2], sizeof(s5ka3dfx_ev_vt_p1[2]));
			break;

			case EV_PLUS_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p2[0], sizeof(s5ka3dfx_ev_vt_p2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p2[1], sizeof(s5ka3dfx_ev_vt_p2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p2[2], sizeof(s5ka3dfx_ev_vt_p2[2]));
			break;

			case EV_PLUS_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p3[0], sizeof(s5ka3dfx_ev_vt_p3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p3[1], sizeof(s5ka3dfx_ev_vt_p3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p3[2], sizeof(s5ka3dfx_ev_vt_p3[2]));
			break;

			case EV_PLUS_4:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p4[0], sizeof(s5ka3dfx_ev_vt_p4[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p4[1], sizeof(s5ka3dfx_ev_vt_p4[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_p4[2], sizeof(s5ka3dfx_ev_vt_p4[2]));
			break;	
			
			default:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[0], sizeof(s5ka3dfx_ev_vt_default[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[1], sizeof(s5ka3dfx_ev_vt_default[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_vt_default[2], sizeof(s5ka3dfx_ev_vt_default[2]));				
			break;
		}
	}
	else
	{
		switch(ev_value)
		{	
			case EV_MINUS_4:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m4[0], sizeof(s5ka3dfx_ev_m4[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m4[1], sizeof(s5ka3dfx_ev_m4[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m4[2], sizeof(s5ka3dfx_ev_m4[2]));
			break;

			case EV_MINUS_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m3[0], sizeof(s5ka3dfx_ev_m3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m3[1], sizeof(s5ka3dfx_ev_m3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m3[2], sizeof(s5ka3dfx_ev_m3[2]));
			break;

			
			case EV_MINUS_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m2[0], sizeof(s5ka3dfx_ev_m2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m2[1], sizeof(s5ka3dfx_ev_m2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m2[2], sizeof(s5ka3dfx_ev_m2[2]));
			break;
			
			case EV_MINUS_1:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m1[0], sizeof(s5ka3dfx_ev_m1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m1[1], sizeof(s5ka3dfx_ev_m1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_m1[2], sizeof(s5ka3dfx_ev_m1[2]));
			break;

			case EV_DEFAULT:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[0], sizeof(s5ka3dfx_ev_default[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[1], sizeof(s5ka3dfx_ev_default[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[2], sizeof(s5ka3dfx_ev_default[2]));
			break;

			case EV_PLUS_1:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p1[0], sizeof(s5ka3dfx_ev_p1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p1[1], sizeof(s5ka3dfx_ev_p1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p1[2], sizeof(s5ka3dfx_ev_p1[2]));
			break;

			case EV_PLUS_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p2[0], sizeof(s5ka3dfx_ev_p2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p2[1], sizeof(s5ka3dfx_ev_p2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p2[2], sizeof(s5ka3dfx_ev_p2[2]));
			break;

			case EV_PLUS_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p3[0], sizeof(s5ka3dfx_ev_p3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p3[1], sizeof(s5ka3dfx_ev_p3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p3[2], sizeof(s5ka3dfx_ev_p3[2]));
			break;

			case EV_PLUS_4:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p4[0], sizeof(s5ka3dfx_ev_p4[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p4[1], sizeof(s5ka3dfx_ev_p4[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_p4[2], sizeof(s5ka3dfx_ev_p4[2]));
			break;	
			
			default:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[0], sizeof(s5ka3dfx_ev_default[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[1], sizeof(s5ka3dfx_ev_default[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_ev_default[2], sizeof(s5ka3dfx_ev_default[2]));				
			break;
		}
	}
	if (err < 0)
	{
		v4l_info(client, "%s: register set failed\n", __func__);
		return -EIO;
	}
	return err;
}

/* set sensor register values for adjusting whitebalance, both auto and manual */
static int s5ka3dfx_set_wb(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s:  value : %d \n", __func__, ctrl->value);

	switch(ctrl->value)
	{
	case WHITE_BALANCE_AUTO:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[0], sizeof(s5ka3dfx_wb_auto[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[1], sizeof(s5ka3dfx_wb_auto[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[2], sizeof(s5ka3dfx_wb_auto[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[3], sizeof(s5ka3dfx_wb_auto[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[4], sizeof(s5ka3dfx_wb_auto[4]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_auto[5], sizeof(s5ka3dfx_wb_auto[5]));			
		break;

	case WHITE_BALANCE_SUNNY:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[0], sizeof(s5ka3dfx_wb_sunny[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[1], sizeof(s5ka3dfx_wb_sunny[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[2], sizeof(s5ka3dfx_wb_sunny[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[3], sizeof(s5ka3dfx_wb_sunny[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[4], sizeof(s5ka3dfx_wb_sunny[4]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_sunny[5], sizeof(s5ka3dfx_wb_sunny[5]));		
		break;

	case WHITE_BALANCE_CLOUDY:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[0], sizeof(s5ka3dfx_wb_cloudy[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[1], sizeof(s5ka3dfx_wb_cloudy[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[2], sizeof(s5ka3dfx_wb_cloudy[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[3], sizeof(s5ka3dfx_wb_cloudy[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[4], sizeof(s5ka3dfx_wb_cloudy[4]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_cloudy[5], sizeof(s5ka3dfx_wb_cloudy[5]));		
		break;

	case WHITE_BALANCE_TUNGSTEN:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[0], sizeof(s5ka3dfx_wb_tungsten[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[1], sizeof(s5ka3dfx_wb_tungsten[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[2], sizeof(s5ka3dfx_wb_tungsten[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[3], sizeof(s5ka3dfx_wb_tungsten[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[4], sizeof(s5ka3dfx_wb_tungsten[4]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_tungsten[5], sizeof(s5ka3dfx_wb_tungsten[5]));		
		break;

	case WHITE_BALANCE_FLUORESCENT:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[0], sizeof(s5ka3dfx_wb_fluorescent[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[1], sizeof(s5ka3dfx_wb_fluorescent[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[2], sizeof(s5ka3dfx_wb_fluorescent[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[3], sizeof(s5ka3dfx_wb_fluorescent[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[4], sizeof(s5ka3dfx_wb_fluorescent[4]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_wb_fluorescent[5], sizeof(s5ka3dfx_wb_fluorescent[5]));		
		break;

	default:
		dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
		err = 0;
		break;

	}
	return err;
}

/* set sensor register values for adjusting color effect */
static int s5ka3dfx_set_effect(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s: value : %d \n", __func__, ctrl->value);

	switch(ctrl->value)
	{
	case IMAGE_EFFECT_NONE:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_none[0], sizeof(s5ka3dfx_effect_none[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_none[1], sizeof(s5ka3dfx_effect_none[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_none[2], sizeof(s5ka3dfx_effect_none[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_none[3], sizeof(s5ka3dfx_effect_none[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_none[4], sizeof(s5ka3dfx_effect_none[4]));		
		break;

	case IMAGE_EFFECT_BNW:		//Gray
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_gray[0], sizeof(s5ka3dfx_effect_gray[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_gray[1], sizeof(s5ka3dfx_effect_gray[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_gray[2], sizeof(s5ka3dfx_effect_gray[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_gray[3], sizeof(s5ka3dfx_effect_gray[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_gray[4], sizeof(s5ka3dfx_effect_gray[4]));
		break;

	case IMAGE_EFFECT_SEPIA:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_sepia[0], sizeof(s5ka3dfx_effect_sepia[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_sepia[1], sizeof(s5ka3dfx_effect_sepia[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_sepia[2], sizeof(s5ka3dfx_effect_sepia[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_sepia[3], sizeof(s5ka3dfx_effect_sepia[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_sepia[4], sizeof(s5ka3dfx_effect_sepia[4]));
		break;

	case IMAGE_EFFECT_AQUA:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_aqua[0], sizeof(s5ka3dfx_effect_aqua[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_aqua[1], sizeof(s5ka3dfx_effect_aqua[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_aqua[2], sizeof(s5ka3dfx_effect_aqua[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_aqua[3], sizeof(s5ka3dfx_effect_aqua[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_aqua[4], sizeof(s5ka3dfx_effect_aqua[4]));
		break;

	case IMAGE_EFFECT_NEGATIVE:
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_negative[0], sizeof(s5ka3dfx_effect_negative[0]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_negative[1], sizeof(s5ka3dfx_effect_negative[1]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_negative[2], sizeof(s5ka3dfx_effect_negative[2]));	
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_negative[3], sizeof(s5ka3dfx_effect_negative[3]));
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_effect_negative[4], sizeof(s5ka3dfx_effect_negative[4]));
		break;

	default:
		dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
		err = 0;
		break;

	}
	
	return err;
}

/* set sensor register values for frame rate(fps) setting */
static int s5ka3dfx_set_frame_rate(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);

	int err = -EINVAL;
	int i = 0;

	dev_dbg(&client->dev, "%s: value : %d \n", __func__, ctrl->value);
	
	printk(KERN_DEBUG "state->vt_mode : %d \n", state->vt_mode);
	if(state->vt_mode == 1)
	{
		switch(ctrl->value)
		{
		case 7:
			for (i = 0; i < S5KA3DFX_VT_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_vt_fps_7[i], sizeof(s5ka3dfx_vt_fps_7[i]));
			}
			break;

		case 10:
			for (i = 0; i < S5KA3DFX_VT_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_vt_fps_10[i], sizeof(s5ka3dfx_vt_fps_10[i]));
			}
			break;
			
		case 15:
			for (i = 0; i < S5KA3DFX_VT_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_vt_fps_15[i], sizeof(s5ka3dfx_vt_fps_15[i]));
			}
			break;

		default:
			dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
			err = 0;
			break;
		}
	}
	else
	{
		switch(ctrl->value)
		{
		case 7:
			for (i = 0; i < S5KA3DFX_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_fps_7[i], sizeof(s5ka3dfx_fps_7[i]));
			}
			break;

		case 10:
			for (i = 0; i < S5KA3DFX_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_fps_10[i], sizeof(s5ka3dfx_fps_10[i]));
			}
			break;
			
		case 15:
			for (i = 0; i < S5KA3DFX_FPS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_fps_15[i], sizeof(s5ka3dfx_fps_15[i]));
			}
			break;

		default:
			dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
			err = 0;
			break;
		}
	}
	return err;
}

/* set sensor register values for adjusting blur effect */
static int s5ka3dfx_set_blur(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s: value : %d \n", __func__, ctrl->value);
	
	printk(KERN_DEBUG "state->vt_mode : %d \n", state->vt_mode);
	if(state->vt_mode == 1)
	{
		switch(ctrl->value)
		{
			case BLUR_LEVEL_0:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_none[0], sizeof(s5ka3dfx_blur_vt_none[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_none[1], sizeof(s5ka3dfx_blur_vt_none[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_none[2], sizeof(s5ka3dfx_blur_vt_none[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_none[3], sizeof(s5ka3dfx_blur_vt_none[3]));
				break;

			case BLUR_LEVEL_1:	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p1[0], sizeof(s5ka3dfx_blur_vt_p1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p1[1], sizeof(s5ka3dfx_blur_vt_p1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p1[2], sizeof(s5ka3dfx_blur_vt_p1[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p1[3], sizeof(s5ka3dfx_blur_vt_p1[3]));
				break;

			case BLUR_LEVEL_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p2[0], sizeof(s5ka3dfx_blur_vt_p2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p2[1], sizeof(s5ka3dfx_blur_vt_p2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p2[2], sizeof(s5ka3dfx_blur_vt_p2[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p2[3], sizeof(s5ka3dfx_blur_vt_p2[3]));
				break;

			case BLUR_LEVEL_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p3[0], sizeof(s5ka3dfx_blur_vt_p3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p3[1], sizeof(s5ka3dfx_blur_vt_p3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p3[2], sizeof(s5ka3dfx_blur_vt_p3[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_vt_p3[3], sizeof(s5ka3dfx_blur_vt_p3[3]));
				break;

			default:
				dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
				err = 0;
				break;

		}
	}
	else
	{
		switch(ctrl->value)
		{
			case BLUR_LEVEL_0:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_none[0], sizeof(s5ka3dfx_blur_none[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_none[1], sizeof(s5ka3dfx_blur_none[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_none[2], sizeof(s5ka3dfx_blur_none[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_none[3], sizeof(s5ka3dfx_blur_none[3]));
				break;

			case BLUR_LEVEL_1:	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p1[0], sizeof(s5ka3dfx_blur_p1[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p1[1], sizeof(s5ka3dfx_blur_p1[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p1[2], sizeof(s5ka3dfx_blur_p1[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p1[3], sizeof(s5ka3dfx_blur_p1[3]));
				break;

			case BLUR_LEVEL_2:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p2[0], sizeof(s5ka3dfx_blur_p2[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p2[1], sizeof(s5ka3dfx_blur_p2[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p2[2], sizeof(s5ka3dfx_blur_p2[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p2[3], sizeof(s5ka3dfx_blur_p2[3]));
				break;

			case BLUR_LEVEL_3:
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p3[0], sizeof(s5ka3dfx_blur_p3[0]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p3[1], sizeof(s5ka3dfx_blur_p3[1]));
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p3[2], sizeof(s5ka3dfx_blur_p3[2]));	
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_blur_p3[3], sizeof(s5ka3dfx_blur_p3[3]));
				break;

			default:
				dev_dbg(&client->dev, "%s: Not Support value \n", __func__);
				err = 0;
				break;
		}		
	}
	return err;
}

static int s5ka3dfx_check_dataline_stop(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	int err = -EINVAL, i;

	dev_dbg(&client->dev, "%s\n", __func__);

	for (i = 0; i < 2; i++) {
		err = s5ka3dfx_i2c_write(sd, s5ka3dfx_dataline_stop[i], \
					sizeof(s5ka3dfx_dataline_stop[i]));
		if (err < 0)
		{
			v4l_info(client, "%s: register set failed\n", __func__);
			return -EIO;
		}
	}
	state->check_dataline = 0;
	err = s5ka3dfx_reset(sd);
	if (err < 0)
	{
		v4l_info(client, "%s: register set failed\n", __func__);
		return -EIO;
	}
	return err;
}

/* if you need, add below some functions below */

static int s5ka3dfx_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	struct s5ka3dfx_userset userset = state->userset;
	int err = -EINVAL;

	dev_dbg(&client->dev, "%s: id : 0x%08lx \n", __func__, ctrl->id);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;

	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;

	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;

	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;

#if 0
	case V4L2_CID_CAM_FRAMESIZE_INDEX:
		ctrl->value = s5ka3dfx_get_framesize_index(sd);
		err = 0;
		break;
#endif

	default:
		dev_dbg(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}
	
	return err;
}

static int s5ka3dfx_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef S5KA3DFX_COMPLETE
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);

	int err = -EINVAL;

	printk(KERN_DEBUG "s5ka3dfx_s_ctrl() : ctrl->id 0x%08lx, ctrl->value %d \n",ctrl->id, ctrl->value);

	switch (ctrl->id) {

#if 0		
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", __func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_ev_bias[ctrl->value], \
			sizeof(s5ka3dfx_regs_ev_bias[ctrl->value]));
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", \
			__func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_awb_enable[ctrl->value], \
			sizeof(s5ka3dfx_regs_awb_enable[ctrl->value]));
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_WHITE_BALANCE_PRESET\n", \
			__func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_wb_preset[ctrl->value], \
			sizeof(s5ka3dfx_regs_wb_preset[ctrl->value]));
		break;

	case V4L2_CID_COLORFX:
		dev_dbg(&client->dev, "%s: V4L2_CID_COLORFX\n", __func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_color_effect[ctrl->value], \
			sizeof(s5ka3dfx_regs_color_effect[ctrl->value]));
		break;

	case V4L2_CID_CONTRAST:
		dev_dbg(&client->dev, "%s: V4L2_CID_CONTRAST\n", __func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_contrast_bias[ctrl->value], \
			sizeof(s5ka3dfx_regs_contrast_bias[ctrl->value]));
		break;

	case V4L2_CID_SATURATION:
		dev_dbg(&client->dev, "%s: V4L2_CID_SATURATION\n", __func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_saturation_bias[ctrl->value], \
			sizeof(s5ka3dfx_regs_saturation_bias[ctrl->value]));
		break;

	case V4L2_CID_SHARPNESS:
		dev_dbg(&client->dev, "%s: V4L2_CID_SHARPNESS\n", __func__);
		err = s5ka3dfx_write_regs(sd, \
			(unsigned char *) s5ka3dfx_regs_sharpness_bias[ctrl->value], \
			sizeof(s5ka3dfx_regs_sharpness_bias[ctrl->value]));
		break;	

	/* The camif supports only a few frame resolutions. 
 	 * Through this call, camif can set the camera resolution with given index.
 	 * Typically, camif gets the index through g_ctrl call with this ID.
 	 */

 	case V4L2_CID_CAM_FRAMESIZE_INDEX:
		err = s5ka3dfx_set_framesize_index(sd, ctrl->value);
        break;
#endif

	case V4L2_CID_CAMERA_BRIGHTNESS:	//V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_BRIGHTNESS\n", __func__);
		err = s5ka3dfx_set_brightness(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE: //V4L2_CID_AUTO_WHITE_BALANCE:
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", __func__);
		err = s5ka3dfx_set_wb(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_EFFECT:	//V4L2_CID_COLORFX:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_EFFECT\n", __func__);
		err = s5ka3dfx_set_effect(sd, ctrl);
		break;

	case V4L2_CID_CAMERA_FRAME_RATE:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_FRAME_RATE\n", __func__);
		err = s5ka3dfx_set_frame_rate(sd, ctrl);	
		break;
		
	case V4L2_CID_CAMERA_VGA_BLUR:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_FRAME_RATE\n", __func__);
		err = s5ka3dfx_set_blur(sd, ctrl);	
		break;

	case V4L2_CID_CAMERA_VT_MODE:
		state->vt_mode = ctrl->value;
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_VT_MODE : state->vt_mode %d \n", __func__, state->vt_mode);
		err = 0;
		break;

	case V4L2_CID_CAMERA_CHECK_DATALINE:
		state->check_dataline = ctrl->value;
		err = 0;
		break;	

	case V4L2_CID_CAMERA_CHECK_DATALINE_STOP:
		err = s5ka3dfx_check_dataline_stop(sd);
		break;

	case V4L2_CID_CAM_PREVIEW_ONOFF:
		if(state->check_previewdata == 0)
		{
			err = 0;
		}
		else
		{
			err = -EIO;	
		}
		break;

	//s1_camera [ Defense process by ESD input ] _[
	case V4L2_CID_CAMERA_RESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_RESET \n", __func__);
		err = s5ka3dfx_reset(sd);
		break;
	// _]

	default:
		dev_dbg(&client->dev, "%s: no support control in camera sensor, S5KA3DFX\n", __func__);
		//err = -ENOIOCTLCMD;
		err = 0;
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);
	return err;
#else
	return 0;
#endif
}

static int s5ka3dfx_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	int err = -EINVAL, i;

	//v4l_info(client, "%s: camera initialization start : state->vt_mode %d \n", __func__, state->vt_mode);
	printk(KERN_DEBUG "camera initialization start, state->vt_mode : %d \n", state->vt_mode); 
	printk(KERN_DEBUG "state->check_dataline : %d \n", state->check_dataline); 
	if(state->vt_mode == 0)
	{
		if(state->check_dataline)
		{	
			for (i = 0; i < S5KA3DFX_DATALINE_REGS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_dataline[i], \
							sizeof(s5ka3dfx_dataline[i]));
				if (err < 0)
				{
					v4l_info(client, "%s: register set failed\n", \
					__func__);
					
					break;
				}	
			}
		}
		else
		{
			for (i = 0; i < S5KA3DFX_INIT_REGS; i++) {
				err = s5ka3dfx_i2c_write(sd, s5ka3dfx_init_reg[i], \
							sizeof(s5ka3dfx_init_reg[i]));
				if (err < 0)
				{
					v4l_info(client, "%s: register set failed\n", \
					__func__);
					
					break;
				}	
			}
		}
	}
	else
	{
		for (i = 0; i < S5KA3DFX_INIT_VT_REGS; i++) {
			err = s5ka3dfx_i2c_write(sd, s5ka3dfx_init_vt_reg[i], \
						sizeof(s5ka3dfx_init_vt_reg[i]));
			if (err < 0)
			{
				v4l_info(client, "%s: register set failed\n", \
				__func__);
				
				break;
			}	
		}
	}

	if (err < 0) {
		//This is preview fail 
		state->check_previewdata = 100;
		v4l_err(client, "%s: camera initialization failed. err(%d)\n", \
			__func__, state->check_previewdata);
		return -EIO;	/* FIXME */	
	}

	//This is preview success
	state->check_previewdata = 0;
	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time therefor,
 * it is not necessary to be initialized on probe time. except for version checking
 * NOTE: version checking is optional
 */
static int s5ka3dfx_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5ka3dfx_state *state = to_state(sd);
	struct s5ka3dfx_platform_data *pdata;

	dev_dbg(&client->dev, "fetching platform data\n");

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_dbg(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

static const struct v4l2_subdev_core_ops s5ka3dfx_core_ops = {
	.init = s5ka3dfx_init,	/* initializing API */
	.s_config = s5ka3dfx_s_config,	/* Fetch platform data */
	.queryctrl = s5ka3dfx_queryctrl,
	.querymenu = s5ka3dfx_querymenu,
	.g_ctrl = s5ka3dfx_g_ctrl,
	.s_ctrl = s5ka3dfx_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5ka3dfx_video_ops = {
	.s_crystal_freq = s5ka3dfx_s_crystal_freq,
	.g_fmt = s5ka3dfx_g_fmt,
	.s_fmt = s5ka3dfx_s_fmt,
	.enum_framesizes = s5ka3dfx_enum_framesizes,
	.enum_frameintervals = s5ka3dfx_enum_frameintervals,
	.enum_fmt = s5ka3dfx_enum_fmt,
	.try_fmt = s5ka3dfx_try_fmt,
	.g_parm = s5ka3dfx_g_parm,
	.s_parm = s5ka3dfx_s_parm,
};

static const struct v4l2_subdev_ops s5ka3dfx_ops = {
	.core = &s5ka3dfx_core_ops,
	.video = &s5ka3dfx_video_ops,
};

/*
 * s5ka3dfx_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5ka3dfx_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct s5ka3dfx_state *state;
	struct v4l2_subdev *sd;

	state = kzalloc(sizeof(struct s5ka3dfx_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, S5KA3DFX_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5ka3dfx_ops);

	dev_dbg(&client->dev, "s5ka3dfx has been probed\n");
	return 0;
}


static int s5ka3dfx_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id s5ka3dfx_id[] = {
	{ S5KA3DFX_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5ka3dfx_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = S5KA3DFX_DRIVER_NAME,
	.probe = s5ka3dfx_probe,
	.remove = s5ka3dfx_remove,
	.id_table = s5ka3dfx_id,
};

MODULE_DESCRIPTION("Samsung Electronics S5KA3DFX UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");

