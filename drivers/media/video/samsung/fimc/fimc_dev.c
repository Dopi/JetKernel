/* linux/drivers/media/video/samsung/fimc_core.c
 *
 * Core file for Samsung Camera Interface (FIMC) driver
 *
 * Dongsoo Kim, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/sec/
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * Note: This driver supports common i2c client driver style
 * which uses i2c_board_info for backward compatibility and
 * new v4l2_subdev as well.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <media/v4l2-device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <plat/fimc.h>

#ifdef CONFIG_CPU_FREQ
#include <plat/s5pc11x-dvfs.h>
#endif

#include "fimc.h"


#define CLEAR_FIMC2_BUFF

extern int hw_version_check(void);

struct fimc_global *fimc_dev;
struct s3c_platform_camera cam_struct_g;

int fimc_dma_alloc(struct fimc_control *ctrl, struct fimc_buf_set *bs, int i, int align)
{
	dma_addr_t end, *curr;

	mutex_lock(&ctrl->lock);

	end = ctrl->mem.base + ctrl->mem.size;
	curr = &ctrl->mem.curr;

	if (!bs->length[i])
		return -EINVAL;

	if (!align) {
		if (*curr + bs->length[i] > end) {
			goto overflow;
		} else {
			bs->base[i] = *curr;
			bs->garbage[i] = 0;
			*curr += bs->length[i];
		}
	} else {
		if (ALIGN(*curr, align) + bs->length[i] > end)
			goto overflow;
		else {
			bs->base[i] = ALIGN(*curr, align);
			bs->garbage[i] = ALIGN(*curr, align) - *curr;
			*curr += (bs->length[i] + bs->garbage[i]);
		}
	}

	mutex_unlock(&ctrl->lock);

	return 0;

overflow:
	bs->base[i] = 0;
	bs->length[i] = 0;
	bs->garbage[i] = 0;

	mutex_unlock(&ctrl->lock);

	return -ENOMEM;
}

void fimc_dma_free(struct fimc_control *ctrl, struct fimc_buf_set *bs, int i)
{
	mutex_lock(&ctrl->lock);

	if (bs->base[i]) {
		ctrl->mem.curr -= (bs->length[i] + bs->garbage[i]);
		bs->base[i] = 0;
		bs->length[i] = 0;
		bs->garbage[i] = 0;
	}

	mutex_unlock(&ctrl->lock);
}

static inline u32 fimc_irq_out(struct fimc_control *ctrl)
{
	struct fimc_buf_set *buf_set;

	/* Interrupt pendding clear */
	fimc_hwset_clear_irq(ctrl);

	if(!list_empty(&ctrl->out->inq)){
		list_for_each_entry(buf_set, &ctrl->out->inq, list){
			if(buf_set->state == VIDEOBUF_ACTIVE){
				buf_set->flags &= ~V4L2_BUF_FLAG_QUEUED;
				buf_set->flags |= V4L2_BUF_FLAG_DONE;
				buf_set->state = VIDEOBUF_DONE;

				if(list_is_last(&buf_set->list, &ctrl->out->inq)){
		ctrl->status = FIMC_STREAMON_IDLE;
		} else {
					buf_set = list_first_entry(&buf_set->list, struct fimc_buf_set, list);
					fimc_outdev_set_src_addr(ctrl, ctrl->out->buf[buf_set->id].base);
					if (ctrl->out->fbuf.base){
						fimc_update_out_addr(ctrl, (dma_addr_t)ctrl->out->fbuf.base);
						fimc_outdev_start_camif(ctrl);
		}
					buf_set->state = VIDEOBUF_ACTIVE;
	}

				wake_up_interruptible(&ctrl->wq);
				break;
	}
	}
}

	return 0;
}

static inline void fimc_irq_cap(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;

	fimc_hwset_clear_irq(ctrl);
	fimc_hwget_overflow_state(ctrl);
	wake_up_interruptible(&ctrl->wq);

	cap->irq = 1;
}

static irqreturn_t fimc_irq(int irq, void *dev_id)
{
	struct fimc_control *ctrl = (struct fimc_control *) dev_id;

	if (ctrl->cap)
		fimc_irq_cap(ctrl);
	else if (ctrl->out)
		fimc_irq_out(ctrl);

	return IRQ_HANDLED;
}

static
struct fimc_control *fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct resource *res;
	int id, mdev_id, irq;

	id = pdev->id;
	mdev_id = S3C_MDEV_FIMC0 + id;
	pdata = to_fimc_plat(&pdev->dev);

	ctrl = get_fimc_ctrl(id);
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &fimc_video_device[id];
	ctrl->vd->minor = id;

	/* alloc from bank1 as default */
	ctrl->mem.base = s3c_get_media_memory_node(mdev_id, 1);
	ctrl->mem.size = s3c_get_media_memsize_node(mdev_id, 1);
	ctrl->mem.curr = ctrl->mem.base;

	ctrl->status = FIMC_STREAMOFF;
	ctrl->limit = &fimc_limits[id];

	sprintf(ctrl->name, "%s%d", FIMC_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);

	atomic_set(&ctrl->in_use, 0);
	mutex_init(&ctrl->lock);
	mutex_init(&ctrl->v4l2_lock);
	spin_lock_init(&ctrl->lock_in);
	spin_lock_init(&ctrl->lock_out);
	init_waitqueue_head(&ctrl->wq);

	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to get io memory region\n",
			__func__);
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - res->start + 1,
			pdev->name);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to request io memory region\n",
			__func__);
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		dev_err(ctrl->dev, "%s: failed to remap io region\n",
			__func__);
		return NULL;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (request_irq(irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		dev_err(ctrl->dev, "%s: request_irq failed\n", __func__);

	fimc_reset(ctrl);

	return ctrl;
}

static int fimc_unregister_controller(struct platform_device *pdev)
{
	struct fimc_control *ctrl;
	int id = pdev->id;

	ctrl = get_fimc_ctrl(id);
	iounmap(ctrl->regs);
	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static void fimc_mmap_open(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);

	atomic_inc(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static void fimc_mmap_close(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);

	atomic_dec(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static struct vm_operations_struct fimc_mmap_ops = {
	.open	= fimc_mmap_open,
	.close	= fimc_mmap_close,
};

static inline int fimc_mmap_out(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	u32 start_phy_addr = 0;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;
	u32 buf_length = 0;
	int pri_data = 0;

	buf_length = ctrl->out->buf[idx].length[FIMC_ADDR_Y] + \
				ctrl->out->buf[idx].length[FIMC_ADDR_CB] + \
				ctrl->out->buf[idx].length[FIMC_ADDR_CR];
	if (size > buf_length) {
		dev_err(ctrl->dev, "Requested mmap size is too big\n");
		return -EINVAL;
	}

	pri_data = (ctrl->id * 0x10) + idx;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &fimc_mmap_ops;
	vma->vm_private_data = (void *)pri_data;

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		dev_err(ctrl->dev, "writable mapping must be shared\n");
		return -EINVAL;
	}

	start_phy_addr = ctrl->out->buf[idx].base[FIMC_ADDR_Y];
	pfn = __phys_to_pfn(start_phy_addr);

	if (remap_pfn_range(vma, vma->vm_start, pfn, size,
						vma->vm_page_prot)) {
		dev_err(ctrl->dev, "mmap fail\n");
		return -EINVAL;
	}

	vma->vm_ops->open(vma);

	ctrl->out->buf[idx].flags |= V4L2_BUF_FLAG_MAPPED;

	return 0;
}

static inline int fimc_mmap_cap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	/*
	 * page frame number of the address for a source frame
	 * to be stored at.
	 */
	pfn = __phys_to_pfn(ctrl->cap->bufs[idx].base[0]);

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		dev_err(ctrl->dev, "%s: writable mapping must be shared\n",
			__func__);
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		dev_err(ctrl->dev, "%s: mmap fail\n", __func__);
		return -EINVAL;
	}
	
	return 0;
}

static int fimc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	int ret;

	if (ctrl->cap)
		ret = fimc_mmap_cap(filp, vma);
	else
		ret = fimc_mmap_out(filp, vma);

	return ret;
}

static u32 fimc_poll(struct file *filp, poll_table *wait)
{
	struct fimc_control *ctrl = filp->private_data;
	struct fimc_capinfo *cap = ctrl->cap;
	u32 mask = 0;

	if (cap) {
		if (cap->irq || (ctrl->status != FIMC_STREAMON)) {
			mask = POLLIN | POLLRDNORM;
			cap->irq = 0;
		} else {
			poll_wait(filp, &ctrl->wq, wait);
		}
	}

	return mask;
}

static
ssize_t fimc_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	return 0;
}

static
ssize_t fimc_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	return 0;
}

u32 fimc_mapping_rot_flip(u32 rot, u32 flip)
{
	u32 ret = 0;

	switch (rot) {
	case 0:
		if (flip & FIMC_XFLIP)
			ret |= FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 90:
		ret = FIMC_ROT;
		if (flip & FIMC_XFLIP)
			ret |= FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 180:
		ret = (FIMC_XFLIP | FIMC_YFLIP);
		if (flip & FIMC_XFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret &= ~FIMC_YFLIP;
		break;

	case 270:
		ret = (FIMC_XFLIP | FIMC_YFLIP | FIMC_ROT);
		if (flip & FIMC_XFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & FIMC_YFLIP)
			ret &= ~FIMC_YFLIP;
		break;
	}

	return ret;
}

int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift)
{
	if (src >= tar * 64) {
		return -EINVAL;
	} else if (src >= tar * 32) {
		*ratio = 32;
		*shift = 5;
	} else if (src >= tar * 16) {
		*ratio = 16;
		*shift = 4;
	} else if (src >= tar * 8) {
		*ratio = 8;
		*shift = 3;
	} else if (src >= tar * 4) {
		*ratio = 4;
		*shift = 2;
	} else if (src >= tar * 2) {
		*ratio = 2;
		*shift = 1;
	} else {
		*ratio = 1;
		*shift = 0;
	}

	return 0;
}

void fimc_clk_en(struct fimc_control *ctrl, bool on)
{
	if(on){
		if(!ctrl->clk->usage){
			if(!ctrl->out) {/* To avoid printing debug messages 
				during OUTPUT operation. */
				dev_dbg(ctrl->dev, "clock enabled.\n");
			}
			clk_enable(ctrl->clk);
		}
	} else {
		while(ctrl->clk->usage > 0){
			if(!ctrl->out) {/* To avoid printing debug messages 
				during OUTPUT operation. */
				dev_dbg(ctrl->dev, "clock disabled.\n");
			}
			clk_disable(ctrl->clk);
		}
	}	

}

static int fimc_open(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	int ret;

#ifdef CLEAR_FIMC2_BUFF
	unsigned int *fimc2_buff;
#endif

	ctrl = video_get_drvdata(video_devdata(filp));
	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	if (atomic_read(&ctrl->in_use)) {
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&ctrl->in_use);
	}

	fimc_clk_en(ctrl, true);

	/* Apply things to interface register */
	fimc_reset(ctrl);
	filp->private_data = ctrl;

	ctrl->fb.open_fifo	= s3cfb_open_fifo;
	ctrl->fb.close_fifo	= s3cfb_close_fifo;

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_WIDTH,
					(unsigned long)&ctrl->fb.lcd_hres);
	if (ret < 0)
		dev_err(ctrl->dev,  "Fail: S3CFB_GET_LCD_WIDTH\n");

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_HEIGHT,
					(unsigned long)&ctrl->fb.lcd_vres);
	if (ret < 0)
		dev_err(ctrl->dev,  "Fail: S3CFB_GET_LCD_HEIGHT\n");

	ctrl->status = FIMC_STREAMOFF;

	if(0 != ctrl->id)
		fimc_clk_en(ctrl, false);

	mutex_unlock(&ctrl->lock);

#ifdef CONFIG_CPU_FREQ
	// added by jamie to set minimum cpu freq (2009.10.30)
	if (0 == ctrl->id)
    	s5pc110_lock_dvfs_high_level(DVFS_LOCK_TOKEN_2, 2);
#endif 

#ifdef CLEAR_FIMC2_BUFF
	fimc2_buff = (unsigned int*)ioremap(ctrl->mem.base, CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K);
	memset(fimc2_buff, 0, CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K);
	iounmap(fimc2_buff);
#endif

	return 0;

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int fimc_release(struct file *filp)
{
	struct fimc_control *ctrl = filp->private_data;
	struct s3c_platform_fimc *pdata;
	int ret = 0, i;

	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	atomic_dec(&ctrl->in_use);
	filp->private_data = NULL;

	/* FIXME: turning off actual working camera */
	if (ctrl->cam && ctrl->id != 2) {
		/* shutdown the MCLK */
		clk_disable(ctrl->cam->clk);

		/* Unload the subdev (camera sensor) module, reset related status flags */
		fimc_release_subdev(ctrl);		
	}

	if (ctrl->cap) {
		mutex_unlock(&ctrl->lock);

		for (i = 0; i < FIMC_CAPBUFS; i++)
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 0);

		mutex_lock(&ctrl->lock);
		kfree(ctrl->cap);
		ctrl->cap = NULL;
	}

	if (ctrl->out) {
		if (ctrl->status != FIMC_STREAMOFF) {
			ret = fimc_outdev_stop_streaming(ctrl);
			if (ret < 0)
				dev_err(ctrl->dev,
					"Fail: fimc_stop_streaming\n");
			ctrl->status = FIMC_STREAMOFF;
		}

		kfree(ctrl->out);
		ctrl->out = NULL;
	}

#ifdef CONFIG_CPU_FREQ
	// added by jamie to set minimum cpu freq (2009.10.30)
	if (0 == ctrl->id)
		s5pc110_unlock_dvfs_high_level(DVFS_LOCK_TOKEN_2);
#endif

	fimc_clk_en(ctrl, false);

	mutex_unlock(&ctrl->lock);

	dev_info(ctrl->dev, "%s: successfully released\n", __func__);

	return 0;
}

static const struct v4l2_file_operations fimc_fops = {
	.owner = THIS_MODULE,
	.open = fimc_open,
	.release = fimc_release,
	.ioctl = video_ioctl2,
	.read = fimc_read,
	.write = fimc_write,
	.mmap = fimc_mmap,
	.poll = fimc_poll,
};

static void fimc_vdev_release(struct video_device *vdev)
{
	kfree(vdev);
}

struct video_device fimc_video_device[FIMC_DEVICES] = {
	[0] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[1] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[2] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
};


static int fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;
	struct clk *srclk;
	int i;

	pdata = to_fimc_plat(&pdev->dev);

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		cam = pdata->camera[i];
		if (!cam)
			break;

		srclk = clk_get(&pdev->dev, cam->srclk_name);
		if (IS_ERR(srclk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
					__func__);
			clk_put(srclk);
			return -EINVAL;
		}

		/* mclk */
		cam->clk = clk_get(&pdev->dev, cam->clk_name);
		if (IS_ERR(cam->clk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
					__func__);
			clk_put(cam->clk);
			clk_put(srclk);
			return -EINVAL;
		}

		if (cam->clk->set_parent) {
			cam->clk->parent = srclk;
			cam->clk->set_parent(cam->clk, srclk);
		}

		/* set rate for mclk */
		if (cam->clk->set_rate) {
			cam->clk->set_rate(cam->clk, cam->clk_rate);
		}

		clk_put(cam->clk);
		clk_put(srclk);

		/* Assign camera device to fimc */
		memcpy(&fimc_dev->camera[i], cam, sizeof(*cam));
		fimc_dev->camera_isvalid[i] = 1;
		fimc_dev->camera[i].initialized = 0;
	}

	fimc_dev->active_camera = -1;
	fimc_dev->initialized = 1;

	return 0;
}

static int __devinit fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct clk *srclk;
	int ret;

	if (!fimc_dev) {
		fimc_dev = kzalloc(sizeof(*fimc_dev), GFP_KERNEL);
		if (!fimc_dev) {
			dev_err(&pdev->dev, "%s: not enough memory\n",
				__func__);
			goto err_fimc;
		}
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "%s: cannot register fimc controller\n",
			__func__);
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* fimc source clock */
	srclk = clk_get(&pdev->dev, pdata->srclk_name);
	if (IS_ERR(srclk)) {
		dev_err(&pdev->dev,
				"%s: failed to get source clock of fimc\n",
				__func__);
		goto err_clk_io;
	}

	/* fimc clock */
	ctrl->clk = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(ctrl->clk)) {
		dev_err(&pdev->dev, "%s: failed to get fimc clock source\n",
			__func__);
		goto err_clk_io;
	}

	/* set parent clock */
	if (ctrl->clk->set_parent) {
		ctrl->clk->parent = srclk;
		ctrl->clk->set_parent(ctrl->clk, srclk);
	}

	/* set clockrate for fimc interface block */
	if (ctrl->clk->set_rate) {
		ctrl->clk->set_rate(ctrl->clk, pdata->clk_rate);
		dev_info(&pdev->dev, "fimc set clock rate to %d\n",
				pdata->clk_rate);
	}

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "%s: v4l2 device register failed\n",
			__func__);
		goto err_clk_io;
	}

	/* things to initialize once */
	if (!fimc_dev->initialized) {
		ret = fimc_init_global(pdev);
		if (ret)
			goto err_global;
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "%s: cannot register video driver\n",
			__func__);
		goto err_global;
	}

	video_set_drvdata(ctrl->vd, ctrl);

	if(1 == hw_version_check())
	{
		if (1 == ctrl->id)
			pdata->hw_ver = 0x50;
		else
			pdata->hw_ver = 0x45;
	}

	dev_info(&pdev->dev, "controller %d registered successfully\n",
		ctrl->id);

	return 0;

err_global:
	clk_disable(ctrl->clk);
	clk_put(ctrl->clk);

err_clk_io:
	fimc_unregister_controller(pdev);

err_fimc:
	return -EINVAL;

}

static int fimc_remove(struct platform_device *pdev)
{
	fimc_unregister_controller(pdev);

	kfree(fimc_dev);
	fimc_dev = NULL;

	return 0;
}

#ifdef CONFIG_PM
int fimc_suspend(struct platform_device *dev, pm_message_t state)
{
        struct fimc_control *ctrl;
        int id = dev->id;

        ctrl = &fimc_dev->ctrl[id];

	fimc_save_regs(ctrl);

        return 0;
}

int fimc_resume(struct platform_device *dev)
{
        struct fimc_control *ctrl;
        int id = dev->id;

        ctrl = &fimc_dev->ctrl[id];

	fimc_load_regs(ctrl);

	return 0;
}
#else
#define fimc_suspend	NULL
#define fimc_resume	NULL
#endif

static struct platform_driver fimc_driver = {
	.probe		= fimc_probe,
	.remove		= fimc_remove,
	.suspend	= fimc_suspend,
	.resume		= fimc_resume,
	.driver		= {
		.name	= FIMC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int fimc_register(void)
{
	platform_driver_register(&fimc_driver);

	return 0;
}

static void fimc_unregister(void)
{
	platform_driver_unregister(&fimc_driver);
}

late_initcall(fimc_register);
module_exit(fimc_unregister);

MODULE_AUTHOR("Dongsoo, Kim <dongsoo45.kim@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Camera Interface (FIMC) driver");
MODULE_LICENSE("GPL");

