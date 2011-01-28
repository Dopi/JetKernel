/* rtc-samsungjet: RTC driver for Samsung S8000 Jet
 *
 * Copyright (C) 2010 Vaclav Peroutka <vaclavpe@gmail.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h> 
#include <linux/rtc.h>
#include <linux/bcd.h>

#define HOUR_12                         (1 << 7)
#define HOUR_PM                         (1 << 5)

extern int is_pmic_initialized(void);
extern unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length);
extern unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length);
#define MAX8906_RTC_ID (0xD0)

/* as simple as can be, and no simpler. */
struct samsungjet_rtc {
	struct rtc_device *rtc;
	spinlock_t lock;
};

static int samsungjet_read_time(struct device *dev, struct rtc_time *tm)
{
	struct samsungjet_rtc *p = dev_get_drvdata(dev);
	unsigned long flags;
	int retval = -EOPNOTSUPP;
	unsigned char i2cdata;

	spin_lock_irqsave(&p->lock, flags);
	if (is_pmic_initialized()) {
	  retval = 0;

	  pmic_read( MAX8906_RTC_ID, 0x00, &i2cdata, 1);
	  tm->tm_sec = bcd2bin(i2cdata & 0x7f);
	  pmic_read( MAX8906_RTC_ID, 0x01, &i2cdata, 1);
	  tm->tm_min = bcd2bin(i2cdata & 0x7f);
	  pmic_read( MAX8906_RTC_ID, 0x02, &i2cdata, 1);
	  if (i2cdata & HOUR_12) {
	    tm->tm_hour = bcd2bin(i2cdata & 0x1f);
	    if (i2cdata & HOUR_PM) {
	      tm->tm_hour += 12;
	    }
	  } else {
	    tm->tm_hour = bcd2bin(i2cdata & 0x3f);
	  }
	  pmic_read( MAX8906_RTC_ID, 0x03, &i2cdata, 1);
	  tm->tm_wday = bcd2bin(i2cdata & 0x7);
	  pmic_read( MAX8906_RTC_ID, 0x04, &i2cdata, 1);
	  tm->tm_mday = bcd2bin(i2cdata & 0x3f);
	  pmic_read( MAX8906_RTC_ID, 0x05, &i2cdata, 1);
	  tm->tm_mon = bcd2bin(i2cdata & 0x7f) - 1; // some strange correction by vaclavpe
	  pmic_read( MAX8906_RTC_ID, 0x06, &i2cdata, 1);
	  tm->tm_year = bcd2bin(i2cdata)+100; // it works like that
	  /* pmic_read( MAX8906_RTC_ID, 0x07, &yearm, 1); //here is 0x20 for 21th century */
	  /* tm->tm_dummyvalue = bcd2bin(i2cdata & 0x7f); */
	  //printk("[%s] should be correct\n",__FUNCTION__);
	} else {
	  printk("[%s] PMIC not yet ready\n",__FUNCTION__);
	}

	spin_unlock_irqrestore(&p->lock, flags);

	return retval;
}

static int samsungjet_set_time(struct device *dev, struct rtc_time *tm)
{
	struct samsungjet_rtc *p = dev_get_drvdata(dev);
	unsigned long flags;
	int ret;
	int retval = -EOPNOTSUPP;

	spin_lock_irqsave(&p->lock, flags);
	printk("[%s] dummy",__FUNCTION__);
	/* buf[M41T80_REG_SEC] = */
	/* 	bin2bcd(tm->tm_sec) | (buf[M41T80_REG_SEC] & ~0x7f); */
	/* buf[M41T80_REG_MIN] = */
	/* 	bin2bcd(tm->tm_min) | (buf[M41T80_REG_MIN] & ~0x7f); */
	/* buf[M41T80_REG_HOUR] = */
	/* 	bin2bcd(tm->tm_hour) | (buf[M41T80_REG_HOUR] & ~0x3f) ; */
	/* buf[M41T80_REG_WDAY] = */
	/* 	(tm->tm_wday & 0x07) | (buf[M41T80_REG_WDAY] & ~0x07); */
	/* buf[M41T80_REG_DAY] = */
	/* 	bin2bcd(tm->tm_mday) | (buf[M41T80_REG_DAY] & ~0x3f); */
	/* buf[M41T80_REG_MON] = */
	/* 	bin2bcd(tm->tm_mon + 1) | (buf[M41T80_REG_MON] & ~0x1f); */
	/* /\* assume 20YY not 19YY *\/ */
	/* buf[M41T80_REG_YEAR] = bin2bcd(tm->tm_year % 100);  */

	spin_unlock_irqrestore(&p->lock, flags);

	return retval;
}

static const struct rtc_class_ops samsungjet_rtc_ops = {
	.read_time = samsungjet_read_time,
	.set_time = samsungjet_set_time,
};

static int __devinit samsungjet_rtc_probe(struct platform_device *dev)
{
	struct samsungjet_rtc *p;

	p = kzalloc(sizeof (*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	spin_lock_init(&p->lock);

	p->rtc = rtc_device_register("rtc-samsungjet", &dev->dev, &samsungjet_rtc_ops,
					THIS_MODULE);
	if (IS_ERR(p->rtc)) {
		int err = PTR_ERR(p->rtc);
		printk("[%s] device_register_error %d\n",__FUNCTION__, err);
		kfree(p);
		return err;
	}

	platform_set_drvdata(dev, p);

	return 0;
}

static int samsungjet_rtc_remove(struct platform_device *dev)
{
	struct samsungjet_rtc *p = platform_get_drvdata(dev);

	rtc_device_unregister(p->rtc);
	kfree(p);

	return 0;
}

/* static int samsungjet_rtc_suspend(struct platform_device *dev) */
/* { */
/*   return 0; */
/* } */

/* static int samsungjet_rtc_resume(struct platform_device *dev) */
/* { */
/*   return 0; */
/* } */

static struct platform_driver samsungjet_rtc_driver = {
	.driver = {
		.name = "rtc-samsungjet",
		.owner = THIS_MODULE,
	},
	.probe = samsungjet_rtc_probe,
	.remove = samsungjet_rtc_remove,
	/* .suspend = samsungjet_rtc_suspend, */
	/* .resume = samsungjet_rtc_resume, */
};

int __init samsungjet_rtc_init(void)
{
  return platform_driver_register(&samsungjet_rtc_driver);
}

void __exit samsungjet_rtc_exit(void)
{
	platform_driver_unregister(&samsungjet_rtc_driver);
}

module_init(samsungjet_rtc_init);
module_exit(samsungjet_rtc_exit);

MODULE_AUTHOR("Vaclav Peroutka <vaclavpe@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RTC driver for Samsung S8000 Jet");
