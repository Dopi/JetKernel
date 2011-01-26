/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Pötzl <herbert@13thfloor.at>
 *	- added clock (de-)allocation code
 *
 * 2005-03-06: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - h1940_ -> s3c24xx (this driver is now also used on the n30
 *        machines :P)
 *      - Debug messages are now enabled with the config option
 *        TOUCHSCREEN_S3C_DEBUG
 *      - Changed the way the value are read
 *      - Input subsystem should now work
 *      - Use ioremap and readl/writel
 *
 * 2005-03-23: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - Make use of some undocumented features of the touchscreen
 *        controller
 *
 */


#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/uaccess.h>

#include <plat/regs-adc.h>
#include <plat/adc.h>
#include <mach/irqs.h>

#define ADC_MINOR 	131
#define ADC_INPUT_PIN   _IOW('S', 0x0c, unsigned long)

#define ADC_WITH_TOUCHSCREEN

static struct clk	*adc_clock;
static void __iomem 	*base_addr;
static int adc_port =  0;
struct s3c_adc_mach_info *plat_data;

static int ready_to_work = 0;

#ifdef ADC_WITH_TOUCHSCREEN
static DEFINE_MUTEX(adc_mutex);
struct mutex *g_s3c_adc_mutex = &adc_mutex;

#ifndef ADC_WITH_TOUCHSCREEN
static struct resource	*adc_mem;
#endif

static unsigned long data_for_ADCCON;
static unsigned long data_for_ADCTSC;

static void s3c_adc_save_SFR_on_ADC(void) {

	data_for_ADCCON = readl(base_addr+S3C_ADCCON);
	data_for_ADCTSC = readl(base_addr+S3C_ADCTSC);
}

static void s3c_adc_restore_SFR_on_ADC(void) {

	writel(data_for_ADCCON, base_addr+S3C_ADCCON);
	writel(data_for_ADCTSC, base_addr+S3C_ADCTSC);
}
#else
static DEFINE_MUTEX(adc_mutex);
#endif

static int s3c_adc_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO " s3c_adc_open() entered\n");
	return 0;
}

int s3c_adc_convert(void)
{
	unsigned int adc_return = 0;
	unsigned long data0;
	unsigned long data1;
	int i = 0;
	static int extradelay=0;

	if (!ready_to_work) {
		pr_err("%s:E: tried to read ADC before ready to work\n", __func__);
		return -EIO;
	}
	
	writel((readl(base_addr+S3C_ADCCON) & ~S3C_ADCCON_MUXMASK)
			| S3C_ADCCON_SELMUX(adc_port), base_addr+S3C_ADCCON);

	/* Normal Operation Mode */
	writel(readl(base_addr+S3C_ADCCON) & ~S3C_ADCCON_STDBM,
			base_addr+S3C_ADCCON);

	do {
		writel(readl(base_addr+S3C_ADCCON) | S3C_ADCCON_ENABLE_START,
				base_addr+S3C_ADCCON);

		// Change adc conversion wait time from 10us to 11us after 
		//  a first failure. 
		// We are using PCLK at 66Mhz for all DVFS clock ranges, 
		// BUT, in S3C6410X_UM_Rev1.01_080729 page 39-9, it states that
		//  "During ADC conversion PCLK (Max. 50MHz) is used."
		// ADC conversion time when PCLK 50Mhz is 
		//  5us(conversion time) + 5.1us(when DELAY in ADCDLY is 0xff) = 10.1us
		udelay(10+extradelay); 

		data0 = readl(base_addr+S3C_ADCCON);
		if (!(data0 & S3C_ADCCON_ENABLE_START) && (data0 & S3C_ADCCON_ECFLG)) {
			data1 = readl(base_addr+S3C_ADCDAT0);
			break;
		} else {
			extradelay = 2;
			pr_err("%s:E: read ADC failed(i=%d,port=%d)\n",
					__func__, i, adc_port);
			if (++i > 1)
				goto __end__;
		}
	} while (1);

	if (plat_data->resolution == 12)
	{
		adc_return = data1 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
	} else {
		adc_return = data1 & S3C_ADCDAT0_XPDATA_MASK;
	}

__end__:
	/* Standby Mode */
	writel(readl(base_addr+S3C_ADCCON) | S3C_ADCCON_STDBM,
			base_addr+S3C_ADCCON);

	return adc_return;
}

#if 0
static int s3c_adc_get(struct s3c_adc_request *req)
{
	unsigned adc_channel = req->channel;
	int adc_value_ret = 0;

	adc_value_ret = s3c_adc_convert();

	req->callback(adc_channel,req->param,adc_value_ret);

	return 0;
}
#endif

int s3c_adc_get_adc_data(int channel)
{	
	int adc_value = 0;
	int cur_adc_port = 0;

#ifdef ADC_WITH_TOUCHSCREEN
        mutex_lock(&adc_mutex);
	s3c_adc_save_SFR_on_ADC();
#else
        mutex_lock(&adc_mutex);
#endif

	cur_adc_port = adc_port;
	adc_port = channel;

	adc_value = s3c_adc_convert();

	adc_port = cur_adc_port;

#ifdef ADC_WITH_TOUCHSCREEN
	s3c_adc_restore_SFR_on_ADC();
	mutex_unlock(&adc_mutex);
#else
	mutex_unlock(&adc_mutex);
#endif

	pr_debug("%s : Converted Value: %03d\n", __FUNCTION__, adc_value);

	return adc_value;
}
EXPORT_SYMBOL(s3c_adc_get_adc_data);


static ssize_t
s3c_adc_read(struct file *file, char __user * buffer,
		size_t size, loff_t * pos)
{
	int  adc_value = 0;

	printk(KERN_INFO " s3c_adc_read() entered\n");

#ifdef ADC_WITH_TOUCHSCREEN
        mutex_lock(&adc_mutex);
	s3c_adc_save_SFR_on_ADC();
#endif

	adc_value = s3c_adc_convert();

#ifdef ADC_WITH_TOUCHSCREEN
	s3c_adc_restore_SFR_on_ADC();
	mutex_unlock(&adc_mutex);
#endif

	printk(KERN_INFO " Converted Value: %03d\n", adc_value);

	if (copy_to_user(buffer, &adc_value, sizeof(unsigned int))) {
		return -EFAULT;
	}
	return sizeof(unsigned int);
}


static int s3c_adc_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{

       printk(KERN_INFO " s3c_adc_ioctl(cmd:: %d) entered\n", cmd);

	switch (cmd) {

		case ADC_INPUT_PIN:
			  adc_port = (unsigned int) arg;

                       if (adc_port >= 4)
                            printk(" %d is already reserved for TouchScreen\n", adc_port);
                      return 0;

              default:
			return -ENOIOCTLCMD;

	}
}

static struct file_operations s3c_adc_fops = {
	.owner		= THIS_MODULE,
	.read		= s3c_adc_read,
	.open		= s3c_adc_open,
	.ioctl		= s3c_adc_ioctl,
};

static struct miscdevice s3c_adc_miscdev = {
	.minor		= ADC_MINOR,
	.name		= "adc",
	.fops		= &s3c_adc_fops,
};

static struct s3c_adc_mach_info *s3c_adc_get_platdata(struct device *dev)
{
	if(dev->platform_data != NULL)
	{
		printk(KERN_INFO "ADC platform data read\n");
		return (struct s3c_adc_mach_info*) dev->platform_data;
	}else{
		printk(KERN_INFO "No ADC platform data \n");
		return 0;
	}
}

/*
 * The functions for inserting/removing us as a module.
 */

static int __init s3c_adc_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct device *dev;
	int ret;
	int size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev = &pdev->dev;

	if(res == NULL){
		dev_err(dev,"no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;
	
#ifndef ADC_WITH_TOUCHSCREEN
	adc_mem = request_mem_region(res->start, size, pdev->name);
	if(adc_mem == NULL){
		dev_err(dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}
#endif

	base_addr = ioremap(res->start, size);
	if(base_addr ==  NULL){
		dev_err(dev,"fail to ioremap() region\n");
		ret = -ENOENT;
		goto err_map;
	}
	
	adc_clock = clk_get(&pdev->dev, "adc");

	if(IS_ERR(adc_clock)){
		dev_err(dev,"failed to fine ADC clock source\n");
		ret = PTR_ERR(adc_clock);
		goto err_clk;
	}

	clk_enable(adc_clock);

	/* read platform data from device struct */
	plat_data = s3c_adc_get_platdata(&pdev->dev);

	if ((plat_data->presc & 0xff) > 0)
		writel(S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(plat_data->presc & 0xff), base_addr + S3C_ADCCON);
	else
		writel(0, base_addr+S3C_ADCCON);

	/* Initialise registers */
	if ((plat_data->delay & 0xffff) > 0)
		writel(plat_data->delay & 0xffff, base_addr + S3C_ADCDLY);

	if (plat_data->resolution == 12) 
		writel(readl(base_addr + S3C_ADCCON) | S3C_ADCCON_RESSEL_12BIT, base_addr + S3C_ADCCON);

	ret = misc_register(&s3c_adc_miscdev);
	if (ret) {
		printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
			ADC_MINOR, ret);
		goto err_clk;
	}

	ready_to_work = 1;
	printk(KERN_INFO "S3C64XX ADC driver successfully probed !\n");

	return 0;

err_clk:
	clk_disable(adc_clock);
	clk_put(adc_clock);

err_map:
	iounmap(base_addr);

#ifndef ADC_WITH_TOUCHSCREEN
err_req:
	release_resource(adc_mem);
	kfree(adc_mem);
#endif
	return ret;
}


static int s3c_adc_remove(struct platform_device *dev)
{
	printk(KERN_INFO "s3c_adc_remove() of ADC called !\n");
	return 0;
}

#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

static int s3c_adc_suspend(struct platform_device *dev, pm_message_t state)
{
	ready_to_work = 0;

	adccon = readl(base_addr+S3C_ADCCON);
	adctsc = readl(base_addr+S3C_ADCTSC);
	adcdly = readl(base_addr+S3C_ADCDLY);

	clk_disable(adc_clock);

	return 0;
}

static int s3c_adc_resume(struct platform_device *pdev)
{
	clk_enable(adc_clock);

	writel(adccon, base_addr+S3C_ADCCON);
	writel(adctsc, base_addr+S3C_ADCTSC);
	writel(adcdly, base_addr+S3C_ADCDLY);

	ready_to_work = 1;

	return 0;
}
#else
#define s3c_adc_suspend NULL
#define s3c_adc_resume  NULL
#endif

static struct platform_driver s3c_adc_driver = {
       .probe          = s3c_adc_probe,
       .remove         = s3c_adc_remove,
       .suspend        = s3c_adc_suspend,
       .resume         = s3c_adc_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-adc",
	},
};

static char banner[] __initdata = KERN_INFO "S3C64XX ADC driver, (c) 2007 Samsung Electronics\n";

int __init s3c_adc_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_adc_driver);
}

void __exit s3c_adc_exit(void)
{
	platform_driver_unregister(&s3c_adc_driver);
}

module_init(s3c_adc_init);
module_exit(s3c_adc_exit);

MODULE_AUTHOR("boyko.lee@samsung.com>");
MODULE_DESCRIPTION("S3C64XX ADC driver");
MODULE_LICENSE("GPL");
