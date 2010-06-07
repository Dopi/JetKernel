/* Typhoon Radio Card driver for radio support
 * (c) 1999 Dr. Henrik Seidel <Henrik.Seidel@gmx.de>
 *
 * Card manufacturer:
 * http://194.18.155.92/idc/prod2.idc?nr=50753&lang=e
 *
 * Notes on the hardware
 *
 * This card has two output sockets, one for speakers and one for line.
 * The speaker output has volume control, but only in four discrete
 * steps. The line output has neither volume control nor mute.
 *
 * The card has auto-stereo according to its manual, although it all
 * sounds mono to me (even with the Win/DOS drivers). Maybe it's my
 * antenna - I really don't know for sure.
 *
 * Frequency control is done digitally.
 *
 * Volume control is done digitally, but there are only four different
 * possible values. So you should better always turn the volume up and
 * use line control. I got the best results by connecting line output
 * to the sound card microphone input. For such a configuration the
 * volume control has no effect, since volume control only influences
 * the speaker output.
 *
 * There is no explicit mute/unmute. So I set the radio frequency to a
 * value where I do expect just noise and turn the speaker volume down.
 * The frequency change is necessary since the card never seems to be
 * completely silent.
 *
 * Converted to V4L2 API by Mauro Carvalho Chehab <mchehab@infradead.org>
 */

#include <linux/module.h>	/* Modules                        */
#include <linux/init.h>		/* Initdata                       */
#include <linux/ioport.h>	/* request_region		  */
#include <linux/proc_fs.h>	/* radio card status report	  */
#include <linux/seq_file.h>
#include <asm/io.h>		/* outb, outb_p                   */
#include <asm/uaccess.h>	/* copy to/from user              */
#include <linux/videodev2.h>	/* kernel radio structs           */
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

#include <linux/version.h>      /* for KERNEL_VERSION MACRO     */
#define RADIO_VERSION KERNEL_VERSION(0,1,1)
#define BANNER "Typhoon Radio Card driver v0.1.1\n"

static struct v4l2_queryctrl radio_qctrl[] = {
	{
		.id            = V4L2_CID_AUDIO_MUTE,
		.name          = "Mute",
		.minimum       = 0,
		.maximum       = 1,
		.default_value = 1,
		.type          = V4L2_CTRL_TYPE_BOOLEAN,
	},{
		.id            = V4L2_CID_AUDIO_VOLUME,
		.name          = "Volume",
		.minimum       = 0,
		.maximum       = 65535,
		.step          = 1<<14,
		.default_value = 0xff,
		.type          = V4L2_CTRL_TYPE_INTEGER,
	}
};


#ifndef CONFIG_RADIO_TYPHOON_PORT
#define CONFIG_RADIO_TYPHOON_PORT -1
#endif

#ifndef CONFIG_RADIO_TYPHOON_MUTEFREQ
#define CONFIG_RADIO_TYPHOON_MUTEFREQ 0
#endif

#ifndef CONFIG_PROC_FS
#undef CONFIG_RADIO_TYPHOON_PROC_FS
#endif

struct typhoon_device {
	unsigned long in_use;
	int iobase;
	int curvol;
	int muted;
	unsigned long curfreq;
	unsigned long mutefreq;
	struct mutex lock;
};

static void typhoon_setvol_generic(struct typhoon_device *dev, int vol);
static int typhoon_setfreq_generic(struct typhoon_device *dev,
				   unsigned long frequency);
static int typhoon_setfreq(struct typhoon_device *dev, unsigned long frequency);
static void typhoon_mute(struct typhoon_device *dev);
static void typhoon_unmute(struct typhoon_device *dev);
static int typhoon_setvol(struct typhoon_device *dev, int vol);

static void typhoon_setvol_generic(struct typhoon_device *dev, int vol)
{
	mutex_lock(&dev->lock);
	vol >>= 14;				/* Map 16 bit to 2 bit */
	vol &= 3;
	outb_p(vol / 2, dev->iobase);		/* Set the volume, high bit. */
	outb_p(vol % 2, dev->iobase + 2);	/* Set the volume, low bit. */
	mutex_unlock(&dev->lock);
}

static int typhoon_setfreq_generic(struct typhoon_device *dev,
				   unsigned long frequency)
{
	unsigned long outval;
	unsigned long x;

	/*
	 * The frequency transfer curve is not linear. The best fit I could
	 * get is
	 *
	 * outval = -155 + exp((f + 15.55) * 0.057))
	 *
	 * where frequency f is in MHz. Since we don't have exp in the kernel,
	 * I approximate this function by a third order polynomial.
	 *
	 */

	mutex_lock(&dev->lock);
	x = frequency / 160;
	outval = (x * x + 2500) / 5000;
	outval = (outval * x + 5000) / 10000;
	outval -= (10 * x * x + 10433) / 20866;
	outval += 4 * x - 11505;

	outb_p((outval >> 8) & 0x01, dev->iobase + 4);
	outb_p(outval >> 9, dev->iobase + 6);
	outb_p(outval & 0xff, dev->iobase + 8);
	mutex_unlock(&dev->lock);

	return 0;
}

static int typhoon_setfreq(struct typhoon_device *dev, unsigned long frequency)
{
	typhoon_setfreq_generic(dev, frequency);
	dev->curfreq = frequency;
	return 0;
}

static void typhoon_mute(struct typhoon_device *dev)
{
	if (dev->muted == 1)
		return;
	typhoon_setvol_generic(dev, 0);
	typhoon_setfreq_generic(dev, dev->mutefreq);
	dev->muted = 1;
}

static void typhoon_unmute(struct typhoon_device *dev)
{
	if (dev->muted == 0)
		return;
	typhoon_setfreq_generic(dev, dev->curfreq);
	typhoon_setvol_generic(dev, dev->curvol);
	dev->muted = 0;
}

static int typhoon_setvol(struct typhoon_device *dev, int vol)
{
	if (dev->muted && vol != 0) {	/* user is unmuting the card */
		dev->curvol = vol;
		typhoon_unmute(dev);
		return 0;
	}
	if (vol == dev->curvol)		/* requested volume == current */
		return 0;

	if (vol == 0) {			/* volume == 0 means mute the card */
		typhoon_mute(dev);
		dev->curvol = vol;
		return 0;
	}
	typhoon_setvol_generic(dev, vol);
	dev->curvol = vol;
	return 0;
}

static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *v)
{
	strlcpy(v->driver, "radio-typhoon", sizeof(v->driver));
	strlcpy(v->card, "Typhoon Radio", sizeof(v->card));
	sprintf(v->bus_info, "ISA");
	v->version = RADIO_VERSION;
	v->capabilities = V4L2_CAP_TUNER;
	return 0;
}

static int vidioc_g_tuner(struct file *file, void *priv,
					struct v4l2_tuner *v)
{
	if (v->index > 0)
		return -EINVAL;

	strcpy(v->name, "FM");
	v->type = V4L2_TUNER_RADIO;
	v->rangelow = (87.5*16000);
	v->rangehigh = (108*16000);
	v->rxsubchans = V4L2_TUNER_SUB_MONO;
	v->capability = V4L2_TUNER_CAP_LOW;
	v->audmode = V4L2_TUNER_MODE_MONO;
	v->signal = 0xFFFF;     /* We can't get the signal strength */
	return 0;
}

static int vidioc_s_tuner(struct file *file, void *priv,
					struct v4l2_tuner *v)
{
	if (v->index > 0)
		return -EINVAL;

	return 0;
}

static int vidioc_s_frequency(struct file *file, void *priv,
					struct v4l2_frequency *f)
{
	struct typhoon_device *typhoon = video_drvdata(file);

	typhoon->curfreq = f->frequency;
	typhoon_setfreq(typhoon, typhoon->curfreq);
	return 0;
}

static int vidioc_g_frequency(struct file *file, void *priv,
					struct v4l2_frequency *f)
{
	struct typhoon_device *typhoon = video_drvdata(file);

	f->type = V4L2_TUNER_RADIO;
	f->frequency = typhoon->curfreq;

	return 0;
}

static int vidioc_queryctrl(struct file *file, void *priv,
					struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(radio_qctrl); i++) {
		if (qc->id && qc->id == radio_qctrl[i].id) {
			memcpy(qc, &(radio_qctrl[i]),
						sizeof(*qc));
			return 0;
		}
	}
	return -EINVAL;
}

static int vidioc_g_ctrl(struct file *file, void *priv,
					struct v4l2_control *ctrl)
{
	struct typhoon_device *typhoon = video_drvdata(file);

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		ctrl->value = typhoon->muted;
		return 0;
	case V4L2_CID_AUDIO_VOLUME:
		ctrl->value = typhoon->curvol;
		return 0;
	}
	return -EINVAL;
}

static int vidioc_s_ctrl (struct file *file, void *priv,
					struct v4l2_control *ctrl)
{
	struct typhoon_device *typhoon = video_drvdata(file);

	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:
		if (ctrl->value)
			typhoon_mute(typhoon);
		else
			typhoon_unmute(typhoon);
		return 0;
	case V4L2_CID_AUDIO_VOLUME:
		typhoon_setvol(typhoon, ctrl->value);
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

static struct typhoon_device typhoon_unit =
{
	.iobase		= CONFIG_RADIO_TYPHOON_PORT,
	.curfreq	= CONFIG_RADIO_TYPHOON_MUTEFREQ,
	.mutefreq	= CONFIG_RADIO_TYPHOON_MUTEFREQ,
};

static int typhoon_exclusive_open(struct file *file)
{
	return test_and_set_bit(0, &typhoon_unit.in_use) ? -EBUSY : 0;
}

static int typhoon_exclusive_release(struct file *file)
{
	clear_bit(0, &typhoon_unit.in_use);
	return 0;
}

static const struct v4l2_file_operations typhoon_fops = {
	.owner		= THIS_MODULE,
	.open           = typhoon_exclusive_open,
	.release        = typhoon_exclusive_release,
	.ioctl		= video_ioctl2,
};

static const struct v4l2_ioctl_ops typhoon_ioctl_ops = {
	.vidioc_querycap    = vidioc_querycap,
	.vidioc_g_tuner     = vidioc_g_tuner,
	.vidioc_s_tuner     = vidioc_s_tuner,
	.vidioc_g_audio     = vidioc_g_audio,
	.vidioc_s_audio     = vidioc_s_audio,
	.vidioc_g_input     = vidioc_g_input,
	.vidioc_s_input     = vidioc_s_input,
	.vidioc_g_frequency = vidioc_g_frequency,
	.vidioc_s_frequency = vidioc_s_frequency,
	.vidioc_queryctrl   = vidioc_queryctrl,
	.vidioc_g_ctrl      = vidioc_g_ctrl,
	.vidioc_s_ctrl      = vidioc_s_ctrl,
};

static struct video_device typhoon_radio = {
	.name		= "Typhoon Radio",
	.fops           = &typhoon_fops,
	.ioctl_ops 	= &typhoon_ioctl_ops,
	.release	= video_device_release_empty,
};

#ifdef CONFIG_RADIO_TYPHOON_PROC_FS

static int typhoon_proc_show(struct seq_file *m, void *v)
{
	#ifdef MODULE
	    #define MODULEPROCSTRING "Driver loaded as a module"
	#else
	    #define MODULEPROCSTRING "Driver compiled into kernel"
	#endif

	seq_puts(m, BANNER);
	seq_puts(m, "Load type: " MODULEPROCSTRING "\n\n");
	seq_printf(m, "frequency = %lu kHz\n",
		typhoon_unit.curfreq >> 4);
	seq_printf(m, "volume = %d\n", typhoon_unit.curvol);
	seq_printf(m, "mute = %s\n", typhoon_unit.muted ?
		"on" : "off");
	seq_printf(m, "iobase = 0x%x\n", typhoon_unit.iobase);
	seq_printf(m, "mute frequency = %lu kHz\n",
		typhoon_unit.mutefreq >> 4);
	return 0;
}

static int typhoon_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, typhoon_proc_show, NULL);
}

static const struct file_operations typhoon_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= typhoon_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif /* CONFIG_RADIO_TYPHOON_PROC_FS */

MODULE_AUTHOR("Dr. Henrik Seidel");
MODULE_DESCRIPTION("A driver for the Typhoon radio card (a.k.a. EcoRadio).");
MODULE_LICENSE("GPL");

static int io = -1;
static int radio_nr = -1;

module_param(io, int, 0);
MODULE_PARM_DESC(io, "I/O address of the Typhoon card (0x316 or 0x336)");
module_param(radio_nr, int, 0);

#ifdef MODULE
static unsigned long mutefreq;
module_param(mutefreq, ulong, 0);
MODULE_PARM_DESC(mutefreq, "Frequency used when muting the card (in kHz)");
#endif

static int __init typhoon_init(void)
{
#ifdef MODULE
	if (io == -1) {
		printk(KERN_ERR "radio-typhoon: You must set an I/O address with io=0x316 or io=0x336\n");
		return -EINVAL;
	}
	typhoon_unit.iobase = io;

	if (mutefreq < 87000 || mutefreq > 108500) {
		printk(KERN_ERR "radio-typhoon: You must set a frequency (in kHz) used when muting the card,\n");
		printk(KERN_ERR "radio-typhoon: e.g. with \"mutefreq=87500\" (87000 <= mutefreq <= 108500)\n");
		return -EINVAL;
	}
	typhoon_unit.mutefreq = mutefreq;
#endif /* MODULE */

	printk(KERN_INFO BANNER);
	mutex_init(&typhoon_unit.lock);
	io = typhoon_unit.iobase;
	if (!request_region(io, 8, "typhoon")) {
		printk(KERN_ERR "radio-typhoon: port 0x%x already in use\n",
		       typhoon_unit.iobase);
		return -EBUSY;
	}

	video_set_drvdata(&typhoon_radio, &typhoon_unit);
	if (video_register_device(&typhoon_radio, VFL_TYPE_RADIO, radio_nr) < 0) {
		release_region(io, 8);
		return -EINVAL;
	}
	printk(KERN_INFO "radio-typhoon: port 0x%x.\n", typhoon_unit.iobase);
	printk(KERN_INFO "radio-typhoon: mute frequency is %lu kHz.\n",
	       typhoon_unit.mutefreq);
	typhoon_unit.mutefreq <<= 4;

	/* mute card - prevents noisy bootups */
	typhoon_mute(&typhoon_unit);

#ifdef CONFIG_RADIO_TYPHOON_PROC_FS
	if (!proc_create("driver/radio-typhoon", 0, NULL, &typhoon_proc_fops))
		printk(KERN_ERR "radio-typhoon: registering /proc/driver/radio-typhoon failed\n");
#endif

	return 0;
}

static void __exit typhoon_cleanup_module(void)
{

#ifdef CONFIG_RADIO_TYPHOON_PROC_FS
	remove_proc_entry("driver/radio-typhoon", NULL);
#endif

	video_unregister_device(&typhoon_radio);
	release_region(io, 8);
}

module_init(typhoon_init);
module_exit(typhoon_cleanup_module);

