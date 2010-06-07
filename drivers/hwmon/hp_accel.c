/*
 *  hp_accel.c - Interface between LIS3LV02DL driver and HP ACPI BIOS
 *
 *  Copyright (C) 2007-2008 Yan Burman
 *  Copyright (C) 2008 Eric Piel
 *  Copyright (C) 2008-2009 Pavel Machek
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
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/freezer.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <acpi/acpi_drivers.h>
#include <asm/atomic.h>
#include "lis3lv02d.h"

#define DRIVER_NAME     "lis3lv02d"
#define ACPI_MDPS_CLASS "accelerometer"

/* Delayed LEDs infrastructure ------------------------------------ */

/* Special LED class that can defer work */
struct delayed_led_classdev {
	struct led_classdev led_classdev;
	struct work_struct work;
	enum led_brightness new_brightness;

	unsigned int led;		/* For driver */
	void (*set_brightness)(struct delayed_led_classdev *data, enum led_brightness value);
};

static inline void delayed_set_status_worker(struct work_struct *work)
{
	struct delayed_led_classdev *data =
			container_of(work, struct delayed_led_classdev, work);

	data->set_brightness(data, data->new_brightness);
}

static inline void delayed_sysfs_set(struct led_classdev *led_cdev,
			      enum led_brightness brightness)
{
	struct delayed_led_classdev *data = container_of(led_cdev,
			     struct delayed_led_classdev, led_classdev);
	data->new_brightness = brightness;
	schedule_work(&data->work);
}

/* HP-specific accelerometer driver ------------------------------------ */

/* For automatic insertion of the module */
static struct acpi_device_id lis3lv02d_device_ids[] = {
	{"HPQ0004", 0}, /* HP Mobile Data Protection System PNP */
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, lis3lv02d_device_ids);


/**
 * lis3lv02d_acpi_init - ACPI _INI method: initialize the device.
 * @handle: the handle of the device
 *
 * Returns AE_OK on success.
 */
acpi_status lis3lv02d_acpi_init(acpi_handle handle)
{
	return acpi_evaluate_object(handle, METHOD_NAME__INI, NULL, NULL);
}

/**
 * lis3lv02d_acpi_read - ACPI ALRD method: read a register
 * @handle: the handle of the device
 * @reg:    the register to read
 * @ret:    result of the operation
 *
 * Returns AE_OK on success.
 */
acpi_status lis3lv02d_acpi_read(acpi_handle handle, int reg, u8 *ret)
{
	union acpi_object arg0 = { ACPI_TYPE_INTEGER };
	struct acpi_object_list args = { 1, &arg0 };
	unsigned long long lret;
	acpi_status status;

	arg0.integer.value = reg;

	status = acpi_evaluate_integer(handle, "ALRD", &args, &lret);
	*ret = lret;
	return status;
}

/**
 * lis3lv02d_acpi_write - ACPI ALWR method: write to a register
 * @handle: the handle of the device
 * @reg:    the register to write to
 * @val:    the value to write
 *
 * Returns AE_OK on success.
 */
acpi_status lis3lv02d_acpi_write(acpi_handle handle, int reg, u8 val)
{
	unsigned long long ret; /* Not used when writting */
	union acpi_object in_obj[2];
	struct acpi_object_list args = { 2, in_obj };

	in_obj[0].type          = ACPI_TYPE_INTEGER;
	in_obj[0].integer.value = reg;
	in_obj[1].type          = ACPI_TYPE_INTEGER;
	in_obj[1].integer.value = val;

	return acpi_evaluate_integer(handle, "ALWR", &args, &ret);
}

static int lis3lv02d_dmi_matched(const struct dmi_system_id *dmi)
{
	adev.ac = *((struct axis_conversion *)dmi->driver_data);
	printk(KERN_INFO DRIVER_NAME ": hardware type %s found.\n", dmi->ident);

	return 1;
}

/* Represents, for each axis seen by userspace, the corresponding hw axis (+1).
 * If the value is negative, the opposite of the hw value is used. */
static struct axis_conversion lis3lv02d_axis_normal = {1, 2, 3};
static struct axis_conversion lis3lv02d_axis_y_inverted = {1, -2, 3};
static struct axis_conversion lis3lv02d_axis_x_inverted = {-1, 2, 3};
static struct axis_conversion lis3lv02d_axis_z_inverted = {1, 2, -3};
static struct axis_conversion lis3lv02d_axis_xy_rotated_left = {-2, 1, 3};
static struct axis_conversion lis3lv02d_axis_xy_rotated_left_usd = {-2, 1, -3};
static struct axis_conversion lis3lv02d_axis_xy_swap_inverted = {-2, -1, 3};
static struct axis_conversion lis3lv02d_axis_xy_rotated_right = {2, -1, 3};
static struct axis_conversion lis3lv02d_axis_xy_swap_yz_inverted = {2, -1, -3};

#define AXIS_DMI_MATCH(_ident, _name, _axis) {		\
	.ident = _ident,				\
	.callback = lis3lv02d_dmi_matched,		\
	.matches = {					\
		DMI_MATCH(DMI_PRODUCT_NAME, _name)	\
	},						\
	.driver_data = &lis3lv02d_axis_##_axis		\
}

#define AXIS_DMI_MATCH2(_ident, _class1, _name1,	\
				_class2, _name2,	\
				_axis) {		\
	.ident = _ident,				\
	.callback = lis3lv02d_dmi_matched,		\
	.matches = {					\
		DMI_MATCH(DMI_##_class1, _name1),	\
		DMI_MATCH(DMI_##_class2, _name2),	\
	},						\
	.driver_data = &lis3lv02d_axis_##_axis		\
}
static struct dmi_system_id lis3lv02d_dmi_ids[] = {
	/* product names are truncated to match all kinds of a same model */
	AXIS_DMI_MATCH("NC64x0", "HP Compaq nc64", x_inverted),
	AXIS_DMI_MATCH("NC84x0", "HP Compaq nc84", z_inverted),
	AXIS_DMI_MATCH("NX9420", "HP Compaq nx9420", x_inverted),
	AXIS_DMI_MATCH("NW9440", "HP Compaq nw9440", x_inverted),
	AXIS_DMI_MATCH("NC2510", "HP Compaq 2510", y_inverted),
	AXIS_DMI_MATCH("NC8510", "HP Compaq 8510", xy_swap_inverted),
	AXIS_DMI_MATCH("HP2133", "HP 2133", xy_rotated_left),
	AXIS_DMI_MATCH("NC653x", "HP Compaq 653", xy_rotated_left_usd),
	AXIS_DMI_MATCH("NC673x", "HP Compaq 673", xy_rotated_left_usd),
	AXIS_DMI_MATCH("NC651xx", "HP Compaq 651", xy_rotated_right),
	AXIS_DMI_MATCH("NC671xx", "HP Compaq 671", xy_swap_yz_inverted),
	/* Intel-based HP Pavilion dv5 */
	AXIS_DMI_MATCH2("HPDV5_I",
			PRODUCT_NAME, "HP Pavilion dv5",
			BOARD_NAME, "3603",
			x_inverted),
	/* AMD-based HP Pavilion dv5 */
	AXIS_DMI_MATCH2("HPDV5_A",
			PRODUCT_NAME, "HP Pavilion dv5",
			BOARD_NAME, "3600",
			y_inverted),
	{ NULL, }
/* Laptop models without axis info (yet):
 * "NC6910" "HP Compaq 6910"
 * HP Compaq 8710x Notebook PC / Mobile Workstation
 * "NC2400" "HP Compaq nc2400"
 * "NX74x0" "HP Compaq nx74"
 * "NX6325" "HP Compaq nx6325"
 * "NC4400" "HP Compaq nc4400"
 */
};

static void hpled_set(struct delayed_led_classdev *led_cdev, enum led_brightness value)
{
	acpi_handle handle = adev.device->handle;
	unsigned long long ret; /* Not used when writing */
	union acpi_object in_obj[1];
	struct acpi_object_list args = { 1, in_obj };

	in_obj[0].type          = ACPI_TYPE_INTEGER;
	in_obj[0].integer.value = !!value;

	acpi_evaluate_integer(handle, "ALED", &args, &ret);
}

static struct delayed_led_classdev hpled_led = {
	.led_classdev = {
		.name			= "hp::hddprotect",
		.default_trigger	= "none",
		.brightness_set		= delayed_sysfs_set,
		.flags                  = LED_CORE_SUSPENDRESUME,
	},
	.set_brightness = hpled_set,
};

static acpi_status
lis3lv02d_get_resource(struct acpi_resource *resource, void *context)
{
	if (resource->type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		struct acpi_resource_extended_irq *irq;
		u32 *device_irq = context;

		irq = &resource->data.extended_irq;
		*device_irq = irq->interrupts[0];
	}

	return AE_OK;
}

static void lis3lv02d_enum_resources(struct acpi_device *device)
{
	acpi_status status;

	status = acpi_walk_resources(device->handle, METHOD_NAME__CRS,
					lis3lv02d_get_resource, &adev.irq);
	if (ACPI_FAILURE(status))
		printk(KERN_DEBUG DRIVER_NAME ": Error getting resources\n");
}

static s16 lis3lv02d_read_16(acpi_handle handle, int reg)
{
	u8 lo, hi;

	adev.read(handle, reg - 1, &lo);
	adev.read(handle, reg, &hi);
	/* In "12 bit right justified" mode, bit 6, bit 7, bit 8 = bit 5 */
	return (s16)((hi << 8) | lo);
}

static s16 lis3lv02d_read_8(acpi_handle handle, int reg)
{
	s8 lo;
	adev.read(handle, reg, &lo);
	return lo;
}

static int lis3lv02d_add(struct acpi_device *device)
{
	int ret;

	if (!device)
		return -EINVAL;

	adev.device = device;
	adev.init = lis3lv02d_acpi_init;
	adev.read = lis3lv02d_acpi_read;
	adev.write = lis3lv02d_acpi_write;
	strcpy(acpi_device_name(device), DRIVER_NAME);
	strcpy(acpi_device_class(device), ACPI_MDPS_CLASS);
	device->driver_data = &adev;

	lis3lv02d_acpi_read(device->handle, WHO_AM_I, &adev.whoami);
	switch (adev.whoami) {
	case LIS_DOUBLE_ID:
		printk(KERN_INFO DRIVER_NAME ": 2-byte sensor found\n");
		adev.read_data = lis3lv02d_read_16;
		adev.mdps_max_val = 2048;
		break;
	case LIS_SINGLE_ID:
		printk(KERN_INFO DRIVER_NAME ": 1-byte sensor found\n");
		adev.read_data = lis3lv02d_read_8;
		adev.mdps_max_val = 128;
		break;
	default:
		printk(KERN_ERR DRIVER_NAME
			": unknown sensor type 0x%X\n", adev.whoami);
		return -EINVAL;
	}

	/* If possible use a "standard" axes order */
	if (dmi_check_system(lis3lv02d_dmi_ids) == 0) {
		printk(KERN_INFO DRIVER_NAME ": laptop model unknown, "
				 "using default axes configuration\n");
		adev.ac = lis3lv02d_axis_normal;
	}

	INIT_WORK(&hpled_led.work, delayed_set_status_worker);
	ret = led_classdev_register(NULL, &hpled_led.led_classdev);
	if (ret)
		return ret;

	/* obtain IRQ number of our device from ACPI */
	lis3lv02d_enum_resources(adev.device);

	ret = lis3lv02d_init_device(&adev);
	if (ret) {
		flush_work(&hpled_led.work);
		led_classdev_unregister(&hpled_led.led_classdev);
		return ret;
	}

	return ret;
}

static int lis3lv02d_remove(struct acpi_device *device, int type)
{
	if (!device)
		return -EINVAL;

	lis3lv02d_joystick_disable();
	lis3lv02d_poweroff(device->handle);

	flush_work(&hpled_led.work);
	led_classdev_unregister(&hpled_led.led_classdev);

	return lis3lv02d_remove_fs();
}


#ifdef CONFIG_PM
static int lis3lv02d_suspend(struct acpi_device *device, pm_message_t state)
{
	/* make sure the device is off when we suspend */
	lis3lv02d_poweroff(device->handle);
	return 0;
}

static int lis3lv02d_resume(struct acpi_device *device)
{
	/* put back the device in the right state (ACPI might turn it on) */
	mutex_lock(&adev.lock);
	if (adev.usage > 0)
		lis3lv02d_poweron(device->handle);
	else
		lis3lv02d_poweroff(device->handle);
	mutex_unlock(&adev.lock);
	return 0;
}
#else
#define lis3lv02d_suspend NULL
#define lis3lv02d_resume NULL
#endif

/* For the HP MDPS aka 3D Driveguard */
static struct acpi_driver lis3lv02d_driver = {
	.name  = DRIVER_NAME,
	.class = ACPI_MDPS_CLASS,
	.ids   = lis3lv02d_device_ids,
	.ops = {
		.add     = lis3lv02d_add,
		.remove  = lis3lv02d_remove,
		.suspend = lis3lv02d_suspend,
		.resume  = lis3lv02d_resume,
	}
};

static int __init lis3lv02d_init_module(void)
{
	int ret;

	if (acpi_disabled)
		return -ENODEV;

	ret = acpi_bus_register_driver(&lis3lv02d_driver);
	if (ret < 0)
		return ret;

	printk(KERN_INFO DRIVER_NAME " driver loaded.\n");

	return 0;
}

static void __exit lis3lv02d_exit_module(void)
{
	acpi_bus_unregister_driver(&lis3lv02d_driver);
}

MODULE_DESCRIPTION("Glue between LIS3LV02Dx and HP ACPI BIOS and support for disk protection LED.");
MODULE_AUTHOR("Yan Burman, Eric Piel, Pavel Machek");
MODULE_LICENSE("GPL");

module_init(lis3lv02d_init_module);
module_exit(lis3lv02d_exit_module);

