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
#include <plat/egpio.h>

#include "../s3c_camif.h"

#include "s5k4ca.h"

// function define
//#define CONFIG_LOAD_FILE
#define I2C_BURST_MODE //dha23 100325 카메라 기동 시간 줄이기 위해 I2C Burst mode 사용.
// Purpose of verifying I2C operaion. must be ignored later.
//#define LOCAL_CONFIG_S5K4CA_I2C_TEST
#define CONFIG_FLASH_AAT1271A //dha23 101004

static struct i2c_driver s5k4ca_driver;

int cam_flash_on = 0; //dha23 101004
int locked_ae_awb = 0; //dha23 101004

static void s5k4ca_sensor_gpio_init(void);
void s5k4ca_sensor_enable(void);
static void s5k4ca_sensor_disable(void);

static int s5k4ca_sensor_init(void);
static void s5k4ca_sensor_exit(void);

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size);


#ifdef CONFIG_FLASH_AAT1271A
	extern int aat1271a_flash_init(void);
	extern void aat1271a_flash_exit(void);
	extern void aat1271a_flash_camera_control(int ctrl);
	extern void aat1271a_flash_movie_control(int ctrl);
	extern void aat1271a_flash_torch_camera_control(int ctrl); //hjkang_DE28	
#endif

#ifdef CONFIG_LOAD_FILE
	static int s5k4ca_regs_table_write(char *name);
#endif

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
static camif_cis_t s5k4ca_data = {
	itu_fmt:       	CAMIF_ITU601,
	order422:      	CAMIF_CRYCBY,
	camclk:        	24000000,		
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

/* #define S5K4CA_ID	0x78 */
enum {
	SCENE_MODE_BASE = -1,
	SCENE_MODE_AUTO,
	SCENE_MODE_PORTRAIT,
	SCENE_MODE_LANDSCAPE,
	SCENE_MODE_NIGHT,
	SCENE_MODE_BEACH,
	SCENE_MODE_SNOW,
	SCENE_MODE_SUNSET,
	SCENE_MODE_FIREWORKS,
	SCENE_MODE_MAX,
};

static int previous_scene_mode = SCENE_MODE_BASE;
static int previous_WB_mode = 0;
static int af_mode = -1;
static unsigned short lux_value = 0;

static unsigned short AFPosition = 0x00FF; 
static unsigned short DummyAFPosition = 0x00FE; 

static int s5k4ca_sensor_read(struct i2c_client *client,
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

static int s5k4ca_sensor_write(struct i2c_client *client,unsigned short subaddr, unsigned short val)
{
	if(subaddr == 0xdddd)
	{
/*	
			if (val == 0x0010)
				msleep(10);
			else if (val == 0x0020)
				msleep(20);
			else if (val == 0x0030)
				msleep(30);
			else if (val == 0x0040)
				msleep(40);
			else if (val == 0x0050)
				msleep(50);
			else if (val == 0x0100)
*/			
			msleep(val);
			printk("delay time(%d msec)\n", val);
	}	
	else
	{					
		unsigned char buf[4];
		struct i2c_msg msg = { client->addr, 0, 4, buf };

		buf[0] = (subaddr >> 8);
		buf[1] = (subaddr & 0xFF);
		buf[2] = (val >> 8);
		buf[3] = (val & 0xFF);

		return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	}
	return 0;
}

static int s5k4ca_sensor_write_list(struct i2c_client *client, struct samsung_short_t *list,char *name)
{
	int i, ret;
	ret = 0;

	printk("s5k4ca_sensor_write_list( %s ) \n", name); 
#ifdef CONFIG_LOAD_FILE 
	s5k4ca_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
		if(s5k4ca_sensor_write(client, list[i].subaddr, list[i].value) < 0)
			return -1;
	}
#endif
	return ret;
}

#ifdef I2C_BURST_MODE //dha23 100325

#define BURST_MODE_SET			1
#define BURST_MODE_END			2
#define NORMAL_MODE_SET			3
#define MAX_INDEX				1000
static int s5k4ca_sensor_burst_write_list(struct i2c_client *client, struct samsung_short_t *list,char *name)
{
	__u8 temp_buf[MAX_INDEX];
	int index_overflow = 1;
	int new_addr_start = 0;
	int burst_mode = NORMAL_MODE_SET;
	unsigned short pre_subaddr = 0;
	struct i2c_msg msg = { client->addr, 0, 4, temp_buf };
	int i=0, ret=0;
	unsigned int index = 0;
	
	printk("s5k4ca_sensor_burst_write_list( %s ) \n", name); 
#ifdef CONFIG_LOAD_FILE 
	s5k4ca_regs_table_write(name);	
#else
	for (i = 0; list[i].subaddr != 0xffff; i++)
	{
		if(list[i].subaddr == 0xdddd)
		{
		    /*
			if (list[i].value == 0x0010)
				msleep(10);
			else if (list[i].value == 0x0020)
				msleep(20);
			else if (list[i].value == 0x0030)
				msleep(30);
			else if (list[i].value == 0x0040)
				msleep(40);
			else if (list[i].value == 0x0050)
				msleep(50);
			else if (list[i].value == 0x0100)
				msleep(100);
		    */
		    msleep(list[i].value);
			printk("delay 0x%04x, value 0x%04x\n", list[i].subaddr, list[i].value);
		}	
		else
		{					
			if( list[i].subaddr == list[i+1].subaddr )
			{
				burst_mode = BURST_MODE_SET;
				if((list[i].subaddr != pre_subaddr) || (index_overflow == 1))
				{
					new_addr_start = 1;
					index_overflow = 0;
				}
			}
			else
			{
				if(burst_mode == BURST_MODE_SET)
				{
					burst_mode = BURST_MODE_END;
					if(index_overflow == 1)
					{
						new_addr_start = 1;
						index_overflow = 0;
					}
				}
				else
				{
					burst_mode = NORMAL_MODE_SET;
				}
			}

			if((burst_mode == BURST_MODE_SET) || (burst_mode == BURST_MODE_END))
			{
				if(new_addr_start == 1)
				{
					index = 0;
					memset(temp_buf, 0x00 ,1000);
					index_overflow = 0;

					temp_buf[index] = (list[i].subaddr >> 8);
					temp_buf[++index] = (list[i].subaddr & 0xFF);

					new_addr_start = 0;
				}
				
				temp_buf[++index] = (list[i].value >> 8);
				temp_buf[++index] = (list[i].value & 0xFF);
				
				if(burst_mode == BURST_MODE_END)
				{
					msg.len = ++index;

					ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("i2c_transfer fail ! \n");
						return -1;
					}
				}
				else if( index >= MAX_INDEX-1 )
				{
					index_overflow = 1;
					msg.len = ++index;
					
					ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
					if( ret < 0)
					{
						printk("I2C_transfer Fail ! \n");
						return -1;
					}
				}
				
			}
			else
			{
				memset(temp_buf, 0x00 ,4);
			
				temp_buf[0] = (list[i].subaddr >> 8);
				temp_buf[1] = (list[i].subaddr & 0xFF);
				temp_buf[2] = (list[i].value >> 8);
				temp_buf[3] = (list[i].value & 0xFF);

				msg.len = 4;
				ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
				if( ret < 0)
				{
					printk("I2C_transfer Fail ! \n");
					return -1;
				}
			}
		}
		
		pre_subaddr = list[i].subaddr;
	}
#endif
	return ret;
}

#endif


static int s5k4ca_sensor_ae_awb_lock(struct i2c_client *client)
{
	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
	s5k4ca_sensor_write(client, 0x0028, 0x7000);
	s5k4ca_sensor_write(client, 0x002A, 0x0578);
	s5k4ca_sensor_write(client, 0x0F12, 0x0075);

	locked_ae_awb = 1; //dha23 101004
	return 0;
}

static int s5k4ca_sensor_ae_awb_unlock(struct i2c_client *client)
{
	if(locked_ae_awb == 1) //dha23 101004
	{
		if(previous_WB_mode == 0 && previous_scene_mode != SCENE_MODE_SUNSET) //dha23 101022
		{
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
			s5k4ca_sensor_write(client, 0x0028, 0x7000);
			s5k4ca_sensor_write(client, 0x002A, 0x0578);
			s5k4ca_sensor_write(client, 0x0F12, 0x007F);
		}
		else
		{
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
			s5k4ca_sensor_write(client, 0x0028, 0x7000);
			s5k4ca_sensor_write(client, 0x002A, 0x0578);
			s5k4ca_sensor_write(client, 0x0F12, 0x0077);
		}
		
		locked_ae_awb = 0; //dha23 101004
	}
	return 0;
}

static void s5k4ca_sensor_get_id(struct i2c_client *client)
{
	unsigned short id = 0;
	
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x01FA);
	s5k4ca_sensor_read(client, 0x0F12, &id);

	printk("Sensor ID(0x%04x) is %s!\n", id, (id == 0x4CA4) ? "Valid" : "Invalid"); 
}

static void s5k4ca_sensor_gpio_init(void)
{
	I2C_CAM_DIS;
	MCAM_RST_DIS;
	VCAM_RST_DIS;
	CAM_PWR_DIS;
	AF_PWR_DIS;
	MCAM_STB_DIS;
	VCAM_STB_DIS;
}

#if defined(CONFIG_LDO_LP8720)
extern void	s5k4ca_sensor_power_init(void);	
#endif

void s5k4ca_sensor_enable(void)
{
	s5k4ca_sensor_gpio_init();

	MCAM_STB_EN;

	/* > 0 ms */
	msleep(1);

	AF_PWR_EN;	

#if defined(CONFIG_LDO_LP8720)
	s5k4ca_sensor_power_init();	
#endif

	CAM_PWR_EN;

	/* > 0 ms */
	msleep(1);

	/* MCLK Set */
	clk_set_rate(cam_clock, s5k4ca_data.camclk);

	/* MCLK Enable */
	clk_enable(cam_clock);
	clk_enable(cam_hclk);
	
	msleep(1);

	MCAM_RST_EN;
	
	msleep(40);
	
	I2C_CAM_EN;
}

static void s5k4ca_sensor_disable(void)
{
	I2C_CAM_DIS;
	
	MCAM_STB_DIS;

	/* > 20 cycles */
	msleep(1);

	/* MCLK Disable */
	clk_disable(cam_clock);
	clk_disable(cam_hclk);

	/* > 0 ms */
	msleep(1);

	MCAM_RST_DIS;

	/* > 0 ms */
	msleep(1);

	AF_PWR_DIS;

	CAM_PWR_DIS;
}

static int sensor_init(struct i2c_client *client)
{
	int ret = 0;

#ifdef I2C_BURST_MODE //dha23 100325	
	if(s5k4ca_sensor_burst_write_list(client,s5k4ca_init0_04,"s5k4ca_init0_04") < 0)
		return -1;
#else
	if(s5k4ca_sensor_write_list(client,s5k4ca_init0_04,"s5k4ca_init0_04") < 0)
		return -1;
#endif
	msleep(10);	
	af_mode = -1;

	/* Check Sensor ID */
	s5k4ca_sensor_get_id(client);
#ifdef I2C_BURST_MODE //dha23 100325	
	ret = s5k4ca_sensor_burst_write_list(client,s5k4ca_init1_04,"s5k4ca_init1_04");
#else
	ret = s5k4ca_sensor_write_list(client,s5k4ca_init1_04,"s5k4ca_init1_04");
#endif
	
	//s5k4ca_sensor_change_size(client, SENSOR_XGA);
	return 0;
}

static int s5k4ca_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
//	return i2c_probe(adap, &s5k4ca_addr_data, s5k4cagx_attach);
	struct i2c_client *c;
	c = client;
	s5k4ca_data.sensor = c;
	i2c_set_clientdata(client, c);
	return 0;
}

static int s5k4ca_remove(struct i2c_client *client)
{
	//i2c_detach_client(client);
	return 0;
}

static int s5k4ca_sensor_mode_set(struct i2c_client *client, int type)
{
	unsigned short light;
	int delay = 0;

	printk("Sensor Mode ");

	if (type & SENSOR_PREVIEW)
	{	
		printk("-> Preview ");
#ifdef I2C_BURST_MODE //dha23 100325				
		s5k4ca_sensor_burst_write_list(client,s5k4ca_preview_04,"s5k4ca_preview_04");
#else
		s5k4ca_sensor_write_list(client,s5k4ca_preview_04,"s5k4ca_preview_04");
#endif
	}

	else if (type & SENSOR_CAPTURE)
	{	
		printk("-> Capture ");

		s5k4ca_sensor_ae_awb_unlock(client);

		s5k4ca_sensor_write(client, 0x002C, 0x7000);	
		s5k4ca_sensor_write(client, 0x002E, 0x12FE);
		s5k4ca_sensor_read(client, 0x0F12, &light);
		lux_value = light;

		if (previous_scene_mode == SCENE_MODE_FIREWORKS) //dha23 101022 /* fireworks use own capture routine */
		{
			printk("Snapshot : firework capture\n");
			s5k4ca_sensor_write_list(client,s5k4ca_snapshot_fireworks_04,"s5k4ca_snapshot_fireworks_04");
		}
		else
		{
			if (light <= 0x20) /* Low light */
			{	
				printk("Snapshot : Low Light\n");

				if(previous_scene_mode == SCENE_MODE_NIGHT) //dha23 101022
				{
					printk("Snapshot : Night Mode\n");

					s5k4ca_sensor_write_list(client,s5k4ca_snapshot_nightmode_04,"s5k4ca_snapshot_nightmode_04");
				}
				else
				{
					printk("Snapshot : Normal mode \n");
					delay = 1000; //dha23 101128
					s5k4ca_sensor_write_list(client,s5k4ca_snapshot_normal_low_04,"s5k4ca_snapshot_normal_low_04");
				}
			}
			else
			{
				printk("Snapshot : Normal Light\n");
				delay = 150; //dha23 101128
				s5k4ca_sensor_write_list(client,s5k4ca_capture_04,"s5k4ca_capture_04");
			}
		}
	}
	else if (type & SENSOR_FLASH_CAP_LOW)
	{	
		printk("-> Flash Low Light Capture\n");
		delay = 300;

		s5k4ca_sensor_write_list(client,s5k4ca_flashcapture_low_04,"s5k4ca_flashcapture_low_04");
		printk("SENSOR_FLASH_CAP_LOW :: delay time(%d msec)\n", delay);
	}
	else if (type & SENSOR_FLASH_CAPTURE)
	{	
		printk("-> Flash Normal Light Capture\n");
		delay = 300;

		s5k4ca_sensor_write_list(client,s5k4ca_flashcapture_04,"s5k4ca_flashcapture_04");
		printk("SENSOR_FLASH_CAPTURE :: delay time(%d msec)\n", delay);
	}
	else if (type & SENSOR_CAMCORDER )
	{
		printk("-> Record\n");
					
		s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
		s5k4ca_sensor_write(client, 0x0028, 0x7000); 

		s5k4ca_sensor_write(client, 0x002A, 0x030E);  
		s5k4ca_sensor_write(client, 0x0F12, 0x00DF);  //030E = 00FF 입력위해 다른값 임시입력

		s5k4ca_sensor_write(client, 0x002A, 0x030C); 
		s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

		msleep(130); //AF Manual 명령 인식위한 1frame delay (저조도 7.5fps 133ms 고려)
		//여기까지 lens 움직임 없음
		s5k4ca_sensor_write(client, 0x002A, 0x030E);  
		s5k4ca_sensor_write(client, 0x0F12, 0x00E0);  // 030E = 00FF 입력. lens 움직임 

		msleep(50);  //lens가 목표지점까지 도달하기 위해 필요한 delay
        
		delay = 300;
		s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix_04,"s5k4ca_fps_15fix_04");
		printk("SENSOR_CAMCORDER :: delay time(%d msec)\n", delay);
	}

	msleep(delay);

	return 0;
	
}

static int s5k4ca_sensor_change_size(struct i2c_client *client, int size)
{
	switch (size) {
		case SENSOR_XGA:
			s5k4ca_sensor_mode_set(client, SENSOR_PREVIEW);
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_mode_set(client, SENSOR_CAPTURE);
			break;		
	
		default:
			printk("Unknown Size! (Only XGA & QXGA)\n");
			break;
	}

	return 0;
}

static int s5k4ca_sensor_af_control(struct i2c_client *client, int type)
{

    int count = 50;
    int tmpVal = 0;
    int ret = 0;

    switch (type)
    {
        case 0: // release
            printk("[CAM-SENSOR] Focus Mode -> release\n"); 
			
            s5k4ca_sensor_ae_awb_unlock(client); // unlock AWB/AE

            //af move to initial position.

            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
            s5k4ca_sensor_write(client, 0x0028, 0x7000);

            s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // dummy lens position
			
            s5k4ca_sensor_write(client, 0x002A, 0x030C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF manual
            
            msleep(130);

            s5k4ca_sensor_write(client, 0x002A, 0x030E);
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position

            msleep(50);

            break;

        case 1: // AF start
			printk("Focus Mode -> Single\n");

            if( cam_flash_on ) //dha23 101004
            {
                printk("AE/AWB is not locked for flash!!! \n");
            }
            else
            {
                s5k4ca_sensor_ae_awb_lock(client); // lock AWB/AE
            }

            s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
            s5k4ca_sensor_write(client, 0x0028, 0x7000); 

#if 0 // move to af success case
            s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

            msleep(130); //AF Manual 명령 인식위한 1frame delay (저조도 7.5fps 133ms 고려)
                         //여기까지 lens 움직임 없음
#endif                         
           
            //s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            //s5k4ca_sensor_write(client, 0x0F12, 0x0003); // AF Freeze 
            //msleep(50);  

            //AF freeze 를 하면 AF power off를 하게 되어 약간의 power 소모 개선이 있습니다.
            //필요에따라 사용하시기 바랍니다.

#if 0 // move lens to searching position (??)
            s5k4ca_sensor_write(client, 0x002A, 0x030E); 
            s5k4ca_sensor_write(client, 0x0F12, AFPosition);
#endif 

            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0002); // AF Single 
            
            msleep(260); // delay 2 frames before af status check
            
            do // remove low light check
            {
                if( count == 0)
                    break;

                s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
                s5k4ca_sensor_write(client, 0x002C, 0x7000);    
                s5k4ca_sensor_write(client, 0x002E, 0x130E);
                msleep(100);
                s5k4ca_sensor_read(client, 0x0F12, (unsigned short *)(&tmpVal)); 

                count--;

                printk("CAM 3M AF Status Value = %x \n", tmpVal); 

            }
            while( (tmpVal & 0x3) != 0x3 && (tmpVal & 0x3) != 0x2 );

            if(count == 0) // af timeout : move lens to initial position
            {
                s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
                s5k4ca_sensor_write(client, 0x0028, 0x7000); 
                
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);  //030E = 00FF 입력위해 다른값 임시입력
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); 
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 
                
                msleep(130); //AF Manual 명령 인식위한 1frame delay (저조도 7.5fps 133ms 고려)
                             //여기까지 lens 움직임 없음
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, AFPosition);  // 030E = 00FF 입력. lens 움직임 
                
                msleep(50);  //lens가 목표지점까지 도달하기 위해 필요한 delay
                	
                ret = 0;
                printk("CAM 3M AF_Single Mode Fail.==> TIMEOUT \n");
                
            }

            if((tmpVal & 0x3) == 0x02) // af fail : move lens to initial position
            {
                s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
                s5k4ca_sensor_write(client, 0x0028, 0x7000); 
                
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);  //030E = 00FF 입력위해 다른값 임시입력
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); 
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 
                
                msleep(130); //AF Manual 명령 인식위한 1frame delay (저조도 7.5fps 133ms 고려)
                             //여기까지 lens 움직임 없음
                s5k4ca_sensor_write(client, 0x002A, 0x030E);  
                s5k4ca_sensor_write(client, 0x0F12, AFPosition);  // 030E = 00FF 입력. lens 움직임 
                
                msleep(50);  //lens가 목표지점까지 도달하기 위해 필요한 delay
                
                ret = 0;
                
                printk("CAM 3M AF_Single Mode Fail.==> FAIL \n");
            }

            if((tmpVal & 0x3) == 0x3) // af success
            {
                ret = 1;
                
                s5k4ca_sensor_write(client, 0x002A, 0x030C); // set to Manual for next snapshot and Single AF
                s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual     
                
                printk("CAM 3M AF_Single Mode SUCCESS. \r\n");
            }
            
            printk("CAM:3M AF_SINGLE SET \r\n");
            break;
            
        case 2: // auto
            printk("[CAM-SENSOR] =Focus Mode -> auto\n");
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x161C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x82A8); // Set Normal AF Mode
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // dummy lens position
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);    
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50); // Lens가 초기 위치로 돌아가는 이동시간.           
            
            af_mode = 2;
            break;

        case 3: // infinity
            printk("[CAM-SENSOR] =Focus Mode -> infinity\n");
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 3;
            break;

        case 4: // macro
            printk("[CAM-SENSOR] =Focus Mode -> Macro\n");
            
            DummyAFPosition = 0x0048;
            AFPosition = 0x0040;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x161C);
            s5k4ca_sensor_write(client, 0x0F12, 0xA2A8);  // Set Macro AF mode
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);	
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // set dummy position
            
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);	
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);	
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 4;
            break;

        case 5: // fixed, same as infinity
            printk("[CAM-SENSOR] =Focus Mode -> fixed\n");	
            
            DummyAFPosition = 0x00FE;
            AFPosition = 0x00FF;
            
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, DummyAFPosition); // set dummy position
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);    
            s5k4ca_sensor_write(client, 0x0F12, AFPosition); // move lens to initial position
            
            msleep(50);
            
            af_mode = 5;
            break;
            
        default:
            break;
    }
               
    return ret;
}

static int s5k4ca_sensor_change_effect(struct i2c_client *client, int type)
{
	
	printk("Effects Mode ");

	switch (type)
	{
		case 0:
			printk("-> Mode None\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_off_04,"s5k4ca_effect_off_04");
			break;

		case 1:
			printk("-> Mode Gray\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_gray_04,"s5k4ca_effect_gray_04");
			break;

		case 2:
			printk("-> Mode Sepia\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_sepia_04,"s5k4ca_effect_sepia_04");
			break;

		case 3:
			printk("-> Mode Negative\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_negative_04,"s5k4ca_effect_negative_04");
			break;
		
		case 4:
			printk("-> Mode Aqua\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_aqua_04,"s5k4ca_effect_aqua_04");
			break;

		case 5:
			printk("-> Mode Sketch\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_sketch_04,"s5k4ca_effect_sketch_04");
			break;

		default:
			printk("-> Mode None\n");
			s5k4ca_sensor_write_list(client,s5k4ca_effect_off_04,"s5k4ca_effect_off_04");
			break;
			
	}

	return 0;

}

static int s5k4ca_sensor_change_br(struct i2c_client *client, int type)
{

	printk("Brightness Mode \n");

	switch (type)
	{
		case 0: 
			printk("-> Brightness Minus 4\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus4_04,"s5k4ca_br_minus4_04");
			break;

		case 1:
			printk("-> Brightness Minus 3\n");	
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus3_04,"s5k4ca_br_minus3_04");
			break;
		
		case 2:
			printk("-> Brightness Minus 2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus2_04,"s5k4ca_br_minus2_04");
			break;
		
		case 3:				
			printk("-> Brightness Minus 1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus1_04,"s5k4ca_br_minus1_04");
			break;
		
		case 4:
			printk("-> Brightness Zero\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_zero_04,"s5k4ca_br_zero_04");
			break;

		case 5:
			printk("-> Brightness Plus 1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus1_04,"s5k4ca_br_plus1_04");
			break;

		case 6:
			printk("-> Brightness Plus 2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus2_04,"s5k4ca_br_plus2_04");
			break;

		case 7:
			printk("-> Brightness Plus 3\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus3_04,"s5k4ca_br_plus3_04");
			break;

		case 8:
			printk("-> Brightness Plus 4\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_plus4_04,"s5k4ca_br_plus4_04");
			break;

		default :
			printk("-> Brightness Minus 4\n");
			s5k4ca_sensor_write_list(client,s5k4ca_br_minus4_04,"s5k4ca_br_minus4_04");
			break;
		
	}

	return 0;
}

static int s5k4ca_sensor_set_framerate(struct i2c_client *client, int type) //hjkang_DC11
{
	printk("Set FrameRate : %d\n",type);
	switch(type)
	{
		case 0:
			s5k4ca_sensor_write_list(client,s5k4ca_fps_nonfix_04,"s5k4ca_fps_nonfix_04"); //camcorder -> camera hjkang_DC18
			break;

		case 15:
			s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix_04,"s5k4ca_fps_15fix_04"); //camera -> camcorder
			break;

		default:
			break;
	}
	return 0;
}

static int s5k4ca_sensor_change_wb(struct i2c_client *client, int type)
{
	
	printk("White Balance Mode ");

	switch (type)
	{
		case 0:
			printk("-> WB auto mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_auto_04,"s5k4ca_wb_auto_04");
			break;
		
		case 1:
			printk("-> WB Sunny mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_sunny_04,"s5k4ca_wb_sunny_04");
			break;

		case 2:
			printk("-> WB Cloudy mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_cloudy_04,"s5k4ca_wb_cloudy_04");
			break;

		case 3:
			printk("-> WB Flourescent mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_fluorescent_04,"s5k4ca_wb_fluorescent_04");
			break;

		case 4:
			printk("-> WB Tungsten mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_tungsten_04,"s5k4ca_wb_tungsten_04");
			break;

		default :
			printk("-> WB auto mode\n");
			s5k4ca_sensor_write_list(client,s5k4ca_wb_auto_04,"s5k4ca_wb_auto_04");
			break;

	}

	previous_WB_mode = type;

	return 0;
}

static int s5k4ca_sensor_change_contrast(struct i2c_client *client, int type)
{
	
	printk("[CAM-SENSOR] =Contras Mode %d",type);
#if 0	
	switch (type)
	{
		case 0:
			printk("-> Contrast -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_m2,"s5k4ca_contrast_m2");
			break;
		case 1:
			printk("-> Contrast -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_m1,"s5k4ca_contrast_m1");
			break;
		default :
		case 2:
			printk("-> Contrast 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_0,"s5k4ca_contrast_0");
			break;
		case 3:
			printk("-> Contrast +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_p1,"s5k4ca_contrast_p1");
			break;
		case 4:
			printk("-> Contrast +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_contrast_P2,"s5k4ca_contrast_P2");
			break;
	}
#endif
	return 0;
}

static int s5k4ca_sensor_change_saturation(struct i2c_client *client, int type)
{
	printk("[CAM-SENSOR] =Saturation Mode %d",type);
#if 0	
	switch (type)
	{
		case 0:
			printk("-> Saturation -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_m2,"s5k4ca_Saturation_m2");
			break;
		case 1:
			printk("-> Saturation -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_m1,"s5k4ca_Saturation_m1");
			break;
		case 2:
		default :
			printk("-> Saturation 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_0,"s5k4ca_Saturation_0");
			break;
		case 3:
			printk("-> Saturation +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_p1,"s5k4ca_Saturation_p1");
			break;
		case 4:
			printk("-> Saturation +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Saturation_P2,"s5k4ca_Saturation_P2");
			break;
	}
#endif
	return 0;
}

static int s5k4ca_sensor_change_sharpness(struct i2c_client *client, int type)
{
	printk("[CAM-SENSOR] =Sharpness Mode %d",type);
#if 0	
	switch (type)
	{
		case 0:
			printk("-> Sharpness -2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_m2,"s5k4ca_Sharpness_m2");
			break;
		case 1:
			printk("-> Sharpness -1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_m1,"s5k4ca_Sharpness_m1");
			break;
		case 2:
		default :
			printk("-> Sharpness 0\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_0,"s5k4ca_Sharpness_0");
			break;
		case 3:
			printk("-> Sharpness +1\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_p1,"s5k4ca_Sharpness_p1");
			break;
		case 4:
			printk("-> Sharpness +2\n");
			s5k4ca_sensor_write_list(client,s5k4ca_Sharpness_P2,"s5k4ca_Sharpness_P2");
			break;
	}
#endif
	return 0;
}

static int s5k4ca_sensor_change_iso(struct i2c_client *client, int type)
{
	printk("[CAM-SENSOR] =Iso Mode %d",type);
#if 0	
	switch (type)
	{
		case 0:
		default :
			printk("-> ISO AUTO\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso_auto,"s5k4ca_iso_auto");
			break;
		case 1:
			printk("-> ISO 50\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso50,"s5k4ca_iso50");
			break;
		case 2:
			printk("-> ISO 100\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso100,"s5k4ca_iso100");
			break;
		case 3:
			printk("-> ISO 200\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso200,"s5k4ca_iso200");
			break;
		case 4:
			printk("-> ISO 400\n");
			s5k4ca_sensor_write_list(client,s5k4ca_iso400,"s5k4ca_iso400");
			break;
	}
#endif
	return 0;
}


static int s5k4ca_sensor_change_scene_mode(struct i2c_client *client, int type)
{

	printk("[CAM-SENSOR] =Scene Mode %d",type);
	if(previous_scene_mode != SCENE_MODE_AUTO && type != SCENE_MODE_AUTO)
	{
		printk("-> Pre-auto-set");
#ifdef I2C_BURST_MODE //dha23 101028				
		s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
#else
		s5k4ca_sensor_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
#endif
	}

	switch (type)
	{
		case SCENE_MODE_AUTO:
			printk("-> auto\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_mode_off_04,"s5k4ca_scene_mode_off_04");
#endif
			break;
			
		case SCENE_MODE_PORTRAIT:
			printk("-> portrait\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_portrait_on_04,"s5k4ca_scene_portrait_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_portrait_on_04,"s5k4ca_scene_portrait_on_04");
#endif
			break;
			
		case SCENE_MODE_LANDSCAPE:
			printk("-> landscape\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_landscape_on_04,"s5k4ca_scene_landscape_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_landscape_on_04,"s5k4ca_scene_landscape_on_04");
#endif
			break;
			
		case SCENE_MODE_NIGHT:
			printk("-> night\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_nightmode_on_04,"s5k4ca_scene_nightmode_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_nightmode_on_04,"s5k4ca_scene_nightmode_on_04");
#endif
			break;
			
		case SCENE_MODE_BEACH:
			printk("-> beach snow\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_beach_snow_on_04,"s5k4ca_scene_beach_snow_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_beach_snow_on_04,"s5k4ca_scene_beach_snow_on_04");
#endif
			break;
			
		case SCENE_MODE_SNOW:
			printk("-> beach snow\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_beach_snow_on_04,"s5k4ca_scene_beach_snow_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_beach_snow_on_04,"s5k4ca_scene_beach_snow_on_04");
#endif
			break;
			
		case SCENE_MODE_SUNSET:
			printk("-> sunset\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_sunset_on_04,"s5k4ca_scene_sunset_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_sunset_on_04,"s5k4ca_scene_sunset_on_04");
#endif
			break;
			
		case SCENE_MODE_FIREWORKS:
			printk("-> fireworks\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_scene_fireworks_on_04,"s5k4ca_scene_fireworks_on_04");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_scene_fireworks_on_04,"s5k4ca_scene_fireworks_on_04");
#endif
			break;

		default :
			printk("-> UnKnow Scene Mode\n");
			break;
	}

	previous_scene_mode = type;
	
	return 0;
}

static int s5k4ca_sensor_photometry(struct i2c_client *client, int type)
{
	
	printk("Metering Mode ");

	switch (type)
	{
		case 0:
			printk("-> spot\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_metering_spot,"s5k4ca_metering_spot");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_metering_spot,"s5k4ca_metering_spot");
#endif
			break;
		case 1:
			printk("-> matrix\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_metering_matrix,"s5k4ca_metering_matrix");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_metering_matrix,"s5k4ca_metering_matrix");
#endif
			break;
		case 2:
			printk("-> center\n");
#ifdef I2C_BURST_MODE //dha23 101028				
			s5k4ca_sensor_burst_write_list(client,s5k4ca_metering_center,"s5k4ca_metering_center");
#else
			s5k4ca_sensor_write_list(client,s5k4ca_metering_center,"s5k4ca_metering_center");
#endif
			break;
		default :
			printk("-> UnKnown Metering Mode\n");
			break;
	
	}
	return 0;	

}

static int s5k4ca_sensor_user_read(struct i2c_client *client, s5k4ca_t *r_data)
{
	s5k4ca_sensor_write(client, 0x002C, r_data->page);
	s5k4ca_sensor_write(client, 0x002E, r_data->subaddr);
	return s5k4ca_sensor_read(client, 0x0F12, &(r_data->value));
}

static int s5k4ca_sensor_user_write(struct i2c_client *client, unsigned short *w_data)
{
	return s5k4ca_sensor_write(client, w_data[0], w_data[1]);
}

static int s5k4ca_sensor_exif_read(struct i2c_client *client, exif_data_t *exif_data)
{
	int ret = 0;
	
//unsigned short lux = 0;
	unsigned short extime = 0;
	unsigned short iso_val = 0;

	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// exposure time
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x1C3C);
//	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &extime);

/*	msleep(100);

	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	/// Incident Light value 
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x12FE);
	msleep(100);
	s5k4ca_sensor_read(client, 0x0F12, &lux); */

    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    /// exposure time
    s5k4ca_sensor_write(client, 0x002C, 0x7000);
    s5k4ca_sensor_write(client, 0x002E, 0x12FA);
//  msleep(100);
    s5k4ca_sensor_read(client, 0x0F12, &iso_val);

    
    iso_val = iso_val * 10;
    iso_val = iso_val / 256;

/*
    if(iso_val >= 10 && iso_val < 12)
        exif_data->iso = 64;
    else if(iso_val >= 12 && iso_val < 18)
        exif_data->iso = 100;
    else if(iso_val >= 18 && iso_val < 26)
        exif_data->iso = 125;
    else if(iso_val >= 26 && iso_val < 35)
        exif_data->iso = 200;
    else if(iso_val >= 36 && iso_val < 46)
        exif_data->iso = 250;
    else if(iso_val >= 46 && iso_val < 56)
        exif_data->iso = 320;
    else if(iso_val >= 56 && iso_val < 66)
        exif_data->iso = 400;
    else if(iso_val >= 66 && iso_val < 76)
        exif_data->iso = 500;
    else if(iso_val >= 76 && iso_val < 96)
        exif_data->iso = 640;
    else if(iso_val >= 96)
        exif_data->iso = 800;
    else 
        exif_data->iso = 0;
*/

    if (iso_val < 10)
    {
        printk("[CAM-SENSOR] =%s iso_val_read(%d) < 1.0 \n",__func__,iso_val);
        exif_data->iso = 50;
    }
    else if(iso_val >= 10 && iso_val < 19)
        exif_data->iso = 50;
    else if(iso_val >= 19 && iso_val < 23)
        exif_data->iso = 100;
    else if(iso_val >= 23 && iso_val < 28)
        exif_data->iso = 200;
    else if(iso_val >= 28)
    {
        printk("[CAM-SENSOR] =%s iso_val_read(%d) < 2.8 \n",__func__,iso_val);
        exif_data->iso = 400;
    }
    else 
    {
        printk("[CAM-SENSOR] =%s iso_val_read(%d) unknown range \n",__func__,iso_val);
        exif_data->iso = 0;
    }

	exif_data->exposureTime = extime/100;
	exif_data->lux = lux_value;
    printk("[CAM-SENSOR] =%s iso_val=%d, extime=%d, lux=%d,\n",__func__,exif_data->iso, extime/100,lux_value);


	return ret;
}

static int s5k4ca_sensor_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	struct v4l2_control *ctrl;
	unsigned short *w_data;		/* To support user level i2c */	
	s5k4ca_short_t *r_data;
	exif_data_t *exif_data;

	int ret=0;

	switch (cmd)
	{
		case SENSOR_INIT:
			ret = sensor_init(client);
			break;

		case USER_ADD:
			break;

		case USER_EXIT:
			s5k4ca_sensor_exit();
			break;

		case SENSOR_EFFECT:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_effect(client, ctrl->value);
			break;

		case SENSOR_BRIGHTNESS:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_br(client, ctrl->value);
			break;

		case SENSOR_FRAMERATE: //hjkang_DC11
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_set_framerate(client, ctrl->value);
			break;

		case SENSOR_WB:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_wb(client, ctrl->value);
			break;

		case SENSOR_SCENE_MODE:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_scene_mode(client, ctrl->value);
			break;

		case SENSOR_ISO:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_iso(client, ctrl->value);
			break;

		case SENSOR_CONTRAST:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_contrast(client, ctrl->value);
			break;

		case SENSOR_SATURATION:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_saturation(client, ctrl->value);
			break;

		case SENSOR_SHARPNESS:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_change_sharpness(client, ctrl->value);
			break;

		case SENSOR_AF:
			ctrl = (struct v4l2_control *)arg;
			ret = s5k4ca_sensor_af_control(client, ctrl->value);
			break;

		case SENSOR_MODE_SET:
			ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_mode_set(client, ctrl->value);
			break;

		case SENSOR_XGA:
			s5k4ca_sensor_change_size(client, SENSOR_XGA);	
			break;

		case SENSOR_QXGA:
			s5k4ca_sensor_change_size(client, SENSOR_QXGA);	
			break;

		case SENSOR_QSVGA:
			s5k4ca_sensor_change_size(client, SENSOR_QSVGA);
			break;

		case SENSOR_VGA:
			s5k4ca_sensor_change_size(client, SENSOR_VGA);
			break;

		case SENSOR_SVGA:
			s5k4ca_sensor_change_size(client, SENSOR_SVGA);
			break;

		case SENSOR_SXGA:
			s5k4ca_sensor_change_size(client, SENSOR_SXGA);
			break;

		case SENSOR_UXGA:
			s5k4ca_sensor_change_size(client, SENSOR_UXGA);
			break;

		case SENSOR_USER_WRITE:
			w_data = (unsigned short *)arg;
			s5k4ca_sensor_user_write(client, w_data);
			break;

		case SENSOR_USER_READ:
			r_data = (s5k4ca_short_t *)arg;
			s5k4ca_sensor_user_read(client, (s5k4ca_t *)r_data);
			break;
	
		case SENSOR_FLASH_CAMERA:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			printk("[SENSOR_FLASH_CAMERA] ctrl->value = %d ctrl->id = %d\n", ctrl->value, ctrl->id);

			cam_flash_on = ctrl->value; //dha23 100527

			if(ctrl->id == 1)
			    aat1271a_flash_camera_control(ctrl->value);	
			else if(ctrl->id == 2)
			    aat1271a_flash_movie_control(ctrl->value);	
			else if(ctrl->id == 3)
			   aat1271a_flash_torch_camera_control(ctrl->value);
#endif			
			break;

		case SENSOR_FLASH_MOVIE:
			ctrl = (struct v4l2_control *)arg;
#ifdef CONFIG_FLASH_AAT1271A
			aat1271a_flash_movie_control(ctrl->value);	
#endif
			break;

		case SENSOR_EXIF_DATA:
			exif_data = (exif_data_t *)arg;
			s5k4ca_sensor_exif_read(client, exif_data);	
			break;

		case SENSOR_PHOTOMETRY:
		    ctrl = (struct v4l2_control *)arg;
			s5k4ca_sensor_photometry(client, ctrl->value);
			break;
		

		default:
		    printk("[CAM-SENSOR] no command type %d \n",cmd);
			break;
	}

	return ret;
}

static const struct i2c_device_id s5k4ca_id[] = {
	{ "s5k4ca", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4ca_id);

static struct i2c_driver s5k4ca_driver = {
	.driver = {
		.name = "s5k4ca",
	},
	.probe = s5k4ca_probe,
	.remove = s5k4ca_remove,
	.id_table = s5k4ca_id,
	.command = s5k4ca_sensor_command
};


#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

static char *s5k4ca_regs_table = NULL;

static int s5k4ca_regs_table_size;

void s5k4ca_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int i;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());
#if 0
	filp = filp_open("/data/camera/s5k4ca.h", O_RDONLY, 0);
#else
	filp = filp_open("/sdcard/s5k4ca.h", O_RDONLY, 0);
#endif
	if (IS_ERR(filp)) {
		printk("file open error\n");
		return;
	}
	l = filp->f_path.dentry->d_inode->i_size;	
	printk("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	if (dp == NULL) {
		printk("Out of Memory\n");
		filp_close(filp, current->files);
	}
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	if (ret != l) {
		printk("Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return;
	}

	filp_close(filp, current->files);
	
	set_fs(fs);

	s5k4ca_regs_table = dp;
	
	s5k4ca_regs_table_size = l;

	*((s5k4ca_regs_table + s5k4ca_regs_table_size) - 1) = '\0';

	printk("s5k4ca_regs_table 0x%08x, %ld\n", dp, l);
}

void s5k4ca_regs_table_exit(void)
{
	printk("%s %d\n", __func__, __LINE__);
	if (s5k4ca_regs_table) {
		kfree(s5k4ca_regs_table);
		s5k4ca_regs_table = NULL;
	}	
}

static int s5k4ca_regs_table_write(char *name)
{
	char *start, *end, *reg, *data;	
	unsigned short addr, value;
	char reg_buf[7], data_buf[7];

	*(reg_buf + 6) = '\0';
	*(data_buf + 6) = '\0';

	start = strstr(s5k4ca_regs_table, name);
	
	end = strstr(start, "};");

	while (1) {	
		/* Find Address */	
		reg = strstr(start,"{ 0x");		
		if (reg)
			start = (reg + 16);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */	
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 2), 6);	
			memcpy(data_buf, (reg + 10), 6);	
			addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16); 
			value = (unsigned short)simple_strtoul(data_buf, NULL, 16); 
//			printk("addr 0x%04x, value 0x%04x\n", addr, value);
/*
			if (addr == 0xdddd)
			{
			    if (value == 0x0010)
					mdelay(10);
				else if (value == 0x0020)
					mdelay(20);
				else if (value == 0x0030)
					mdelay(30);
				else if (value == 0x0040)
					mdelay(40);
				else if (value == 0x0050)
					mdelay(50);
				else if (value == 0x0100)
					mdelay(100);
	
				mdelay(value);

				printk("delay 0x%04x, value 0x%04x\n", addr, value);
			}	
			else
*/			
				s5k4ca_sensor_write(s5k4ca_data.sensor, addr, value);
		}
	}

	return 0;
}

#endif

static int s5k4ca_sensor_init(void)
{
	int ret;

	cam_flash_on = 0; //dha23 101004
#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_init();
#endif

//	s5k4ca_sensor_enable();
	
	s3c_camif_open_sensor(&s5k4ca_data);

	if (s5k4ca_data.sensor == NULL)
		if ((ret = i2c_add_driver(&s5k4ca_driver)))
			return ret;

	if (s5k4ca_data.sensor == NULL) {
		i2c_del_driver(&s5k4ca_driver);	
		return -ENODEV;
	}

	s3c_camif_register_sensor(&s5k4ca_data);
	
	return 0;
}

static void s5k4ca_sensor_exit(void)
{
	s5k4ca_sensor_disable();

#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_exit();
#endif
	
	if (s5k4ca_data.sensor != NULL)
		s3c_camif_unregister_sensor(&s5k4ca_data);
}

static struct v4l2_input s5k4ca_input = {
	.index		= 0,
	.name		= "Camera Input (S5K4CA)",
	.type		= V4L2_INPUT_TYPE_CAMERA,
	.audioset	= 1,
	.tuner		= 0,
	.std		= V4L2_STD_PAL_BG | V4L2_STD_NTSC_M,
	.status		= 0,
};

static struct v4l2_input_handler s5k4ca_input_handler = {
	s5k4ca_sensor_init,
	s5k4ca_sensor_exit	
};

#ifdef CONFIG_VIDEO_SAMSUNG
static int s5k4ca_sensor_add(void)
{
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif
	int ret = i2c_add_driver(&s5k4ca_driver);
	if(ret < 0) {
		printk("\n I2c adding failed\n");
		return ret;
	}
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}

static void s5k4ca_sensor_remove(void)
{
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}

module_init(s5k4ca_sensor_add)
module_exit(s5k4ca_sensor_remove)

MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("I2C Client Driver For FIMC V4L2 Driver");
MODULE_LICENSE("GPL");
#else
int s5k4ca_sensor_add(void)
{
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_init();
#endif	
#ifdef LOCAL_CONFIG_S5K4CA_I2C_TEST
	return s5k4ca_sensor_init();
#else
	return s3c_camif_add_sensor(&s5k4ca_input, &s5k4ca_input_handler);
#endif
}

void s5k4ca_sensor_remove(void)
{
	if (s5k4ca_data.sensor != NULL)
		i2c_del_driver(&s5k4ca_driver);
#ifdef CONFIG_FLASH_AAT1271A
	aat1271a_flash_exit();
#endif
	s3c_camif_remove_sensor(&s5k4ca_input, &s5k4ca_input_handler);
}
#endif
