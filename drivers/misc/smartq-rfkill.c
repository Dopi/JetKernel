/*
 * smartQ5/7 Bluetooth rfkill power control via GPIO
 *
 * Copyright (C) 2009 Riversky <fang.hetian@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/smartq-rfkill.h>

static int smartq_rfkill_set_power(void *data, enum rfkill_user_states state)
{
	int nshutdown_gpio =  *(int *)data;
	
	if (!data)
		return -1;
	
	switch (state) {
	case RFKILL_USER_STATE_UNBLOCKED:
		gpio_set_value(nshutdown_gpio, 1);
		break;
	case RFKILL_USER_STATE_SOFT_BLOCKED:
		gpio_set_value(nshutdown_gpio, 0);
		break;
	default:
		printk(KERN_ERR "invalid bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int smartq_rfkill_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct smartq_rfkill_platform_data *pdata = pdev->dev.platform_data;
	enum rfkill_user_states  default_state = RFKILL_USER_STATE_SOFT_BLOCKED; 	 /* off */

#if 0
	rc = gpio_request(pdata->nshutdown_gpio, "smartq_nshutdown_gpio");
	if (unlikely(rc))
		return rc;
#endif

	rc = gpio_direction_output(pdata->nshutdown_gpio, 0);
	if (unlikely(rc))
		return rc;

	rfkill_set_default(RFKILL_TYPE_BLUETOOTH, default_state);
	smartq_rfkill_set_power(NULL, default_state);

	pdata->rfkill = rfkill_alloc(&pdev->dev, RFKILL_TYPE_BLUETOOTH);
	if (unlikely(!pdata->rfkill))
		return -ENOMEM;

	pdata->rfkill->name = "smartq";
	pdata->rfkill->state = default_state;
	/* userspace cannot take exclusive control */
	pdata->rfkill->user_claim_unsupported = 1;
	pdata->rfkill->user_claim = 0;
	pdata->rfkill->data = (void *) &pdata->nshutdown_gpio;
	pdata->rfkill->toggle_radio = smartq_rfkill_set_power;

	rc = rfkill_register(pdata->rfkill);

	if (unlikely(rc))
		rfkill_free(pdata->rfkill);

	return 0;
}

static int smartq_rfkill_remove(struct platform_device *pdev)
{
	struct smartq_rfkill_platform_data *pdata = pdev->dev.platform_data;

	rfkill_unregister(pdata->rfkill);
	rfkill_free(pdata->rfkill);
#if 0	
	gpio_free(pdata->nshutdown_gpio);
#endif

	return 0;
}

static struct platform_driver smartq_rfkill_platform_driver = {
	.probe = smartq_rfkill_probe,
	.remove = smartq_rfkill_remove,
	.driver = {
		   .name = "smartq-rfkill",
		   .owner = THIS_MODULE,
	   },
};

static int __init smartq_rfkill_init(void)
{
	return platform_driver_register(&smartq_rfkill_platform_driver);
}

static void __exit smartq_rfkill_exit(void)
{
	platform_driver_unregister(&smartq_rfkill_platform_driver);
}

module_init(smartq_rfkill_init);
module_exit(smartq_rfkill_exit);

MODULE_DESCRIPTION("smartq-rfkill");
MODULE_AUTHOR("Riversky <fang.hetian@gmail.com>");
MODULE_LICENSE("GPL");

