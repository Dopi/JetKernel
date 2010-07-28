/* linux/drivers/media/video/samsung/tv20/s5p_tv_base.c
 *
 * Entry file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <mach/map.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "s5p_tv.h"

#ifdef COFIG_TVOUT_DBG
#define S5P_TV_BASE_DEBUG 1
#endif
//mkh:
//#define S5P_TV_BASE_DEBUG 1

#ifdef S5P_TV_BASE_DEBUG
#define BASEPRINTK(fmt, args...) \
	printk("[TVBASE] %s: " fmt, __FUNCTION__ , ## args)
#else
#define BASEPRINTK(fmt, args...)
#endif

#define TVOUT_CLK_INIT(dev, clk, name) 							\
	clk= clk_get(dev, name);							\
	if(clk == NULL) { 								\
		printk(KERN_ERR  "failed to find %s clock source\n", name);		\
		return -ENOENT;								\
	}										\
	clk_enable(clk)

#define TVOUT_IRQ_INIT(x, ret, dev, num, jump, ftn, m_name) 				\
	x = platform_get_irq(dev, num); 						\
	if (x <0 ) {									\
		printk(KERN_ERR  "failed to get %s irq resource\n", m_name);		\
		ret = -ENOENT; 								\
		goto jump;								\
	}										\
	ret = request_irq(x, ftn, IRQF_DISABLED, dev->name, dev) ;			\
	if (ret != 0) {									\
		printk(KERN_ERR  "failed to install %s irq (%d)\n", m_name, ret);	\
		goto jump;								\
	}										\
	while(0)



static int ref_count_tv;
static int ref_count_g0;
static int ref_count_g1;


static struct mutex	*mutex_for_fo = NULL;
static struct mutex	*mutex_for_i2c= NULL;	

s5p_tv_status 	s5ptv_status;
s5p_tv_vo 	s5ptv_overlay[2];

static struct work_struct ws_hpd;
spinlock_t slock_hpd;

static struct i2c_driver hdcp_i2c_driver;
static bool hdcp_i2c_drv_state = false;

const static u16 ignore[] = { I2C_CLIENT_END };
const static u16 normal_addr[] = {(S5P_HDCP_I2C_ADDR >> 1), I2C_CLIENT_END };
const static u16 *forces[] = { NULL };





static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_addr,
	.probe		= ignore,
	.ignore		= ignore,
	.forces		= forces,
};

/*
 * i2c client drv.  - register client drv
 */
static int hdcp_i2c_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;

	c = kzalloc(sizeof(*c), GFP_KERNEL);

	if (!c)
		return -ENOMEM;

	strcpy(c->name, "s5p_ddc_client");

	c->addr = addr;

	c->adapter = adap;

	c->driver = &hdcp_i2c_driver;

	s5ptv_status.hdcp_i2c_client = c;

	dev_info(&adap->dev, "s5p_ddc_client attached "
		"into s5p_ddc_port successfully\n");

	return i2c_attach_client(c);
}

static int hdcp_i2c_attach_adapter(struct i2c_adapter *adap)
{
	int ret = 0;

	ret = i2c_probe(adap, &addr_data, hdcp_i2c_attach);

	if (ret) {
		dev_err(&adap->dev, 
			"failed to attach s5p_hdcp_port driver\n");
		ret = -ENODEV;
	}

	return ret;
}

static int hdcp_i2c_detach(struct i2c_client *client)
{
	dev_info(&client->adapter->dev, "s5p_ddc_client detached "
		"from s5p_ddc_port successfully\n");

	i2c_detach_client(client);

	return 0;
}

static struct i2c_driver hdcp_i2c_driver = {
	.driver = {
		.name = "s5p_ddc_port",
	},
	.id = I2C_DRIVERID_S5P_HDCP,
	.attach_adapter = hdcp_i2c_attach_adapter,
	.detach_client = hdcp_i2c_detach,
};

/*
 * ftn for irq 
 */

/*
static irqreturn_t s5p_tvenc_irq(int irq, void *dev_id)
{

	printk("\n +++++++++++++TVIRQ\n");
	return IRQ_HANDLED;
}
*/

static void set_ddc_port(void)
{
	mutex_lock(mutex_for_i2c);
	
	if(s5ptv_status.hpd_status) {

		if (!hdcp_i2c_drv_state)
			/* cable : plugged, drv : unregistered */
			if (i2c_add_driver(&hdcp_i2c_driver))
				printk(KERN_INFO "HDCP port add failed\n");

		/* changed drv. status */
		hdcp_i2c_drv_state = true;
		

		/* cable inserted -> removed */
		__s5p_set_hpd_detection(true, s5ptv_status.hdcp_en, 
			s5ptv_status.hdcp_i2c_client);
		
	} else {

		if (hdcp_i2c_drv_state)
			/* cable : unplugged, drv : registered */
			i2c_del_driver(&hdcp_i2c_driver);
		
		/* changed drv. status */
		hdcp_i2c_drv_state = false;

		/* cable removed -> inserted */
		__s5p_set_hpd_detection(false, s5ptv_status.hdcp_en,
			s5ptv_status.hdcp_i2c_client);
	}
	
	mutex_unlock(mutex_for_i2c);
}

static irqreturn_t __s5p_hpd_irq(int irq, void *dev_id)
{



	if(gpio_get_value(S5PC11X_GPH1(5)))
	{
	printk("\n cable inserted \n");
	set_irq_type(IRQ_EINT13, IRQ_TYPE_EDGE_FALLING);
	}
	else
	{
	printk("\n cable removed\n");
	set_irq_type(IRQ_EINT13,IRQ_TYPE_EDGE_RISING);
	}

#if 0
	spin_lock_irq(&slock_hpd);
	
#ifdef CONFIG_CPU_S5PC110
//mkh:
s5ptv_status.hpd_status = gpio_get_value(S5PC11X_GPH0(5))
                ? false:true;
#else
	s5ptv_status.hpd_status = gpio_get_value(S5PC1XX_GPH0(5)) 
		? false:true;
#endif	
	if(s5ptv_status.hpd_status){
		
		set_irq_type(IRQ_EINT5, IRQ_TYPE_EDGE_RISING);
		
	}else{
		set_irq_type(IRQ_EINT5, IRQ_TYPE_EDGE_FALLING);			

	}

	if (s5ptv_status.hdcp_en)
		schedule_work(&ws_hpd);

	spin_unlock_irq(&slock_hpd);

	BASEPRINTK("hpd_status = %d\n", s5ptv_status.hpd_status);
#endif		
	return IRQ_HANDLED;
}
#ifdef CONFIG_CPU_S5PC110
#if 0
static int tv_phy_power( bool on )
{
#if 0
	if (on) {
		/* on */
		clk_enable(s5ptv_status.i2c_phy_clk);
		
		__s5p_hdmi_phy_power(true);		
		
	} else {
		/* 
		 * for preventing hdmi hang up when restart 
		 * switch to internal clk - SCLK_DAC, SCLK_PIXEL 
		 */
		__s5p_tv_clk_change_internal();
			
		__s5p_hdmi_phy_power(false);
		
		clk_disable(s5ptv_status.i2c_phy_clk);
	}
	
#endif
	return 0;
}
#endif
int s5p_tv_clk_gate( bool on )
{
	if (on) {
		clk_enable(s5ptv_status.vp_clk);
		clk_enable(s5ptv_status.mixer_clk);
		clk_enable(s5ptv_status.tvenc_clk);
		clk_enable(s5ptv_status.hdmi_clk);
			
	} else {
	
		/* off */
		clk_disable(s5ptv_status.vp_clk);
		clk_disable(s5ptv_status.mixer_clk);
		clk_disable(s5ptv_status.tvenc_clk);
		clk_disable(s5ptv_status.hdmi_clk);

	}

	return 0;
}
EXPORT_SYMBOL(s5p_tv_clk_gate);
static int __devinit tv_clk_get(struct platform_device *pdev, struct _s5p_tv_status *ctrl)
{
	/* tvenc clk */
	ctrl->tvenc_clk = clk_get(&pdev->dev, "tvenc");

	if(IS_ERR(ctrl->tvenc_clk)) { 							
		printk(KERN_ERR  "failed to find %s clock source\n", "tvenc");	
		return -ENOENT;							
	}								

	/* vp clk */
	ctrl->vp_clk = clk_get(&pdev->dev, "vp");

	if(IS_ERR(ctrl->vp_clk)) { 							
		printk(KERN_ERR  "failed to find %s clock source\n", "vp");	
		return -ENOENT;							
	}								

	/* mixer clk */
	ctrl->mixer_clk = clk_get(&pdev->dev, "mixer");

	if(IS_ERR(ctrl->mixer_clk)) { 							
		printk(KERN_ERR  "failed to find %s clock source\n", "mixer");	
		return -ENOENT;							
	}								

	/* hdmi clk */
	ctrl->hdmi_clk = clk_get(&pdev->dev, "hdmi");

	if(IS_ERR(ctrl->hdmi_clk)) { 							
		printk(KERN_ERR  "failed to find %s clock source\n", "hdmi");	
		return -ENOENT;							
	}	

	#if 0
	/* i2c-hdmiphy clk */
	ctrl->i2c_phy_clk= clk_get(&pdev->dev, "i2c-hdmiphy");

	if(IS_ERR(ctrl->i2c_phy_clk)) { 							
		printk(KERN_ERR  "failed to find %s clock source\n", "i2c-hdmiphy");	
		return -ENOENT;							
	}	
	#endif	

	return 0;
}
#else
#define s5p_tv_clk_gate NULL
#define tv_phy_power NULL
#define tv_clk_get NULL
#endif

/*
 * ftn for video
 */
static int s5p_tv_v_open(struct file *file)
{
	int ret = 0,err;
	ref_count_tv ++ ;
	mutex_lock(mutex_for_fo);

	if (s5ptv_status.tvout_output_enable) {
		mutex_unlock(mutex_for_fo);
		goto re_open;
		BASEPRINTK("tvout drv. already used !!\n");
		ret =  -EBUSY;
		goto drv_used;
	}

#ifdef CONFIG_CPU_S5PC110
	s5p_tv_clk_gate( true );
#endif
	
#ifdef CONFIG_CPU_S5PC110

	err = gpio_request(S5PC11X_GPJ4(4),"TV_EN");
	udelay(50);
	gpio_direction_output(S5PC11X_GPJ4(4),1);
	gpio_set_value(S5PC11X_GPJ4(4),1);
	udelay(50);

	err = gpio_request(S5PC11X_GPJ2(6),"EAR_SEL");
	udelay(50);
	gpio_direction_output(S5PC11X_GPJ2(6),0);
	gpio_set_value(S5PC11X_GPJ2(6),0);
	udelay(50);
#endif

	_s5p_tv_if_init_param();

	s5p_tv_v4l2_init_param();

	mutex_unlock(mutex_for_fo);
	#if 0
	mutex_lock(mutex_for_i2c);
	/* for ddc(hdcp port) */
	if(s5ptv_status.hpd_status) {
		if (i2c_add_driver(&hdcp_i2c_driver)) 
			BASEPRINTK("HDCP port add failed\n");
		hdcp_i2c_drv_state = true;
	} else 
		hdcp_i2c_drv_state = false;

	mutex_unlock(mutex_for_i2c);
	#endif
	printk("\n\nTV open success\n\n");
re_open:
	/* for i2c probing */
	udelay(100);
	
	return 0;

drv_used:
	mutex_unlock(mutex_for_fo);
	return ret;
}

int s5p_tv_v_read(struct file *filp, char *buf, size_t count,
		  loff_t *f_pos)
{
	return 0;
}

int s5p_tv_v_write(struct file *filp, const char *buf, size_t
		   count, loff_t *f_pos)
{
	return 0;
}

int s5p_tv_v_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

int s5p_tv_v_release(struct file *filp)
{
#if 1 
	ref_count_tv -- ;

   if(ref_count_tv <= 0)
    {

	_s5p_vlayer_stop();
	_s5p_tv_if_stop();

	s5ptv_status.hdcp_en = false;

	s5ptv_status.tvout_output_enable = false;

	/* 
	 * drv. release
	 *        - just check drv. state reg. or not.
	 */
	mutex_lock(mutex_for_i2c);
	
	if (hdcp_i2c_drv_state) {
		i2c_del_driver(&hdcp_i2c_driver);
		hdcp_i2c_drv_state = false;
	}

	mutex_unlock(mutex_for_i2c);
	
	#ifdef CONFIG_CPU_S5PC110
	s5p_tv_clk_gate(false);
	//tv_phy_power( false );
	#endif
    }


#endif

	return 0;
}

/*
 * ftn for graphic(video output overlay)
 */
 /*
static int check_layer(dev_t dev)
{
	int id = 0;
	int layer = 0;

	id = MINOR(dev);

	if (id < TVOUT_MINOR_GRP0 || id > TVOUT_MINOR_GRP1)
		BASEPRINTK("grp layer invalid\n");
		
	layer = (id == TVOUT_MINOR_GRP0) ? 0:1;

	return layer;

}
*/

static int vo_open(int layer, struct file *file)
{
	int ret = 0;
	mutex_lock(mutex_for_fo);

	/* check tvout path available!! */
	if (!s5ptv_status.tvout_output_enable) {
		BASEPRINTK("check tvout start !!\n");
		ret =  -EACCES;
		goto resource_busy;
	}

	/*
	if (s5ptv_status.grp_layer_enable[layer]) {
		BASEPRINTK("grp %d layer is busy!!\n", layer);
		ret =  -EBUSY;
		goto resource_busy;
	}
	*/

	/* set layer info.!! */
	s5ptv_overlay[layer].index = layer;

	/* set file private data.!! */
	file->private_data = &s5ptv_overlay[layer];

	mutex_unlock(mutex_for_fo);

	return 0;

resource_busy:
	mutex_unlock(mutex_for_fo);

	return ret;
}

int vo_release(int layer, struct file *filp)
{
	_s5p_grp_stop(layer);

	return 0;
}

/* modified for 2.6.29 v4l2-dev.c */
static int s5p_tv_vo0_open(struct file *file)
{
	ref_count_g0++;
	vo_open(0, file);
	return 0;
}

static int s5p_tv_vo0_release(struct file *file)
{
	ref_count_g0 --;
	
	if(ref_count_g0 <=0)
	vo_release(0,file);

	return 0;
}

static int s5p_tv_vo1_open(struct file *file)
{
	ref_count_g1++;
	vo_open(1, file);
	return 0;	
}

static int s5p_tv_vo1_release(struct file *file)
{	
	ref_count_g1 --;	

	if(ref_count_g1 <= 0)
	vo_release(1,file);

	return 0;	
}

/*
 * struct for video
 */
static struct v4l2_file_operations s5p_tv_v_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_tv_v_open,
	.read		= s5p_tv_v_read,
	.write		= s5p_tv_v_write,
	.ioctl		= s5p_tv_v_ioctl,
	.mmap		= s5p_tv_v_mmap,
	.release	= s5p_tv_v_release
};

/*
 * struct for graphic0
 */
static struct v4l2_file_operations s5p_tv_vo0_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_tv_vo0_open,
	.ioctl		= s5p_tv_vo_ioctl,
	.release	= s5p_tv_vo0_release
};

/*
 * struct for graphic1
 */
static struct v4l2_file_operations s5p_tv_vo1_fops = {
	.owner		= THIS_MODULE,
	.open		= s5p_tv_vo1_open,
	.ioctl		= s5p_tv_vo_ioctl,
	.release	= s5p_tv_vo1_release
};


void s5p_tv_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device s5p_tvout[S5P_TVMAX_CTRLS] = {
	[0] = {
		.name = "S5PC1xx TVOUT for Video",
		//.type2 = V4L2_CAP_VIDEO_OUTPUT,
		.fops = &s5p_tv_v_fops,
		.ioctl_ops = &s5p_tv_v4l2_v_ops,
		.release  = s5p_tv_vdev_release,
		.minor = TVOUT_MINOR_VIDEO,
		.tvnorms = V4L2_STD_ALL_HD,
	},
	[1] = {
		.name = "S5PC1xx TVOUT Overlay0",
		//.type2 = V4L2_CAP_VIDEO_OUTPUT_OVERLAY,
		.fops = &s5p_tv_vo0_fops,
		.ioctl_ops = &s5p_tv_v4l2_vo_ops,
		.release  = s5p_tv_vdev_release,
		.minor = TVOUT_MINOR_GRP0,
		.tvnorms = V4L2_STD_ALL_HD,
	},
	[2] = {
		.name = "S5PC1xx TVOUT Overlay1",
		//.type2 = V4L2_CAP_VIDEO_OUTPUT_OVERLAY,
		.fops = &s5p_tv_vo1_fops,
		.ioctl_ops = &s5p_tv_v4l2_vo_ops,
		.release  = s5p_tv_vdev_release,
		.minor = TVOUT_MINOR_GRP1,
		.tvnorms = V4L2_STD_ALL_HD,
	},
};

/*
 *  Probe
 */
static int __init s5p_tv_probe(struct platform_device *pdev)
{
	int 	irq_num;
	int 	ret;
	int 	i;
	int 	err;

	ref_count_tv = 0;
	ref_count_g0 = 0;
	ref_count_g1 = 0;
	__s5p_sdout_probe(pdev, 0);
	__s5p_vp_probe(pdev, 1);	
	__s5p_mixer_probe(pdev, 2);

#ifdef CONFIG_CPU_S5PC110	
	tv_clk_get(pdev, &s5ptv_status);
	s5p_tv_clk_gate( true );
#endif
#ifdef CONFIG_CPU_S5PC110	
	__s5p_hdmi_probe(pdev, 3, 4);
#endif

#ifdef CONFIG_CPU_S5PC100	
	__s5p_hdmi_probe(pdev, 3);
	__s5p_tvclk_probe(pdev, 4);
#endif

	/* for dev_dbg err. */

	/* clock */



#ifdef FIX_27M_UNSTABLE_ISSUE /* for smdkc100 pop */
	writel(0x1, S5PC1XX_GPA0_BASE + 0x56c);
#endif
	spin_lock_init(&slock_hpd);
	/* for bh */
	INIT_WORK(&ws_hpd, (void *)set_ddc_port);

	/* check EINT init state */
#ifdef CONFIG_SMDKC110_BOARD
 	s3c_gpio_cfgpin(S5PC11X_GPH1(5), S5PC11X_GPH1_5_HDMI_HPD);
        s3c_gpio_setpull(S5PC11X_GPH1(5), S3C_GPIO_PULL_DOWN);
          
	s5ptv_status.hpd_status = gpio_get_value(S5PC11X_GPH1(5))
                                     ? false:true;

	dev_info(&pdev->dev, "hpd status is cable %s\n", 
		s5ptv_status.hpd_status ? "inserted":"removed");
	
#endif
	/* interrupt */
	TVOUT_IRQ_INIT(irq_num, ret, pdev, 0, out, __s5p_mixer_irq, "mixer");
	TVOUT_IRQ_INIT(irq_num, ret, pdev, 1, out_hdmi_irq, __s5p_hdmi_irq , "hdmi");
	TVOUT_IRQ_INIT(irq_num, ret, pdev, 2, out_tvenc_irq, s5p_tvenc_irq, "tvenc");
	TVOUT_IRQ_INIT(irq_num, ret, pdev, 3, out_hpd_irq, __s5p_hpd_irq, "hpd");

#ifdef CONFIG_S5PC110_JUPITER_BOARD 
	set_irq_type(IRQ_EINT13, IRQ_TYPE_EDGE_RISING);
#endif
	/* v4l2 video device registration */
	for (i = 0;i < S5P_TVMAX_CTRLS;i++) {
		s5ptv_status.video_dev[i] = &s5p_tvout[i];

		if (video_register_device(s5ptv_status.video_dev[i],
				VFL_TYPE_GRABBER, s5p_tvout[i].minor) != 0) {
				
			dev_err(&pdev->dev, 
				"Couldn't register tvout driver.\n");
			return 0;
		}
		else
			dev_info(&pdev->dev, "%s registered successfully \n", s5p_tvout[i].name);
	}
//mkh:
//__s5p_hdmi_init_hpd_onoff(1);
	mutex_for_fo = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);

	if (mutex_for_fo == NULL) {
		dev_err(&pdev->dev, 
			"failed to create mutex handle\n");
		goto out;
	}

	mutex_for_i2c= (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	
	if (mutex_for_i2c == NULL) {
		dev_err(&pdev->dev, 
			"failed to create mutex handle\n");
		goto out;
	}
	#ifdef CONFIG_CPU_S5PC110
	s5p_tv_clk_gate(false);
	#endif
	mutex_init(mutex_for_fo);
	mutex_init(mutex_for_i2c);
	
	return 0;

out_hpd_irq:
	free_irq(IRQ_TVENC, pdev);

out_tvenc_irq:
	free_irq(IRQ_HDMI, pdev);

out_hdmi_irq:
	free_irq(IRQ_MIXER, pdev);

out:
	printk(KERN_ERR "not found (%d). \n", ret);

	return ret;
}

/*
 *  Remove
 */
static int s5p_tv_remove(struct platform_device *pdev)
{
	__s5p_hdmi_release(pdev);
	__s5p_sdout_release(pdev);
	__s5p_mixer_release(pdev);
	__s5p_vp_release(pdev);
#ifdef CONFIG_CPU_S5PC100	
	__s5p_tvclk_release(pdev);
#endif
	i2c_del_driver(&hdcp_i2c_driver);

	clk_disable(s5ptv_status.tvenc_clk);
	clk_disable(s5ptv_status.vp_clk);
	clk_disable(s5ptv_status.mixer_clk);
	clk_disable(s5ptv_status.hdmi_clk);
	clk_disable(s5ptv_status.sclk_hdmi);
	clk_disable(s5ptv_status.sclk_mixer);
	clk_disable(s5ptv_status.sclk_tv);

	clk_put(s5ptv_status.tvenc_clk);
	clk_put(s5ptv_status.vp_clk);
	clk_put(s5ptv_status.mixer_clk);
	clk_put(s5ptv_status.hdmi_clk);
	clk_put(s5ptv_status.sclk_hdmi);
	clk_put(s5ptv_status.sclk_mixer);
	clk_put(s5ptv_status.sclk_tv);

	free_irq(IRQ_MIXER, pdev);
	free_irq(IRQ_HDMI, pdev);
	free_irq(IRQ_TVENC, pdev);
	free_irq(IRQ_EINT5, pdev);

	mutex_destroy(mutex_for_fo);
	mutex_destroy(mutex_for_i2c);

	return 0;
}


/*
 *  Suspend
 */
int s5p_tv_suspend(struct platform_device *dev, pm_message_t state)
{
	/* video layer stop */
        if ( s5ptv_status.vp_layer_enable ) {
	//__s5p_vm_save_reg();
	_s5p_vlayer_stop();
                s5ptv_status.vp_layer_enable = true;
        }
        if ( s5ptv_status.grp_layer_enable[0] ) {
                _s5p_grp_stop(VM_GPR0_LAYER);
                s5ptv_status.grp_layer_enable[VM_GPR0_LAYER] = true;
        }
        if ( s5ptv_status.grp_layer_enable[1] ) {
                _s5p_grp_stop(VM_GPR1_LAYER);
                s5ptv_status.grp_layer_enable[VM_GPR0_LAYER] = true;
        }
        if ( s5ptv_status.tvout_output_enable ) {
	_s5p_tv_if_stop();
                s5ptv_status.tvout_output_enable = true;
                s5ptv_status.tvout_param_available = true;
        }
        s5p_tv_clk_gate( false );
        //tv_phy_power( false );

	return 0;
}

/*
 *  Resume
 */
int s5p_tv_resume(struct platform_device *dev)
{
	/* clk & power on */
 
        s5p_tv_clk_gate( true );
        //tv_phy_power( true );
        if ( s5ptv_status.tvout_output_enable )
                _s5p_tv_if_start();

        if ( s5ptv_status.vp_layer_enable )
                _s5p_vlayer_start();

        /* grp0 layer start */
        if ( s5ptv_status.grp_layer_enable[0] )
                _s5p_grp_start(VM_GPR0_LAYER);
        if ( s5ptv_status.grp_layer_enable[1] )
                _s5p_grp_start(VM_GPR1_LAYER);
	
	return 0;
}

static struct platform_driver s5p_tv_driver = {
	.probe		= s5p_tv_probe,
	.remove		= s5p_tv_remove,
	
	#ifdef CONFIG_PM
	.suspend	= s5p_tv_suspend,
	.resume		= s5p_tv_resume,
	#else
	.suspend 	= NULL,
	.resume  	= NULL,
	#endif
	.driver		= {
		.name	= "s5p-tvout",
		.owner	= THIS_MODULE,
	},
};

static char banner[] __initdata = KERN_INFO "S5PC1XX TVOUT Driver, (c) 2009 Samsung Electronics\n";

int __init s5p_tv_init(void)
{
	int ret;

	printk(banner);

	ret = platform_driver_register(&s5p_tv_driver);

	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);
		return -1;
	}

	return 0;
}

static void __exit s5p_tv_exit(void)
{
	platform_driver_unregister(&s5p_tv_driver);
}

module_init(s5p_tv_init);
module_exit(s5p_tv_exit);

MODULE_AUTHOR("SangPil Moon");
MODULE_DESCRIPTION("SS5PC1XX TVOUT driver");
MODULE_LICENSE("GPL");
