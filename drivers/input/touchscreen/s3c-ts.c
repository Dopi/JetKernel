/* linux/drivers/input/touchscreen/s3c-ts.c
 *
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
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Potzl <herbert@13thfloor.at>
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
 * 2006-09-05: Ryu Euiyoul <ryu.real@gmail.com>
 *      - added power management suspend and resume code
 *
 * 2010-08-01: Dopi <Dopi711@googlemail.com>
 *      - added calibration code
 *
 * 2010-11-15: Vaclav Peroutka <vaclavpe@gmail.com>
 *      - added function for reading AIN0-AIN3
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
#include <linux/clk.h>
#include <linux/earlysuspend.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <plat/regs-adc.h>
#include <plat/ts.h>
#include <mach/irqs.h>

#include <plat/gpio-cfg.h>

spinlock_t adc_lock = SPIN_LOCK_UNLOCKED;

#define CONFIG_TOUCHSCREEN_S3C_DEBUG
#undef CONFIG_TOUCHSCREEN_S3C_DEBUG

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
#define DEBUG_LVL    KERN_ERR
#define DPRINTK(x...) printk(DEBUG_LVL "S3C TouchScreen: " x)
#define IPRINTK(x...) printk(KERN_INFO "S3C TouchScreen: " x)
#else
#define DPRINTK(x...)           /* !!!! */
#define IPRINTK(x...)           /* !!!! */
#endif

#define CONFIG_TOUCHSCREEN_S3C_DEBUG_SPECIAL
#undef CONFIG_TOUCHSCREEN_S3C_DEBUG_SPECIAL

#define TOUCHSCREEN_S3C_GET_CALIBRATION	// enable calibration logging
#undef TOUCHSCREEN_S3C_GET_CALIBRATION

#ifdef TOUCHSCREEN_S3C_GET_CALIBRATION
	int calibrate_count=0;
	int calibrate_min_x=S3C_ADCDAT0_XPDATA_MASK_12BIT << ts->shift;
	int calibrate_max_x=0;
	int calibrate_min_y=S3C_ADCDAT1_YPDATA_MASK_12BIT << ts->shift;
	int calibrate_max_y=0;
#endif // TOUCHSCREEN_S3C_GET_CALIBRATION

#ifdef CONFIG_S3C_TS_CALIBRATION
	int xmin = 0;
	int xmax = S3C_ADCDAT0_XPDATA_MASK_12BIT;	// FIXME: only valid for 12bit 
	int ymin = 0;
	int ymax = S3C_ADCDAT1_YPDATA_MASK_12BIT;	// FIXME: only valid for 12bit 

module_param(xmin, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(xmin, "S3C-TS calibration x minimum value");
module_param(xmax, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(xmax, "S3C-TS calibration x maximum value");
module_param(ymin, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(ymin, "S3C-TS calibration y minimum value");
module_param(ymax, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(ymax, "S3C-TS calibration y maximum value");
#endif // CONFIG_S3C_TS_CALIBRATION

#ifdef CONFIG_HAS_EARLYSUSPEND
void s3c_ts_early_suspend(struct early_suspend *h);
void s3c_ts_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

/* For ts->dev.id.version */
#define S3C_TSVERSION	0x0101

#define WAIT4INT(x)  (((x)<<8) | \
		     S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
		     S3C_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
		     S3C_ADCTSC_AUTO_PST | S3C_ADCTSC_XY_PST(0))

#define SEL_MUX_MASK	S3C_ADCMUX_MASK

#define YP_PRESSURE_ADCTSC	(S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XM_SEN | \
				S3C_ADCTSC_PULL_UP_DISABLE | S3C_ADCTSC_XY_PST(0))
#define YP_SEL_MUX		S3C_ADCMUX_YP	

#define XP_PRESSURE_ADCTSC	(S3C_ADCTSC_XP_SEN | S3C_ADCTSC_XM_SEN | \
				S3C_ADCTSC_PULL_UP_DISABLE | S3C_ADCTSC_XY_PST(0))
#define XP_SEL_MUX		S3C_ADCMUX_XP

#define YM_PRESSURE_ADCTSC	(S3C_ADCTSC_XP_SEN | S3C_ADCTSC_XM_SEN | \
				S3C_ADCTSC_PULL_UP_DISABLE | S3C_ADCTSC_XY_PST(0))
#define YM_SEL_MUX		S3C_ADCMUX_YM

/* Touchscreen default configuration */
struct s3c_ts_mach_info s3c_ts_default_cfg __initdata = {
	.delay			= 50000,	// 50000
	.presc			= 49,
	.oversampling_shift	= 2,
	.resol_bit		= 12		// 10
};

/*
 * Definitions & global arrays.
 */
static char *s3c_ts_name = "S3C TouchScreen";
static void __iomem 		*ts_base;
static struct resource		*ts_mem;
static struct resource		*ts_irq;
static struct clk		*ts_clock;
static struct s3c_ts_info 	*ts;

/*
 * Touchscreen switch enable / disable
 */
#ifdef CONFIG_JET_OPTION
int s3c_ts_switch_enable(struct s3c_ts_info *ts)
{
	if(ts->ts_switch_claimed == 1) {
		s3c_gpio_setpull(GPIO_TOUCH_EN, S3C_GPIO_PULL_NONE);
		gpio_direction_output(GPIO_TOUCH_EN, GPIO_LEVEL_HIGH);

		IPRINTK("ts_switch enabled\n", s3c_ts_name);
	}

	return 0;
}

int s3c_ts_switch_disable(struct s3c_ts_info *ts)
{
	if(ts->ts_switch_claimed == 1) {
		s3c_gpio_setpull(GPIO_TOUCH_EN, S3C_GPIO_PULL_DOWN);
		gpio_direction_input(GPIO_TOUCH_EN);

		IPRINTK("ts_switch disabled\n", s3c_ts_name);
	}

	return 0;
}

int s3c_ts_switch_claim(struct s3c_ts_info *ts)
{
	int ret;

	ret = gpio_request(GPIO_TOUCH_EN,"s3c_ts_switch");
	if(ret == 0) {
		ts->ts_switch_claimed = 1;
		s3c_gpio_cfgpin(GPIO_TOUCH_EN, S3C_GPIO_SFN(GPIO_TOUCH_EN_AF));
		IPRINTK("Requesting GPIO%d succeded\n", s3c_ts_name, GPIO_TOUCH_EN);
	}
	else {
		ts->ts_switch_claimed = 0;
		IPRINTK("Requesting GPIO%d failed ERR %d\n", s3c_ts_name, GPIO_TOUCH_EN, ret);
	}

	return ret;	
}

int s3c_ts_switch_release(struct s3c_ts_info *ts)
{
	if(ts->ts_switch_claimed == 1) {
		s3c_ts_switch_disable(ts);
		gpio_free(GPIO_TOUCH_EN);
		ts->ts_switch_claimed = 0;

		IPRINTK("GPIO%d released\n", s3c_ts_name, GPIO_TOUCH_EN);
	}
	
	return 0;
}
#endif /* CONFIG_JET_OPTION */

static int curr_measure;

enum {
	XP_PRESSURE = 1,
	YM_PRESSURE,
	YP_PRESSURE,
};

/*
 * Store measured data for pressure calculation
 * [0] = R, [1] = XP, [2] = YM, [3] = YP
 */
static unsigned long long pressure_info[4];


static void touch_start_pressure_measure(int data)
{
	unsigned long adc_sel_mux=0, adc_tsc=0, adccon_reg;

	switch (data) {
	case YP_PRESSURE:
		adc_sel_mux = YP_SEL_MUX;
		adc_tsc = YP_PRESSURE_ADCTSC;
		break;
	case XP_PRESSURE:
		adc_sel_mux = XP_SEL_MUX;
		adc_tsc = XP_PRESSURE_ADCTSC;
		break;
	case YM_PRESSURE:
		adc_sel_mux = YM_SEL_MUX;
		adc_tsc = YM_PRESSURE_ADCTSC;
		break;
	default:
		break;
	}

	writel(adc_tsc , ts_base+S3C_ADCTSC);

	writel(adc_sel_mux, ts_base+S3C_ADCMUX);

	adccon_reg = readl(ts_base+S3C_ADCCON);
	adccon_reg |= (1<<0);
	writel(adccon_reg, ts_base+S3C_ADCCON);

	curr_measure = data;
}


static void check_valid_pressure(void)
{
	touch_start_pressure_measure(XP_PRESSURE);
}

static void touch_timer_fire(unsigned long data)
{
	unsigned long data0;
	unsigned long data1;
	int updown;

	data0 = readl(ts_base+S3C_ADCDAT0);
	data1 = readl(ts_base+S3C_ADCDAT1);

	updown = (!(data0 & S3C_ADCDAT0_UPDOWN)) &&
		(!(data1 & S3C_ADCDAT1_UPDOWN));

	if (updown) {
		if (ts->count) {

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
			{
				struct timeval tv;
				do_gettimeofday(&tv);
				DPRINTK("T: %06d, X: %03ld, Y: %03ld\n", (int)tv.tv_usec, ts->xp, ts->yp);
			}
#endif
#ifdef TOUCHSCREEN_S3C_GET_CALIBRATION
			if(ts->xp < calibrate_min_x) calibrate_min_x = ts->xp;
			if(ts->xp > calibrate_max_x) calibrate_max_x = ts->xp;
			if(ts->yp < calibrate_min_y) calibrate_min_y = ts->yp;
			if(ts->yp > calibrate_max_y) calibrate_max_y = ts->yp;
			calibrate_count++;

			if(calibrate_count=50000)
			{
				IPRINTK("Calibration Xmin: %03ld, Xmax: %03ld, Ymin: %03ld\n, Ymax: %03ld", 
					calibrate_min_x >> ts->shift, calibrate_max_x >> ts->shift, 
					calibrate_min_y >> ts->shift, calibrate_max_y >> ts->shift);

				calibrate_count=0;
				//calibrate_min_x=S3C_ADCDAT0_XPDATA_MASK_12BIT << ts->shift;
				//calibrate_max_x=0;
				//calibrate_min_y=S3C_ADCDAT1_YPDATA_MASK_12BIT << ts->shift;
				//calibrate_max_y=0;
			}
#endif
			ts->xp = (ts->xp >> ts->shift);
			ts->yp = (ts->yp >> ts->shift);
			if (ts->pressure < ts->threshold_pressure) {
				input_report_abs(ts->dev, ABS_X, ts->xp);
				input_report_abs(ts->dev, ABS_Y, ts->yp);

				input_report_key(ts->dev, BTN_TOUCH, 1);
				input_report_abs(ts->dev, ABS_PRESSURE, 1);
				input_sync(ts->dev);
			}
		}

		ts->xp = 0;
		ts->yp = 0;
		ts->count = 0;

#if !defined(CONFIG_TOUCHSCREEN_PRESSURE_CHECK)
		writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST,
				ts_base+S3C_ADCTSC);
		writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START,
				ts_base+S3C_ADCCON);
#else
		check_valid_pressure();
#endif
	} else {

		ts->count = 0;

		input_report_key(ts->dev, BTN_TOUCH, 0);
		input_report_abs(ts->dev, ABS_PRESSURE, 0);
		input_sync(ts->dev);

		writel(WAIT4INT(0), ts_base+S3C_ADCTSC);
	}
}

static struct timer_list touch_timer =
		TIMER_INITIALIZER(touch_timer_fire, 0, 0);

static irqreturn_t stylus_updown(int irqno, void *param)
{
	unsigned long data0;
	unsigned long data1;
	int updown;

	spin_lock( &adc_lock);

	data0 = readl(ts_base+S3C_ADCDAT0);
	data1 = readl(ts_base+S3C_ADCDAT1);

	updown = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 &
				S3C_ADCDAT1_UPDOWN));

	DPRINTK("  %c\n",	updown ? 'D' : 'U');

	/* TODO we should never get an interrupt with updown set while
	 * the timer is running, but maybe we ought to verify that the
	 * timer isn't running anyways. */

	if (updown)
#if !defined(CONFIG_TOUCHSCREEN_PRESSURE_CHECK)
		touch_timer_fire(0);
#else
		check_valid_pressure();
#endif
	if (ts->s3c_adc_con == ADC_TYPE_2) {
		__raw_writel(0x0, ts_base+S3C_ADCCLRWK);
		__raw_writel(0x0, ts_base+S3C_ADCCLRINT);
	}

	return IRQ_HANDLED;
}

static int calc_pressure(void)
{
	if (pressure_info[2] <= pressure_info[1])
		return -EINVAL;

	pressure_info[0] = ts->resistance * pressure_info[3] *
		((unsigned int)pressure_info[2] * 100000 /
		 (unsigned int)pressure_info[1] - 100000);

	do_div(pressure_info[0], (2^ts->resol_bit)*100000);
	DPRINTK("Raw pressure [%llu]\n", pressure_info[0]);
	ts->pressure = (int)(pressure_info[0]);
	DPRINTK("pressure = %d\n", ts->pressure);

	return 0;
}

static irqreturn_t stylus_action(int irqno, void *param)
{
	unsigned long data0;
	unsigned long data1;

	data0 = readl(ts_base+S3C_ADCDAT0);
	data1 = readl(ts_base+S3C_ADCDAT1);

#if defined(CONFIG_TOUCHSCREEN_PRESSURE_CHECK)
	if (curr_measure) {
		pressure_info[curr_measure] = (0xfff & data0);
		curr_measure++;

		if (curr_measure > 2) {
			if (curr_measure > 3) {
				curr_measure = 0;
		/*
		* If there is wrong value measured
		* Pressure check will be start again
		*/
				if (calc_pressure()) {
					check_valid_pressure();
					goto int_clear;
				}
			}
			goto start_conversion;
		} else {
			touch_start_pressure_measure(curr_measure);
		}
		goto int_clear;
	}
#endif

start_conversion:

	if (ts->resol_bit == 12) {
#if defined(CONFIG_TOUCHSCREEN_NEW)
		ts->yp += S3C_ADCDAT0_XPDATA_MASK_12BIT -
				(data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT);
		ts->xp += S3C_ADCDAT1_YPDATA_MASK_12BIT -
				(data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT);
#elif defined(CONFIG_TOUCHSCREEN_JET)
		ts->xp += S3C_ADCDAT0_XPDATA_MASK_12BIT -
				(data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT);
		ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
#else
		ts->xp += data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
		ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
#endif
	} else {
#if defined(CONFIG_TOUCHSCREEN_NEW)
		ts->yp += S3C_ADCDAT0_XPDATA_MASK -
				(data0 & S3C_ADCDAT0_XPDATA_MASK);
		ts->xp += S3C_ADCDAT1_YPDATA_MASK -
				(data1 & S3C_ADCDAT1_YPDATA_MASK);
#elif defined(CONFIG_TOUCHSCREEN_JET)
		ts->xp += S3C_ADCDAT0_XPDATA_MASK -
				(data0 & S3C_ADCDAT0_XPDATA_MASK);
		ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK;
#else
		ts->xp += data0 & S3C_ADCDAT0_XPDATA_MASK;
		ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK;
#endif
	}

	ts->count++;
	DPRINTK("count [%d]\n", ts->count);

	if (ts->count < (1<<ts->shift)) {
		writel(S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, ts_base+S3C_ADCTSC);
		writel(readl(ts_base+S3C_ADCCON) | S3C_ADCCON_ENABLE_START,
			ts_base+S3C_ADCCON);
	} else {
		mod_timer(&touch_timer, jiffies+1);
		writel(WAIT4INT(1), ts_base+S3C_ADCTSC);
		spin_unlock( &adc_lock);
	}

int_clear:

	if (ts->s3c_adc_con == ADC_TYPE_2) {
		__raw_writel(0x0, ts_base+S3C_ADCCLRWK);
		__raw_writel(0x0, ts_base+S3C_ADCCLRINT);
	}

	return IRQ_HANDLED;
}


static struct s3c_ts_mach_info *s3c_ts_get_platdata(struct device *dev)
{
	if (dev->platform_data != NULL)
		return (struct s3c_ts_mach_info *)dev->platform_data;

	return &s3c_ts_default_cfg;
}

static unsigned long data_for_ADCCON;
static unsigned long data_for_ADCTSC;

int s3c_ts_get_adc_data(int channel)
{	
	int adc_value = 0;
	int i = 0;
	unsigned long int data0, data1;


	spin_lock(&adc_lock);
	/* save data */
	disable_irq(IRQ_ADC);
	disable_irq(IRQ_PENDN);
	del_timer(&touch_timer);

	data_for_ADCCON = readl(ts_base+S3C_ADCCON);
	data_for_ADCTSC = readl(ts_base+S3C_ADCTSC);

	/* do real measurement */
	writel( 0x3fc4, ts_base+S3C_ADCCON); // standby
	writel( 0x58, ts_base+S3C_ADCTSC);
	data0 = (data_for_ADCCON & 0xffffffc0); // save prescaler etc value for ADCCON

	writel(data0 | S3C_ADCCON_SELMUX(channel) | S3C_ADCCON_STDBM, ts_base+S3C_ADCCON); // set channel
	writel(data0 | S3C_ADCCON_SELMUX(channel), ts_base+S3C_ADCCON); // go out from STANDBY

	data0 = readl(ts_base+S3C_ADCCON);
	writel(data0 | S3C_ADCCON_ENABLE_START, ts_base+S3C_ADCCON); // start confersion
	
	do {
		udelay(10);
		data0 = readl(ts_base+S3C_ADCCON);
		//printk("S3C-TS: ADCCON=0x%08lX\n",data0);
		if (!(data0 & S3C_ADCCON_ENABLE_START) && (data0 & S3C_ADCCON_ECFLG)) {
			data1 = readl(ts_base+S3C_ADCDAT0);
			break;
		} else {
			if (++i > 150) {
			  //	  printk("%s:E: read ADC failed(i=%d,port=%d, ADCON=0x%08lX)\n",__func__, (int)i, channel,data0);
			  goto __end__;
			}
		}
	} while (1);
	adc_value = data1 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
	printk("%s : AIN%d ,Converted Value: %08lX\n", __FUNCTION__, channel, data1);

 __end__:
	/* restore data for TS */
	writel(data_for_ADCCON, ts_base+S3C_ADCCON);
	writel(data_for_ADCTSC, ts_base+S3C_ADCTSC);

	spin_unlock( &adc_lock);

	add_timer(&touch_timer);
	enable_irq(IRQ_ADC);
	enable_irq(IRQ_PENDN);

	return adc_value;
}
EXPORT_SYMBOL(s3c_ts_get_adc_data); 


/*
 * The functions for inserting/removing us as a module.
 */
static int __init s3c_ts_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev;
	struct input_dev *input_dev;
	struct s3c_ts_mach_info *s3c_ts_cfg;
	int ret, size;

	dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(dev, "no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;
	ts_mem = request_mem_region(res->start, size, pdev->name);
	if (ts_mem == NULL) {
		dev_err(dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	ts_base = ioremap(res->start, size);
	if (ts_base == NULL) {
		dev_err(dev, "failed to ioremap() region\n");
		ret = -EINVAL;
		goto err_map;
	}

	ts_clock = clk_get(&pdev->dev, "adc");
	if (IS_ERR(ts_clock)) {
		dev_err(dev, "failed to find watchdog clock source\n");
		ret = PTR_ERR(ts_clock);
		goto err_clk;
	}

	clk_enable(ts_clock);

	s3c_ts_cfg = s3c_ts_get_platdata(&pdev->dev);

	if ((s3c_ts_cfg->presc&0xff) > 0)
		writel(S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(s3c_ts_cfg->presc&0xFF),
			ts_base+S3C_ADCCON);
	else
		writel(0, ts_base+S3C_ADCCON);


	/* Initialise registers */
	if ((s3c_ts_cfg->delay&0xffff) > 0)
		writel(s3c_ts_cfg->delay & 0xffff, ts_base+S3C_ADCDLY);

	if (s3c_ts_cfg->resol_bit == 12) {
		switch (s3c_ts_cfg->s3c_adc_con) {
		case ADC_TYPE_2:
			writel(readl(ts_base+S3C_ADCCON)|S3C_ADCCON_RESSEL_12BIT,
				ts_base+S3C_ADCCON);
			break;

		case ADC_TYPE_1:
			writel(readl(ts_base+S3C_ADCCON)|S3C_ADCCON_RESSEL_12BIT_1,
				ts_base+S3C_ADCCON);
			break;

		default:
			dev_err(dev, "Touchscreen over this type of AP isn't supported !\n");
			break;
		}
	}
	printk(KERN_INFO "%s: delay=%d oversampling=%d \n", s3c_ts_name, s3c_ts_cfg->delay & 0xffff, s3c_ts_cfg->oversampling_shift); // DEBUG
#ifdef CONFIG_S3C_TS_CALIBRATION
	printk(KERN_INFO "%s: calibration: xmin=%d xmax=%d ymin=%d ymax=%d\n", s3c_ts_name, xmin,xmax,ymin,ymax);
#endif

	writel(WAIT4INT(0), ts_base+S3C_ADCTSC);

	ts = kzalloc(sizeof(struct s3c_ts_info), GFP_KERNEL);

	input_dev = input_allocate_device();

	if (!input_dev) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	ts->dev = input_dev;

	ts->dev->evbit[0] = ts->dev->evbit[0] =
			BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	ts->dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	if (s3c_ts_cfg->resol_bit == 12) {
#ifdef CONFIG_S3C_TS_CALIBRATION
		input_set_abs_params(ts->dev, ABS_X, xmin, xmax, 0, 0);		// simple calibration
		input_set_abs_params(ts->dev, ABS_Y, ymin, ymax, 0, 0);		// simple calibration
#else
		input_set_abs_params(ts->dev, ABS_X, 0, 0xFFF, 0, 0);
		input_set_abs_params(ts->dev, ABS_Y, 0, 0xFFF, 0, 0);
#endif
	} else {
		input_set_abs_params(ts->dev, ABS_X, 0, 0x3FF, 0, 0);
		input_set_abs_params(ts->dev, ABS_Y, 0, 0x3FF, 0, 0);
	}

	input_set_abs_params(ts->dev, ABS_PRESSURE, 0, 1, 0, 0);

	sprintf(ts->phys, "input(ts)");

	ts->dev->name = s3c_ts_name;
	ts->dev->phys = ts->phys;
	ts->dev->id.bustype = BUS_RS232;
	ts->dev->id.vendor = 0xDEAD;
	ts->dev->id.product = 0xBEEF;
	ts->dev->id.version = S3C_TSVERSION;

	ts->shift = s3c_ts_cfg->oversampling_shift;
	ts->resol_bit = s3c_ts_cfg->resol_bit;
	ts->s3c_adc_con = s3c_ts_cfg->s3c_adc_con;
	ts->resistance = s3c_ts_cfg->panel_resistance;
	ts->threshold_pressure = s3c_ts_cfg->threshold;
	/* For IRQ_PENDUP */
	ts_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (ts_irq == NULL) {
		dev_err(dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq;
	}

	ret = request_irq(ts_irq->start, stylus_updown, IRQF_SAMPLE_RANDOM,
			"s3c_updown", ts);
	if (ret != 0) {
		dev_err(dev, "s3c_ts.c: Could not allocate ts IRQ_PENDN !\n");
		ret = -EIO;
		goto err_irq;
	}
	else
		printk(KERN_INFO "s3c_ts.c: TS_IRQ registered (%d) \n", ts_irq->start); // DEBUG

	/* For IRQ_ADC */
	ts_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (ts_irq == NULL) {
		dev_err(dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq;
	}
	ret = request_irq(ts_irq->start, stylus_action, IRQF_SAMPLE_RANDOM,
			"s3c_action", ts);
	if (ret != 0) {
		dev_err(dev, "s3c_ts.c: Could not allocate ts IRQ_ADC !\n");
		ret =  -EIO;
		goto err_irq;
	}
	else
		printk(KERN_INFO "s3c_ts.c: ADC_IRQ registered (%d) \n", ts_irq->start); // DEBUG

#ifdef CONFIG_JET_OPTION
	/* Claim & enable TS switch */
	ret = s3c_ts_switch_claim(ts);
	if (ret != 0) {
		ret =  -EIO;
		goto fail;
	}
	s3c_ts_switch_enable(ts);
#endif /* CONFIG_JET_OPTION */
		
	printk(KERN_INFO "%s got loaded successfully : %d bits\n", s3c_ts_name, s3c_ts_cfg->resol_bit);

	/* All went ok, so register to the input system */
	ret = input_register_device(ts->dev);
	if (ret) {
		dev_err(dev, "s3c_ts.c: Could not register input device(touchscreen)!\n");
		ret = -EIO;
		goto fail;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = s3c_ts_early_suspend;
	ts->early_suspend.resume = s3c_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	size = s3c_ts_get_adc_data(0);
	printk("ADC channel 0 = 0x%03X\n",size);
	size = s3c_ts_get_adc_data(1);
	printk("ADC channel 1 = 0x%03X\n",size);
	size = s3c_ts_get_adc_data(2);
	printk("ADC channel 2 = 0x%03X\n",size);
	size = s3c_ts_get_adc_data(3);
	printk("ADC channel 3 = 0x%03X\n",size);
	return 0;

fail:
	free_irq(ts_irq->start, ts->dev);
	free_irq(ts_irq->end, ts->dev);

err_irq:
	input_free_device(input_dev);
	kfree(ts);

err_alloc:
	clk_disable(ts_clock);
	clk_put(ts_clock);

err_clk:
	iounmap(ts_base);

err_map:
	release_resource(ts_mem);
	kfree(ts_mem);

err_req:
	return ret;
}

static int s3c_ts_remove(struct platform_device *dev)
{
	printk(KERN_INFO "%s: s3c_ts_remove() of TS called !\n", s3c_ts_name);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#ifdef CONFIG_JET_OPTION
	s3c_ts_switch_release(ts);
#endif /* CONFIG_JET_OPTION */

	disable_irq(IRQ_ADC);
	disable_irq(IRQ_PENDN);
	free_irq(IRQ_PENDN, ts->dev);
	free_irq(IRQ_ADC, ts->dev);

	if (ts_clock) {
		clk_disable(ts_clock);
		clk_put(ts_clock);
		ts_clock = NULL;
	}

	input_unregister_device(ts->dev);
	iounmap(ts_base);

	return 0;
}

#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

static int s3c_ts_suspend(struct platform_device *dev, pm_message_t state)
{
	adccon = readl(ts_base+S3C_ADCCON);
	adctsc = readl(ts_base+S3C_ADCTSC);
	adcdly = readl(ts_base+S3C_ADCDLY);

	disable_irq(IRQ_ADC);
	disable_irq(IRQ_PENDN);

	clk_disable(ts_clock);

#ifdef CONFIG_JET_OPTION
	s3c_ts_switch_disable(ts);
#endif /* CONFIG_JET_OPTION */

	return 0;
}

static int s3c_ts_resume(struct platform_device *pdev)
{
#ifdef CONFIG_JET_OPTION
	s3c_ts_switch_enable(ts);
#endif /* CONFIG_JET_OPTION */

	clk_enable(ts_clock);

	writel(adccon, ts_base+S3C_ADCCON);
	writel(adctsc, ts_base+S3C_ADCTSC);
	writel(adcdly, ts_base+S3C_ADCDLY);
	writel(WAIT4INT(0), ts_base+S3C_ADCTSC);

	enable_irq(IRQ_ADC);
	enable_irq(IRQ_PENDN);
	return 0;
}
#else
#define s3c_ts_suspend NULL
#define s3c_ts_resume  NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
void s3c_ts_early_suspend(struct early_suspend *h)
{
	s3c_ts_suspend(NULL, PMSG_SUSPEND);
}

void s3c_ts_late_resume(struct early_suspend *h)
{
	s3c_ts_resume(NULL);
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

static struct platform_driver s3c_ts_driver = {
       .probe          = s3c_ts_probe,
       .remove         = s3c_ts_remove,
       .suspend        = s3c_ts_suspend,
       .resume         = s3c_ts_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-ts",
	},
};

static char banner[] __initdata = KERN_INFO "S3C Touchscreen driver, (c) 2008 Samsung Electronics\n";

static int __init s3c_ts_init(void)
{
	int res;

	printk(banner);
	res = platform_driver_register(&s3c_ts_driver);

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG_SPECIAL
	while (1==1)
	{
		unsigned long data0;
		unsigned long data1;
		int adc;

		for(adc=0; adc<4; adc++)
		{

			writel(readl(ts_base+S3C_ADCCON)|S3C_ADCCON_RESSEL_12BIT,
				ts_base+S3C_ADCCON);

			data0 = readl(ts_base+S3C_ADCDAT0);
			data1 = readl(ts_base+S3C_ADCDAT1);

		}
	}
#endif // #ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG_SPECIAL

	return res;
}

static void __exit s3c_ts_exit(void)
{
	platform_driver_unregister(&s3c_ts_driver);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_DESCRIPTION("S3C touchscreen driver");
MODULE_LICENSE("GPL");
