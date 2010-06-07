/* A driver for the D-Link DSB-R100 USB radio and Gemtek USB Radio 21.
 The device plugs into both the USB and an analog audio input, so this thing
 only deals with initialisation and frequency setting, the
 audio data has to be handled by a sound driver.

 Major issue: I can't find out where the device reports the signal
 strength, and indeed the windows software appearantly just looks
 at the stereo indicator as well.  So, scanning will only find
 stereo stations.  Sad, but I can't help it.

 Also, the windows program sends oodles of messages over to the
 device, and I couldn't figure out their meaning.  My suspicion
 is that they don't have any:-)

 You might find some interesting stuff about this module at
 http://unimut.fsk.uni-heidelberg.de/unimut/demi/dsbr

 Copyright (c) 2000 Markus Demleitner <msdemlei@cl.uni-heidelberg.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

 History:

 Version 0.44:
	Add suspend/resume functions, fix unplug of device,
	a lot of cleanups and fixes by Alexey Klimov <klimov.linux@gmail.com>

 Version 0.43:
	Oliver Neukum: avoided DMA coherency issue

 Version 0.42:
	Converted dsbr100 to use video_ioctl2
	by Douglas Landgraf <dougsland@gmail.com>

 Version 0.41-ac1:
	Alan Cox: Some cleanups and fixes

 Version 0.41:
	Converted to V4L2 API by Mauro Carvalho Chehab <mchehab@infradead.org>

 Version 0.40:
	Markus: Updates for 2.6.x kernels, code layout changes, name sanitizing

 Version 0.30:
	Markus: Updates for 2.5.x kernel and more ISO compliant source

 Version 0.25:
	PSL and Markus: Cleanup, radio now doesn't stop on device close

 Version 0.24:
	Markus: Hope I got these silly VIDEO_TUNER_LOW issues finally
	right.  Some minor cleanup, improved standalone compilation

 Version 0.23:
	Markus: Sign extension bug fixed by declaring transfer_buffer unsigned

 Version 0.22:
	Markus: Some (brown bag) cleanup in what VIDIOCSTUNER returns,
	thanks to Mike Cox for pointing the problem out.

 Version 0.21:
	Markus: Minor cleanup, warnings if something goes wrong, lame attempt
	to adhere to Documentation/CodingStyle

 Version 0.2:
	Brad Hards <bradh@dynamite.com.au>: Fixes to make it work as non-module
	Markus: Copyright clarification

 Version 0.01: Markus: initial release

*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <linux/usb.h>

/*
 * Version Information
 */
#include <linux/version.h>	/* for KERNEL_VERSION MACRO	*/

#define DRIVER_VERSION "v0.44"
#define RADIO_VERSION KERNEL_VERSION(0, 4, 4)

static struct v4l2_queryctrl radio_qctrl[] = {
	{
		.id            = V4L2_CID_AUDIO_MUTE,
		.name          = "Mute",
		.minimum       = 0,
		.maximum       = 1,
		.default_value = 1,
		.type          = V4L2_CTRL_TYPE_BOOLEAN,
	},
/* HINT: the disabled controls are only here to satify kradio and such apps */
	{       .id             = V4L2_CID_AUDIO_VOLUME,
		.flags          = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id             = V4L2_CID_AUDIO_BALANCE,
		.flags          = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id             = V4L2_CID_AUDIO_BASS,
		.flags          = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id             = V4L2_CID_AUDIO_TREBLE,
		.flags          = V4L2_CTRL_FLAG_DISABLED,
	},
	{
		.id             = V4L2_CID_AUDIO_LOUDNESS,
		.flags          = V4L2_CTRL_FLAG_DISABLED,
	},
};

#define DRIVER_AUTHOR "Markus Demleitner <msdemlei@tucana.harvard.edu>"
#define DRIVER_DESC "D-Link DSB-R100 USB FM radio driver"

#define DSB100_VENDOR 0x04b4
#define DSB100_PRODUCT 0x1002

/* Commands the device appears to understand */
#define DSB100_TUNE 1
#define DSB100_ONOFF 2

#define TB_LEN 16

/* Frequency limits in MHz -- these are European values.  For Japanese
devices, that would be 76 and 91.  */
#define FREQ_MIN  87.5
#define FREQ_MAX 108.0
#define FREQ_MUL 16000

#define videodev_to_radio(d) container_of(d, struct dsbr100_device, videodev)

static int usb_dsbr100_probe(struct usb_interface *intf,
			     const struct usb_device_id *id);
static void usb_dsbr100_disconnect(struct usb_interface *intf);
static int usb_dsbr100_open(struct file *file);
static int usb_dsbr100_close(struct file *file);
static int usb_dsbr100_suspend(struct usb_interface *intf,
						pm_message_t message);
static int usb_dsbr100_resume(struct usb_interface *intf);

static int radio_nr = -1;
module_param(radio_nr, int, 0);

/* Data for one (physical) device */
struct dsbr100_device {
	struct usb_device *usbdev;
	struct video_device videodev;
	u8 *transfer_buffer;
	struct mutex lock;	/* buffer locking */
	int curfreq;
	int stereo;
	int users;
	int removed;
	int muted;
};

static struct usb_device_id usb_dsbr100_device_table [] = {
	{ USB_DEVICE(DSB100_VENDOR, DSB100_PRODUCT) },
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_dsbr100_device_table);

/* USB subsystem interface */
static struct usb_driver usb_dsbr100_driver = {
	.name			= "dsbr100",
	.probe			= usb_dsbr100_probe,
	.disconnect		= usb_dsbr100_disconnect,
	.id_table		= usb_dsbr100_device_table,
	.suspend		= usb_dsbr100_suspend,
	.resume			= usb_dsbr100_resume,
	.reset_resume		= usb_dsbr100_resume,
	.supports_autosuspend	= 0,
};

/* Low-level device interface begins here */

/* switch on radio */
static int dsbr100_start(struct dsbr100_device *radio)
{
	int retval;
	int request;

	mutex_lock(&radio->lock);

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		USB_REQ_GET_STATUS,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x00, 0xC7, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = USB_REQ_GET_STATUS;
		goto usb_control_msg_failed;
	}

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		DSB100_ONOFF,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x01, 0x00, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = DSB100_ONOFF;
		goto usb_control_msg_failed;
	}

	radio->muted = 0;
	mutex_unlock(&radio->lock);
	return (radio->transfer_buffer)[0];

usb_control_msg_failed:
	mutex_unlock(&radio->lock);
	dev_err(&radio->usbdev->dev,
		"%s - usb_control_msg returned %i, request %i\n",
			__func__, retval, request);
	return retval;

}

/* switch off radio */
static int dsbr100_stop(struct dsbr100_device *radio)
{
	int retval;
	int request;

	mutex_lock(&radio->lock);

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		USB_REQ_GET_STATUS,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x16, 0x1C, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = USB_REQ_GET_STATUS;
		goto usb_control_msg_failed;
	}

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		DSB100_ONOFF,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x00, 0x00, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = DSB100_ONOFF;
		goto usb_control_msg_failed;
	}

	radio->muted = 1;
	mutex_unlock(&radio->lock);
	return (radio->transfer_buffer)[0];

usb_control_msg_failed:
	mutex_unlock(&radio->lock);
	dev_err(&radio->usbdev->dev,
		"%s - usb_control_msg returned %i, request %i\n",
			__func__, retval, request);
	return retval;

}

/* set a frequency, freq is defined by v4l's TUNER_LOW, i.e. 1/16th kHz */
static int dsbr100_setfreq(struct dsbr100_device *radio, int freq)
{
	int retval;
	int request;

	freq = (freq / 16 * 80) / 1000 + 856;
	mutex_lock(&radio->lock);

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		DSB100_TUNE,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		(freq >> 8) & 0x00ff, freq & 0xff,
		radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = DSB100_TUNE;
		goto usb_control_msg_failed;
	}

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		USB_REQ_GET_STATUS,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x96, 0xB7, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = USB_REQ_GET_STATUS;
		goto usb_control_msg_failed;
	}

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		USB_REQ_GET_STATUS,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE |  USB_DIR_IN,
		0x00, 0x24, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		request = USB_REQ_GET_STATUS;
		goto usb_control_msg_failed;
	}

	radio->stereo = !((radio->transfer_buffer)[0] & 0x01);
	mutex_unlock(&radio->lock);
	return (radio->transfer_buffer)[0];

usb_control_msg_failed:
	radio->stereo = -1;
	mutex_unlock(&radio->lock);
	dev_err(&radio->usbdev->dev,
		"%s - usb_control_msg returned %i, request %i\n",
			__func__, retval, request);
	return retval;
}

/* return the device status.  This is, in effect, just whether it
sees a stereo signal or not.  Pity. */
static void dsbr100_getstat(struct dsbr100_device *radio)
{
	int retval;

	mutex_lock(&radio->lock);

	retval = usb_control_msg(radio->usbdev,
		usb_rcvctrlpipe(radio->usbdev, 0),
		USB_REQ_GET_STATUS,
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
		0x00 , 0x24, radio->transfer_buffer, 8, 300);

	if (retval < 0) {
		radio->stereo = -1;
		dev_err(&radio->usbdev->dev,
			"%s - usb_control_msg returned %i, request %i\n",
				__func__, retval, USB_REQ_GET_STATUS);
	} else {
		radio->stereo = !(radio->transfer_buffer[0] & 0x01);
	}

	mutex_unlock(&radio->lock);
}

/* USB subsystem interface begins here */

/*
 * Handle unplugging of the device.
 * We call video_unregister_device in any case.
 * The last function called in this procedure is
 * usb_dsbr100_video_device_release
 */
static void usb_dsbr100_disconnect(struct usb_interface *intf)
{
	struct dsbr100_device *radio = usb_get_intfdata(intf);

	usb_set_intfdata (intf, NULL);

	mutex_lock(&radio->lock);
	radio->removed = 1;
	mutex_unlock(&radio->lock);

	video_unregister_device(&radio->videodev);
}


static int vidioc_querycap(struct file *file, void *priv,
					struct v4l2_capability *v)
{
	strlcpy(v->driver, "dsbr100", sizeof(v->driver));
	strlcpy(v->card, "D-Link R-100 USB FM Radio", sizeof(v->card));
	sprintf(v->bus_info, "USB");
	v->version = RADIO_VERSION;
	v->capabilities = V4L2_CAP_TUNER;
	return 0;
}

static int vidioc_g_tuner(struct file *file, void *priv,
				struct v4l2_tuner *v)
{
	struct dsbr100_device *radio = video_drvdata(file);

	/* safety check */
	if (radio->removed)
		return -EIO;

	if (v->index > 0)
		return -EINVAL;

	dsbr100_getstat(radio);
	strcpy(v->name, "FM");
	v->type = V4L2_TUNER_RADIO;
	v->rangelow = FREQ_MIN * FREQ_MUL;
	v->rangehigh = FREQ_MAX * FREQ_MUL;
	v->rxsubchans = V4L2_TUNER_SUB_MONO | V4L2_TUNER_SUB_STEREO;
	v->capability = V4L2_TUNER_CAP_LOW;
	if(radio->stereo)
		v->audmode = V4L2_TUNER_MODE_STEREO;
	else
		v->audmode = V4L2_TUNER_MODE_MONO;
	v->signal = 0xffff;     /* We can't get the signal strength */
	return 0;
}

static int vidioc_s_tuner(struct file *file, void *priv,
				struct v4l2_tuner *v)
{
	struct dsbr100_device *radio = video_drvdata(file);

	/* safety check */
	if (radio->removed)
		return -EIO;

	if (v->index > 0)
		return -EINVAL;

	return 0;
}

static int vidioc_s_frequency(struct file *file, void *priv,
				struct v4l2_frequency *f)
{
	struct dsbr100_device *radio = video_drvdata(file);
	int retval;

	/* safety check */
	if (radio->removed)
		return -EIO;

	radio->curfreq = f->frequency;
	retval = dsbr100_setfreq(radio, radio->curfreq);
	if (retval < 0)
		dev_warn(&radio->usbdev->dev, "Set frequency failed\n");
	return 0;
}

static int vidioc_g_frequency(struct file *file, void *priv,
				struct v4l2_frequency *f)
{
	struct dsbr100_device *radio = video_drvdata(file);

	/* safety check */
	if (radio->removed)
		return -EIO;

	f->type = V4L2_TUNER_RADIO;
	f->frequency = radio->curfreq;
	return 0;
}

static int vidioc_queryctrl(struct file *file, void *priv,
				struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(radio_qctrl); i++) {
		if (qc->id && qc->id == radio_qctrl[i].id) {
			memcpy(qc, &(radio_qctrl[i]), sizeof(*qc));
			return 0;
		}
	}
	return -EINVAL;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
				struct v4l2_control *ctrl)
{
	struct dsbr100_device *radio = video_drvdata(file);

	/* safety check */
	if (radio->removed)
		return -EIO;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		ctrl->value = radio->muted;
		return 0;
	}
	return -EINVAL;
}

static int vidioc_s_ctrl(struct file *file, void *priv,
				struct v4l2_control *ctrl)
{
	struct dsbr100_device *radio = video_drvdata(file);
	int retval;

	/* safety check */
	if (radio->removed)
		return -EIO;

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		if (ctrl->value) {
			retval = dsbr100_stop(radio);
			if (retval < 0) {
				dev_warn(&radio->usbdev->dev,
					 "Radio did not respond properly\n");
				return -EBUSY;
			}
		} else {
			retval = dsbr100_start(radio);
			if (retval < 0) {
				dev_warn(&radio->usbdev->dev,
					 "Radio did not respond properly\n");
				return -EBUSY;
			}
		}
		return 0;
	}
	return -EINVAL;
}

static int vidioc_g_audio(struct file *file, void *priv,
				struct v4l2_audio *a)
{
	if (a->index > 1)
		return -EINVAL;

	strcpy(a->name, "Radio");
	a->capability = V4L2_AUDCAP_STEREO;
	return 0;
}

static int vidioc_g_input(struct file *filp, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int vidioc_s_input(struct file *filp, void *priv, unsigned int i)
{
	if (i != 0)
		return -EINVAL;
	return 0;
}

static int vidioc_s_audio(struct file *file, void *priv,
					struct v4l2_audio *a)
{
	if (a->index != 0)
		return -EINVAL;
	return 0;
}

static int usb_dsbr100_open(struct file *file)
{
	struct dsbr100_device *radio = video_drvdata(file);
	int retval;

	lock_kernel();
	radio->users = 1;
	radio->muted = 1;

	retval = dsbr100_start(radio);
	if (retval < 0) {
		dev_warn(&radio->usbdev->dev,
			 "Radio did not start up properly\n");
		radio->users = 0;
		unlock_kernel();
		return -EIO;
	}

	retval = dsbr100_setfreq(radio, radio->curfreq);
	if (retval < 0)
		dev_warn(&radio->usbdev->dev,
			"set frequency failed\n");

	unlock_kernel();
	return 0;
}

static int usb_dsbr100_close(struct file *file)
{
	struct dsbr100_device *radio = video_drvdata(file);
	int retval;

	if (!radio)
		return -ENODEV;

	radio->users = 0;
	if (!radio->removed) {
		retval = dsbr100_stop(radio);
		if (retval < 0) {
			dev_warn(&radio->usbdev->dev,
				"dsbr100_stop failed\n");
		}

	}
	return 0;
}

/* Suspend device - stop device. */
static int usb_dsbr100_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct dsbr100_device *radio = usb_get_intfdata(intf);
	int retval;

	retval = dsbr100_stop(radio);
	if (retval < 0)
		dev_warn(&intf->dev, "dsbr100_stop failed\n");

	dev_info(&intf->dev, "going into suspend..\n");

	return 0;
}

/* Resume device - start device. */
static int usb_dsbr100_resume(struct usb_interface *intf)
{
	struct dsbr100_device *radio = usb_get_intfdata(intf);
	int retval;

	retval = dsbr100_start(radio);
	if (retval < 0)
		dev_warn(&intf->dev, "dsbr100_start failed\n");

	dev_info(&intf->dev, "coming out of suspend..\n");

	return 0;
}

/* free data structures */
static void usb_dsbr100_video_device_release(struct video_device *videodev)
{
	struct dsbr100_device *radio = videodev_to_radio(videodev);

	kfree(radio->transfer_buffer);
	kfree(radio);
}

/* File system interface */
static const struct v4l2_file_operations usb_dsbr100_fops = {
	.owner		= THIS_MODULE,
	.open		= usb_dsbr100_open,
	.release	= usb_dsbr100_close,
	.ioctl		= video_ioctl2,
};

static const struct v4l2_ioctl_ops usb_dsbr100_ioctl_ops = {
	.vidioc_querycap    = vidioc_querycap,
	.vidioc_g_tuner     = vidioc_g_tuner,
	.vidioc_s_tuner     = vidioc_s_tuner,
	.vidioc_g_frequency = vidioc_g_frequency,
	.vidioc_s_frequency = vidioc_s_frequency,
	.vidioc_queryctrl   = vidioc_queryctrl,
	.vidioc_g_ctrl      = vidioc_g_ctrl,
	.vidioc_s_ctrl      = vidioc_s_ctrl,
	.vidioc_g_audio     = vidioc_g_audio,
	.vidioc_s_audio     = vidioc_s_audio,
	.vidioc_g_input     = vidioc_g_input,
	.vidioc_s_input     = vidioc_s_input,
};

/* V4L2 interface */
static struct video_device dsbr100_videodev_data = {
	.name		= "D-Link DSB-R 100",
	.fops		= &usb_dsbr100_fops,
	.ioctl_ops 	= &usb_dsbr100_ioctl_ops,
	.release	= usb_dsbr100_video_device_release,
};

/* check if the device is present and register with v4l and usb if it is */
static int usb_dsbr100_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	struct dsbr100_device *radio;
	int retval;

	radio = kmalloc(sizeof(struct dsbr100_device), GFP_KERNEL);

	if (!radio)
		return -ENOMEM;

	radio->transfer_buffer = kmalloc(TB_LEN, GFP_KERNEL);

	if (!(radio->transfer_buffer)) {
		kfree(radio);
		return -ENOMEM;
	}

	mutex_init(&radio->lock);
	radio->videodev = dsbr100_videodev_data;

	radio->removed = 0;
	radio->users = 0;
	radio->usbdev = interface_to_usbdev(intf);
	radio->curfreq = FREQ_MIN * FREQ_MUL;
	video_set_drvdata(&radio->videodev, radio);
	retval = video_register_device(&radio->videodev, VFL_TYPE_RADIO, radio_nr);
	if (retval < 0) {
		dev_err(&intf->dev, "couldn't register video device\n");
		kfree(radio->transfer_buffer);
		kfree(radio);
		return -EIO;
	}
	usb_set_intfdata(intf, radio);
	return 0;
}

static int __init dsbr100_init(void)
{
	int retval = usb_register(&usb_dsbr100_driver);
	printk(KERN_INFO KBUILD_MODNAME ": " DRIVER_VERSION ":"
	       DRIVER_DESC "\n");
	return retval;
}

static void __exit dsbr100_exit(void)
{
	usb_deregister(&usb_dsbr100_driver);
}

module_init (dsbr100_init);
module_exit (dsbr100_exit);

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");
