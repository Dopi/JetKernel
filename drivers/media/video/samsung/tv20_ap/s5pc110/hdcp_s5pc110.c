/* linux/drivers/media/video/samsung/tv20/s5pc100/hdcp_s5pc100.c
 *
 * hdcp raw ftn  file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/irq.h>


#include <asm/io.h>
#include <mach/gpio.h>

#include "tv_out_s5pc110.h"
#include "regs/regs-hdmi.h"

/* for Operation check */
#ifdef COFIG_TVOUT_RAW_DBG
#define S5P_HDCP_DEBUG 1
//#define S5P_HDCP_I2C_DEBUG 1
//#define S5P_HDCP_AUTH_DEBUG 1
#endif

#ifdef S5P_HDCP_DEBUG
#define HDCPPRINTK(fmt, args...) \
	printk("\t\t[HDCP] %s: " fmt, __FUNCTION__ , ## args)
//	printk("\t\t\t[S5P_HAES_CON] 0x%08x \n",readl(hdmi_base + S5P_HDCP_Ri_0));\
//	printk("\t\t\t[S5P_HAES_CON] 0x%08x \n",readl(hdmi_base + S5P_HDCP_Ri_1))
#else
#define HDCPPRINTK(fmt, args...)
#endif

/* for i2c bus check */
#ifdef S5P_HDCP_I2C_DEBUG
#define I2CPRINTK(fmt, args...) \
	printk("\t\t\t\t[I2C] %s: " fmt, __FUNCTION__ , ## args)
#else
#define I2CPRINTK(fmt, args...)
#endif

/* for authentication key check */
#ifdef S5P_HDCP_AUTH_DEBUG
#define AUTHPRINTK(fmt, args...) \
	printk("\t\t\t[AUTHKEY] %s: " fmt, __FUNCTION__ , ## args)
#else
#define AUTHPRINTK(fmt, args...)
#endif

//#define USE_PJ_UPDATE
//#define HAES_DEBUG

extern void __iomem		*hdmi_base;

enum hdmi_run_mode {
	DVI_MODE,
	HDMI_MODE
};

enum hdmi_resolution {
	SD480P,
	SD480I,
	WWSD480P,
	HD720P,
	SD576P,
	WWSD576P,
	HD1080I
};

enum hdmi_color_bar_type {
	HORIZONTAL,
	VERTICAL
};

enum hdcp_event {
	/* Stop HDCP */
	HDCP_EVENT_STOP,
	/* Start HDCP*/
	HDCP_EVENT_START,
	/* Start to read Bksv,Bcaps */
	HDCP_EVENT_READ_BKSV_START,
	/* Start to write Aksv,An */
	HDCP_EVENT_WRITE_AKSV_START,
	/* Start to check if Ri is equal to Rj */
	HDCP_EVENT_CHECK_RI_START,
	/* Start 2nd authentication process */
	HDCP_EVENT_SECOND_AUTH_START
};

enum hdcp_state {
	NOT_AUTHENTICATED, 
	RECEIVER_READ_READY, 
	BCAPS_READ_DONE, 
	BKSV_READ_DONE, 
	AN_WRITE_DONE, 
	AKSV_WRITE_DONE, 
	FIRST_AUTHENTICATION_DONE, 
	SECOND_AUTHENTICATION_RDY, 
	RECEIVER_FIFOLSIT_READY, 
	SECOND_AUTHENTICATION_DONE,
};

/*
 * Below CSC_TYPE is temporary. CSC_TYPE enum. 
 * may be included in SetSD480pVars_60Hz etc.
 *
 * LR : Limited Range (16~235)
 * FR : Full Range (0~255)
 */
enum hdmi_intr_src {
	WAIT_FOR_ACTIVE_RX,
	WDT_FOR_REPEATER,
	EXCHANGE_KSV,
	UPDATE_P_VAL,
	UPDATE_R_VAL,
	AUDIO_OVERFLOW,
	AUTHEN_ACK,
	UNKNOWN_INT
};

const u8 hdcp_key[288] = {
	0xf8, 0x81, 0x24, 0xe6, 0xca, 0x07, 0xa3, 0x7b, 0x46, 0x6f, 0x88, 0x69, 0xf2, 0x4d, 0x1e, 0x22,
	0x7c, 0x91, 0x2a, 0x3b, 0xc3, 0x3d, 0xb4, 0xbd, 0x28, 0x91, 0x6e, 0x8b, 0x75, 0x27, 0x14, 0x46,
	0x20, 0x17, 0xf4, 0x09, 0x9d, 0x52, 0x72, 0xf3, 0x0a, 0x11, 0x0e, 0x11, 0x65, 0x86, 0x28, 0xe1,
	0x13, 0xc4, 0x1f, 0xf2, 0xd9, 0x9c, 0xfd, 0x82, 0x95, 0xee, 0x18, 0xe0, 0xef, 0xcb, 0x5f, 0x49,
	0x98, 0x19, 0xc6, 0xc2, 0x4c, 0x2d, 0x00, 0xa0, 0x0a, 0x44, 0x7b, 0x14, 0x73, 0x06, 0x0d, 0x6a,
	0xf5, 0x22, 0x19, 0x96, 0x00, 0xcb, 0x57, 0x67, 0x0e, 0x6a, 0x33, 0xb0, 0x35, 0x4f, 0x86, 0xe5,
	0x29, 0xbb, 0x9d, 0xc4, 0x38, 0xde, 0x8e, 0x5e, 0x20, 0xed, 0xc8, 0x46, 0x58, 0x98, 0x45, 0x7c,
	0xf8, 0xff, 0x21, 0x45, 0x20, 0x50, 0x41, 0x8b, 0xdb, 0xf1, 0x76, 0x8b, 0xff, 0x22, 0xe0, 0x15,
	0x02, 0x40, 0x65, 0xe0, 0x67, 0xb6, 0xd2, 0xc4, 0xcd, 0xde, 0x25, 0x05, 0x5f, 0x60, 0x85, 0x1b,
	0x97, 0x00, 0xed, 0x78, 0x52, 0x98, 0xde, 0x46, 0x0c, 0x84, 0x5f, 0x3b, 0xd9, 0x61, 0x02, 0xec,
	0xd2, 0x58, 0x3b, 0xff, 0x83, 0x42, 0x80, 0xc5, 0x46, 0x38, 0xc6, 0xbf, 0x10, 0x7e, 0xee, 0xf3,
	0xf1, 0x24, 0xfd, 0x86, 0xe8, 0x69, 0x45, 0x1c, 0xc9, 0x13, 0x2f, 0x8a, 0x3c, 0xfd, 0x68, 0xc6,
	0x20, 0x5c, 0x45, 0xed, 0x96, 0xe3, 0x9a, 0xa7, 0xef, 0x1d, 0xda, 0x56, 0x5e, 0xbe, 0x30, 0xa2,
	0x14, 0xdb, 0x55, 0xb8, 0x97, 0x53, 0xd4, 0xf1, 0x78, 0x6b, 0x7d, 0x27, 0x16, 0x0e, 0xc5, 0x7d,
	0xc7, 0xc2, 0x53, 0x4b, 0xed, 0x12, 0xea, 0x58, 0x38, 0xd0, 0xf5, 0x3e, 0x3d, 0xe3, 0xe5, 0xf1,
	0x4c, 0xeb, 0xb0, 0x7e, 0x4b, 0x16, 0x6f, 0xf5, 0xce, 0x24, 0xd6, 0x8b, 0x86, 0x1a, 0x47, 0x52,
	0xf5, 0x6f, 0x0a, 0x7d, 0xf5, 0x15, 0xdf, 0x6e, 0xba, 0x03, 0xcc, 0x4f, 0x1a, 0x35, 0x4b, 0x00,
	0x78, 0xc1, 0xfd, 0x7b, 0xa6, 0x05, 0x19, 0xc9, 0xae, 0xd3, 0xac, 0xbd, 0x3a, 0x9c, 0x57, 0x9d
};

struct s5p_hdcp_info {
	bool	is_repeater;
	bool	hpd_status;
	u32	time_out;
	u32	hdcp_enable;

	spinlock_t 	lock;
	
	struct i2c_client 	*client;

	wait_queue_head_t 	waitq;
	enum hdcp_event 	event;
	enum hdcp_state 	auth_status;

	struct work_struct  	work;
};

static struct s5p_hdcp_info hdcp_info = {
	.is_repeater 	= false,
	.time_out	= 0,
	.hdcp_enable	= false,
	.client		= NULL,
	.event 		= HDCP_EVENT_STOP,
	.auth_status	= NOT_AUTHENTICATED,
	
};

#define HDCP_RI_OFFSET          0x08
#define INFINITE		0xffffffff

#define DO_NOT_TRANSMIT		(0)
#define HDMI_SYS_ENABLE		(1 << 0)
#define HDMI_ASP_ENABLE		(1 << 2)
#define HDMI_ASP_DISABLE	(~HDMI_ASP_ENABLE)

#define MAX_DEVS_EXCEEDED          (0x1 << 7)
#define MAX_CASCADE_EXCEEDED       (0x1 << 3)

#define MAX_CASCADE_EXCEEDED_ERROR 	(-1)
#define MAX_DEVS_EXCEEDED_ERROR    	(-2)
#define REPEATER_ILLEGAL_DEVICE_ERROR	(-3)

#define AINFO_SIZE		1
#define BCAPS_SIZE		1
#define BSTATUS_SIZE		2
#define SHA_1_HASH_SIZE	20

#define KSV_FIFO_READY	(0x1 << 5)

/* spmoon for test : it's not in manual */
#define SET_HDCP_KSV_WRITE_DONE		(0x1 << 3)
#define CLEAR_HDCP_KSV_WRITE_DONE	(~SET_HDCP_KSV_WRITE_DONE)

#define SET_HDCP_KSV_LIST_EMPTY 	(0x1 << 2)
#define CLEAR_HDCP_KSV_LIST_EMPTY	(~SET_HDCP_KSV_LIST_EMPTY)
#define SET_HDCP_KSV_END		(0x1 << 1)
#define CLEAR_HDCP_KSV_END		(~SET_HDCP_KSV_END)
#define SET_HDCP_KSV_READ		(0x1 << 0)
#define CLEAR_HDCP_KSV_READ		(~SET_HDCP_KSV_READ)

#define SET_HDCP_SHA_VALID_READY	(0x1 << 1)
#define CLEAR_HDCP_SHA_VALID_READY	(~SET_HDCP_SHA_VALID_READY)
#define SET_HDCP_SHA_VALID	(0x1 << 0)
#define CLEAR_HDCP_SHA_VALID	(~SET_HDCP_SHA_VALID)

#define TRANSMIT_EVERY_VSYNC	(0x1 << 1)

/* must be checked */
extern u8 hdcp_protocol_status; // 0 - hdcp stopped, 1 - hdcp started, 2 - hdcp reset
/*
void __s5p_hdcp_reset(void);
bool __s5p_stop_hdcp(void);
bool __s5p_start_hdcp(void);

void debug_register(void)
{
	printk("S5P_HDMI_CON_0  : 0x%08x\n",readl(hdmi_base+S5P_HDMI_CON_0 ));
	printk("S5P_HDMI_CON_1 : 0x%08x\n",readl(hdmi_base+S5P_HDMI_CON_1));
	printk("S5P_HDMI_CON_2 : 0x%08x\n",readl(hdmi_base+S5P_HDMI_CON_2));
	printk("S5P_STATUS : 0x%08x\n",readl(hdmi_base+S5P_STATUS));
	printk("S5P_STATUS_EN : 0x%08x\n",readl(hdmi_base+S5P_STATUS_EN));
	printk("S5P_HPD : 0x%08x\n",readl(hdmi_base+S5P_HPD));
	printk("S5P_MODE_SEL : 0x%08x\n",readl(hdmi_base+S5P_MODE_SEL));
	printk("S5P_ENC_EN : 0x%08x\n",readl(hdmi_base+S5P_ENC_EN));
	printk("S5P_HDCP_RX_SHA1_0_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_0_0));
	printk("S5P_HDCP_RX_SHA1_0_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_0_1));
	printk("S5P_HDCP_RX_SHA1_0_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_0_2));
	printk("S5P_HDCP_RX_SHA1_0_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_0_3));
	printk("S5P_HDCP_RX_SHA1_1_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_1_0));
	printk("S5P_HDCP_RX_SHA1_1_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_1_1));
	printk("S5P_HDCP_RX_SHA1_1_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_1_2));
	printk("S5P_HDCP_RX_SHA1_1_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_1_3));
	printk("S5P_HDCP_RX_SHA1_2_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_2_0));
	printk("S5P_HDCP_RX_SHA1_2_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_2_1));
	printk("S5P_HDCP_RX_SHA1_2_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_2_2));
	printk("S5P_HDCP_RX_SHA1_2_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_2_3));
	printk("S5P_HDCP_RX_SHA1_3_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_3_0));
	printk("S5P_HDCP_RX_SHA1_3_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_3_1));
	printk("S5P_HDCP_RX_SHA1_3_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_3_2));
	printk("S5P_HDCP_RX_SHA1_3_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_3_3));
	printk("S5P_HDCP_RX_SHA1_4_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_4_0));
	printk("S5P_HDCP_RX_SHA1_4_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_4_1));
	printk("S5P_HDCP_RX_SHA1_4_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_4_2));
	printk("S5P_HDCP_RX_SHA1_4_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_SHA1_4_3));
	printk("S5P_HDCP_RX_KSV_0_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_0_0));
	printk("S5P_HDCP_RX_KSV_0_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_0_1));
	printk("S5P_HDCP_RX_KSV_0_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_0_2));
	printk("S5P_HDCP_RX_KSV_0_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_0_3));
	printk("S5P_HDCP_RX_KSV_0_4 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_0_4));
	printk("S5P_HDCP_RX_KSV_LIST_CTRL : 0x%08x\n",readl(hdmi_base+S5P_HDCP_RX_KSV_LIST_CTRL));
	printk("S5P_HDCP_AUTH_STATUS : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AUTH_STATUS));
	printk("S5P_HDCP_CTRL : 0x%08x\n",readl(hdmi_base+S5P_HDCP_CTRL));
	printk("S5P_HDCP_CHECK_RESULT : 0x%08x\n",readl(hdmi_base+S5P_HDCP_CHECK_RESULT));
	printk("S5P_HDCP_BKSV_0_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BKSV_0_0));
	printk("S5P_HDCP_BKSV_0_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BKSV_0_1));
	printk("S5P_HDCP_BKSV_0_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BKSV_0_2));
	printk("S5P_HDCP_BKSV_0_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BKSV_0_3));
	printk("S5P_HDCP_BKSV_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BKSV_1));
	printk("S5P_HDCP_AKSV_0_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AKSV_0_0));
	printk("S5P_HDCP_AKSV_0_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AKSV_0_1));
	printk("S5P_HDCP_AKSV_0_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AKSV_0_2));
	printk("S5P_HDCP_AKSV_0_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AKSV_0_3));
	printk("S5P_HDCP_AKSV_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_AKSV_1));
	printk("S5P_HDCP_An_0_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_0_0));
	printk("S5P_HDCP_An_0_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_0_1));
	printk("S5P_HDCP_An_0_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_0_2));
	printk("S5P_HDCP_An_0_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_0_3));
	printk("S5P_HDCP_An_1_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_1_0));
	printk("S5P_HDCP_An_1_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_1_1));
	printk("S5P_HDCP_An_1_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_1_2));
	printk("S5P_HDCP_An_1_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_An_1_3));
	printk("S5P_HDCP_BCAPS : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BCAPS));
	printk("S5P_HDCP_BSTATUS_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BSTATUS_0));
	printk("S5P_HDCP_BSTATUS_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_BSTATUS_1));
	printk("S5P_HDCP_Ri_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_Ri_0));
	printk("S5P_HDCP_Ri_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_Ri_1));
	printk("S5P_HDCP_Pj : 0x%08x\n",readl(hdmi_base+S5P_HDCP_Pj));
	printk("S5P_HDCP_OFFSET_TX_0 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_OFFSET_TX_0));
	printk("S5P_HDCP_OFFSET_TX_1 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_OFFSET_TX_1));
	printk("S5P_HDCP_OFFSET_TX_2 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_OFFSET_TX_2));
	printk("S5P_HDCP_OFFSET_TX_3 : 0x%08x\n",readl(hdmi_base+S5P_HDCP_OFFSET_TX_3));
	printk("S5P_HDCP_CYCLE_AA : 0x%08x\n",readl(hdmi_base+S5P_HDCP_CYCLE_AA));
	printk("S5P_HAES_CON : 0x%08x\n",readl(hdmi_base+S5P_HAES_CON));
	printk("S5P_HAES_DATA_SIZE_L : 0x%08x\n",readl(hdmi_base+S5P_HAES_DATA_SIZE_L));
	printk("S5P_HAES_DATA_SIZE_H : 0x%08x\n",readl(hdmi_base+S5P_HAES_DATA_SIZE_H));
	//printk("S5P_HAES_DATA : 0x%08x\n",readl(hdmi_base+S5P_HAES_DATA));
}


static count = 0;

void reset_register(void)
{
	writel(readl(hdmi_base + S5P_HDMI_CON_0) & HDMI_DIS, 
		hdmi_base + S5P_HDMI_CON_0);	
	
	switch(count) {

case 0:		writel(0x7,hdmi_base+S5P_HDMI_CON_0);
case 1:		writel(0x0,hdmi_base+S5P_HDMI_CON_1);
case 2:		writel(0x0,hdmi_base+S5P_HDMI_CON_2);
case 3:		writel(0x0,hdmi_base+S5P_STATUS);
case 4:		writel(0x0,hdmi_base+S5P_STATUS_EN);
case 5:		writel(0x0,hdmi_base+S5P_HPD);
case 6:		writel(0x2,hdmi_base+S5P_MODE_SEL);
case 7:		writel(0x0,hdmi_base+S5P_ENC_EN);
case 8:		writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_0_0);
case 9:		writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_0_1);
case 10:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_0_2);
case 11:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_0_3);
case 12:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_1_0);
case 13:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_1_1);
case 14:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_1_2);
case 15:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_1_3);
case 16:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_2_0);
case 17:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_2_1);
case 18:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_2_2);
case 19:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_2_3);
case 20:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_3_0);
case 21:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_3_1);
case 22:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_3_2);
case 23:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_3_3);
case 24:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_4_0);
case 25:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_4_1);
case 26:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_4_2);
case 27:	writel(0x0,hdmi_base+S5P_HDCP_RX_SHA1_4_3);
case 28:	writel(0x0,hdmi_base+S5P_HDCP_RX_KSV_0_0);
case 29:	writel(0x0,hdmi_base+S5P_HDCP_RX_KSV_0_1);
case 30:	writel(0x0,hdmi_base+S5P_HDCP_RX_KSV_0_2);
case 31:	writel(0x0,hdmi_base+S5P_HDCP_RX_KSV_0_3);
case 32:	writel(0x0,hdmi_base+S5P_HDCP_RX_KSV_0_4);
case 33:	writel(0x1,hdmi_base+S5P_HDCP_RX_KSV_LIST_CTRL);
case 34:	writel(0x0,hdmi_base+S5P_HDCP_AUTH_STATUS);
case 35:	writel(0x0,hdmi_base+S5P_HDCP_CTRL);
case 36:	writel(0x0,hdmi_base+S5P_HDCP_CHECK_RESULT);
case 37:	writel(0x0,hdmi_base+S5P_HDCP_BKSV_0_0);
case 38:	writel(0x0,hdmi_base+S5P_HDCP_BKSV_0_1);
case 39:	writel(0x0,hdmi_base+S5P_HDCP_BKSV_0_2);
case 40:	writel(0x0,hdmi_base+S5P_HDCP_BKSV_0_3);
case 41:	writel(0x0,hdmi_base+S5P_HDCP_BKSV_1);
case 42:	writel(0x0,hdmi_base+S5P_HDCP_AKSV_0_0);
case 43:	writel(0x0,hdmi_base+S5P_HDCP_AKSV_0_1);
case 44:	writel(0x0,hdmi_base+S5P_HDCP_AKSV_0_2);
case 45:	writel(0x0,hdmi_base+S5P_HDCP_AKSV_0_3);
case 46:	writel(0x0,hdmi_base+S5P_HDCP_AKSV_1);
case 47:	writel(0x0,hdmi_base+S5P_HDCP_An_0_0);
case 48:	writel(0x0,hdmi_base+S5P_HDCP_An_0_1);
case 49:	writel(0x0,hdmi_base+S5P_HDCP_An_0_2);
case 50:	writel(0x0,hdmi_base+S5P_HDCP_An_0_3);
case 51:	writel(0x0,hdmi_base+S5P_HDCP_An_1_0);
case 52:	writel(0x0,hdmi_base+S5P_HDCP_An_1_1);
case 53:	writel(0x0,hdmi_base+S5P_HDCP_An_1_2);
case 54:	writel(0x0,hdmi_base+S5P_HDCP_An_1_3);
case 55:	writel(0x0,hdmi_base+S5P_HDCP_BCAPS);
case 56:	writel(0x0,hdmi_base+S5P_HDCP_BSTATUS_0);
case 57:	writel(0x0,hdmi_base+S5P_HDCP_BSTATUS_1);
//case 58:	writel(0x0,hdmi_base+S5P_HDCP_Ri_0);
//case 59:	writel(0x0,hdmi_base+S5P_HDCP_Ri_1);
case 60:	writel(0x0,hdmi_base+S5P_HDCP_Pj);
case 61:	writel(0x0,hdmi_base+S5P_HDCP_OFFSET_TX_0);
case 62:	writel(0x0,hdmi_base+S5P_HDCP_OFFSET_TX_1);
case 63:	writel(0x0,hdmi_base+S5P_HDCP_OFFSET_TX_2);
case 64:	writel(0x0,hdmi_base+S5P_HDCP_OFFSET_TX_3);
case 65:	writel(0xff,hdmi_base+S5P_HDCP_CYCLE_AA);
case 66:	writel(0x0,hdmi_base+S5P_HAES_CON);
case 67:	writel(0x20,hdmi_base+S5P_HAES_DATA_SIZE_L);
case 68:	writel(0x1,hdmi_base+S5P_HAES_DATA_SIZE_H);
//case 69:	writel(0x0,hdmi_base+S5P_HAES_DATA);
	}


	writel(readl(hdmi_base + S5P_HDMI_CON_0) | HDMI_EN, 
		hdmi_base + S5P_HDMI_CON_0);	
	
	printk("=========count : %d\n",count);
	if ( count > 55)
		count++;
	else
		count += 10;

}
*/

#if 0 /* MUSTBECHECKED : C110 hdcp sfr was changed */

/*
 * Read the HDCP data from Rx by using IIC
 */
static int hdcp_i2c_read(struct i2c_client *client, u8 subaddr, u8 *data, u16 len)
{
	u8 addr = subaddr;
	int ret = 0;

	struct i2c_msg msg[] = {
		[0] = { 
			.addr = client->addr, 
			.flags = 0,
			.len = 1, 
			.buf = &addr
		}, 
		[1] = { 
			.addr = client->addr, 
			.flags = I2C_M_RD,
			.len = len,
			.buf = data 
		}
	};

	if (!hdcp_info.client) {
		HDCPPRINTK("DDC port is not available!!"
		       "Check hdmi receiver's DDC Port \n");
		return -EIO;
	}

	I2CPRINTK("sub addr = 0x%08x, data len = %d\n", subaddr, len);

	if (i2c_transfer(client->adapter, msg, 2) != 2)
		ret = -EIO;

	I2CPRINTK("ret :%d\n", ret);

#ifdef S5P_HDCP_I2C_DEBUG
	{
		int loop = 0;
		HDCPPRINTK("read_data :: \n");
		printk("\t\t\t");

		for (loop = 0;loop < len;loop++)
			printk("0x%02x  ", data[loop]);

		printk("\n");
	}
#endif
	return ret;
}

/*
 * Write the HDCP data to receiver by using IIC 
 * 	- use i2c_master_send()
 */
static int hdcp_i2c_write(struct i2c_client *client, u8 *data, u16 len)
{
	int ret = 0;

	if (!hdcp_info.client) {
		HDCPPRINTK("DDC port is not available!!"
		       "Check hdmi receiver's DDC Port \n");
		return -EIO;
	}

	I2CPRINTK("sub addr = 0x%08x, data len = %d\n", 
		data[0], len);

	if (i2c_master_send(client, (const char *) data, len) != len)
		ret = -EIO;

	I2CPRINTK("ret :%d\n", ret);

	return ret;
}

static bool is_hdmi_mode(void)
{
	u8 bstatus[BSTATUS_SIZE];
	bool ret = false;

	ret = hdcp_i2c_read(hdcp_info.client, HDCP_BStatus, bstatus, BSTATUS_SIZE);
	if (ret == false){
		HDCPPRINTK("Can't read BStatus data from i2c bus\n");
		return false;
	}

	HDCPPRINTK("BStatus[1]: 0x%x\n", bstatus[1]);

	if (bstatus[1] & (0x1<<4))
		return true;
	else
		return false;
}

/*
 * 1st Authentication step func.
 * Write the Ainfo data to Rx
 */
static bool write_ainfo(void)
{
	int ret = 0;
	u8 ainfo[2];

	ainfo[0] = HDCP_Ainfo;
	ainfo[1] = 0;

	ret = hdcp_i2c_write(hdcp_info.client, ainfo, 2);
	if(ret < 0)
		HDCPPRINTK("Can't write ainfo data through i2c bus\n");

	return (ret<0) ? false : true;
}

/*
 * Write the An data to Rx
 */
static bool write_an(void)
{
	int ret = 0;
	u8 an[AN_SIZE+1];

	an[0] = HDCP_An;

	// Read An from HDMI
	an[1] = readb(hdmi_base + S5P_HDCP_An_0_0);
	an[2] = readb(hdmi_base + S5P_HDCP_An_0_1);
	an[3] = readb(hdmi_base + S5P_HDCP_An_0_2);
	an[4] = readb(hdmi_base + S5P_HDCP_An_0_3);
	an[5] = readb(hdmi_base + S5P_HDCP_An_1_0);
	an[6] = readb(hdmi_base + S5P_HDCP_An_1_1);
	an[7] = readb(hdmi_base + S5P_HDCP_An_1_2);
	an[8] = readb(hdmi_base + S5P_HDCP_An_1_3);

	ret = hdcp_i2c_write(hdcp_info.client, an, AN_SIZE + 1);
	if(ret < 0)
		HDCPPRINTK("Can't write an data through i2c bus\n");
	
#ifdef S5P_HDCP_AUTH_DEBUG
	{
		u16 i = 0;
		for (i = 1; i < AN_SIZE + 1; i++) {
			AUTHPRINTK("HDCPAn[%d]: 0x%x \n", i, an[i]);
		}
	}
#endif		

	return (ret<0) ? false : true;
}

/*
 * Write the Aksv data to Rx
 */
static bool write_aksv(void)
{
	int ret = 0;	
	u8 aksv[AKSV_SIZE+1];

	aksv[0] = HDCP_Aksv;

	// Read Aksv from HDMI
	aksv[1] = readb(hdmi_base + S5P_HDCP_AKSV_0_0);
	aksv[2] = readb(hdmi_base + S5P_HDCP_AKSV_0_1);
	aksv[3] = readb(hdmi_base + S5P_HDCP_AKSV_0_2);
	aksv[4] = readb(hdmi_base + S5P_HDCP_AKSV_0_3);
	aksv[5] = readb(hdmi_base + S5P_HDCP_AKSV_1);

	ret = hdcp_i2c_write(hdcp_info.client,  aksv, AKSV_SIZE + 1);
	if(ret < 0)
		HDCPPRINTK("Can't write aksv data through i2c bus\n");

#ifdef S5P_HDCP_AUTH_DEBUG
	{
		u16 i = 0;
		for (i = 1; i < AKSV_SIZE + 1; i++) {
			AUTHPRINTK("HDCPAksv[%d]: 0x%x\n", i, aksv[i]);
		}
	}
#endif

	return (ret<0) ? false : true;
}

static bool read_bcaps(void)
{
	int ret = 0;
	u8 bcaps[BCAPS_SIZE] = {0};

	ret = hdcp_i2c_read(hdcp_info.client, HDCP_Bcaps, bcaps, BCAPS_SIZE);

	if(ret < 0){
		HDCPPRINTK("Can't read bcaps data from i2c bus\n");
		return false;
	}

	writel(bcaps[0], hdmi_base + S5P_HDCP_BCAPS);

	HDCPPRINTK("BCAPS(from i2c) : 0x%08x\n", bcaps[0]);

	if (bcaps[0] & REPEATER_SET)
		hdcp_info.is_repeater = true;
	else
		hdcp_info.is_repeater = false;

	HDCPPRINTK("attached device type :  %s !! \n\r",
		   hdcp_info.is_repeater ? "REPEATER" : "SINK");
	HDCPPRINTK("BCAPS(from sfr) = 0x%08x\n", 
		readl(hdmi_base + S5P_HDCP_BCAPS));

	return true;
}

static bool read_again_bksv(void)
{
	u8 bk_sv[BKSV_SIZE] = {0,0,0,0,0};
	u8 i = 0;
	u8 j = 0;
	u32 no_one = 0;
	u32 no_zero = 0;
	u32 result = 0;
	int ret = 0;
	
	ret = hdcp_i2c_read(hdcp_info.client, HDCP_Bksv, bk_sv, BKSV_SIZE);

	if(ret < 0){
		HDCPPRINTK("Can't read bk_sv data from i2c bus\n");
		return false;
	}

#ifdef S5P_HDCP_AUTH_DEBUG
	for (i=0; i<BKSV_SIZE; i++)
		AUTHPRINTK("i2c read : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

	for (i = 0; i < BKSV_SIZE; i++) {

		for (j = 0; j < 8; j++) {

			result = bk_sv[i] & (0x1 << j);

			if (result == 0)
				no_zero += 1;
			else
				no_one += 1;
		}
	}

	if ((no_zero == 20) && (no_one == 20)) {
		HDCPPRINTK("Suucess: no_zero, and no_one is 20\n");

		writel(bk_sv[0], hdmi_base + S5P_HDCP_BKSV_0_0);
		writel(bk_sv[1], hdmi_base + S5P_HDCP_BKSV_0_1);
		writel(bk_sv[2], hdmi_base + S5P_HDCP_BKSV_0_2);
		writel(bk_sv[3], hdmi_base + S5P_HDCP_BKSV_0_3);
		writel(bk_sv[4], hdmi_base + S5P_HDCP_BKSV_1);	

#ifdef S5P_HDCP_AUTH_DEBUG
		for (i=0; i<BKSV_SIZE; i++)
			AUTHPRINTK("set reg : Bksv[%d]: 0x%x\n", i, bk_sv[i]);

		//writel(HDCP_ENC_ENABLE, hdmi_base + S5P_ENC_EN);
#endif		
		return true;
	} else {
		HDCPPRINTK("Failed: no_zero or no_one is NOT 20\n");
		return false;
	}
}

static bool read_bksv(void)
{
	u8 bk_sv[BKSV_SIZE] = {0,0,0,0,0};
	
	int i = 0;
	int j = 0;

	u32 no_one = 0;
	u32 no_zero = 0;
	u32 result = 0;
	u32 count = 0;
	int ret = 0;

	ret = hdcp_i2c_read(hdcp_info.client, HDCP_Bksv, bk_sv, BKSV_SIZE);

	if(ret < 0){
		HDCPPRINTK("Can't read bk_sv data from i2c bus\n");
		return false;
	}

#ifdef S5P_HDCP_AUTH_DEBUG
	for (i=0; i<BKSV_SIZE; i++)
		AUTHPRINTK("i2c read : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif

	for (i = 0; i < BKSV_SIZE; i++) {

		for (j = 0; j < 8; j++) {

			result = bk_sv[i] & (0x1 << j);

			if (result == 0)
				no_zero ++;
			else
				no_one ++;
		}
	}

	if ((no_zero == 20) && (no_one == 20)) {

		writel(bk_sv[0], hdmi_base + S5P_HDCP_BKSV_0_0);
		writel(bk_sv[1], hdmi_base + S5P_HDCP_BKSV_0_1);
		writel(bk_sv[2], hdmi_base + S5P_HDCP_BKSV_0_2);
		writel(bk_sv[3], hdmi_base + S5P_HDCP_BKSV_0_3);
		writel(bk_sv[4], hdmi_base + S5P_HDCP_BKSV_1);	
		
#ifdef S5P_HDCP_AUTH_DEBUG
		for (i=0; i<BKSV_SIZE; i++)
			AUTHPRINTK("set reg : Bksv[%d]: 0x%x\n", i, bk_sv[i]);
#endif
		
		HDCPPRINTK("Success: no_zero, and no_one is 20\n");
	
	} else {
		
		HDCPPRINTK("Failed: no_zero or no_one is NOT 20\n");

		//writel(HDCP_ENC_DISABLE, hdmi_base + S5P_ENC_EN);
		
		while(! read_again_bksv()) {
			
			count ++;
			
			mdelay(20);

			if(count == 140)
				return false;
		}		
	}

	return true;
}

/*
 * Compare the 'P' value of Tx with that of Rx
 */
static bool compare_p_val(void)
{
	/* ref. to HDCP Revision v1.2 p.89 */
	int ret = 0;
	u8 tx_p_val1, rx_p_val1;
	u8 tx_p_val2, rx_p_val2;
	u32 count = 0;

	tx_p_val1 = readl(hdmi_base + S5P_HDCP_Pj);
	ret = hdcp_i2c_read(hdcp_info.client, HDCP_Pj, &rx_p_val1, 0x1);
	if(ret < 0){
		HDCPPRINTK("Can't read p_1 data from i2c bus\n");
		return false;
	}

	while (tx_p_val1 != rx_p_val1) {
		tx_p_val2 = readl(hdmi_base + S5P_HDCP_Pj);
		
		ret = hdcp_i2c_read(hdcp_info.client, HDCP_Pj, &rx_p_val2, 0x1);
		if(ret < 0){
			HDCPPRINTK("Can't read p_2 data from i2c bus\n");
			return false;
		}

		/* Check for stable Pj values */
		if ((tx_p_val1 == tx_p_val2) && (rx_p_val1 == rx_p_val2)) {
			// Count mismatches
			count++;   

			if (count >= 3)
				break;

			tx_p_val1 = tx_p_val2;
			rx_p_val1 = rx_p_val2;

			/* Wait for any of the values to change */
			do {
				tx_p_val2 = readl(hdmi_base + S5P_HDCP_Pj);
				ret = hdcp_i2c_read(hdcp_info.client, HDCP_Pj, 
							&rx_p_val2, 0x1);
				if(ret < 0){
					HDCPPRINTK("Can't read p_2 data" 
						"from i2c bus\n");
					return false;
				}
			} while ((tx_p_val2 == tx_p_val1) && 
				(rx_p_val2 == rx_p_val1));
		}

		tx_p_val1 = tx_p_val2;
		rx_p_val1 = rx_p_val2;

	}

	return ((count < 3) ? true : false);

}

/*
 * Compare the R value of Tx with that of Rx
 */
static bool compare_r_val(void)
{
	int ret = 0;
	u8 ri[2] = {0,0};
	u8 rj[2] = {0,0};
	u16 i;

	for (i = 0; i < R_VAL_RETRY_CNT; i++) {
		/* Read R value from Tx */
		ri[0] = readl(hdmi_base + S5P_HDCP_Ri_0);
		ri[1] = readl(hdmi_base + S5P_HDCP_Ri_1);

		/* Read R value from Rx */
		ret = hdcp_i2c_read(hdcp_info.client, HDCP_Ri, rj, 2);
		if(ret < 0){
			HDCPPRINTK("Can't read r data from i2c bus\n");
			return false;
		}

#ifdef S5P_HDCP_AUTH_DEBUG
		AUTHPRINTK("retries :: %d\n", i);
		printk("\t\t\t Rx(ddc) ->");
		printk("rj[0]: 0x%02x, rj[1]: 0x%02x\n", rj[0], rj[1]);
		printk("\t\t\t Tx(register) ->");
		printk("ri[0]: 0x%02x, ri[1]: 0x%02x\n", ri[0], ri[1]);
#endif

		/* Compare R value */
		if ((ri[0] == rj[0]) && (ri[1] == rj[1]) && (ri[0] | ri[1])) {
			writel(Ri_MATCH_RESULT__YES, 
				hdmi_base + S5P_HDCP_CHECK_RESULT);
			HDCPPRINTK("R0, R0' is matched!!\n");
			ret = true;
			break;
		} else {
			writel(Ri_MATCH_RESULT__NO, 
				hdmi_base + S5P_HDCP_CHECK_RESULT);
			HDCPPRINTK("R0, R0' is not matched!!\n");
			ret = false;
		}
		
	}

	return ret ? true:false;
}

/*
 * Reset Authentication
 */
static void reset_authentication(void)
{
	u32 hdmi_con0;

	hdmi_con0 = readl(hdmi_base + S5P_HDMI_CON_0);
	
	/* Disable encryption */
	writel(HDCP_ENC_DIS, hdmi_base + S5P_ENC_EN);
	/* Ri is not matched */
	writel(Ri_MATCH_RESULT__NO, hdmi_base + S5P_HDCP_CHECK_RESULT);
	/* Disable HDMI */
	writel(HDMI_DIS & hdmi_con0, hdmi_base + S5P_HDMI_CON_0);
	/* Enable HDMI */
	writel(HDMI_EN | hdmi_con0, hdmi_base + S5P_HDMI_CON_0);
	/* Init check result */
	writel(CLEAR_ALL_RESULTS, hdmi_base + S5P_HDCP_CHECK_RESULT);
}

static bool make_aes_key(void)
{
	u32  aes_reg_val;


	aes_reg_val = readl(hdmi_base + S5P_HAES_CON);
	aes_reg_val = SCRAMBLER_KEY_START_EN;

	/* Start generation of AES key */
	writel(aes_reg_val, hdmi_base + S5P_HAES_CON);

	do {
		aes_reg_val = readl(hdmi_base + S5P_HAES_CON);
	} while (!(aes_reg_val & SCRAMBLER_KEY_DONE));

	return true;

	/* for debugging AES key */
#ifdef HAES_DEBUG
	{
		u32  aes_key_val[32];
		u16 i;

		/* Enable to debug AES key after scrambling */
		aes_reg_val |= (0x1 << 5);

		writel(aes_reg_val, hdmi_base + S5P_HAES_CON);

		for (i = 0; i < 32; i++) {
			aes_key_val[i] = readl(hdmi_base + 
						(S5P_HAES_CON + 0x200 + 4 * i));
			HDCPPRINTK("HDCPAesKey Val[%d]: 0x%2x\n", 
				i, aes_key_val[i]);
		}
	}
#endif

}

static void set_av_mute_on_off(u32 on_off)
{
	u32  gcp_byte;

	HDCPPRINTK("state : %s \n\r", on_off ? 
			"av mute on":"av mute off");

	if (on_off == 1)
		gcp_byte = 0x01;
	else
		gcp_byte = 0x10;

	writel(gcp_byte, hdmi_base + S5P_GCP_BYTE1);

	/* 
	 * packet will be transmitted 
	 * within 384 cycles after active sync.
	 */
	writel(TRANSMIT_EVERY_VSYNC, hdmi_base + S5P_GCP_CON);

}

/*
 * HAES function
 */
static void start_decrypting(const u8 *hdcp_key, u32 hdcp_key_size)
{
	u32 i = 0;
	u32 aes_start = 0;
	u32 aes_reg_val = 0;

	make_aes_key();

	writel(hdcp_key_size, hdmi_base + S5P_HAES_DATA_SIZE_L);

	for (i = 0; i < hdcp_key_size; i++) {
		writel(hdcp_key[i], hdmi_base + S5P_HAES_DATA);
	}

	aes_reg_val = readl(hdmi_base + S5P_HAES_CON);
	aes_reg_val |= HAES_START_EN;
	
	writel(aes_reg_val, hdmi_base + S5P_HAES_CON);

	do {
		aes_start = readl(hdmi_base + S5P_HAES_CON);
	} while (aes_start & HAES_START_EN);

#ifdef HAES_DEBUG
	{
		u8 decrypted_data[288];

		HDCPPRINTK("[HDCP key] \n");

		printk("\t\t\t\t");
		for (i = 0; i < hdcp_key_size; i++) {

			printk("0x%02x, ", hdcp_key[i]);

			if((!(i%6)) && i)
				printk("\n\t\t\t\t");
		}

		/* Set AES debug mode */
		writel(0x1 << 0, hdmi_base + S5P_HAES_CON + 0x80);

		printk("\n");
		
		HDCPPRINTK("[Decrypted Data]\n");

		printk("\t\t\t\t");
		for (i = 0; i < hdcp_key_size; i++) {
			decrypted_data[i] = 
				readl(hdmi_base + S5P_HAES_CON + 0xb0);

			printk("0x%2x, ", decrypted_data[i]);
			
			if((!(i%6)) && i)
				printk("\n\t\t\t\t");
		}

		printk("\n");
	}
#endif
}

/*
 * Start encryption
 */
static void start_encryption(void)
{
	u32  hdcp_status;

	/* Ri == Ri' |Ready the compared result of Ri */
	writel(Ri_MATCH_RESULT__YES, hdmi_base + S5P_HDCP_CHECK_RESULT);

	do {
		hdcp_status = readl(hdmi_base + S5P_STATUS);
		/* Wait for STATUS[7] to '1'*/
	} while ((hdcp_status & AUTHENTICATED) != AUTHENTICATED);

	/* Start encryption */
	writel(HDCP_ENC_ENABLE, hdmi_base + S5P_ENC_EN);

}

/*
 * Check  whether Rx is repeater or not
 */
static int check_repeater(void)
{
	int ret = 0;

	u8 i = 0;
	u16 j = 0;
	
	u8 bcaps[BCAPS_SIZE] = {0};
	u8 status[BSTATUS_SIZE] = {0,0};
	u8 rx_v[SHA_1_HASH_SIZE];
	u8 ksv_list[HDCP_MAX_DEVS*HDCP_KSV_SIZE];
	
	u32 hdcp_ctrl;
	u32 dev_cnt;
	u32 stat;	

	u64 init_time;
	u64 end_time;
	u64 present_time;
	
	bool ksv_fifo_ready = false;

	memset(rx_v, 0x0, SHA_1_HASH_SIZE);
	memset(ksv_list, 0x0, HDCP_MAX_DEVS*HDCP_KSV_SIZE);
	
//	writel(0x0, hdmi_base+S5P_HDCP_AUTH_STATUS);
//	writel(0x0, hdmi_base+S5P_HDCP_RX_KSV_LIST_CTRL);
	
	/* Check KSV_FIFO Ready during 5 sec. */
//	init_time = jiffies;
	
//	end_time = jiffies + 5*HZ;

	while(j <= 500) {
		ret = hdcp_i2c_read(hdcp_info.client, HDCP_Bcaps, 
				bcaps, BCAPS_SIZE);

		if(ret < 0){
			HDCPPRINTK("Can't read bcaps data from i2c bus\n");
			return false;
		}

		if (bcaps[0] & KSV_FIFO_READY) {
			HDCPPRINTK("ksv fifo is ready\n");
			ksv_fifo_ready = true;
			writel(bcaps[0], hdmi_base + S5P_HDCP_BCAPS);
			break;
		}else{
			HDCPPRINTK("ksv fifo is not ready\n");
			ksv_fifo_ready = false;
			mdelay(10);
			j++;
		}
	
	}

	if (j == 500) {
		HDCPPRINTK("ksv fifo check timeout occurred!!\n");
		return false;
	}
	
	if (ksv_fifo_ready) {
		hdcp_ctrl = readl(hdmi_base + S5P_HDCP_CTRL);
		hdcp_ctrl &= CLEAR_REPEATER_TIMEOUT;
		writel(hdcp_ctrl, hdmi_base + S5P_HDCP_CTRL);
	} else
		return false;

	/*
	 * Check MAX_CASCADE_EXCEEDED 
	 * or MAX_DEVS_EXCEEDED indicator
	 */
	ret = hdcp_i2c_read(hdcp_info.client, HDCP_BStatus, 
		status, BSTATUS_SIZE);
	
	if(ret < 0){
		HDCPPRINTK("Can't read status data from i2c bus\n");
		return false;
	}
	
	/* MAX_CASCADE_EXCEEDED || MAX_DEVS_EXCEEDED */
	if (status[1] & MAX_CASCADE_EXCEEDED) {
		HDCPPRINTK("MAX_CASCADE_EXCEEDED\n");
		return MAX_CASCADE_EXCEEDED_ERROR;
	} else if(status[0] & MAX_DEVS_EXCEEDED) {
		HDCPPRINTK("MAX_CASCADE_EXCEEDED\n");
		return MAX_DEVS_EXCEEDED_ERROR;
	}

	//SPMOON - it's not in linux src
	//writel(Ri_MATCH_RESULT__YES, hdmi_base + S5P_HDCP_CHECK_RESULT);
	
	writel(status[0], hdmi_base + S5P_HDCP_BSTATUS_0);
	writel(status[1], hdmi_base + S5P_HDCP_BSTATUS_1);
		 
	/* Read KSV list */
	dev_cnt = (*status) & 0x7f;

	HDCPPRINTK("status[0] :0x%08x, status[1] :0x%08x!!\n",
		status[0],status[1]);

	if(dev_cnt) {

		u32 val;

		/* read ksv */
		ret = hdcp_i2c_read(hdcp_info.client, HDCP_KSVFIFO, ksv_list, 
				dev_cnt * HDCP_KSV_SIZE);
		if (ret <0) {
			HDCPPRINTK("Can't read ksv fifo!!\n");
			return false;
		}

		/* write ksv */
		for (i = 0; i < dev_cnt; i++) {
			
			writel(ksv_list[(i*5) + 0], hdmi_base + S5P_HDCP_RX_KSV_0_0);
			writel(ksv_list[(i*5) + 1], hdmi_base + S5P_HDCP_RX_KSV_0_1);
			writel(ksv_list[(i*5) + 2], hdmi_base + S5P_HDCP_RX_KSV_0_2);
			writel(ksv_list[(i*5) + 3], hdmi_base + S5P_HDCP_RX_KSV_0_3);
			writel(ksv_list[(i*5) + 4], hdmi_base + S5P_HDCP_RX_KSV_0_4);
			
			if ( i != (dev_cnt - 1)) { /* if it's not end */
				/* it's not in manual */
				writel(SET_HDCP_KSV_WRITE_DONE, S5P_HDCP_RX_KSV_LIST_CTRL);
				
				mdelay(20);

				/* check ksv readed */
				do
				{	
					if (!hdcp_info.hdcp_enable)
						return false;
					
					stat = readl(hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL);
					
				}while (!(stat & SET_HDCP_KSV_READ));
				
				
				HDCPPRINTK("read complete\n");

			}	

			HDCPPRINTK("HDCP_RX_KSV_1 = 0x%x\n\r",
				readl(hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL));
			HDCPPRINTK("i : %d, dev_cnt : %d, val = 0x%08x\n",i,dev_cnt,val);
		}

		/* end of ksv */
		val = readl(hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL);
		val |= SET_HDCP_KSV_END|SET_HDCP_KSV_WRITE_DONE;
		writel(val, hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL);

		HDCPPRINTK("HDCP_RX_KSV_1 = 0x%x\n\r",
				readl(hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL));
		HDCPPRINTK("i : %d, dev_cnt : %d, val = 0x%08x\n",i,dev_cnt,val);

	} else {
		
		//mdelay(200);
		
		writel(SET_HDCP_KSV_LIST_EMPTY, 
			hdmi_base + S5P_HDCP_RX_KSV_LIST_CTRL);
	}


	/* Read SHA-1 from receiver */
	ret = hdcp_i2c_read(hdcp_info.client, HDCP_SHA1, 
		rx_v, SHA_1_HASH_SIZE);
	
	if (ret < 0) {
		HDCPPRINTK("Can't read sha_1_hash data from i2c bus\n");
		return false;
	}

	for(i = 0; i < SHA_1_HASH_SIZE; i++) {
		HDCPPRINTK("SHA_1 rx :: %x\n",rx_v[i]);
	}

	/* write SHA-1 to register */
	writel(rx_v[0], hdmi_base + S5P_HDCP_RX_SHA1_0_0);
	writel(rx_v[1], hdmi_base + S5P_HDCP_RX_SHA1_0_1); 
	writel(rx_v[2], hdmi_base + S5P_HDCP_RX_SHA1_0_2);
	writel(rx_v[3], hdmi_base + S5P_HDCP_RX_SHA1_0_3);
	writel(rx_v[4], hdmi_base + S5P_HDCP_RX_SHA1_1_0); 
	writel(rx_v[5], hdmi_base + S5P_HDCP_RX_SHA1_1_1); 
	writel(rx_v[6], hdmi_base + S5P_HDCP_RX_SHA1_1_2); 
	writel(rx_v[7], hdmi_base + S5P_HDCP_RX_SHA1_1_3);  
	writel(rx_v[8], hdmi_base + S5P_HDCP_RX_SHA1_2_0); 
	writel(rx_v[9], hdmi_base + S5P_HDCP_RX_SHA1_2_1); 
	writel(rx_v[10], hdmi_base + S5P_HDCP_RX_SHA1_2_2);  
	writel(rx_v[11], hdmi_base + S5P_HDCP_RX_SHA1_2_3);  
	writel(rx_v[12], hdmi_base + S5P_HDCP_RX_SHA1_3_0); 
	writel(rx_v[13], hdmi_base + S5P_HDCP_RX_SHA1_3_1);  
	writel(rx_v[14], hdmi_base + S5P_HDCP_RX_SHA1_3_2); 
	writel(rx_v[15], hdmi_base + S5P_HDCP_RX_SHA1_3_3); 
	writel(rx_v[16], hdmi_base + S5P_HDCP_RX_SHA1_4_0);  
	writel(rx_v[17], hdmi_base + S5P_HDCP_RX_SHA1_4_1);  
	writel(rx_v[18], hdmi_base + S5P_HDCP_RX_SHA1_4_2); 
	writel(rx_v[19], hdmi_base + S5P_HDCP_RX_SHA1_4_3);  

	/* SHA write done, and wait for SHA computation being done */
	mdelay(1);
	
	/* check authentication success or not */
	stat = readl(hdmi_base + S5P_HDCP_AUTH_STATUS);
		
	HDCPPRINTK("auth status %d\n",stat);

	if (stat & SET_HDCP_SHA_VALID_READY) {

		HDCPPRINTK("SHA valid ready 0x%x \n\r",stat);

		stat = readl(hdmi_base + S5P_HDCP_AUTH_STATUS);

		if (stat & SET_HDCP_SHA_VALID) {

			HDCPPRINTK("SHA valid 0x%x \n\r",stat);
		
			ret = true;
		} else {
			HDCPPRINTK("SHA valid ready, but not valid 0x%x \n\r",stat);
			ret = false;
		}
		
	} else {
	
		HDCPPRINTK("SHA not ready 0x%x \n\r",stat);
		ret = false;
	}
		

	/* clear all validate bit */
	writel(0x0, hdmi_base + S5P_HDCP_AUTH_STATUS);

	return ret;

}

/*
 * Check whether the HDCP event occurred or not
 */
 /*
static bool __s5p_is_occurred_hdcp_event(void)
{
	u32  status_val;

	status_val = readl(hdmi_base + S5P_STATUS);

	return (((status_val == (0x1 << 0) || status_val == (0x1 << 1) ||
		  status_val == (0x1 << 2) || status_val == (0x1 << 3) ||
		  status_val == (0x1 << 4))) ? true : false);
}
*/

static bool try_read_receiver(void)
{
	u8 i = 0;
	bool ret = false;

	for(i = 0; i < 40; i++)	{
		
		mdelay(250);

		if (hdcp_info.auth_status != RECEIVER_READ_READY){

			HDCPPRINTK("hdcp stat. changed!!"
				"failed attempt no = %d\n\r",i);

			return false;
		}

		ret = read_bcaps();

		if (ret) {
			
			HDCPPRINTK("succeeded at attempt no= %d \n\r",i);
			
			return true;
			
		} else
			HDCPPRINTK("can't read bcaps!!"
				"failed attempt no=%d\n\r",i);
	}

	return false;
}


/*
 * stop  - stop functions are only called under running HDCP
 */
bool __s5p_stop_hdcp(void)
{
	u32  sfr_val;

	HDCPPRINTK("HDCP ftn. Stop!!\n");

	hdcp_protocol_status = 0;

	hdcp_info.time_out 	= INFINITE;
	hdcp_info.event 	= HDCP_EVENT_STOP;
	hdcp_info.auth_status 	= NOT_AUTHENTICATED;
	hdcp_info.hdcp_enable 	= false;

	//hdcp_info.client	= NULL;

	/* 3. disable hdcp control reg. */
	sfr_val = readl(hdmi_base + S5P_HDCP_CTRL);
	sfr_val &= ( ENABLE_1_DOT_1_FEATURE_DIS 
			& CLEAR_REPEATER_TIMEOUT 
			& EN_PJ_DIS
			& CP_DESIRED_DIS);
	writel(sfr_val, hdmi_base + S5P_HDCP_CTRL);

	/* 2. disable aes_data_size & haes_con reg. */
	sfr_val = readl(hdmi_base + S5P_HAES_CON);
	sfr_val &= SCRAMBLER_KEY_START_DIS;
	writel(sfr_val, hdmi_base + S5P_HAES_CON);

	/* 1-3. disable hdmi hpd reg. */
	writel(CABLE_UNPLUGGED, hdmi_base + S5P_HPD);

	/* 1-2. disable hdmi status enable reg. */
	sfr_val = readl(hdmi_base + S5P_STATUS_EN);
	sfr_val &= HDCP_STATUS_DIS_ALL;
 	writel(sfr_val, hdmi_base + S5P_STATUS_EN);

	/* 1-1. clear all status pending */
	sfr_val = readl(hdmi_base + S5P_STATUS);
	sfr_val |= HDCP_STATUS_EN_ALL;
	writel(sfr_val, hdmi_base + S5P_STATUS);

	/* disable encryption */
	writel(HDCP_ENC_DISABLE, hdmi_base + S5P_ENC_EN);

	/* clear result */
	writel(Ri_MATCH_RESULT__NO, hdmi_base + S5P_HDCP_CHECK_RESULT);
	writel(readl(hdmi_base + S5P_HDMI_CON_0) & HDMI_DIS, 
		hdmi_base + S5P_HDMI_CON_0);	
	writel(readl(hdmi_base + S5P_HDMI_CON_0) | HDMI_EN, 
		hdmi_base + S5P_HDMI_CON_0);	
	writel(CLEAR_ALL_RESULTS, hdmi_base + S5P_HDCP_CHECK_RESULT);

	/* hdmi disable */
	/*
	sfr_val = readl(hdmi_base + S5P_HDMI_CON_0);
	sfr_val &= ~(PWDN_ENB_NORMAL | HDMI_EN | ASP_EN);
	writel( sfr_val, hdmi_base + S5P_HDMI_CON_0);
	*/
	HDCPPRINTK( "\tSTATUS \t0x%08x\n",readl(hdmi_base + S5P_STATUS));
	HDCPPRINTK( "\tSTATUS_EN \t0x%08x\n",readl(hdmi_base + S5P_STATUS_EN));
	HDCPPRINTK( "\tHPD \t0x%08x\n",readl(hdmi_base + S5P_HPD));
	HDCPPRINTK( "\tHDCP_CTRL \t0x%08x\n",readl(hdmi_base + S5P_HDCP_CTRL));
	HDCPPRINTK( "\tMODE_SEL \t0x%08x\n",readl(hdmi_base + S5P_MODE_SEL));
	HDCPPRINTK( "\tENC_EN \t0x%08x\n",readl(hdmi_base + S5P_ENC_EN));	
	HDCPPRINTK( "\tHDMI_CON_0 \t0x%08x\n",readl(hdmi_base + S5P_HDMI_CON_0));	

	return true;
}


void __s5p_hdcp_reset(void)
{

	__s5p_stop_hdcp();

	hdcp_protocol_status = 2;

	HDCPPRINTK("HDCP ftn. reset!!\n");
	
	/* set SW HPD OFF and ON to initialize HDCP state machine */
//	writel(CABLE_UNPLUGGED, hdmi_base+S5P_HPD);
//	writel(CABLE_PLUGGED, hdmi_base+S5P_HPD);

	//writel(HDCP_ENC_ENABLE, hdmi_base + S5P_ENC_EN);

//	__s5p_start_hdcp();

}

/*
 * start  - start functions are only called under stopping HDCP
 */
bool __s5p_start_hdcp(void)
{		
	u32  sfr_val;
	
	hdcp_info.event 	= HDCP_EVENT_STOP;
	hdcp_info.time_out 	= INFINITE;
	hdcp_info.auth_status 	= NOT_AUTHENTICATED;	

	HDCPPRINTK("HDCP ftn. Start!!\n");

	hdcp_protocol_status = 1;
/*
	if (!hdcp_info.client) {
		HDCPPRINTK("DDC port is not available!!"
		       "Check hdmi receiver's DDC Port \n");
		return false;
	}		
*/
	if ( !read_bcaps()) {
		HDCPPRINTK("can't read ddc port!\n");
		__s5p_hdcp_reset();
//		return false;
	}
	
	/* for av mute */
	writel(DO_NOT_TRANSMIT, hdmi_base + S5P_GCP_CON);

	/* 
	 * 1-1. set hdmi status enable reg. 
	 * Update_Ri_int_en should be enabled after 
	 * s/w gets ExchangeKSV_int.
	 */
	writel(HDCP_STATUS_EN_ALL, hdmi_base + S5P_STATUS_EN);
	
	/* 1-2. set hdmi hpd reg. */
	writel(CABLE_PLUGGED, hdmi_base+S5P_HPD);

	/* 
	 * 1-3. set hdmi offset & cycle_aa reg. 
	 * HDCP memory read cycle count(0x4 is recommanded) 
	 */
	writel(0x00, hdmi_base + S5P_HDCP_OFFSET_TX_0);

	writel(0xA0, hdmi_base + S5P_HDCP_OFFSET_TX_1);

	writel(0x00, hdmi_base + S5P_HDCP_OFFSET_TX_2);

	writel(0x00, hdmi_base + S5P_HDCP_OFFSET_TX_3);

	writel(0x04, hdmi_base + S5P_HDCP_CYCLE_AA);
	
	/* 2. set aes_data_size & haes_con reg. */
	start_decrypting(hdcp_key, 288);
	
	/* 
	 * 3. set hdcp control reg.
	 * Disable advance cipher option, Enable CP(Content Protection),
	 * Disable time-out (This bit is only available in a REPEATER)
	 * Disable XOR shift,Disable Pj port update,Use external key
	 */
	sfr_val = 0;
	sfr_val |= CP_DESIRED_EN;
	writel(sfr_val, hdmi_base + S5P_HDCP_CTRL);
	
	hdcp_info.hdcp_enable = true;
	
	HDCPPRINTK( "\tSTATUS \t0x%08x\n",readl(hdmi_base + S5P_STATUS));
	HDCPPRINTK( "\tSTATUS_EN \t0x%08x\n",readl(hdmi_base + S5P_STATUS_EN));
	HDCPPRINTK( "\tHPD \t0x%08x\n",readl(hdmi_base + S5P_HPD));
	HDCPPRINTK( "\tHDCP_CTRL \t0x%08x\n",readl(hdmi_base + S5P_HDCP_CTRL));
	HDCPPRINTK( "\tMODE_SEL \t0x%08x\n",readl(hdmi_base + S5P_MODE_SEL));
	HDCPPRINTK( "\tENC_EN \t0x%08x\n",readl(hdmi_base + S5P_ENC_EN));
	HDCPPRINTK( "\tHDMI_CON_0 \t0x%08x\n",readl(hdmi_base + S5P_HDMI_CON_0));


	return true;
}


static void bksv_start_bh(void)
{
	bool ret = false;

	HDCPPRINTK("HDCP_EVENT_READ_BKSV_START bh\n");

	hdcp_info.auth_status = RECEIVER_READ_READY;

	ret = read_bcaps();

	if(!ret) {
		
		ret = try_read_receiver();
		
		if(!ret) {
			HDCPPRINTK("Can't read bcaps!! retry failed!!\n" 
				"\t\t\t\thdcp ftn. will be stopped\n");

			__s5p_stop_hdcp();
			return;	
		}
	}
		
	hdcp_info.auth_status = BCAPS_READ_DONE;

	ret = read_bksv();

	if(!ret) {
		HDCPPRINTK("Can't read bksv!!" 
			"hdcp ftn. will be reset\n");

		__s5p_stop_hdcp();
		return;
	}

	hdcp_info.auth_status = BKSV_READ_DONE;

	HDCPPRINTK("authentication status : bksv is done (0x%08x)\n", 
		hdcp_info.auth_status);
}

static void second_auth_start_bh(void)
{
	u8 count = 0;
	bool ret = false;

	int ret_err;
	
	u32 bcaps;

	HDCPPRINTK("HDCP_EVENT_SECOND_AUTH_START bh\n");

	ret = read_bcaps();
	
	if (!ret) {
		
		ret = try_read_receiver();
		
		if (!ret) {
			
			HDCPPRINTK("Can't read bcaps!! retry failed!!\n" 
				"\t\t\t\thdcp ftn. will be stopped\n");

			__s5p_stop_hdcp();
			return;
		}

	}

	bcaps = readl(hdmi_base + S5P_HDCP_BCAPS);
	bcaps &= (KSV_FIFO_READY);
	
	if (!bcaps) {
		
		HDCPPRINTK("ksv fifo is not ready\n");

		do {
			count ++;
			
			ret = read_bcaps();

			if (!ret) {
				
				ret = try_read_receiver();
				
				if (!ret) {
		
					__s5p_stop_hdcp();
				}
				
				return;

			}

			bcaps = readl(hdmi_base + S5P_HDCP_BCAPS);
			bcaps &= (KSV_FIFO_READY);

			if (bcaps) {
				HDCPPRINTK("bcaps retries : %d\n",count);
				break;
			}
			
			mdelay(100);
			
			if (!hdcp_info.hdcp_enable) {

				__s5p_stop_hdcp();
				
				return;

			}
			
		} while(count <= 50);
		
		/* wait times exceeded 5 seconds */
		if (count >50) {

			hdcp_info.time_out = INFINITE;

			/* time-out (This bit is only available in a REPEATER) */
			writel(readl(hdmi_base + S5P_HDCP_CTRL) | 0x1 << 2,
				hdmi_base + S5P_HDCP_CTRL);

			__s5p_hdcp_reset();

			return;
		}
	}

	HDCPPRINTK("ksv fifo ready\n");

	ret_err = check_repeater();

	if (ret_err == true) {
		u32 flag;
		
		hdcp_info.auth_status = SECOND_AUTHENTICATION_DONE;
		HDCPPRINTK("second authentication done!!\n");

		flag = readb(hdmi_base + S5P_STATUS);
		HDCPPRINTK("hdcp state : %s authenticated!!\n",
			flag & AUTHENTICATED ? "":"not not");
	

		start_encryption();
	} else if (ret_err == false) {
		/* i2c error */
		HDCPPRINTK("repeater check error!!\n");
		__s5p_hdcp_reset();
	} else {
		if (ret_err == REPEATER_ILLEGAL_DEVICE_ERROR) {
			/* 
			 * No need to start the HDCP 
			 * in case of invalid KSV (revocation case)
			 */
			HDCPPRINTK("illegal dev. error!!\n");	

			__s5p_stop_hdcp();
		} else {
			/* 
			 * MAX_CASCADE_EXCEEDED_ERROR
			 * MAX_DEVS_EXCEEDED_ERROR
			 */
			HDCPPRINTK("repeater check error(MAX_EXCEEDED)!!\n");
			__s5p_hdcp_reset();
		}
	}
}

static bool write_aksv_start_bh(void)
{
	bool ret = false;

	HDCPPRINTK("HDCP_EVENT_WRITE_AKSV_START bh\n");

	if (hdcp_info.auth_status != BKSV_READ_DONE) {
		HDCPPRINTK("bksv is not ready!!\n");
		return false;
	}

	ret = write_ainfo();
	if(!ret)
		return false;

	HDCPPRINTK("ainfo write done!!\n");	
	
	ret = write_an();
	if(!ret)
		return false;
	
	hdcp_info.auth_status = AN_WRITE_DONE;
	
	HDCPPRINTK("an write done!!\n");			
	
	ret = write_aksv();
	if(!ret)
		return false;

	/*
	 * Wait for 100ms. Transmitter must not read 
	 * Ro' value sooner than 100ms after writing 
	 * Aksv
	 */
	mdelay(100);

	hdcp_info.auth_status = AKSV_WRITE_DONE;

	HDCPPRINTK("aksv write done!!\n");

	return ret;			
}

static bool check_ri_start_bh(void)
{
	bool ret = false;


	HDCPPRINTK("HDCP_EVENT_CHECK_RI_START bh\n");

	if (hdcp_info.auth_status == AKSV_WRITE_DONE ||
	    hdcp_info.auth_status == FIRST_AUTHENTICATION_DONE ||
	    hdcp_info.auth_status == SECOND_AUTHENTICATION_DONE) {

		ret = compare_r_val();

		if (ret) {
			
			if (hdcp_info.auth_status == AKSV_WRITE_DONE) {
				/* 
				 * Check whether HDMI receiver is 
				 * repeater or not
				 */
				if (hdcp_info.is_repeater)
					hdcp_info.auth_status 
						= SECOND_AUTHENTICATION_RDY;
				else {
					hdcp_info.auth_status 
						= FIRST_AUTHENTICATION_DONE;
					start_encryption();
				}
			}
		
		} else {
		
			HDCPPRINTK("authentication reset\n");
#if 0
			/* Authentication reset step */
			writel(HDCP_ENC_DIS, hdmi_base + S5P_ENC_EN);
			writel(0x2, hdmi_base + S5P_HDCP_CHECK_RESULT);
			writel(0x0, hdmi_base + S5P_HDMI_CON_0);
			writel(0x3, hdmi_base + S5P_HDMI_CON_0);
			writel(0x0, hdmi_base + S5P_HDCP_CHECK_RESULT);
#endif
			__s5p_hdcp_reset();
		}

		HDCPPRINTK("auth_status = 0x%08x\n",
			hdcp_info.auth_status);

		
		return true;
	}

	HDCPPRINTK("aksv_write or first/second" 
		" authentication is not done\n");
	
	return false;
}

/* 
 * bottom half for hdmi interrupt
 *
 */
static void hdcp_work(void *arg)
{
	//HDCPPRINTK("event : 0x%08x \n\r", hdcp_info.event);

	/* 
	 * I2C int. was occurred 
	 * for reading Bksv and Bcaps
	 */
	
	if (hdcp_info.event & (1 << HDCP_EVENT_READ_BKSV_START)) {
		
		bksv_start_bh();

		/* clear event */
		//spin_lock_bh(&hdcp_info.lock);
		hdcp_info.event &= ~( 1 << HDCP_EVENT_READ_BKSV_START );
		//spin_unlock_bh(&hdcp_info.lock);
	}

	/* 
	 * Watchdog timer int. was occurred 
	 * for checking repeater
	 */
	if (hdcp_info.event & (1 << HDCP_EVENT_SECOND_AUTH_START)) {

		second_auth_start_bh();

		/* clear event */	
		//spin_lock_bh(&hdcp_info.lock);
		hdcp_info.event &= ~(1 << HDCP_EVENT_SECOND_AUTH_START);
		//spin_unlock_bh(&hdcp_info.lock);
	}

	/* 
	 * An_Write int. was occurred 
	 * for writing Ainfo, An and Aksv
	 */
	if (hdcp_info.event & (1 << HDCP_EVENT_WRITE_AKSV_START)) {

		write_aksv_start_bh();

		/* clear event */
		//spin_lock_bh(&hdcp_info.lock);
		hdcp_info.event  &= ~(1 << HDCP_EVENT_WRITE_AKSV_START);
		//spin_unlock_bh(&hdcp_info.lock);
	}		
		
	/* 
	 * Ri int. was occurred 
	 * for comparing Ri and Ri'(from HDMI sink) 
	 */
	if (hdcp_info.event & (1 << HDCP_EVENT_CHECK_RI_START)) {


		check_ri_start_bh();

		/* clear event */
		//spin_lock_bh(&hdcp_info.lock);
		hdcp_info.event &= ~(1 << HDCP_EVENT_CHECK_RI_START);
		//spin_unlock_bh(&hdcp_info.lock);
	}	

}

/*
bool __s5p_is_decrypting_done(void)
{
	SEQPRINTK("()++\n");
	
	return (((readl(hdmi_base + S5P_HAES_CON) & (0x1 << 0)) == 0x0) 
		? true : false);
}
*/

void __s5p_init_hdcp(bool hpd_status, struct i2c_client *ddc_port)
{

	HDCPPRINTK("HDCP ftn. Init!!\n");
	
	hdcp_info.client = ddc_port;
	
	/* for bh */
	INIT_WORK(&hdcp_info.work, (work_func_t)hdcp_work);

	init_waitqueue_head(&hdcp_info.waitq);

	/* for dev_dbg err. */
	spin_lock_init(&hdcp_info.lock);

}

#endif
/*
 * HDCP ISR.
 * If HDCP IRQ occurs, set hdcp_event and wake up the waitqueue.
 */
irqreturn_t __s5p_hdmi_irq(int irq, void *dev_id)
{
	u8 flag;
	u32 event;

	event = 0;

	/* check HDCP Status */
	flag = readb(hdmi_base + S5P_STATUS);
	HDCPPRINTK("irq_status : 0x%08x\n", readb(hdmi_base + S5P_STATUS));

	HDCPPRINTK("hdcp state : %s authenticated!!\n",
		flag & AUTHENTICATED ? "":"not not");

	spin_lock_irq(&hdcp_info.lock);
	
	/* 
	 * processing interrupt 
	 * interrupt processing seq. is firstly set event for workqueue,
	 * and interrupt pending clear. 'flag|' was used for preventing
	 * to clear AUTHEN_ACK.- it caused many problem. be careful.
	 */
	/* I2C INT */
	if (flag & WTFORACTIVERX_INT_OCCURRED) {
		event |= (1 << HDCP_EVENT_READ_BKSV_START);
		writeb(flag | WTFORACTIVERX_INT_OCCURRED, 
			hdmi_base + S5P_STATUS);
	}

	/* AN INT */
	if (flag & EXCHANGEKSV_INT_OCCURRED) {
		event |= (1 << HDCP_EVENT_WRITE_AKSV_START);
		writeb(flag | EXCHANGEKSV_INT_OCCURRED, 
			hdmi_base + S5P_STATUS);
	}

	/* RI INT */
	if (flag & UPDATE_RI_INT_OCCURRED) {
		event |= (1 << HDCP_EVENT_CHECK_RI_START);
		writeb(flag | UPDATE_RI_INT_OCCURRED, 
			hdmi_base + S5P_STATUS);
	}

	/* WATCHDOG INT */
	if (flag & WATCHDOG_INT_OCCURRED) {
		event |= (1 << HDCP_EVENT_SECOND_AUTH_START);
		writeb(flag | WATCHDOG_INT_OCCURRED, 
			hdmi_base + S5P_STATUS);
	}

	if (!event) {
		HDCPPRINTK("unknown irq.\n");
		return IRQ_HANDLED;
	}

	hdcp_info.event |= event;

	schedule_work(&hdcp_info.work);

	spin_unlock_irq(&hdcp_info.lock);

	return IRQ_HANDLED;
}

bool __s5p_set_hpd_detection(bool detection, bool hdcp_enabled, 
				struct i2c_client *client)
{
	u32 hpd_reg_val = 0;

	/* hdcp_enabled is status of tvout_sys */
	/*
	if (hdcp_enabled) {
		if (detection) {
			hdcp_info.client = client;
			__s5p_start_hdcp();
		} else {
			hdcp_info.client = NULL;
			__s5p_stop_hdcp();
		}
	} else {
	*/
	
	if (detection) 
		hpd_reg_val = CABLE_PLUGGED;
	else {
		hpd_reg_val = CABLE_UNPLUGGED;
	}
	
	writel(hpd_reg_val, hdmi_base + S5P_HPD);
	
	HDCPPRINTK("HPD status :: 0x%08x\n\r", 
		readl(hdmi_base + S5P_HPD));

	return true;
}

