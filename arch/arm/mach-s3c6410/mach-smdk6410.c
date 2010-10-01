/* linux/arch/arm/mach-s3c6410/mach-smdk6410.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/smsc911x.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/reboot.h>
#include <linux/pwm_backlight.h>
#include <linux/android_pmem.h>
#include <linux/i2c/pmic.h>

#ifdef CONFIG_SMDK6410_WM1190_EV1
#include <linux/mfd/wm8350/core.h>
#include <linux/mfd/wm8350/pmic.h>
#endif

#include <video/platform_lcd.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/regs-fb.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-sys.h>
#include <plat/regs-clock.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/fb.h>
#include <plat/regs-rtc.h>

#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/regs-lcd.h>
#include <plat/pm.h>
#include <plat/media.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/reserved_mem.h>
#include <mach/param.h>
#include <mach/volans_gpio_table.h>

#include <mach/sec_headset.h>

//#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <linux/usb/ch9.h>

///* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
//#define S3C_USB_CLKSRC	1
//#if defined (CONFIG_LCD_4)
//#define USB_HOST_PORT2_EN 1
//#else
//#undef USB_HOST_PORT2_EN
//#endif
//#ifdef USB_HOST_PORT2_EN
//#define OTGH_PHY_CLK_VALUE      (0x42)  /* Serial Interface, otg_phy input clk 48Mhz Oscillator */
//#else
//#define OTGH_PHY_CLK_VALUE      (0x02)  /* UTMI Interface, otg_phy input clk 48Mhz Oscillator */
//#endif
//#endif

//#define S3C_USB_CLKSRC	1
//#ifdef USB_HOST_PORT2_EN
#define OTGH_PHY_CLK_VALUE      (0x00) //UTMI interface, OTG_py input clk 48 mhz external crystal /* Serial Interface, otg_phy input clk 48Mhz Oscillator */
//#else
//#define OTGH_PHY_CLK_VALUE      (0x20)  /* UTMI Interface, otg_phy input clk 48Mhz Oscillator */
//#endif
//#endif

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

#define UCON S3C_UCON_DEFAULT
#define ULCON S3C_LCON_CS8 | S3C_LCON_PNONE
#define UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE

#ifndef CONFIG_HIGH_RES_TIMERS
extern struct sys_timer s3c64xx_timer;
#else
extern struct sys_timer sec_timer;
#endif /* CONFIG_HIGH_RES_TIMERS */

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

static struct sec_headset_port sec_headset_port[] = {
        {
		{ // HEADSET detect info
			.eint		= IRQ_EINT(10), 
			.gpio		= GPIO_DET_35,   
			.gpio_af	= GPIO_DET_35_AF  , 
			.low_active 	= 0
		},{ // SEND/END info
			.eint		= IRQ_EINT(11), 
			.gpio		= GPIO_EAR_SEND_END, 
			.gpio_af	= GPIO_EAR_SEND_END_AF, 
			.low_active	= 1
		}
        }
};
 
static struct sec_headset_platform_data sec_headset_data = {
        .port           = sec_headset_port,
        .nheadsets      = ARRAY_SIZE(sec_headset_port),
};

static struct platform_device sec_device_headset = {
        .name           = "sec_headset",
        .id             = -1,
        .dev            = {
                .platform_data  = &sec_headset_data,
        },
};

static struct s3c6410_pmem_setting pmem_setting = {
        .pmem_start = RESERVED_PMEM_START,
        .pmem_size = RESERVED_PMEM,
        .pmem_gpu1_start = GPU1_RESERVED_PMEM_START,
        .pmem_gpu1_size = RESERVED_PMEM_GPU1,
        .pmem_render_start = RENDER_RESERVED_PMEM_START,
        .pmem_render_size = RESERVED_PMEM_RENDER,
        .pmem_stream_start = STREAM_RESERVED_PMEM_START,
        .pmem_stream_size = RESERVED_PMEM_STREAM,
        .pmem_preview_start = PREVIEW_RESERVED_PMEM_START,
        .pmem_preview_size = RESERVED_PMEM_PREVIEW,
        .pmem_picture_start = PICTURE_RESERVED_PMEM_START,
        .pmem_picture_size = RESERVED_PMEM_PICTURE,
        .pmem_jpeg_start = JPEG_RESERVED_PMEM_START,
        .pmem_jpeg_size = RESERVED_PMEM_JPEG,
        .pmem_skia_start = SKIA_RESERVED_PMEM_START,
        .pmem_skia_size = RESERVED_PMEM_SKIA,
};

#ifdef CONFIG_S3C64XX_ADCTS
static struct s3c_adcts_plat_info s3c_adcts_cfgs __initdata = {
	.channel = {
		{ /* 0 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 1 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 2 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 3 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 4 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 5 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 6 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 7 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},
	},
};
#endif

static int uart_current_owner = 0;

static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if ( uart_current_owner )		
		return sprintf(buf, "%s[UART Switch] Current UART owner = PDA \n", buf);	
	else			
		return sprintf(buf, "%s[UART Switch] Current UART owner = MODEM \n", buf);
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
	int switch_sel = 1;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	if (strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0)	{		
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);		
		uart_current_owner = 1;		
		switch_sel |= UART_SEL_MASK;
		printk("[UART Switch] Path : PDA\n");	
	}	

	if (strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {		
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);		
		uart_current_owner = 0;		
		switch_sel &= ~UART_SEL_MASK;
		printk("[UART Switch] Path : MODEM\n");	
	}	

	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);

	return size;
}

static DEVICE_ATTR(uart_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, uart_switch_show, uart_switch_store);

static void saturn_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

        if (gpio_is_valid(GPIO_UART_SEL)) {
                if (gpio_request(GPIO_UART_SEL, S3C_GPIO_LAVEL(GPIO_UART_SEL)))
                        printk(KERN_ERR "Failed to request GPIO_UART_SEL!\n");
                gpio_direction_output(GPIO_UART_SEL, gpio_get_value(GPIO_UART_SEL));
        }
        s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

        if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
                pr_err("Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);

};

static void volans_init_gpio(void)
{
	struct __gpio_config *pgpio;
	unsigned int pin;
	int i;

	for (i = 0; i < ARRAY_SIZE(volans_gpio_table); i++) {
		pgpio = &volans_gpio_table[i];
		pin = pgpio->gpio;

		if (pgpio->level != GPIO_LEVEL_NONE)
			s3c_gpio_setpin(pin, pgpio->level);

		/* off part */
		if (pin < S3C64XX_GPK(0)) {
			s3c_gpio_cfgpin(pin, S3C_GPIO_SFN(pgpio->af));
			s3c_gpio_setpull(pin, pgpio->pull);
			s3c_gpio_slp_cfgpin(pin, pgpio->slp_con);
			s3c_gpio_slp_setpull_updown(pin, pgpio->slp_pull);
		}

		/* alive part */
		else if (pin < S3C64XX_GPO(0)) {
			s3c_gpio_cfgpin(pin, S3C_GPIO_SFN(pgpio->af));
			s3c_gpio_setpull(pin, pgpio->pull);
		}

		/* memory part */
		else {
			s3c_gpio_cfgpin(pin, S3C_GPIO_SFN(pgpio->af));
			s3c_gpio_setpull(pin, pgpio->pull);
			s3c_gpio_slp_cfgpin(pin, pgpio->slp_con);
			s3c_gpio_slp_setpull_updown(pin, pgpio->slp_pull);
		}
	}
}

static struct i2c_gpio_platform_data i2c_pmic_platdata = {
	.sda_pin	= GPIO_PWR_I2C_SDA,
	.scl_pin	= GPIO_PWR_I2C_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1
};

static struct platform_device sec_device_i2c_pmic = {
	.name	= "i2c-gpio",
	.id		= 2,
	.dev.platform_data	= &i2c_pmic_platdata,
};
/*
static struct i2c_gpio_platform_data i2c_touch_platdata = {
	.sda_pin	= GPIO_TOUCH_SDA,
	.scl_pin	= GPIO_TOUCH_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1
};

static struct platform_device sec_device_i2c_touch = {
	.name	= "i2c-gpio",
	.id		= 3,
	.dev.platform_data	= &i2c_touch_platdata,
};
*/

static struct i2c_gpio_platform_data i2c_common_platdata = {
	.sda_pin	= GPIO_FM_SDA,
	.scl_pin	= GPIO_FM_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1
};

static struct platform_device sec_device_i2c_common = {
	.name	= "i2c-gpio",
	.id		= 5,
	.dev.platform_data	= &i2c_common_platdata,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs1[] __initdata = {
};


struct platform_device sec_device_opt = {
        .name   = "gp2a-opt",
        .id             = -1,
};

static struct s3c_uartcfg saturn_uartcfgs[] __initdata = {
	[0] = {	/* Phone */
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {	/* Bluetooth */
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {	/* Serial */
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
};

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_AP_FLM_RXD, S3C_GPIO_SFN(GPIO_AP_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_FLM_TXD, S3C_GPIO_SFN(GPIO_AP_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
	else if (port == 1) {
		s3c_gpio_cfgpin(GPIO_AP_BT_RXD, S3C_GPIO_SFN(GPIO_AP_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_TXD, S3C_GPIO_SFN(GPIO_AP_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_CTS, S3C_GPIO_SFN(GPIO_AP_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_AP_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_RTS, S3C_GPIO_SFN(GPIO_AP_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_AP_BT_RTS, S3C_GPIO_PULL_NONE);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);

void s3c_reset_uart_cfg_gpio(unsigned char port)
{
#if 0  // dgahn.temp
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_AP_FLM_RXD, S3C_GPIO_SFN(GPIO_AP_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_FLM_TXD, S3C_GPIO_SFN(GPIO_AP_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
#endif
	if (port == 1) {
		s3c_gpio_cfgpin(GPIO_AP_BT_RXD, 0);
		s3c_gpio_setpull(GPIO_AP_BT_RXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_TXD, 0);
		s3c_gpio_setpull(GPIO_AP_BT_TXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_CTS, 0);
		s3c_gpio_setpull(GPIO_AP_BT_CTS, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_RTS, 0);
		s3c_gpio_setpull(GPIO_AP_BT_RTS, S3C_GPIO_PULL_DOWN);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_reset_uart_cfg_gpio);

#ifdef CONFIG_FB_S3C_ORG
//TO DO: onderstaande dingen daadwerkelijk toevoegen!
/* framebuffer and LCD setup. */

/* GPF15 = LCD backlight control
 * GPF13 => Panel power
 * GPN5 = LCD nRESET signal
 * PWM_TOUT1 => backlight brightness
 */

void smdk6410_lcd_power_set(struct plat_lcd_data *pd,
				   unsigned int power)
{
	if (power) {
		gpio_direction_output(S3C64XX_GPF(15), 1);

		/* fire nRESET on power up */
		gpio_direction_output(S3C64XX_GPN(5), 0);
		msleep(10);
		gpio_direction_output(S3C64XX_GPN(5), 1);
		msleep(1);
	} else {
		gpio_direction_output(S3C64XX_GPF(15), 0);
	}
}

static struct plat_lcd_data smdk6410_lcd_power_data = {
	.set_power	= smdk6410_lcd_power_set,
};

static struct platform_device smdk6410_lcd_powerdev = {
	.name			= "platform-lcd",
	.dev.parent		= &s3c_device_lcd.dev,
	.dev.platform_data	= &smdk6410_lcd_power_data,
};

static struct s3c_fb_pd_win smdk6410_fb_win0 = {
	/* this is to ensure we use win0 */
	.win_mode	= {
		.pixclock	= 27000000,
		.left_margin	= 216,
		.right_margin	= 40,
		.upper_margin	= 35,
		.lower_margin	= 10,
		.hsync_len	= 1,
		.vsync_len	= 1,
		.xres		= 800,
		.yres		= 480,
	},
	.max_bpp	= 24,
	.default_bpp	= 16,
};

/* 405566 clocks per frame => 60Hz refresh requires 24333960Hz clock */
static struct s3c_fb_platdata smdk6410_lcd_pdata __initdata = {
	.setup_gpio	= s3c64xx_fb_gpio_setup_24bpp,
	.win[0]		= &smdk6410_fb_win0,
	.vidcon0	= VIDCON0_VIDOUT_RGB | VIDCON0_PNRMODE_RGB,
	.vidcon1	= VIDCON1_INV_HSYNC | VIDCON1_INV_VSYNC,
};

#endif


static struct map_desc smdk6410_iodesc[] = {
	{
		.virtual	= (unsigned long)S3C_VA_LCD,
		.pfn		= __phys_to_pfn(S3C64XX_PA_LCD),
		.length	= S3C64XX_SZ_LCD,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C64XX_VA_HOSTIFB),
		.pfn		= __phys_to_pfn(S3C64XX_PA_HOSTIFB),
		.length	= S3C64XX_SZ_HOSTIFB,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C64XX_VA_HOSTIFA),
		.pfn		= __phys_to_pfn(S3C64XX_PA_HOSTIFA),
		.length	= S3C64XX_SZ_HOSTIFA,
		.type		= MT_DEVICE,
	},{
		.virtual	= (unsigned long)(S3C64XX_VA_OTG),
		.pfn		= __phys_to_pfn(S3C64XX_PA_OTG),
		.length	= S3C64XX_SZ_OTG,
		.type		= MT_DEVICE,
	},{
		.virtual	= (unsigned long)(S3C64XX_VA_OTGSFR),
		.pfn		= __phys_to_pfn(S3C64XX_PA_OTGSFR),
		.length	= S3C64XX_SZ_OTGSFR,
		.type		= MT_DEVICE,
	},
};

/*
 * GPIO Buttons
 */


struct platform_device smartq_gpio = {
	.name			= "smartq_gpio",
	.id				= -1,
	.num_resources	= 0,
};


static struct platform_device smartq_battery_device  = {
        .name           = "smartq-battery",
        .id             = -1,
        .num_resources  = 0,
};

//#ifdef CONFIG_ANDROID_PMEM	
#if 0
struct android_pmem_platform_data android_pmem_data = {
	.name 	= "pmem",
	.no_allocator = 0,
	.cached 	= 1,
	.buffered  = 0,
};

static struct platform_device android_pmem_device  = {
        .name           		= "android_pmem",
        .id             			= -1,
        .num_resources  	= 0,
        .dev 				= {
		.platform_data = &android_pmem_data,
	},
};

static int setup_platform_device_pem(struct platform_device *pdev, 
	unsigned long start, unsigned long size)
{
	struct android_pmem_platform_data *pem = pdev->dev.platform_data;
	pem->start = start;
	pem->size = size;
	return 0;
}
#endif

static struct platform_device android_usb_device  = {
        .name           		= "android_usb",
        .id             			= -1,
        .num_resources  	= 0,
};


#ifdef CONFIG_SMARTQ_BT_RFKILL
#include <linux/smartq-rfkill.h>
struct smartq_rfkill_platform_data rfkill_data = {
	.nshutdown_gpio	= S3C64XX_GPL(1),
};

static struct platform_device smartq_rfkill_device  = {
	.name 			= "smartq-rfkill",
        .id             			= -1,
        .num_resources  	= 0,
        .dev 				= {
		.platform_data = &rfkill_data,
	},
};
#endif

static struct platform_device s3c_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};

struct platform_device sec_device_ts = {
        .name   = "qt602240-ts",
        .id             = -1,
};


static struct platform_device *smdk6410_devices[] __initdata = {
#if defined(CONFIG_S3C_DMA_PL080_SOL)
        &s3c_device_dma0,
        &s3c_device_dma1,
        &s3c_device_dma2,
        &s3c_device_dma3,
#endif
#ifdef CONFIG_SMDK6410_SD_CH1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_SMDK6410_SD_CH0
//	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_SMDK6410_SD_CH2
//	&s3c_device_hsmmc2,
#endif
	&s3c_device_i2c0,
#ifdef CONFIG_S3C_DEV_I2C1
	&s3c_device_i2c1,
#endif
#ifdef CONFIG_FB_S3C_ORG
	&smdk6410_lcd_powerdev,
	&s3c_device_fb,
#endif
	&s3c_device_lcd,
	&s3c_device_keypad,
	&s3c_device_ts,
	&s3c_device_vpp,
	&s3c_device_mfc,
	&s3c_device_rotator,
	&s3c_device_jpeg,
	&s3c_device_g3d,
	&s3c_device_dpram,
	&smartq_battery_device,
#ifdef CONFIG_S3C64XX_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif
//	&s3c_device_wdt,
	&s3c_device_rtc,
#ifdef CONFIG_SND_S3C64XX_SOC_I2S
	&s3c64xx_device_iis0,
#endif
//	&s3c_device_iis,
	&s3c_device_usb,
	&s3c_device_usbgadget,
//#ifdef CONFIG_ANDROID_PMEM	
#if 0
	&android_pmem_device,
#endif
	&android_usb_device,
	&sec_device_i2c_pmic,			/* pmic(max8698) i2c. */
//	&sec_device_i2c_touch,
	&sec_device_i2c_common,			/* radio, sound, .. i2c. */
	&sec_device_headset,
	&sec_device_opt,
	&sec_device_ts,
#ifdef CONFIG_S3C64XX_ADCTS
	&s3c_device_adcts,
#endif
//	&smdk6410_smsc911x,
};

#if 0
#ifdef CONFIG_SMDK6410_WM1190_EV1
/* S3C64xx internal logic & PLL */
static struct regulator_init_data wm8350_dcdc1_data = {
	.constraints = {
		.name = "PVDD_INT/PVDD_PLL",
		.min_uV = 1200000,
		.max_uV = 1200000,
		.always_on = 1,
		.apply_uV = 1,
	},
};

/* Memory */
static struct regulator_init_data wm8350_dcdc3_data = {
	.constraints = {
		.name = "PVDD_MEM",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.always_on = 1,
		.state_mem = {
			 .uV = 1800000,
			 .mode = REGULATOR_MODE_NORMAL,
			 .enabled = 1,
		 },
		.initial_state = PM_SUSPEND_MEM,
	},
};

/* USB, EXT, PCM, ADC/DAC, USB, MMC */
static struct regulator_init_data wm8350_dcdc4_data = {
	.constraints = {
		.name = "PVDD_HI/PVDD_EXT/PVDD_SYS/PVCCM2MTV",
		.min_uV = 3000000,
		.max_uV = 3000000,
		.always_on = 1,
	},
};

/* ARM core */
static struct regulator_consumer_supply dcdc6_consumers[] = {
	{
		.supply = "vddarm",
	}
};

static struct regulator_init_data wm8350_dcdc6_data = {
	.constraints = {
		.name = "PVDD_ARM",
		.min_uV = 1000000,
		.max_uV = 1300000,
		.always_on = 1,
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies = ARRAY_SIZE(dcdc6_consumers),
	.consumer_supplies = dcdc6_consumers,
};

/* Alive */
static struct regulator_init_data wm8350_ldo1_data = {
	.constraints = {
		.name = "PVDD_ALIVE",
		.min_uV = 1200000,
		.max_uV = 1200000,
		.always_on = 1,
		.apply_uV = 1,
	},
};

/* OTG */
static struct regulator_init_data wm8350_ldo2_data = {
	.constraints = {
		.name = "PVDD_OTG",
		.min_uV = 3300000,
		.max_uV = 3300000,
		.always_on = 1,
	},
};

/* LCD */
static struct regulator_init_data wm8350_ldo3_data = {
	.constraints = {
		.name = "PVDD_LCD",
		.min_uV = 3000000,
		.max_uV = 3000000,
		.always_on = 1,
	},
};

/* OTGi/1190-EV1 HPVDD & AVDD */
static struct regulator_init_data wm8350_ldo4_data = {
	.constraints = {
		.name = "PVDD_OTGI/HPVDD/AVDD",
		.min_uV = 1200000,
		.max_uV = 1200000,
		.apply_uV = 1,
		.always_on = 1,
	},
};

static struct {
	int regulator;
	struct regulator_init_data *initdata;
} wm1190_regulators[] = {
	{ WM8350_DCDC_1, &wm8350_dcdc1_data },
	{ WM8350_DCDC_3, &wm8350_dcdc3_data },
	{ WM8350_DCDC_4, &wm8350_dcdc4_data },
	{ WM8350_DCDC_6, &wm8350_dcdc6_data },
	{ WM8350_LDO_1, &wm8350_ldo1_data },
	{ WM8350_LDO_2, &wm8350_ldo2_data },
	{ WM8350_LDO_3, &wm8350_ldo3_data },
	{ WM8350_LDO_4, &wm8350_ldo4_data },
};

static int __init smdk6410_wm8350_init(struct wm8350 *wm8350)
{
	int i;

	/* Instantiate the regulators */
	for (i = 0; i < ARRAY_SIZE(wm1190_regulators); i++)
		wm8350_register_regulator(wm8350,
					  wm1190_regulators[i].regulator,
					  wm1190_regulators[i].initdata);

	return 0;
}

static struct wm8350_platform_data __initdata smdk6410_wm8350_pdata = {
	.init = smdk6410_wm8350_init,
	.irq_high = 1,
};
#endif
#endif

#ifdef CONFIG_S3C64XX_ADC
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000, //41237
	.presc 			= 49,
	.oversampling_shift	= 2,//4
	.resol_bit 			= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6410 support 12-bit resolution */
	.delay	= 	10000,
	.presc 	= 	99,
	.resolution = 	12,
};
#endif

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id			= 1,
	.max_brightness	= 255,
	.dft_brightness		= 255,
	.pwm_period_ns	= 78770,
};

static struct platform_device smdk_backlight_device = {
	.name = "pwm-backlight",
	.dev	   = {
		.parent = &s3c_device_timer[1].dev,
		.platform_data = &smdk_backlight_data,
	},
};


static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#else
#define smdk_backlight_register()	do { } while (0)
#endif


static void s3c6410_power_off(void)
{
	int dc_status = 0;
#if defined (CONFIG_LCD_4)
	dc_status = gpio_get_value(S3C64XX_GPL(13));
#else
	dc_status = !gpio_get_value(S3C64XX_GPL(13));
#endif

	printk("Enter %s <%d> dc_status = %s ...\n", __func__, __LINE__, dc_status ? "on" : "off");

	if(dc_status)
		machine_restart(NULL);
	else
		gpio_set_value(S3C64XX_GPK(15), 1);
}
EXPORT_SYMBOL_GPL(s3c6410_power_off);

extern void s3c64xx_reserve_bootmem(void);

static void __init smdk6410_map_io(void)
{
	pm_power_off = s3c6410_power_off;
	s3c64xx_init_io(smdk6410_iodesc, ARRAY_SIZE(smdk6410_iodesc));
	s3c_init_clocks(12000000);
        s3c_init_uarts(saturn_uartcfgs, ARRAY_SIZE(saturn_uartcfgs));
	s3c64xx_reserve_bootmem();
//#ifdef CONFIG_ANDROID_PMEM	
#if 0
	/* map the s3c camera phy_mem to pmem */
	setup_platform_device_pem(&android_pmem_device, 
				(unsigned long)s3c_get_media_memory(S3C_MDEV_PMEM), 
				(unsigned long)s3c_get_media_memsize(S3C_MDEV_PMEM));
#endif
#ifdef CONFIG_FB_S3C_ORG
	u32 tmp;
	/* set the LCD type */
	tmp = __raw_readl(S3C64XX_SPCON);
	tmp &= ~S3C64XX_SPCON_LCD_SEL_MASK;
	tmp |= S3C64XX_SPCON_LCD_SEL_RGB;
	__raw_writel(tmp, S3C64XX_SPCON);

	tmp = __raw_readl(S3C_HOSTIFB_MIFPCON);
	tmp &= ~MIFPCON_LCD_BYPASS;
	__raw_writel(tmp, S3C_HOSTIFB_MIFPCON);
	
#endif
}

static void smdk6410_gpio_init (void)
{
	gpio_request(S3C64XX_GPN(5), 	"LCD_RST");
}

static void __init smdk6410_machine_init(void)
{
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

#ifdef CONFIG_S3C64XX_ADC
	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#ifdef CONFIG_S3C64XX_ADCTS
	s3c_adcts_set_platdata (&s3c_adcts_cfgs);
#endif
	
#ifdef CONFIG_FB_S3C_ORG
	s3c_fb_set_platdata(&smdk6410_lcd_pdata);
#endif
	s3c_fimc0_set_platdata(NULL);
	s3c_fimc1_set_platdata(NULL);

	volans_init_gpio();
	smdk6410_gpio_init();
	//writel(readl(S3C_PCLK_GATE)|S3C_CLKCON_PCLK_GPIO, S3C_PCLK_GATE);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	//s3c6410_add_mem_devices (&pmem_setting);

	platform_add_devices(smdk6410_devices, ARRAY_SIZE(smdk6410_devices));
	s3c6410_pm_init();
	saturn_switch_init();

}

MACHINE_START(SMDK6410, "OMNIA II")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io		= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,

	.init_irq	= s3c6410_init_irq,
	.map_io		= smdk6410_map_io,
	.init_machine	= smdk6410_machine_init,
#ifndef CONFIG_HIGH_RES_TIMERS
        .timer                  = &s3c64xx_timer,
#else
        .timer                  = &sec_timer,
#endif
MACHINE_END

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C_RTCCON) & (S3C_RTCCON_TICEN | S3C_RTCCON_RTCEN );
        writew(tmp, base + S3C_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C_RTCCON);
		writew(tmp & ~ (S3C_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C_RTCCON) & S3C_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp|S3C_RTCCON_RTCEN, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp& ~S3C_RTCCON_CNTSEL, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp & ~S3C_RTCCON_CLKRST, base+S3C_RTCCON);
		}
	}
}
#endif

void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C64XX_GPK(8 + rows);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S3C64XX_GPK(8); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S3C64XX_GPL(0 + columns);

	/* Set all the necessary GPL pins to special-function 0 */
	for (gpio = S3C64XX_GPL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);

/* Initializes OTG Phy. */
void otg_phy_init(int phy_clk_val) {

	writel(readl(S3C_OTHERS)|S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
	writel(0x0, S3C_USBOTG_PHYPWR);		/* Power up */
        writel(OTGH_PHY_CLK_VALUE, S3C_USBOTG_PHYCLK);	//writel(phy_clk_val, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);
        printk("OTG initialized" );

	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(readl(S3C_OTHERS)&~S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(int id) 
{
	struct clk *otg_clk;

        switch (id) {
	case 0: 		/* epll clk */
		writel((readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK)
			|S3C_CLKSRC_EPLL_CLKSEL|S3C_CLKSRC_UHOST_EPLL,
			S3C_CLK_SRC);

		/* USB host colock divider ratio is 2 */
		writel((readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK)
			|(1<<20), S3C_CLK_DIV1);
		break;
	case 1: 		/* oscillator 48M clk , for usb ohci */
		
		otg_clk = clk_get(NULL, "otg");
		clk_enable(otg_clk);
		
		writel(readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK, S3C_CLK_SRC);
		
		otg_phy_init(OTGH_PHY_CLK_VALUE);

		/* USB host colock divider ratio is 1 */
		writel(readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK, S3C_CLK_DIV1);
		break;
	case 2: 		/* oscillator 48M clk , for otg */
		
		otg_clk = clk_get(NULL, "otg");
		clk_enable(otg_clk);
		
		writel(readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK, S3C_CLK_SRC);
		
		otg_phy_init(0x02);
	
		/* USB host colock divider ratio is 1 */
		writel(readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK, S3C_CLK_DIV1);
		break;
	default:
		printk(KERN_INFO "Unknown USB Host Clock Source\n");
		BUG();
		break;
	}

	writel(readl(S3C_HCLK_GATE)|S3C_CLKCON_HCLK_UHOST|S3C_CLKCON_HCLK_SECUR,
		S3C_HCLK_GATE);
	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_UHOST, S3C_SCLK_GATE);

}

EXPORT_SYMBOL(usb_host_clk_en);

static void check_pmic(void)
{	
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(EN1, &reg_buff)) {
		pr_info("%s: BUCK1 (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(EN1, 0);
	}
	if (Get_MAX8698_PM_REG(EN2, &reg_buff)) {
		pr_info("%s: BUCK2 (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(EN2, 0);
	}
	if (Get_MAX8698_PM_REG(ELDO3, &reg_buff)) {
		pr_info("%s: OTGI 1.2V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO4, &reg_buff)) {
		pr_info("%s: VLED 3.3V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: VTF 3.0V (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
	if (Get_MAX8698_PM_REG(ELDO6, &reg_buff)) {
		pr_info("%s: VLCD 1.8V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO7, &reg_buff)) {
		pr_info("%s: VLCD 3.0V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO8, &reg_buff)) {
		pr_info("%s: OTG 3.3V (%d)\n", __func__, reg_buff);
	}
}

static void volans_pmic_halt(void)
{
	gpio_set_value(GPIO_PS_HOLD_PDA, 1);
	s3c_gpio_cfgpin(GPIO_PS_HOLD_PDA, S3C_GPIO_OUTPUT);
	gpio_set_value(GPIO_PS_HOLD_PDA, 0);
}

extern void machine_restart(char * __unused);
void volans_machine_halt(char mode)
{
	if (mode == 'r') {
		if (gpio_get_value(GPIO_VREG_MSMP_26V)) {
			machine_restart(NULL);
		} else if (!gpio_get_value(GPIO_TA_nCONNECTED)) {
			unsigned int *inf = (unsigned int *)__va(VOLANS_PA_BM);
			*inf = VOLANS_BM_MAGIC;

			machine_restart(NULL);
		} else {
			volans_pmic_halt();
		}
	} else if (mode == 'h') {
		volans_pmic_halt();
	}
}
EXPORT_SYMBOL(volans_machine_halt);

unsigned int CURRENT_REV(void)
{
	return 1;
}
EXPORT_SYMBOL(CURRENT_REV);

int (*connected_device_status)(void) = NULL;
EXPORT_SYMBOL(connected_device_status);

void s3c_config_sleep_gpio(void)
{	
	int spcon_val;

	check_pmic();
	//bss s3c_config_gpio_table(ARRAY_SIZE(saturn_sleep_gpio_table),
	//bss		saturn_sleep_gpio_table);

	spcon_val = __raw_readl(S3C64XX_SPCON);
	spcon_val = spcon_val & (~0xFFEC0000);
	__raw_writel(spcon_val, S3C64XX_SPCON);
	__raw_writel(0x20, S3C64XX_SPCONSLP);

	/* mem interface reg config in sleep mode */
	__raw_writel(0x00005000, S3C64XX_MEM0CONSLP0);
	__raw_writel(0x01041595, S3C64XX_MEM0CONSLP1);
	__raw_writel(0x10055000, S3C64XX_MEM1CONSLP);

}
EXPORT_SYMBOL(s3c_config_sleep_gpio);

void s3c_config_wakeup_gpio(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: VTF 3.0V (%d)\n", __func__, reg_buff);
		if (!reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 1);
	}
}
EXPORT_SYMBOL(s3c_config_wakeup_gpio);

void s3c_config_wakeup_source(void)
{
	unsigned int eint0pend_val;

	/* Wake-up source 
	 * ONEDRAM_INT(EINT0), WLAN_HOST_WAKE(EINT1),  
	 * Power key(EINT5), T_FLASH_DETECT(EINT6), INTB(EINT9)
	 * DET_3.5(EINT10), EAR_SEND_END(EINT11),
	 * Hold key(EINT17), TA_CONNECTED(EINT19),
	 * BT_HOST_WAKE(EINT22), CHG_ING(EINT25)
	 */
	eint0pend_val= __raw_readl(S3C64XX_EINT0PEND);
	eint0pend_val |= (0x1 << 25) | (0x1 << 22) | (0x1 << 19) |
		(0x1 << 17) |(0x1 << 11) | (0x1 << 10) | (0x1 << 9)| (0x1 << 6) | (0x1 << 5) | (0x1 << 1) | 0x1;
	__raw_writel(eint0pend_val, S3C64XX_EINT0PEND);

	eint0pend_val = (0x1 << 25) | (0x1 << 22) | (0x1 << 19) |
		(0x1 << 17)|(0x1 << 11) | (0x1 << 10) | (0x1 << 9)| (0x1 << 6) |(0x1 << 5) | (0x1 << 1) | 0x1;
	__raw_writel(~eint0pend_val, S3C64XX_EINT0MASK);

	__raw_writel((0x0FFFFFFF & ~eint0pend_val), S3C_EINT_MASK);	

	/* Alarm Wakeup Enable */
	__raw_writel((__raw_readl(S3C_PWR_CFG) & ~(0x1 << 10)), S3C_PWR_CFG);
}
EXPORT_SYMBOL(s3c_config_wakeup_source);



