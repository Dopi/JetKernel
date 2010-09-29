/* linux/drivers/input/touchscreen/s3c-ts.c
 *
 * $Id: s3c-ts.c,v 1.13 2008/11/20 06:00:55 ihlee215 Exp $
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
#include <linux/mutex.h>
#include <linux/string.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <plat/regs-adc.h>
#include <plat/adcts.h>
#include <plat/ts.h>
#include <mach/irqs.h>

#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#ifdef CONFIG_CPU_FREQ
#include <plat/s3c64xx-dvfs.h>
#endif /* CONFIG_CPU_FREQ */

#ifdef CONFIG_SAMSUNG_CALIBRATION_MODE
#include "samsung_cal.h" 
#endif /* CONFIG_SAMSUNG_CALIBRATION_MODE */
#define CONFIG_TOUCHSCREEN_S3C_DEBUG
#undef CONFIG_TOUCHSCREEN_S3C_DEBUG

#define S3C_TSVERSION   0x0101

struct s3c_ts_data {
        struct input_dev        *dev;
        int                     xp_old;
        int                     yp_old;
};

/*
 * Definitions & global arrays.
 */
static char *s3c_ts_name = "S3C TouchScreen";
static struct s3c_ts_data 	*data;
static struct s3c_ts_mach_info 	*ts;

static u32 touch_count = 0;
static void s3c_ts_done_callback (struct s3c_adcts_value *ts_value)
{
	long i, x_sum=0, y_sum=0, x_mean = 0, y_mean = 0, xp = 0, yp = 0;
	int xp_min, xp_max, yp_min, yp_max;

	if (ts_value->status == TS_STATUS_UP)
	{
//		printk(KERN_INFO "[s3c_ts_done_callback] Touch is released\n"); 
                input_report_key(data->dev, BTN_TOUCH, 0);
                input_sync(data->dev);
		data->xp_old= data->yp_old = -1;				

		touch_count = 0;
		return;
	}
	else if (ts_value->status == TS_STATUS_DOWN_NOW)
	{
		data->xp_old= data->yp_old = -1;
	}

	xp_min = yp_min = (1<<12) ;
	xp_max = yp_max = 0;

	for (i=0; i<ts->sampling_time; i++)
	{
		x_sum += ts_value->xp[i];
		y_sum += ts_value->yp[i];
		if (xp_min>ts_value->xp[i]) xp_min = ts_value->xp[i];
		if (xp_max<ts_value->xp[i]) xp_max = ts_value->xp[i];
		if (yp_min>ts_value->yp[i]) yp_min = ts_value->yp[i];
		if (yp_max<ts_value->yp[i]) yp_max = ts_value->yp[i];
	}

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
	struct timeval tv;
	do_gettimeofday(&tv);
	printk(KERN_INFO "[RAW]T: %06d, X: %03ld, Y: %03ld\n", (int)tv.tv_usec, x_sum, y_sum);
#endif /* CONFIG_TOUCHSCREEN_S3C_DEBUG */
	if (ts->remove_max_min_sampling)
	{
		x_sum -= (xp_min + xp_max);
		y_sum -= (yp_min + yp_min);
		x_mean  = x_sum / (ts->sampling_time - 2);
		y_mean  = y_sum / (ts->sampling_time - 2);
	}
	else
	{
		x_mean = x_sum / (ts->sampling_time);
		y_mean = y_sum / (ts->sampling_time);
	}

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
	{
		printk(KERN_INFO "[MEAN_RAW]X: %03ld, Y: %03ld\n", x_mean, y_mean);
	}
#endif /* CONFIG_TOUCHSCREEN_S3C_DEBUG */

	if (ts->use_tscal)
	{
#ifdef CONFIG_SAMSUNG_CALIBRATION_MODE
		xp=(long) ((current_cal_val[2]+(current_cal_val[0]*x_mean)+(current_cal_val[1]*y_mean))/current_cal_val[6]);
	        yp=(long) ((current_cal_val[5]+(current_cal_val[3]*x_mean)+(current_cal_val[4]*y_mean))/current_cal_val[6]);		
#else /* CONFIG_SAMSUNG_CALIBRATION_MODE */
		xp=(long) ((ts->tscal[2]+(ts->tscal[0]*x_mean)+(ts->tscal[1]*y_mean))/ts->tscal[6]);
	        yp=(long) ((ts->tscal[5]+(ts->tscal[3]*x_mean)+(ts->tscal[4]*y_mean))/ts->tscal[6]);	
#endif /* CONFIG_SAMSUNG_CALIBRATION_MODE */
	}
	else
	{
		xp=x_mean;
		yp=y_mean;
	}

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
	{				
		printk(KERN_INFO "[After Cal]X: %03ld, Y: %03ld\n", xp, yp);
	}
#endif /* CONFIG_TOUCHSCREEN_S3C_DEBUG */

	if((xp!=data->xp_old || yp!=data->yp_old) && (xp >= 0 && yp >= 0) )
	{
		touch_count++;		
			
		if(touch_count == 1)		// check first touch
		{
#ifdef CONFIG_CPU_FREQ
			set_dvfs_perf_level();
#endif /* CONFIG_CPU_FREQ */

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
			printk(KERN_INFO "\nFirst BTN_TOUCH Event(%03ld, %03ld) is discard\n", xp, yp);						
#endif
			return;
		}
		else if(touch_count == 2)
		{
			data->xp_old=xp;
			data->yp_old=yp;
			return;
		}
//		printk(KERN_INFO "[s3c_ts_done_callback] Touch is pressed. x = %d, y = %d\n", data->xp_old, data->yp_old); 
        
		input_report_abs(data->dev, ABS_X, data->xp_old);
                input_report_abs(data->dev, ABS_Y, data->yp_old);
                input_report_abs(data->dev, ABS_Z, 0);

                input_report_key(data->dev, BTN_TOUCH, 1);
                input_sync(data->dev);
		// [SEC_BSP.khLEE 2009.08.27 : If cal mode, driver gathers coordinates for calculating cal factors				
#ifdef CONFIG_SAMSUNG_CALIBRATION_MODE
		if(current_driver_state == CAL_COLLECT_STATE || current_driver_state == CAL_INITIAL_STATE)
		{
			if(current_driver_state == CAL_INITIAL_STATE)
				current_driver_state = CAL_COLLECT_STATE;
				
			// check first touch is in center
			if((calibration_coords.xfb[current_collect_state] - LIMIT_RADIUS <= xp &&
				xp <= calibration_coords.xfb[current_collect_state] + LIMIT_RADIUS) &&
				(calibration_coords.yfb[current_collect_state] - LIMIT_RADIUS <= yp &&
				yp <= calibration_coords.yfb[current_collect_state] + LIMIT_RADIUS))
			{
				// save the ADC data for this coordinates
				calibration_data.x[current_collect_state] = x_mean;
				calibration_data.y[current_collect_state] = y_mean;

				printk(KERN_DEBUG "[s3c-ts] current coordinate: %d, obtained ADC of x: %d, y: %d\n", 
					current_collect_state, calibration_data.x[current_collect_state], calibration_data.y[current_collect_state]);

				// step to next coordinates
				current_collect_state += 1;		

				// check obtaining all coordinates data
				if(current_collect_state == 5)
				{
					current_driver_state = CAL_COLLECT_FINISH_STATE;
					strcpy(send_msg_buf, "CAL_COLLECT_FINISH");
				}							
			}
		}						
#endif /* CONFIG_SAMSUNG_CALIBRATION_MODE */

		data->xp_old=xp;
		data->yp_old=yp;
	}
}

/*
 * The functions for inserting/removing us as a module.
 */
static int __init s3c_ts_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct input_dev *input_dev;
	struct s3c_ts_mach_info * s3c_ts_cfg;
	int ret;
	int err;

	dev = &pdev->dev;

	s3c_ts_cfg = (struct s3c_ts_mach_info *) dev->platform_data;
	if (s3c_ts_cfg == NULL)
		return -EINVAL;

	ts = kzalloc(sizeof(struct s3c_ts_mach_info), GFP_KERNEL);
	data = kzalloc(sizeof(struct s3c_ts_data), GFP_KERNEL);

	memcpy (ts, s3c_ts_cfg, sizeof(struct s3c_ts_mach_info));
	
	input_dev = input_allocate_device();

	if (!input_dev) {
		ret = -ENOMEM;
		goto input_dev_fail;
	}
	
	data->dev = input_dev;

	data->xp_old = data->yp_old = -1;
	data->dev->evbit[0] = data->dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	data->dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

        input_set_abs_params(data->dev, ABS_X, 0, ts->screen_size_x, 0, 0);
        input_set_abs_params(data->dev, ABS_Y, 0, ts->screen_size_y, 0, 0);
	// [SEC_BSP.khLEE 2009.08.24 : Intialize variables for calibration mode
#ifdef CONFIG_SAMSUNG_CALIBRATION_MODE
	ret = init_samsung_cal_mode(ts->screen_size_x, ts->screen_size_y, ts->tscal);
	if(ret) 
	{
		dev_err(dev, "s3c_ts.c: Could not initialization(touchscreen)!\n");
		goto s3c_adcts_register_fail;
	}
#endif /* CONFIG_SAMSUNG_CALIBRATION_MODE */
	// ]

        set_bit(0,data->dev->evbit);
        set_bit(1,data->dev->evbit);
        set_bit(2,data->dev->evbit);
        set_bit(3,data->dev->evbit);
        set_bit(5,data->dev->evbit);

        set_bit(0,data->dev->relbit);
        set_bit(1,data->dev->relbit);

        set_bit(0,data->dev->absbit);
        set_bit(1,data->dev->absbit);
        set_bit(2,data->dev->absbit);

        set_bit(0,data->dev->swbit);

        for(err=0;err<512;err++) set_bit(err,data->dev->keybit);

        input_event(data->dev,5,0,1);

	input_set_abs_params(data->dev, ABS_PRESSURE, 0, 1, 0, 0);

	data->dev->name = s3c_ts_name;
	data->dev->id.bustype = BUS_RS232;
	data->dev->id.vendor = 0xDEAD;
	data->dev->id.product = 0xBEEF;
	data->dev->id.version = S3C_TSVERSION;

	ret = s3c_adcts_register_ts (ts, s3c_ts_done_callback);
	if(ret) {
		dev_err(dev, "s3c_ts.c: Could not register adcts device(touchscreen)!\n");
		ret = -EIO;
		goto s3c_adcts_register_fail;
	}

	/* All went ok, so register to the input system */
	ret = input_register_device(data->dev);
	
	if(ret) {
		dev_err(dev, "s3c_ts.c: Could not register input device(touchscreen)!\n");
		ret = -EIO;
		goto input_register_fail;
	}
	return 0;

input_register_fail:
	s3c_adcts_unregister_ts();

s3c_adcts_register_fail:
	input_free_device (data->dev);

input_dev_fail:
	kfree (ts);
	kfree (data);

	return ret;
}

static int s3c_ts_remove(struct platform_device *dev)
{
	printk(KERN_INFO "s3c_ts_remove() of TS called !\n");

	input_unregister_device(data->dev);
	s3c_adcts_unregister_ts();
	input_free_device (data->dev);
	kfree (ts);
	kfree (data);

	return 0;
}

static struct platform_driver s3c_ts_driver = {
       .probe          = s3c_ts_probe,
       .remove         = s3c_ts_remove,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-ts",
	},
};

static char banner[] __initdata = KERN_INFO "S3C Touchscreen driver, (c) 2009 Samsung Electronics\n";

static int __init s3c_ts_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_ts_driver);
}

static void __exit s3c_ts_exit(void)
{
#ifdef CONFIG_SAMSUNG_CALIBRATION_MODE
	exit_samsung_cal_mode();
#endif /* CONFIG_SAMSUNG_CALIBRATION_MODE */
	platform_driver_unregister(&s3c_ts_driver);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_DESCRIPTION("S3C touchscreen driver");
MODULE_LICENSE("GPL");
