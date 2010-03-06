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
#include <plat/adcts.h>
#include <plat/ts.h>
#include <mach/irqs.h>

//#define S3C_ADCTS_DEBUG

#define WAIT4INT(x)  (((x)<<8) | \
                     S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
                     S3C_ADCTSC_XY_PST(3))
 
#define AUTOPST      (S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
                     S3C_ADCTSC_AUTO_PST | S3C_ADCTSC_XY_PST(0))

#define ADC_START(r,p,x)   (r) | S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(p) | \
			S3C_ADCCON_SELMUX(x) | S3C_ADCCON_ENABLE_START
#define TS_START(r,p)   (r) | S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(p) | \
			S3C_ADCCON_ENABLE_START

#define INT_MODE_UP		1
#define INT_MODE_DOWN		0

#define ADC_RETRY_NUM           5
#define WAIT_EVENT_TIMEOUT      (HZ/100) /* 10ms */

static struct clk			*adc_clock;
static void __iomem 			*base_addr;
static struct s3c_ts_mach_info 		*ts;
static struct s3c_adcts_channel_info 	adc[MAX_ADC_CHANNEL];
static int 				adc_value[MAX_ADC_CHANNEL];
static struct 				s3c_adcts_value ts_value;

static wait_queue_head_t adc_wait[MAX_ADC_CHANNEL];
static int request_adc=0;
static int request_adc_order=0;
static int request_adc_count=0;
static int current_channel=-1; 		/* -1: not working, 0~7: adc, 8:ts */
static int ready_to_adc=1;
static int ready_to_ts=1;
static int ts_int_mode=INT_MODE_DOWN; 	/* 0: down 1:up */
static int ts_status=TS_STATUS_UP;
static int ts_sampling_count=0;
static void (*ts_done_callbacks)(struct s3c_adcts_value *ts_values);
static struct tasklet_struct ts_done_task;

static unsigned int 			irq_updown;
static unsigned int 			irq_adc;

static void start_adcts (void);

static inline void start_hw_adc (int channel)
{
	writel (adc[channel].delay, base_addr + S3C_ADCDLY);
	writel (ADC_START(adc[channel].resol, adc[channel].presc, channel), base_addr+S3C_ADCCON);
}

static inline void start_hw_ts (void)
{
	writel (ts->adcts.delay, base_addr + S3C_ADCDLY);
	writel (S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C_ADCTSC);
	writel (TS_START(ts->adcts.resol, ts->adcts.presc), base_addr+S3C_ADCCON);
}

static inline void end_hw_ts(void)
{
	writel(WAIT4INT(ts_int_mode), base_addr+S3C_ADCTSC);
}

static inline void stop_hw (void)
{
	writel (0, base_addr+S3C_ADCCON);
}

static inline void change_ts_int_mode (int mode)
{
	ts_int_mode = mode;
	writel(WAIT4INT(mode), base_addr+S3C_ADCTSC);
}


static void ts_timer_fire(unsigned long data)
{
	if (ts_status == TS_STATUS_UP && !request_adc_count)
		ready_to_adc = 1;
	else
	{
		ready_to_ts = 1;
		start_adcts();
	}
}

static struct timer_list ts_timer =
                TIMER_INITIALIZER(ts_timer_fire, 0, 0);

static void adc_done(unsigned long data)
{
	ts_done_callbacks(&ts_value);
}

static irqreturn_t irqhandler_adc_done(int irqno, void *param)
{
	unsigned long data0;
        unsigned long data1;

        data0 = readl(base_addr+S3C_ADCDAT0);
        data1 = readl(base_addr+S3C_ADCDAT1);
 
	if (current_channel == -1)
		goto out;

	if (current_channel >= 0 && current_channel <= 7)
	{
		adc_value[current_channel] = data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
		wake_up_interruptible(&adc_wait[current_channel]);
	}
	else
	{
		end_hw_ts();

		if ((ts_done_callbacks == NULL) || (ts_status == TS_STATUS_UP))
			goto out;

		ts_value.xp[ts_sampling_count] = data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;	
		ts_value.yp[ts_sampling_count] = data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;

		if (ts_sampling_count++ < ts->sampling_time-1)
		{
			start_hw_ts();
			goto out;
		}
		else
		{
			ts_value.status = ts_status;
			ts_status = TS_STATUS_DOWN;
			tasklet_schedule (&ts_done_task);
			ready_to_ts = 0;
			mod_timer (&ts_timer, jiffies + (ts->sampling_interval_ms/(1000/HZ)));
		}
	}

	if (ts_status == TS_STATUS_UP && !request_adc_count)
		ready_to_adc = 1;
	else
		start_adcts();

out:
	writel(0x1, base_addr+S3C_ADCCLRINT);

	return IRQ_HANDLED;
}

static irqreturn_t irqhandler_updown(int irqno, void *param)
{

	unsigned long data0;
        unsigned long data1;

        data0 = readl(base_addr+S3C_ADCDAT0);
        data1 = readl(base_addr+S3C_ADCDAT1);
 
	ts_status = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 & S3C_ADCDAT1_UPDOWN));

#ifdef S3C_ADCTS_DEBUG
	printk ("%s: %c\n", __func__, ts_status?'D':'U');
#endif
	if (ts_status)
	{
		change_ts_int_mode (INT_MODE_UP);
		ready_to_ts = 1;
	}
	else
	{
		if (!ready_to_adc && current_channel == TS_CHANNEL)
		{
			writel(0x1, base_addr+S3C_ADCCLRINT);	/* clear IRQ_ADC */
			stop_hw();
		}

		change_ts_int_mode (INT_MODE_DOWN);
		ts_value.status = TS_STATUS_UP;
		tasklet_schedule (&ts_done_task);
		ready_to_ts = 0;
	}

	if (ts_status == TS_STATUS_UP && !request_adc_count)
		ready_to_adc = 1;
	else
		start_adcts();
	
out:
	writel(0x1, base_addr+S3C_ADCCLRINTPNDNUP);

	return IRQ_HANDLED;
}

static int __start_adcts_ts (void)
{
	if (ts_status != TS_STATUS_UP && ready_to_ts)
	{
		current_channel = TS_CHANNEL;
		request_adc &= ~(1<<TS_CHANNEL);
		ts_sampling_count = 0;
		start_hw_ts();
		return true;
	}
	return false;
}

static int __start_adcts_adc (void)
{
	if (request_adc_count)
	{
		int channel = request_adc_order & 0xF;

		current_channel = channel;
		request_adc &= ~(1<<channel);
		request_adc_order >>= 4;
		request_adc_count --;
		start_hw_adc(channel);
		return true;
	}
	return false;
}

static void start_adcts (void)
{
	if (current_channel == TS_CHANNEL)	
	{
		if (!__start_adcts_adc ())
			__start_adcts_ts();
	}
	else
	{
		if (!__start_adcts_ts ())
			__start_adcts_adc();
	}
}

int s3c_adcts_register_ts (struct s3c_ts_mach_info *ts_cfg,
                           void (*done_callback)(struct s3c_adcts_value *ts_value))
{
	ts = ts_cfg;
	ts_done_callbacks = done_callback;

	change_ts_int_mode (INT_MODE_DOWN);
	enable_irq (IRQ_PENDN);

	return 0;
}

EXPORT_SYMBOL(s3c_adcts_register_ts);

int s3c_adcts_unregister_ts (void)
{
	ts_done_callbacks = NULL;
	disable_irq (IRQ_PENDN);

	return 0;
}

EXPORT_SYMBOL(s3c_adcts_unregister_ts);

int s3c_adc_get_adc_data(int channel)
{
	int i;

#ifdef S3C_ADCTS_DEBUG
	printk ("%s: channel=%d",__func__, channel);
#endif

	if (request_adc & (1<<channel))
		return -EINVAL;

	adc_value[channel] = -1;

        for (i=0; i< ADC_RETRY_NUM ; i++)
        {
		if (!(request_adc & (1 << channel)))
		{
			request_adc |= 1 << channel;

			request_adc_order |= (channel << (request_adc_count * 4));
			request_adc_count ++;
		}

                if (ready_to_adc)
                        start_adcts();
 
                wait_event_interruptible_timeout (adc_wait[channel],
                                        adc_value[channel]!=-1, WAIT_EVENT_TIMEOUT);
                if (adc_value[channel] == -1)
                {
			printk ("\n%s: wait_event timeout\n",__func__);
                        stop_hw();
                        ready_to_adc = 1;
                }
                else
                        break;
        }

#ifdef S3C_ADCTS_DEBUG
	printk (" value= %d\n",  adc_value[channel]);
#endif
	return adc_value[channel];
}

EXPORT_SYMBOL(s3c_adc_get_adc_data);

/*
 * The functions for inserting/removing us as a module.
 */

static int __init s3c_adcts_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct device *dev;
	int ret, i;
	int size;
	struct resource     *adcts_irq;
	struct s3c_adcts_plat_info *s3c_adc_cfg;

	dev = &pdev->dev;

	s3c_adc_cfg = (struct s3c_adcts_plat_info *) dev->platform_data;
        if (s3c_adc_cfg == NULL)
                return -EINVAL;

	memcpy (&adc, s3c_adc_cfg, sizeof(struct s3c_adcts_plat_info));
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		dev_err(dev,"no memory resource specified\n");
		ret = -ENOENT;
		goto get_resource_fail;
	}

	size = (res->end - res->start) + 1;
	
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

        /* For IRQ_PENDUP */
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        if (res == NULL) {
                dev_err(dev, "no irq resource specified\n");
                ret = -ENOENT;
                goto err_irq_updown;
        }
 
	irq_updown = res->start;
	printk ("%s: irq_updown = %d\n", __func__, irq_updown);

        ret = request_irq(irq_updown, irqhandler_updown, IRQF_SAMPLE_RANDOM, "s3c_updown", NULL);
        if (ret != 0) {
                dev_err(dev,"s3c_ts.c: Could not allocate ts IRQ_PENDN !\n");
                ret = -EIO;
                goto err_irq_updown;
        }
	disable_irq (irq_updown);
 
        /* For IRQ_ADC */
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
        if (res == NULL) {
                dev_err(dev, "no irq resource specified\n");
                ret = -ENOENT;
                goto err_irq_adc;
        }
        irq_adc = res->start;
	printk ("%s: irq_adc = %d\n", __func__,irq_adc);
        ret = request_irq(irq_adc, irqhandler_adc_done, IRQF_SAMPLE_RANDOM, "s3c_act_done", NULL);
        if (ret != 0) {
                dev_err(dev, "s3c_ts.c: Could not allocate ts IRQ_ADC !\n");
                ret =  -EIO;
                goto err_irq_adc;
        }

	for (i=0; i<MAX_ADC_CHANNEL; i++)
	{
		init_waitqueue_head (&adc_wait[i]);
	}

	tasklet_init (&ts_done_task, adc_done, 0);


	printk(KERN_INFO "S3C64XX ADCTS driver successfully probed !\n");

	return 0;

err_irq_adc:
        free_irq(irq_updown, pdev);

err_irq_updown:
	clk_disable(adc_clock);
	clk_put(adc_clock);


err_clk:
	iounmap(base_addr);

err_map:
get_resource_fail:

	return ret;
}


static int s3c_adcts_remove(struct platform_device *pdev)
{
	int i;

	tasklet_kill (&ts_done_task);

        free_irq(irq_adc, pdev);
        free_irq(irq_updown, pdev);
	clk_disable(adc_clock);
	clk_put(adc_clock);
	iounmap(base_addr);

	printk(KERN_INFO "s3c_adc_remove() of ADC called !\n");
	return 0;
}

#ifdef CONFIG_PM
static int s3c_adcts_suspend(struct platform_device *dev, pm_message_t state)
{
	clk_disable(adc_clock);

	return 0;
}

static int s3c_adcts_resume(struct platform_device *pdev)
{
	clk_enable(adc_clock);

	if (ready_to_adc)
	{
		if (current_channel >= 0 && current_channel < TS_CHANNEL)
			wake_up_interruptible(&adc_wait[current_channel]);
		stop_hw();
	}
	
	ready_to_ts =  1;
	ready_to_adc = 1;
	change_ts_int_mode (INT_MODE_DOWN);
	start_adcts();
	return 0;
}
#else
#define s3c_adc_suspend NULL
#define s3c_adc_resume  NULL
#endif

static struct platform_driver s3c_adcts_driver = {
       .probe          = s3c_adcts_probe,
       .remove         = s3c_adcts_remove,
       .suspend        = s3c_adcts_suspend,
       .resume         = s3c_adcts_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-adcts",
	},
};

static char banner[] __initdata = KERN_INFO "S3C64XX ADC/TS driver, (c) 2009 Samsung Electronics\n";

int __init s3c_adcts_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_adcts_driver);
}

void __exit s3c_adcts_exit(void)
{
	platform_driver_unregister(&s3c_adcts_driver);
}

module_init(s3c_adcts_init);
module_exit(s3c_adcts_exit);

MODULE_AUTHOR("eunki_kim@samsung.com>");
MODULE_DESCRIPTION("S3C64XX ADC/TS driver");
MODULE_LICENSE("GPL");
