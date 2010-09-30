/* drivers/rtc/rtc-s3c.c
 *
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks, <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410/S3C2440/S3C24XX Internal RTC Driver
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/clk.h>
#include <linux/log2.h>

#include <mach/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach/time.h>
#include <plat/regs-rtc.h>

/* I have yet to find an S3C implementation with more than one
 * of these rtc blocks in */

static struct resource *s3c_rtc_mem;

static void __iomem *s3c_rtc_base;
static int s3c_rtc_alarmno = NO_IRQ;
static int s3c_rtc_tickno  = NO_IRQ;
static int s3c_rtc_freq    = 1;

static DEFINE_SPINLOCK(s3c_rtc_pie_lock);
static unsigned int tick_count;

/* common function function  */

extern void s3c_rtc_set_pie(void __iomem *base, uint to);
extern void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq);
extern void s3c_rtc_enable_set(struct platform_device *dev,void __iomem *base, int en);
extern unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val);
extern unsigned int s3c_rtc_read_alarm_status(void __iomem *base);
/* IRQ Handlers */

#define CONFIG_MACH_VOLANS 1
#ifdef CONFIG_MACH_VOLANS
#define RTC_SYNC_INTERVAL	(HZ*3600)

static int s3c_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm);

/* EXPORTED FUNCTIONS. */
void __iomem *get_rtc_base(void)
{
	return s3c_rtc_base;
}

unsigned long current_rtc_seconds(void)
{
	unsigned long sec = 0;
	struct rtc_time tm;

	s3c_rtc_gettime(NULL, &tm);
	rtc_tm_to_time(&tm, &sec);

	return sec;
}

EXPORT_SYMBOL(get_rtc_base);
EXPORT_SYMBOL(current_rtc_seconds);

/* INTERNAL FUNCTIONS. */
static void rtc_sync_handler(struct work_struct * __unused);
static DECLARE_DELAYED_WORK(rtc_sync_work, rtc_sync_handler);

static void volans_update_system_time(void)
{
	struct rtc_time tm;

	struct timespec time;
	struct timeval sync_time;

	time.tv_nsec = 0;

	s3c_rtc_gettime(NULL, &tm);
	rtc_tm_to_time(&tm, &time.tv_sec);

	/* RTC and system time synchronization. */
	do_settimeofday(&time);

	/* Time verification. */
	do_gettimeofday(&sync_time);

	pr_info("%s- RTC: %ld / System Time: %ld\n", __func__,
			time.tv_sec, sync_time.tv_sec);
}

static void rtc_sync_handler(struct work_struct * __unused)
{
	volans_update_system_time();
	schedule_delayed_work(&rtc_sync_work, RTC_SYNC_INTERVAL);
}
#endif	/* CONFIG_MACH_VOLANS */

static irqreturn_t s3c_rtc_alarmirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	rtc_update_irq(rdev, 1, RTC_AF | RTC_IRQF);

	s3c_rtc_set_bit_byte(s3c_rtc_base,S3C2410_INTP,S3C2410_INTP_ALM);

	return IRQ_HANDLED;
}

#if !defined(CONFIG_MACH_VOLANS)
static irqreturn_t s3c_rtc_tickirq(int irq, void *id)
{
	struct rtc_device *rdev = id;

	rtc_update_irq(rdev, 1, RTC_PF | RTC_IRQF);

	s3c_rtc_set_bit_byte(s3c_rtc_base,S3C2410_INTP,S3C2410_INTP_TIC);

	return IRQ_HANDLED;
}
#endif	/* CONFIG_MACH_VOLANS */

/* Update control registers */
static void s3c_rtc_setaie(int to)
{
	unsigned int tmp;

	pr_debug("%s: aie=%d\n", __func__, to);

	tmp = readb(s3c_rtc_base + S3C2410_RTCALM) & ~S3C2410_RTCALM_ALMEN;

	if (to)
		tmp |= S3C2410_RTCALM_ALMEN;

	writeb(tmp, s3c_rtc_base + S3C2410_RTCALM);

}

static int s3c_rtc_setpie(struct device *dev, int enabled)
{
	pr_debug("%s: pie=%d\n", __func__, enabled);

#if !defined(CONFIG_MACH_VOLANS)
	spin_lock_irq(&s3c_rtc_pie_lock);

	s3c_rtc_set_pie(s3c_rtc_base,enabled);

	spin_unlock_irq(&s3c_rtc_pie_lock);
#endif	/* CONFIG_MACH_VOLANS */

	return 0;
}

static int s3c_rtc_setfreq(struct device *dev, int freq)
{
	spin_lock_irq(&s3c_rtc_pie_lock);

	s3c_rtc_set_freq_regs(s3c_rtc_base,freq,s3c_rtc_freq);

	spin_unlock_irq(&s3c_rtc_pie_lock);

	return 0;
}

/* Time read/write */

static int s3c_rtc_gettime(struct device *dev, struct rtc_time *rtc_tm)
{
	unsigned int have_retried = 0;
	void __iomem *base = s3c_rtc_base;

 retry_get_time:
	rtc_tm->tm_min  = readb(base + S3C2410_RTCMIN);
	rtc_tm->tm_hour = readb(base + S3C2410_RTCHOUR);
	rtc_tm->tm_mday = readb(base + S3C2410_RTCDATE);
	rtc_tm->tm_mon  = readb(base + S3C2410_RTCMON);
	rtc_tm->tm_year = readb(base + S3C2410_RTCYEAR);
	rtc_tm->tm_sec  = readb(base + S3C2410_RTCSEC);

	/* the only way to work out wether the system was mid-update
	 * when we read it is to check the second counter, and if it
	 * is zero, then we re-try the entire read
	 */

	if (rtc_tm->tm_sec == 0 && !have_retried) {
		have_retried = 1;
		goto retry_get_time;
	}

	pr_debug("read time %02x.%02x.%02x %02x/%02x/%02x\n",
		 rtc_tm->tm_year, rtc_tm->tm_mon, rtc_tm->tm_mday,
		 rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);

	rtc_tm->tm_sec = bcd2bin(rtc_tm->tm_sec);
	rtc_tm->tm_min = bcd2bin(rtc_tm->tm_min);
	rtc_tm->tm_hour = bcd2bin(rtc_tm->tm_hour);
	rtc_tm->tm_mday = bcd2bin(rtc_tm->tm_mday);
	rtc_tm->tm_mon = bcd2bin(rtc_tm->tm_mon);
	rtc_tm->tm_year = bcd2bin(rtc_tm->tm_year);

	rtc_tm->tm_year += 100;
	rtc_tm->tm_mon -= 1;

	return 0;
}

static int s3c_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	void __iomem *base = s3c_rtc_base;
	int year = tm->tm_year - 100;

	pr_debug("set time %02d.%02d.%02d %02d/%02d/%02d\n",
		 tm->tm_year, tm->tm_mon, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);

	/* we get around y2k by simply not supporting it */

	if (year < 0 || year >= 100) {
		dev_err(dev, "rtc only supports 100 years\n");
		return -EINVAL;
	}

	writeb(bin2bcd(tm->tm_sec),  base + S3C2410_RTCSEC);
	writeb(bin2bcd(tm->tm_min),  base + S3C2410_RTCMIN);
	writeb(bin2bcd(tm->tm_hour), base + S3C2410_RTCHOUR);
	writeb(bin2bcd(tm->tm_mday), base + S3C2410_RTCDATE);
	writeb(bin2bcd(tm->tm_mon + 1), base + S3C2410_RTCMON);
	writeb(bin2bcd(year), base + S3C2410_RTCYEAR);

	return 0;
}

static int s3c_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *alm_tm = &alrm->time;
	void __iomem *base = s3c_rtc_base;
	unsigned int alm_en;

	alm_tm->tm_sec  = readb(base + S3C2410_ALMSEC);
	alm_tm->tm_min  = readb(base + S3C2410_ALMMIN);
	alm_tm->tm_hour = readb(base + S3C2410_ALMHOUR);
	alm_tm->tm_mon  = readb(base + S3C2410_ALMMON);
	alm_tm->tm_mday = readb(base + S3C2410_ALMDATE);
	alm_tm->tm_year = readb(base + S3C2410_ALMYEAR);

	alm_en = readb(base + S3C2410_RTCALM);

	alrm->enabled = (alm_en & S3C2410_RTCALM_ALMEN) ? 1 : 0;

	pr_debug("read alarm %02x %02x.%02x.%02x %02x/%02x/%02x\n",
		 alm_en,
		 alm_tm->tm_year, alm_tm->tm_mon, alm_tm->tm_mday,
		 alm_tm->tm_hour, alm_tm->tm_min, alm_tm->tm_sec);


	/* decode the alarm enable field */

	if (alm_en & S3C2410_RTCALM_SECEN)
		alm_tm->tm_sec = bcd2bin(alm_tm->tm_sec);
	else
		alm_tm->tm_sec = 0xff;

	if (alm_en & S3C2410_RTCALM_MINEN)
		alm_tm->tm_min = bcd2bin(alm_tm->tm_min);
	else
		alm_tm->tm_min = 0xff;

	if (alm_en & S3C2410_RTCALM_HOUREN)
		alm_tm->tm_hour = bcd2bin(alm_tm->tm_hour);
	else
		alm_tm->tm_hour = 0xff;

	if (alm_en & S3C2410_RTCALM_DAYEN)
		alm_tm->tm_mday = bcd2bin(alm_tm->tm_mday);
	else
		alm_tm->tm_mday = 0xff;

	if (alm_en & S3C2410_RTCALM_MONEN) {
		alm_tm->tm_mon = bcd2bin(alm_tm->tm_mon);
		alm_tm->tm_mon -= 1;
	} else {
		alm_tm->tm_mon = 0xff;
	}

	if (alm_en & S3C2410_RTCALM_YEAREN)
		alm_tm->tm_year = bcd2bin(alm_tm->tm_year);
	else
		alm_tm->tm_year = 0xffff;

	return 0;
}

static int s3c_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	void __iomem *base = s3c_rtc_base;
	unsigned int alrm_en;

	int year = tm->tm_year - 100;

	pr_info("s3c_rtc_setalarm: %d, %02x/%02x/%02x %02x.%02x.%02x\n",
		 alrm->enabled,
		 tm->tm_mday & 0xff, tm->tm_mon & 0xff, tm->tm_year & 0xff,
		 tm->tm_hour & 0xff, tm->tm_min & 0xff, tm->tm_sec);

	alrm_en = readb(base + S3C2410_RTCALM) & S3C2410_RTCALM_ALMEN;
	writeb(0x00, base + S3C2410_RTCALM);

	if (tm->tm_sec < 60 && tm->tm_sec >= 0) {
		alrm_en |= S3C2410_RTCALM_SECEN;
		writeb(bin2bcd(tm->tm_sec), base + S3C2410_ALMSEC);
	}

	if (tm->tm_min < 60 && tm->tm_min >= 0) {
		alrm_en |= S3C2410_RTCALM_MINEN;
		writeb(bin2bcd(tm->tm_min), base + S3C2410_ALMMIN);
	}

	if (tm->tm_hour < 24 && tm->tm_hour >= 0) {
		alrm_en |= S3C2410_RTCALM_HOUREN;
		writeb(bin2bcd(tm->tm_hour), base + S3C2410_ALMHOUR);
	}

	if (tm->tm_mday >= 0) {
		alrm_en |= S3C2410_RTCALM_DAYEN;
		writeb(bin2bcd(tm->tm_mday), base + S3C2410_ALMDATE);
	}

	if (tm->tm_mon < 13 && tm->tm_mon >= 0) {
		alrm_en |= S3C2410_RTCALM_MONEN;
		writeb(bin2bcd(tm->tm_mon + 1), base + S3C2410_ALMMON);
	}

	if (year < 100 && year >= 0) {
		alrm_en |= S3C2410_RTCALM_YEAREN;
		writeb(bin2bcd(year), base + S3C2410_ALMYEAR);
	}

	pr_info("setting S3C2410_RTCALM to %08x\n", alrm_en);

	writeb(alrm_en, base + S3C2410_RTCALM);

	s3c_rtc_setaie(alrm->enabled);

	return 0;
}

static int s3c_rtc_ioctl(struct device *dev,
			 unsigned int cmd, unsigned long arg)
{
	unsigned int ret = -ENOIOCTLCMD;

	switch (cmd) {
	case RTC_AIE_OFF:
	case RTC_AIE_ON:
		s3c_rtc_setaie((cmd == RTC_AIE_ON) ? 1 : 0);
		ret = 0;
		break;

	case RTC_PIE_OFF:
	case RTC_PIE_ON:
		tick_count = 0;
		s3c_rtc_setpie(dev,(cmd == RTC_PIE_ON) ? 1 : 0);
		ret = 0;
		break;

	case RTC_IRQP_READ:
		ret = put_user(s3c_rtc_freq, (unsigned long __user *)arg);
		break;

	case RTC_IRQP_SET:
		/* check for power of 2 */

		if ((arg & (arg-1)) != 0 || arg < 1) {
			ret = -EINVAL;
			goto exit;
		}

		pr_debug("s3c2410_rtc: setting frequency %ld\n", arg);

		s3c_rtc_setfreq(dev, arg);
		ret = 0;
		break;

	case RTC_UIE_ON:
	case RTC_UIE_OFF:
		ret = -EINVAL;
	}

 exit:
	return ret;
}

static int s3c_rtc_proc(struct device *dev, struct seq_file *seq)
{
	unsigned int ticnt = readb(s3c_rtc_base + S3C2410_TICNT);

	seq_printf(seq, "periodic_IRQ\t: %s\n",
		     (ticnt & S3C2410_TICNT_ENABLE) ? "yes" : "no" );
	return 0;
}

static int s3c_rtc_open(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);
	int ret;

	ret = request_irq(s3c_rtc_alarmno, s3c_rtc_alarmirq,
			  IRQF_DISABLED,  "s3c2410-rtc alarm", rtc_dev);

	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", s3c_rtc_alarmno, ret);
		return ret;
	}

	return ret;

#if !defined(CONFIG_MACH_VOLANS)
	ret = request_irq(s3c_rtc_tickno, s3c_rtc_tickirq,
			  IRQF_DISABLED,  "s3c2410-rtc tick", rtc_dev);

	if (ret) {
		dev_err(dev, "IRQ%d error %d\n", s3c_rtc_tickno, ret);
		goto tick_err;
	}

	return ret;

 tick_err:
	free_irq(s3c_rtc_alarmno, rtc_dev);
	return ret;
#endif	/* CONFIG_MACH_VOLANS */
}

static void s3c_rtc_release(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct rtc_device *rtc_dev = platform_get_drvdata(pdev);

	/* do not clear AIE here, it may be needed for wake */

	s3c_rtc_setpie(dev, 0);
	free_irq(s3c_rtc_alarmno, rtc_dev);
#if !defined(CONFIG_MACH_VOLANS)
	free_irq(s3c_rtc_tickno, rtc_dev);
#endif	/* CONFIG_MACH_VOLANS */
}

static const struct rtc_class_ops s3c_rtcops = {
	.open		= s3c_rtc_open,
	.release	= s3c_rtc_release,
	.ioctl		= s3c_rtc_ioctl,
	.read_time	= s3c_rtc_gettime,
	.set_time	= s3c_rtc_settime,
	.read_alarm	= s3c_rtc_getalarm,
	.set_alarm	= s3c_rtc_setalarm,
	.irq_set_freq	= s3c_rtc_setfreq,
	.irq_set_state	= s3c_rtc_setpie,
	.proc	        = s3c_rtc_proc,
};

static void s3c_rtc_enable(struct platform_device *pdev, int en)
{
	void __iomem *base = s3c_rtc_base;

	if (s3c_rtc_base == NULL)
		return;

	s3c_rtc_enable_set(pdev,base,en);
}

static int s3c_rtc_remove(struct platform_device *dev)
{
	struct rtc_device *rtc = platform_get_drvdata(dev);

	platform_set_drvdata(dev, NULL);
	rtc_device_unregister(rtc);

	s3c_rtc_setpie(&dev->dev, 0);
	s3c_rtc_setaie(0);

	iounmap(s3c_rtc_base);
	release_resource(s3c_rtc_mem);
	kfree(s3c_rtc_mem);

#ifdef CONFIG_MACH_VOLANS
	s3c_rtc_enable(dev, 0);
	cancel_delayed_work(&rtc_sync_work);
#endif	/* CONFIG_MACH_VOLANS */

	return 0;
}

static int s3c_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	struct resource *res;
	int ret;
	unsigned char bcd_tmp,bcd_loop;

	pr_debug("%s: probe=%p\n", __func__, pdev);

	/* find the IRQs */

	s3c_rtc_tickno = platform_get_irq(pdev, 1);
	if (s3c_rtc_tickno < 0) {
		dev_err(&pdev->dev, "no irq for rtc tick\n");
		return -ENOENT;
	}

	s3c_rtc_alarmno = platform_get_irq(pdev, 0);
	if (s3c_rtc_alarmno < 0) {
		dev_err(&pdev->dev, "no irq for alarm\n");
		return -ENOENT;
	}

	printk("s3c2410_rtc: tick irq %d, alarm irq %d\n",
		 s3c_rtc_tickno, s3c_rtc_alarmno);

	/* get the memory region */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		return -ENOENT;
	}

	s3c_rtc_mem = request_mem_region(res->start,
					 res->end-res->start+1,
					 pdev->name);

	if (s3c_rtc_mem == NULL) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_nores;
	}

	s3c_rtc_base = ioremap(res->start, res->end - res->start + 1);
	if (s3c_rtc_base == NULL) {
		dev_err(&pdev->dev, "failed ioremap()\n");
		ret = -EINVAL;
		goto err_nomap;
	}

	/* check to see if everything is setup correctly */

	s3c_rtc_enable(pdev, 1);

 	pr_debug("s3c2410_rtc: RTCCON=%02x\n",
		 readb(s3c_rtc_base + S3C2410_RTCCON));

	s3c_rtc_setfreq(&pdev->dev, 1);

	device_init_wakeup(&pdev->dev, 1);

	/* register RTC and exit */

	rtc = rtc_device_register("s3c", &pdev->dev, &s3c_rtcops,
				  THIS_MODULE);

	if (IS_ERR(rtc)) {
		dev_err(&pdev->dev, "cannot attach rtc\n");
		ret = PTR_ERR(rtc);
		goto err_nortc;
	}

	rtc->max_user_freq = S3C_MAX_CNT;

	/* check rtc time */
	for (bcd_loop = S3C2410_RTCSEC ; bcd_loop <= S3C2410_RTCYEAR ; bcd_loop +=0x4)
	{
		bcd_tmp = readb(s3c_rtc_base + bcd_loop);
		if(((bcd_tmp & 0xf) > 0x9) || ((bcd_tmp & 0xf0) > 0x90))
			writeb(0, s3c_rtc_base + bcd_loop);
	}

	platform_set_drvdata(pdev, rtc);

#ifdef CONFIG_MACH_VOLANS
	schedule_delayed_work(&rtc_sync_work, RTC_SYNC_INTERVAL);
#endif	/* CONFIG_MACH_VOLANS */

	return 0;

 err_nortc:
	s3c_rtc_enable(pdev, 0);
	iounmap(s3c_rtc_base);

 err_nomap:
	release_resource(s3c_rtc_mem);

 err_nores:
	return ret;
}

#ifdef CONFIG_PM

/* RTC Power management control */
static struct timespec s3c_rtc_delta;
static int ticnt_save;

static int s3c_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct rtc_time tm;
	struct timespec time;

	time.tv_nsec = 0;
	/* save TICNT for anyone using periodic interrupts */
	ticnt_save = readb(s3c_rtc_base + S3C2410_TICNT);

	s3c_rtc_gettime(&pdev->dev, &tm);
	rtc_tm_to_time(&tm, &time.tv_sec);
	save_time_delta(&s3c_rtc_delta, &time);

#ifdef CONFIG_MACH_VOLANS
	cancel_delayed_work(&rtc_sync_work);
#else
	s3c_rtc_enable(pdev, 0);
#endif	/* CONFIG_MACH_VOLANS */
	return 0;
}

static int s3c_rtc_resume(struct platform_device *pdev)
{
#ifdef CONFIG_MACH_VOLANS
	schedule_delayed_work(&rtc_sync_work, HZ/10);
#else
	struct rtc_time tm;
	struct timespec time;

	time.tv_nsec = 0;

	s3c_rtc_enable(pdev, 1);
	s3c_rtc_gettime(&pdev->dev, &tm);
	rtc_tm_to_time(&tm, &time.tv_sec);
	restore_time_delta(&s3c_rtc_delta, &time);
	writeb(ticnt_save, s3c_rtc_base + S3C2410_TICNT);
#endif	/* CONFIG_MACH_VOLANS */

	return 0;
}
#else
#define s3c_rtc_suspend NULL
#define s3c_rtc_resume  NULL
#endif

static struct platform_driver s3c2410_rtc_driver = {
	.probe		= s3c_rtc_probe,
	.remove		= s3c_rtc_remove,
	.suspend	= s3c_rtc_suspend,
	.resume		= s3c_rtc_resume,
	.driver		= {
		.name	= "s3c2410-rtc",
		.owner	= THIS_MODULE,
	},
};

static char __initdata banner[] = "S3C24XX RTC, (c) 2004,2006 Simtec Electronics\n";

static int __init s3c_rtc_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c2410_rtc_driver);
}

static void __exit s3c_rtc_exit(void)
{
	platform_driver_unregister(&s3c2410_rtc_driver);
}

module_init(s3c_rtc_init);
module_exit(s3c_rtc_exit);

MODULE_DESCRIPTION("Samsung S3C RTC Driver");
MODULE_AUTHOR("Ben Dooks <ben@simtec.co.uk>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c2410-rtc");
