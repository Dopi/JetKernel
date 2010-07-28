/* linux/drivers/media/video/samsung/rotator/s3c_rotator.c
 *
 * Driver file for Samsung Image Rotator
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/errno.h> /* error codes */
#include <linux/tty.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/hardware.h>	
#include <asm/uaccess.h>
#include <mach/map.h>	
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/irq.h>
#include <linux/semaphore.h>	
#include <asm/div64.h>
#include <plat/regs-rotator.h>

#include "s3c_rotator_common.h"

struct s3c_rotator_ctrl	s3c_rot;

static int s3c_rotator_irq_num = NO_IRQ;
static struct resource *s3c_rotator_mem;
static void __iomem *s3c_rotator_base;

static wait_queue_head_t waitq_rotator;

static struct mutex *h_rot_mutex;

static inline void s3c_rotator_set_source(ro_params *params)
{
	__raw_writel(S3C_ROT_SRC_HEIGHT(params->src_height) | 
			S3C_ROT_SRC_WIDTH( params->src_width) , s3c_rotator_base + S3C_ROTATOR_SRCSIZEREG);
	__raw_writel(params->src_addr_rgb_y, s3c_rotator_base + S3C_ROTATOR_SRCADDRREG0);
	__raw_writel(params->src_addr_cb, s3c_rotator_base + S3C_ROTATOR_SRCADDRREG1);
	__raw_writel(params->src_addr_cr, s3c_rotator_base + S3C_ROTATOR_SRCADDRREG2);    
}


static inline void s3c_rotator_set_dest(ro_params *params)
{
	__raw_writel(params->dst_addr_rgb_y, s3c_rotator_base + S3C_ROTATOR_DESTADDRREG0);
	__raw_writel(params->dst_addr_cb, s3c_rotator_base + S3C_ROTATOR_DESTADDRREG1);
	__raw_writel(params->dst_addr_cr, s3c_rotator_base + S3C_ROTATOR_DESTADDRREG2);    
}


static inline void s3c_rotator_start(ro_params *params, unsigned mode)
{
	u32 cfg = 0;

	cfg = __raw_readl(s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
	cfg &= ~S3C_ROTATOR_CTRLREG_MASK;
	cfg |= params->src_format | mode;

	__raw_writel(cfg|S3C_ROTATOR_CTRLCFG_START_ROTATE, s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
}


static inline unsigned int s3c_rotator_get_status(void)
{
	unsigned int cfg = 0;
	
	cfg = __raw_readl(s3c_rotator_base + S3C_ROTATOR_STATCFG);
	cfg &= S3C_ROTATOR_STATCFG_STATUS_BUSY_MORE;

	return cfg;
}


static void s3c_rotator_enable_int(void)
{
	unsigned int cfg;

	cfg = __raw_readl(s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
	cfg |= S3C_ROTATOR_CTRLCFG_ENABLE_INT;

	__raw_writel(cfg, s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
}


static void s3c_rotator_disable_int(void)
{
	unsigned int cfg;

	cfg = __raw_readl(s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
	cfg &=~ S3C_ROTATOR_CTRLCFG_ENABLE_INT;

	__raw_writel(cfg, s3c_rotator_base + S3C_ROTATOR_CTRLCFG);
}

#if defined(CONFIG_CPU_S3C6410)
irqreturn_t s3c_rotator_irq(int irq, void *dev_id)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;

	__raw_readl(s3c_rotator_base + S3C_ROTATOR_STATCFG);

	ctrl->status = ROT_IDLE;

	wake_up_interruptible(&waitq_rotator);

	return IRQ_HANDLED;
}
#elif defined(CONFIG_CPU_S5PC100)
irqreturn_t s3c_rotator_irq(int irq, void *dev_id)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	unsigned int cfg;

	cfg = __raw_readl(s3c_rotator_base + S3C_ROTATOR_STATCFG);
	cfg |= S3C_ROTATOR_STATCFG_INT_PEND;

	__raw_writel(cfg, s3c_rotator_base + S3C_ROTATOR_STATCFG);

	ctrl->status = ROT_IDLE;
	wake_up_interruptible(&waitq_rotator);

	return IRQ_HANDLED;
}
#endif

int s3c_rotator_open(struct inode *inode, struct file *file)
{
	ro_params	*params;

	// allocating the rotator instance
	params	= (ro_params *)kmalloc(sizeof(ro_params), GFP_KERNEL);
	if (params == NULL) {
		printk(KERN_ERR "Instance memory allocation was failed\n");
		return -ENOMEM;
	}

	memset(params, 0, sizeof(ro_params));

	file->private_data	= (ro_params *)params;

	s3c_rotator_enable_int();

	return 0;
}


int s3c_rotator_release(struct inode *inode, struct file *file)
{
	ro_params	*params;

	params	= (ro_params *)file->private_data;
	if (params == NULL) {
		printk(KERN_ERR "Can't release s3c_rotator!!\n");
		return -1;
	}

	kfree(params);

	s3c_rotator_disable_int();	

	return 0;
}


static int s3c_rotator_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	ro_params *params;
	ro_params *parg;
	unsigned int mode, divisor = 0;

	if (ctrl->status != ROT_IDLE) {
		printk(KERN_ERR "Rotator is busy.\n");
		return -EBUSY;
	}

	mutex_lock(h_rot_mutex);

	params	        = (ro_params *)file->private_data;
	parg	        = (ro_params *)arg;    

	get_user(params->src_width,     &parg->src_width);
	get_user(params->src_height,    &parg->src_height);

	get_user(params->src_format,    &parg->src_format);
	get_user(params->src_addr_rgb_y,&parg->src_addr_rgb_y);
	get_user(params->src_addr_cb,   &parg->src_addr_cb);
	get_user(params->src_addr_cr,   &parg->src_addr_cr);    

	get_user(params->dst_addr_rgb_y,&parg->dst_addr_rgb_y);    
	get_user(params->dst_addr_cb,   &parg->dst_addr_cb);    
	get_user(params->dst_addr_cr,   &parg->dst_addr_cr);    

	if( (params->src_width > 2048) || (params->src_height > 2048)) {
		printk(KERN_ERR "\n%s: maximum width and height size are 2048\n", __FUNCTION__);
		return -EINVAL;
	}

	switch(params->src_format) {
	case S3C_ROTATOR_CTRLCFG_INPUT_YUV420:
		divisor = 8;
		break;

	case S3C_ROTATOR_CTRLCFG_INPUT_YUV422:	/* fall through */
	case S3C_ROTATOR_CTRLCFG_INPUT_RGB565:	
		divisor = 2;
		break;

	case S3C_ROTATOR_CTRLCFG_INPUT_RGB888:
		divisor = 1;
		break;

	default :
		printk(KERN_ERR "requested src type is not supported!! plz check src format!!\n");
		break;
	}
	
	if((params->src_width % divisor) || (params->src_height % divisor)) {
		printk(KERN_ERR "\n%s: src & dst size is aligned to %d pixel boundary\n", __FUNCTION__, divisor);
		mutex_unlock(h_rot_mutex);
		return -EINVAL;
	}

	switch(cmd) {
	case ROTATOR_90:   
		mode = S3C_ROTATOR_CTRLCFG_DEGREE_90    | S3C_ROTATOR_CTRLCFG_FLIP_BYPASS;
		break;

	case ROTATOR_180:   
		mode = S3C_ROTATOR_CTRLCFG_DEGREE_180   | S3C_ROTATOR_CTRLCFG_FLIP_BYPASS;
		break;

	case ROTATOR_270:   
		mode = S3C_ROTATOR_CTRLCFG_DEGREE_270   | S3C_ROTATOR_CTRLCFG_FLIP_BYPASS;
		break;

	case HFLIP:   
		mode = S3C_ROTATOR_CTRLCFG_DEGREE_BYPASS| S3C_ROTATOR_CTRLCFG_FLIP_HOR;
		break;

	case VFLIP:   
		mode = S3C_ROTATOR_CTRLCFG_DEGREE_BYPASS| S3C_ROTATOR_CTRLCFG_FLIP_VER;
		break;

	default:
		return -EINVAL;
	}

	s3c_rotator_set_source(params);
	s3c_rotator_set_dest(params);
	s3c_rotator_start(params, mode);

	ctrl->status = ROT_RUN;

	if(!(file->f_flags & O_NONBLOCK)) {
		if (interruptible_sleep_on_timeout(&waitq_rotator, ROTATOR_TIMEOUT) == 0) {
			printk(KERN_ERR "\n%s: Waiting for interrupt is timeout\n", __FUNCTION__);
		}
	}

	mutex_unlock(h_rot_mutex);

	return 0;
}


static unsigned int s3c_rotator_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &waitq_rotator, wait);

	if (S3C_ROTATOR_IDLE == s3c_rotator_get_status()) {
		mask = POLLOUT|POLLWRNORM;
	}

	return mask;
}


struct file_operations s3c_rotator_fops = {
	.owner      = THIS_MODULE,
	.open       = s3c_rotator_open,
	.release    = s3c_rotator_release,
	.ioctl      = s3c_rotator_ioctl,
	.poll       = s3c_rotator_poll,
};


static struct miscdevice s3c_rotator_dev = {
	.minor		= ROTATOR_MINOR,
	.name		= "s3c-rotator",
	.fops		= &s3c_rotator_fops,
};


int s3c_rotator_probe(struct platform_device *pdev)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	struct resource		*res;
	int			ret;

	printk(KERN_INFO "s3c_rotator_probe called\n");

	/* Clock setting */
	sprintf(ctrl->clk_name, "%s", S3C_ROT_CLK_NAME);

	ctrl->clock = clk_get(&pdev->dev, ctrl->clk_name);
	if (IS_ERR(ctrl->clock)) {
		printk(KERN_ERR "failed to get rotator clock source\n");
		return EPERM;
	}

	clk_enable(ctrl->clock);

	/* find the IRQs */
	s3c_rotator_irq_num = platform_get_irq(pdev, 0);
	if(s3c_rotator_irq_num <= 0) {
		printk(KERN_ERR "failed to get irq resource\n");
		return -ENOENT;
	}

	ret = request_irq(s3c_rotator_irq_num, s3c_rotator_irq, IRQF_DISABLED, pdev->name, NULL);
	if (ret) {
		printk("request_irq(Rotator) failed.\n");
		return ret;
	}

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) {
		printk(KERN_ERR "failed to get memory region resouce\n");
		return -ENOENT;
	}

	s3c_rotator_mem = request_mem_region(res->start, res->end - res->start + 1, pdev->name);
	if(s3c_rotator_mem == NULL) {
		printk(KERN_ERR "failed to reserved memory region\n");
		return -ENOENT;
	}

	s3c_rotator_base = ioremap(s3c_rotator_mem->start, s3c_rotator_mem->end - res->start + 1);
	if(s3c_rotator_base == NULL) {
		printk(KERN_ERR "failed ioremap\n");
		return -ENOENT;
	}

	init_waitqueue_head(&waitq_rotator);

	ret = misc_register(&s3c_rotator_dev);
	if (ret) {
		printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n", ROTATOR_MINOR, ret);
		return ret;
	}

	h_rot_mutex = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (h_rot_mutex == NULL) {
		printk (KERN_ERR "cannot allocate rotator mutex\n");
		return -ENOENT;
	}
	
	mutex_init(h_rot_mutex);

	printk("s3c_rotator_probe success\n");
    
	return 0;  
}


static int s3c_rotator_remove(struct platform_device *dev)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	clk_disable(ctrl->clock);

	free_irq(s3c_rotator_irq_num, NULL);
	
	if (s3c_rotator_mem != NULL) {   
		printk(KERN_INFO "S3C Rotator Driver, releasing resource\n");
		iounmap(s3c_rotator_base);
		release_resource(s3c_rotator_mem);
		kfree(s3c_rotator_mem);
	}
	
	misc_deregister(&s3c_rotator_dev);

	return 0;
}


static int s3c_rotator_suspend(struct platform_device *dev, pm_message_t state)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	unsigned int 		i = 0;

	if (ctrl->status != ROT_IDLE) {
		ctrl->status = ROT_READY_SLEEP;

		while (i++ > 1000) {
			if (ctrl->status != ROT_IDLE)
				printk(KERN_ERR "Rotator is running.\n");
			else
				break;
		}
	} else {
		ctrl->status = ROT_READY_SLEEP;
	}

	ctrl->status = ROT_SLEEP;
	clk_disable(ctrl->clock);

	return 0;
}


static int s3c_rotator_resume(struct platform_device *pdev)
{
	struct s3c_rotator_ctrl	*ctrl = &s3c_rot;
	
	clk_enable(ctrl->clock);
	ctrl->status = ROT_IDLE;

	s3c_rotator_enable_int();

	return 0;
}


static struct platform_driver s3c_rotator_driver = {
       .probe		= s3c_rotator_probe,
       .remove		= s3c_rotator_remove,
       .suspend		= s3c_rotator_suspend,
       .resume		= s3c_rotator_resume,
       .driver		= {
		    .owner	= THIS_MODULE,
		    .name	= "s3c-rotator",
	},
};


static char banner[] __initdata = KERN_INFO "S3C Rotator Driver, (c) 2008 Samsung Electronics\n";

int __init s3c_rotator_init(void)
{
 	unsigned int ret;
	printk(banner);
	
	ret = platform_driver_register(&s3c_rotator_driver);
	if( ret != 0) {
		printk(KERN_ERR "s3c_rotator_driver platform device register failed\n");
		return -1;
	}

	return 0;
}


void  s3c_rotator_exit(void)
{
	platform_driver_unregister(&s3c_rotator_driver);
	mutex_destroy(h_rot_mutex);
	
	printk("s3c_rotator_driver exit\n");
}

module_init(s3c_rotator_init);
module_exit(s3c_rotator_exit);

MODULE_AUTHOR("Jonghun Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("S3C Rotator Device Driver");
MODULE_LICENSE("GPL");
