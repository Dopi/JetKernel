/* linux/drivers/media/video/samsung/fimc_cfg.c
 *
 * V4L2 Overlay device support file for Samsung Camera Interface (FIMC) driver
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/slab.h>
#include <linux/bootmem.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <plat/media.h>

#include "fimc.h"

int fimc_try_fmt_overlay(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 is_rotate = 0;

	dev_info(ctrl->dev, "%s: called\n\
			top(%d), left(%d), width(%d), height(%d)\n", \
			__func__, f->fmt.win.w.top, f->fmt.win.w.left, \
			f->fmt.win.w.width, f->fmt.win.w.height);

	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running\n");
		return -EBUSY;
	}

	/* Check Overlay Size : Overlay size must be smaller than LCD size. */
#if 1
	// adde by jamie (2009.08.26)
	if (ctrl->out->fbuf.base) return 0;
#endif

	is_rotate = fimc_mapping_rot_flip(ctrl->out->rotate, ctrl->out->flip);
	if (is_rotate & FIMC_ROT) {	/* Landscape mode */
		if (f->fmt.win.w.width > ctrl->fb.lcd_vres) {
			dev_warn(ctrl->dev, "The width is changed %d -> %d\n",
				f->fmt.win.w.width, ctrl->fb.lcd_vres);
			f->fmt.win.w.width = ctrl->fb.lcd_vres;
		}

		if (f->fmt.win.w.height > ctrl->fb.lcd_hres) {
			dev_warn(ctrl->dev, "The height is changed %d -> %d\n",
				f->fmt.win.w.height, ctrl->fb.lcd_hres);
			f->fmt.win.w.height = ctrl->fb.lcd_hres;
		}
	} else {			/* Portrait mode */
		if (f->fmt.win.w.width > ctrl->fb.lcd_hres) {
			dev_warn(ctrl->dev, "The width is changed %d -> %d\n",
				f->fmt.win.w.width, ctrl->fb.lcd_hres);
			f->fmt.win.w.width = ctrl->fb.lcd_hres;
		}

		if (f->fmt.win.w.height > ctrl->fb.lcd_vres) {
			dev_warn(ctrl->dev, "The height is changed %d -> %d\n",
				f->fmt.win.w.height, ctrl->fb.lcd_vres);
			f->fmt.win.w.height = ctrl->fb.lcd_vres;
		}

	}

	return 0;
}

int fimc_g_fmt_vid_overlay(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;

	dev_info(ctrl->dev, "%s: called\n", __func__);

	f->fmt.win = ctrl->out->win;

	return 0;
}

int fimc_s_fmt_vid_overlay(struct file *file, void *fh, struct v4l2_format *f)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	int ret = -1;

	dev_info(ctrl->dev, "%s: called\n", __func__);

	/* Check stream status */
	if (ctrl->status != FIMC_STREAMOFF) {
		dev_err(ctrl->dev, "FIMC is running\n");
		return -EBUSY;
	}

	ret = fimc_try_fmt_overlay(file, fh, f);
	if (ret < 0)
		return ret;

	ctrl->out->win = f->fmt.win;

	return ret;
}

int fimc_g_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 bpp = 1;
	u32 format = ctrl->out->fbuf.fmt.pixelformat;

	dev_info(ctrl->dev, "%s: called\n", __func__);

	fb->capability = ctrl->out->fbuf.capability;
	fb->flags = 0;
	fb->base = ctrl->out->fbuf.base;

	fb->fmt.width = ctrl->out->fbuf.fmt.width;
	fb->fmt.height = ctrl->out->fbuf.fmt.height;
	fb->fmt.pixelformat = ctrl->out->fbuf.fmt.pixelformat;

	if (format == V4L2_PIX_FMT_NV12)
		bpp = 1;
	else if (format == V4L2_PIX_FMT_RGB32)
		bpp = 4;
	else if (format == V4L2_PIX_FMT_RGB565)
		bpp = 2;

	ctrl->out->fbuf.fmt.bytesperline = fb->fmt.width * bpp;
	fb->fmt.bytesperline = ctrl->out->fbuf.fmt.bytesperline;
	fb->fmt.sizeimage = ctrl->out->fbuf.fmt.sizeimage;
	fb->fmt.colorspace = V4L2_COLORSPACE_SMPTE170M;
	fb->fmt.priv = 0;

	return 0;
}

int fimc_s_fbuf(struct file *filp, void *fh, struct v4l2_framebuffer *fb)
{
	struct fimc_control *ctrl = (struct fimc_control *) fh;
	u32 bpp = 1;
	u32 format = fb->fmt.pixelformat;

	dev_info(ctrl->dev, "%s: called\n", __func__);

	ctrl->out->fbuf.capability = V4L2_FBUF_CAP_EXTERNOVERLAY;
	ctrl->out->fbuf.flags = 0;
	ctrl->out->fbuf.base = fb->base;

	if (fb->base) {
		ctrl->out->fbuf.fmt.width = fb->fmt.width;
		ctrl->out->fbuf.fmt.height = fb->fmt.height;
		ctrl->out->fbuf.fmt.pixelformat	= fb->fmt.pixelformat;

		if (format == V4L2_PIX_FMT_NV12)
			bpp = 1;
		else if (format == V4L2_PIX_FMT_RGB32)
			bpp = 4;
		else if (format == V4L2_PIX_FMT_RGB565)
			bpp = 2;

		ctrl->out->fbuf.fmt.bytesperline = fb->fmt.width * bpp;
		ctrl->out->fbuf.fmt.sizeimage = fb->fmt.sizeimage;
		ctrl->out->fbuf.fmt.colorspace = V4L2_COLORSPACE_SMPTE170M;
		ctrl->out->fbuf.fmt.priv = 0;
	}

	return 0;
}

