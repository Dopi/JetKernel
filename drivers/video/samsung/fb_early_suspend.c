/* /drivers/video/samsung/fb_early_suspend.c
 *
 * Copyright (C) 2005-2008 Riversky <fang.hetian@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/console.h>
#include <linux/earlysuspend.h>
#include <linux/kbd_kern.h>
#include <linux/module.h>
#include <linux/vt_kern.h>
#include <linux/wait.h>
#include <linux/fb.h>

extern void s3cfb_set_lcd_power(int);
extern void s3cfb_set_backlight_power(int);
extern void s3cfb_set_backlight_level(int);
extern int s3cfb_get_backlight_level(void);

static int old_backlight_level;

static void fb_early_suspend(struct early_suspend *h)
{
#if 1
	s3cfb_set_lcd_power(0);
	s3cfb_set_backlight_power(0);
#else
	old_backlight_level = s3cfb_get_backlight_level();
#if defined(CONFIG_S3C6410_PWM)
	s3cfb_set_backlight_level(old_backlight_level > 5 ? 5 : 0);
#else
	s3cfb_set_backlight_level(0);
#endif
#endif
}

static void fb_late_resume(struct early_suspend *h)
{
#if 1
	s3cfb_set_lcd_power(1);
	s3cfb_set_backlight_power(1);
#else
#if defined(CONFIG_S3C6410_PWM)
	s3cfb_set_backlight_level(old_backlight_level);
#else
	s3cfb_set_backlight_level(old_backlight_level);
#endif
#endif
}

static struct early_suspend fb_early_suspend_desc = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = fb_early_suspend,
	.resume = fb_late_resume,
};

static int __init fb_early_suspend_init(void)
{
	register_early_suspend(&fb_early_suspend_desc);
	return 0;
}

static void  __exit fb_early_suspend_exit(void)
{
	unregister_early_suspend(&fb_early_suspend_desc);
}

module_init(fb_early_suspend_init);
module_exit(fb_early_suspend_exit);


