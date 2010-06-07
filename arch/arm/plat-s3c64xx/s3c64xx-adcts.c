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
#include <linux/kthread.h>
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

#ifdef CONFIG_TOUCHSCREEN_S3C

#define WAIT4INT_OFF(x)  (((x)<<8) | \
                     S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
                     S3C_ADCTSC_XY_PST(0))

#define WAIT4INT_ON(x)  (((x)<<8) | \
                     S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
                     S3C_ADCTSC_XY_PST(3))
 
#define AUTOPST      (S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
                     S3C_ADCTSC_AUTO_PST | S3C_ADCTSC_XY_PST(0))

#define TS_START(r,p)   (r) | S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(p) | \
			S3C_ADCCON_ENABLE_START

#define INT_MODE_UP			1
#define INT_MODE_DOWN			0
#define CHANNEL_MASK	 (0x7FF)  	  	/* ADC0 ~ ADC7 and TS, TS_UP */

#endif /* CONFIG_TOUCHSCREEN_S3C */

#define ADC_START(r,p,x)   (r) | S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(p) | \
			S3C_ADCCON_SELMUX(x) | S3C_ADCCON_ENABLE_START
#define ADC_STOP	S3C_ADCCON_RESSEL_12BIT | S3C_ADCCON_PRSCEN | S3C_ADCCON_PRSCVL(0xff)

#define WAIT_EVENT_TIMEOUT		(HZ/20)					/* 50ms */
#define ADC_RETRY_NUM			5					/* 5 times */
#define SLEEP_TIMEOUT			(WAIT_EVENT_TIMEOUT*(ADC_RETRY_NUM-1))	/* 200ms */

#define CHANNEL_ADC_MASK (0xFF)    		/* ADC0 ~ ADC7 */

/*  global struct & variable definition */
enum ADCTS_THREAD_STATE {
	ADCTS_THREAD_NONE=0,
	ADCTS_THREAD_RUN,
	ADCTS_THREAD_WAIT,
	ADCTS_THREAD_SLEEP,
	ADCTS_THREAD_WAKEUP,
	ADCTS_THEAD_TERMINATED
};

enum ADCTS_DRIVER_STATE {
	ADCTS_DRIVER_PROBE = 0,
	ADCTS_DRIVER_SUSPEND,
	ADCTS_DRIVER_RESUME,
	ADCTS_DRIVER_REMOVE
};
enum ADCTS_CHANNEL_STATE {
	CHANNEL_NOT_SELECTED=-2,
	CHANNEL_ADC_DONE=-1,
	CHANNEL_ADC0=0,
	CHANNEL_ADC1,
	CHANNEL_ADC2,
	CHANNEL_ADC3,
	CHANNEL_ADC4,
	CHANNEL_ADC5,
	CHANNEL_ADC6,
	CHANNEL_ADC7,
#ifdef CONFIG_TOUCHSCREEN_S3C
	CHANNEL_TS,
	CHANNEL_TS_SCANNING,
	CHANNEL_TS_UP
#endif /* CONFIG_TOUCHSCREEN_S3C */
};

static struct clk			*adc_clock;
static void __iomem 			*base_addr;
static unsigned int 			irq_adc;

static struct s3c_adcts_channel_info 	adc[MAX_ADC_CHANNEL];

static wait_queue_head_t 		adc_wait[MAX_ADC_CHANNEL];
static wait_queue_head_t		adcts_thread_wait;
static struct task_struct     		*adcts_thread_task; 
static int 				adc_value[MAX_ADC_CHANNEL];

static int				adcts_thread_state;
static int				adc_thread_state[MAX_ADC_CHANNEL];
static int				adcts_driver_state;
static int				channel_state;
static unsigned int 			adcts_request_flag =0;

#ifdef CONFIG_TOUCHSCREEN_S3C
static unsigned int 			irq_updown;
static struct s3c_ts_mach_info 		*ts;
static struct s3c_adcts_value 		ts_value;
static int 				ts_int_mode;
static int 				ts_sampling_count;

static void ts_timer_fire(unsigned long data);
static struct timer_list ts_timer =	TIMER_INITIALIZER(ts_timer_fire, 0, 0);
static void (*ts_done_callbacks)(struct s3c_adcts_value *ts_values);
#endif /* CONFIG_TOUCHSCREEN_S3C */

//////////////////////////////////////////////////////////////////////////

static inline void start_hw_adc (int channel)
{
	writel (adc[channel].delay, base_addr + S3C_ADCDLY);
	writel (ADC_START(adc[channel].resol, adc[channel].presc, channel), base_addr+S3C_ADCCON);
}

#ifdef CONFIG_TOUCHSCREEN_S3C
static inline void start_hw_ts (void)
{
//	writel (WAIT4INT_ON(ts_int_mode), base_addr+S3C_ADCTSC);
	writel (ts->adcts.delay, base_addr + S3C_ADCDLY);
//	writel ((ts_int_mode<<8) | S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C_ADCTSC);
	writel (S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C_ADCTSC);
	writel (TS_START(ts->adcts.resol, ts->adcts.presc), base_addr+S3C_ADCCON);
}

static inline void change_ts_int_mode(int int_mode)
{
	ts_int_mode = int_mode;
	writel(WAIT4INT_ON(int_mode), base_addr+S3C_ADCTSC);
}
#endif /* CONFIG_TOUCHSCREEN_S3C */

static inline void stop_hw (void)
{
	writel (ADC_STOP, base_addr+S3C_ADCCON);
}

static void _request_adcts(int channel)
{
#ifdef CONFIG_TOUCHSCREEN_S3C
	/* if TS flags exists, TS flag should be cleared */
	if (channel == CHANNEL_TS_UP)
		adcts_request_flag &= ~((1<<CHANNEL_TS)|(1<<CHANNEL_TS_SCANNING));
#endif /* CONFIG_TOUCHSCREEN_S3C */
		
	adcts_request_flag |= (1<<channel);

	if (adcts_thread_state == ADCTS_THREAD_SLEEP)
	{
		adcts_thread_state = ADCTS_THREAD_WAKEUP;
		wake_up_interruptible(&adcts_thread_wait);
	}
}

#ifdef CONFIG_TOUCHSCREEN_S3C
static void ts_timer_fire(unsigned long data)
{
	_request_adcts(CHANNEL_TS);
}
#endif /* CONFIG_TOUCHSCREEN_S3C */

static irqreturn_t irqhandler_adc_done(int irqno, void *param)
{
	unsigned long data0;
	unsigned long data1;

	stop_hw();

	data0 = readl(base_addr+S3C_ADCDAT0);
	data1 = readl(base_addr+S3C_ADCDAT1);
				
	if (channel_state>=CHANNEL_ADC0 && channel_state<=CHANNEL_ADC7)	// adc
	{
		if (adc_thread_state[channel_state] == ADCTS_THREAD_WAIT)
		{
			adc_thread_state[channel_state] = ADCTS_THREAD_WAKEUP;
			wake_up_interruptible(&adc_wait[channel_state]);

			adc_value[channel_state] = data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
			channel_state = CHANNEL_ADC_DONE;
		}
	}
#ifdef CONFIG_TOUCHSCREEN_S3C
	else if (channel_state == CHANNEL_TS)				// ts
	{
		int ts_status = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 & S3C_ADCDAT1_UPDOWN));
		if (ts_status)
		{
			change_ts_int_mode (INT_MODE_UP);

			ts_value.xp[ts_sampling_count] = data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;	
			ts_value.yp[ts_sampling_count] = data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
			ts_sampling_count++;
		}
		else
		{
			_request_adcts(CHANNEL_TS_UP);
			change_ts_int_mode (INT_MODE_DOWN);
		}
		channel_state = CHANNEL_ADC_DONE;
	}
#endif /* CONFIG_TOUCHSCREEN_S3C */

	if (channel_state==CHANNEL_ADC_DONE && adcts_thread_state==ADCTS_THREAD_WAIT)
	{
		adcts_thread_state = ADCTS_THREAD_WAKEUP;
		wake_up_interruptible(&adcts_thread_wait);
	}

	writel(0x1, base_addr+S3C_ADCCLRINT);
	return IRQ_HANDLED;
}

#ifdef CONFIG_TOUCHSCREEN_S3C
static irqreturn_t irqhandler_updown(int irqno, void *param)
{
	unsigned long data0, data1;
	int	ts_status;

	data0 = readl(base_addr+S3C_ADCDAT0);
	data1 = readl(base_addr+S3C_ADCDAT1);
 
	ts_status = (!(data0 & S3C_ADCDAT0_UPDOWN)) && (!(data1 & S3C_ADCDAT1_UPDOWN));

//#ifdef S3C_ADCTS_DEBUG
#if 1
	printk ("%s: %c\n", __func__, ts_status?'D':'U');
#endif

	if (ts_status) 		// down
	{
		_request_adcts(CHANNEL_TS);
		change_ts_int_mode (INT_MODE_UP);
	}
	else 				// up
	{
		_request_adcts(CHANNEL_TS_UP);
		change_ts_int_mode (INT_MODE_DOWN);
	}

	writel (0x0, base_addr+S3C_ADCUPDN);
	writel (0x1, base_addr+S3C_ADCCLRINTPNDNUP);

	return IRQ_HANDLED;
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
#endif /* CONFIG_TOUCHSCREEN_S3C */

int s3c_adc_get_adc_data(int channel)
{
	int i;

	if(channel >= MAX_ADC_CHANNEL || channel < 0) return -EINVAL;

	if (adcts_request_flag & (1<<channel))
		return -EINVAL;

	for (i=0; i< ADC_RETRY_NUM ; i++)
	{
		_request_adcts (channel);

		adc_thread_state[channel] = ADCTS_THREAD_WAIT;
	        wait_event_interruptible_timeout (adc_wait[channel],
			adc_thread_state[channel]!=ADCTS_THREAD_WAIT, WAIT_EVENT_TIMEOUT);
                
		if (adc_thread_state[channel] == ADCTS_THREAD_WAIT)
		{
		}
	        else 
			break;
	}

	if (i==ADC_RETRY_NUM)
	{
		printk ("%s: wait_event timeout\n",__func__);
		return -EINVAL;
	}

#ifdef S3C_ADCTS_DEBUG
	printk ("%s: value= %d\n", __func__, adc_value[channel]);
#endif
	return adc_value[channel];
}

EXPORT_SYMBOL(s3c_adc_get_adc_data);

static void _adcts_main_thread_sleep (void)
{
	if (adcts_request_flag)
		return;

	adcts_thread_state = ADCTS_THREAD_SLEEP;
       	wait_event_interruptible_timeout (adcts_thread_wait, adcts_thread_state!=ADCTS_THREAD_SLEEP, SLEEP_TIMEOUT);

	adcts_thread_state = ADCTS_THREAD_RUN;
}

#ifdef CONFIG_TOUCHSCREEN_S3C
static void _adcts_main_thread_ts (void)
{
	if (adcts_request_flag & (1<<CHANNEL_TS_UP)) 
	{
		stop_hw();

		adcts_request_flag &= ~(1<<CHANNEL_TS_UP);
		ts_value.status = TS_STATUS_UP;

		ts_done_callbacks(&ts_value);
		return;
	}


	if (adcts_request_flag & ((1<<CHANNEL_TS)|(1<<CHANNEL_TS_SCANNING)))
	{
		if ( ts_int_mode == INT_MODE_DOWN)
		{
			adcts_request_flag &= ~((1<<CHANNEL_TS)|(1<<CHANNEL_TS_SCANNING));
			return;
		}

		if (adcts_request_flag & (1<<CHANNEL_TS))
		{
			ts_sampling_count = 0;
			adcts_request_flag &= ~(1<<CHANNEL_TS);
			adcts_request_flag |= (1<<CHANNEL_TS_SCANNING);
		}


		channel_state=CHANNEL_TS;
		start_hw_ts();

		adcts_thread_state = ADCTS_THREAD_WAIT;
        	wait_event_interruptible_timeout (adcts_thread_wait,
			adcts_thread_state!=ADCTS_THREAD_WAIT, WAIT_EVENT_TIMEOUT);
	
		if (ts_sampling_count >= ts->sampling_time) 
		{
			change_ts_int_mode(INT_MODE_UP);

			adcts_request_flag &= ~(1<<CHANNEL_TS_SCANNING);
			ts_value.status = TS_STATUS_DOWN;

			ts_done_callbacks(&ts_value);
			mod_timer (&ts_timer, jiffies + (ts->sampling_interval_ms/(1000/HZ)));
		}
	}
}
#endif /* CONFIG_TOUCHSCREEN_S3C */

static void _adcts_main_thread_adc (void)
{
	int i;

	for (i=CHANNEL_ADC0; (adcts_request_flag&CHANNEL_ADC_MASK) && (i<=CHANNEL_ADC7); i++)
	{
		if (adcts_request_flag & (1<<i))
		{
			channel_state=i;
			adcts_request_flag &= ~(1<<i);
			start_hw_adc(i);
			adcts_thread_state = ADCTS_THREAD_WAIT;
	        	wait_event_interruptible_timeout (adcts_thread_wait,
				adcts_thread_state!=ADCTS_THREAD_WAIT, WAIT_EVENT_TIMEOUT);
		}
	}
}

static int adcts_main_thread(void *data)
{
	/* change priority to RT scduler and priority 99 */
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	sched_setscheduler_nocheck(current, SCHED_FIFO, &param);

	adcts_thread_state = ADCTS_THREAD_RUN;
	channel_state = CHANNEL_NOT_SELECTED;

	printk ("kadctsd is started\n");

	while (adcts_driver_state!=ADCTS_DRIVER_REMOVE)
	{
#ifdef CONFIG_TOUCHSCREEN_S3C
		/* make sure flag is available */
		if (ts_done_callbacks != NULL)
			adcts_request_flag &= CHANNEL_MASK;
		else
#endif /* CONFIG_TOUCHSCREEN_S3C */
			adcts_request_flag &= CHANNEL_ADC_MASK;

		_adcts_main_thread_sleep();
#ifdef CONFIG_TOUCHSCREEN_S3C
		_adcts_main_thread_ts();
#endif /* CONFIG_TOUCHSCREEN_S3C */
		_adcts_main_thread_adc();
	}
	return 0;
}


/*
 * The functions for inserting/removing us as a module.
 */

static int __init s3c_adcts_probe(struct platform_device *pdev)
{
	struct resource	*res;
	struct device *dev;
	int ret, i;
	int size;
	struct s3c_adcts_plat_info *s3c_adc_cfg;

	adcts_thread_state = ADCTS_THREAD_NONE;
	adcts_driver_state = ADCTS_DRIVER_PROBE;

	for (i=CHANNEL_ADC0 ; i<=CHANNEL_ADC7; i++)
		adc_thread_state[i] = ADCTS_THREAD_RUN;

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

	stop_hw();
#ifdef CONFIG_TOUCHSCREEN_S3C
	ts_done_callbacks = NULL;

	change_ts_int_mode (INT_MODE_DOWN);

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
 #endif /* CONFIG_TOUCHSCREEN_S3C */

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
	init_waitqueue_head (&adcts_thread_wait);

	adcts_thread_task = kthread_run(adcts_main_thread, NULL, "kadctsd");

	printk(KERN_INFO "S3C64XX ADCTS driver successfully probed !\n");

	return 0;

err_irq_adc:
#ifdef CONFIG_TOUCHSCREEN_S3C
        free_irq(irq_updown, pdev);

err_irq_updown:
#endif /* CONFIG_TOUCHSCREEN_S3C */
	clk_disable(adc_clock);
	clk_put(adc_clock);


err_clk:
	iounmap(base_addr);

err_map:
get_resource_fail:
	adcts_driver_state = ADCTS_DRIVER_REMOVE;

	return ret;
}


static int s3c_adcts_remove(struct platform_device *pdev)
{
	adcts_driver_state = ADCTS_DRIVER_REMOVE;	

	kthread_stop (adcts_thread_task);
	free_irq(irq_adc, pdev);
#ifdef CONFIG_TOUCHSCREEN_S3C
	free_irq(irq_updown, pdev);
#endif /* CONFIG_TOUCHSCREEN_S3C */
	clk_disable(adc_clock);
	clk_put(adc_clock);
	iounmap(base_addr);
	printk(KERN_INFO "s3c_adc_remove() of ADC called !\n");
	return 0;
}

#ifdef CONFIG_PM
static int s3c_adcts_suspend(struct platform_device *dev, pm_message_t state)
{
	adcts_driver_state = ADCTS_DRIVER_SUSPEND;	
	stop_hw();
	clk_disable(adc_clock);

	return 0;
}

static int s3c_adcts_resume(struct platform_device *pdev)
{
	int i;
	adcts_driver_state = ADCTS_DRIVER_RESUME;

	clk_enable(adc_clock);

	for (i=0; i<MAX_ADC_CHANNEL;i++)
	{
		if (adcts_request_flag & (1<<i))
		{
			adc_value[i] = -1;
			wake_up_interruptible(&adc_wait[i]);
		}
	}
			
	adcts_request_flag = 0;
	channel_state = CHANNEL_NOT_SELECTED;
#ifdef CONFIG_TOUCHSCREEN_S3C
	change_ts_int_mode (INT_MODE_DOWN);
#endif /* CONFIG_TOUCHSCREEN_S3C */

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
