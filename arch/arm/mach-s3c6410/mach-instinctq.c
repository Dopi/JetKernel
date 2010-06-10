/* linux/arch/arm/mach-s3c6410/mach-instinctq.c
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
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/ctype.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/bootmem.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/param.h>

#include <asm/irq.h>

#include <asm/setup.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/iic.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adcts.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/reserved_mem.h>
#include <plat/pm.h>

#include <mach/gpio.h>
#include <mach/gpio-core.h>

#include <plat/gpio-cfg.h>
#include <linux/android_pmem.h>
#include <linux/i2c/pmic.h>

#include <mach/sec_headset.h>

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

#ifdef CONFIG_SEC_LOG_BUF
struct device *sec_log_dev;
EXPORT_SYMBOL(sec_log_dev);
#endif

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);


void instinctq_init_gpio(void);

#define UCON S3C_UCON_DEFAULT
#define ULCON S3C_LCON_CS8 | S3C_LCON_PNONE
#define UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE

#define S3C64XX_KERNEL_PANIC_DUMP_SIZE 0x8000 /* 32kbytes */
void *S3C64XX_KERNEL_PANIC_DUMP_ADDR;
#ifndef CONFIG_HIGH_RES_TIMERS
extern struct sys_timer s3c64xx_timer;
#else
extern struct sys_timer sec_timer;
#endif /* CONFIG_HIGH_RES_TIMERS */

static struct s3c_uartcfg instinctq_uartcfgs[] __initdata = {
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

#if defined(CONFIG_I2C_GPIO)
static struct i2c_gpio_platform_data i2c2_platdata = {
	.sda_pin		= GPIO_FM_I2C_SDA,
	.scl_pin		= GPIO_FM_I2C_SCL,
	.udelay			= 2,	/* 250KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c2 = {
	.name				= "i2c-gpio",
	.id					= 2,
	.dev.platform_data	= &i2c2_platdata,
};

static struct i2c_gpio_platform_data i2c3_platdata = {
	.sda_pin		= GPIO_PWR_I2C_SDA,
	.scl_pin		= GPIO_PWR_I2C_SCL,
	.udelay			= 2,	/* 250KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c3 = {
	.name				= "i2c-gpio",
	.id					= 3,
	.dev.platform_data	= &i2c3_platdata,
};

#if 0  // dgahn.temp
static struct i2c_gpio_platform_data i2c4_platdata = {
	.sda_pin		= GPIO_EMUL_I2C_SDA,
	.scl_pin		= GPIO_EMUL_I2C_SCL,
	.udelay			= 2,	/* 250KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c4 = {
	.name				= "i2c-gpio",
	.id					= 4,
	.dev.platform_data	= &i2c4_platdata,
};
#endif

// dgahn.smd: move to rev01
static struct i2c_gpio_platform_data i2c5_platdata = {
	.sda_pin		= GPIO_AP_SDA/*GPIO_TOUCH_I2C_SDA*/,
	.scl_pin		= GPIO_AP_SCL/*GPIO_TOUCH_I2C_SCL*/,
	.udelay			= 3,	/* 166KHz */		
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1,
};

static struct platform_device s3c_device_i2c5 = {
	.name				= "i2c-gpio",
	.id					= 5,
	.dev.platform_data	= &i2c5_platdata,
};

#endif

#ifdef CONFIG_FB_S3C_TL2796
struct platform_device sec_device_backlight = {
	.name   = "tl2796-backlight",
	.id     = -1,
};
#elif defined(CONFIG_FB_S3C_S6D05A0)
struct platform_device sec_device_backlight = {
	.name   = "bd6091gu-backlight",
	.id     = -1,
};
#else
struct platform_device sec_device_backlight = {
	.name   = "ams320fs01-backlight",
	.id     = -1,
};
#endif

struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};

#if defined(CONFIG_MACH_QUATTRO) //quattro.tsp
struct platform_device sec_device_ts = {
	.name	= "qt602240-ts",
	.id		= -1,
};
#else
struct platform_device sec_device_ts = {
	.name	= "melfas-ts",
	.id		= -1,
};
#endif

struct platform_device sec_device_max9877 = {
    .name   = "max9877",
	.id     = -1,
};

//mk93.lee
struct platform_device sec_device_battery = {
	.name   = "instinctq-battery",
	.id		= -1,
};

struct platform_device sec_device_rfkill = {
	.name = "bt_rfkill",
	.id = -1,
};

struct platform_device sec_device_btsleep = {
	.name = "bt_sleep",
	.id = -1,
};

#if !defined(CONFIG_JET_OPTION)
//(CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
static struct gpio_led sec_keyled_list[] = {
	{ 
		.name = "keyboard-backlight",
		.gpio = GPIO_MAIN_KEY_LED_EN,
	},
	{ 
		.name = "button-backlight",
		.gpio = GPIO_SUB_KEY_LED_EN,
	},
};

static struct gpio_led_platform_data sec_keyled_data = {
	.num_leds	= ARRAY_SIZE(sec_keyled_list),
	.leds		= sec_keyled_list,
};

static struct platform_device sec_device_keyled = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data	= &sec_keyled_data,
	},
};

struct platform_device sec_device_optjoy = {
	.name   = "optjoy_device",
	.id		= -1,
};
#endif

struct platform_device sec_device_opt = {
	.name   = "gp2a-opt",
	.id		= -1,
};

static struct sec_headset_port sec_headset_port[] = {
        {
		{ // HEADSET detect info
			.eint		= IRQ_EINT(10), 
			.gpio		= GPIO_DET_35,   
			.gpio_af	= GPIO_DET_35_AF  , 
			.low_active 	= 1
		},{ // SEND/END info
			.eint		= IRQ_EINT(11), 
			.gpio		= GPIO_EAR_SEND_END, 
			.gpio_af	= GPIO_EAR_SEND_END_AF, 
			.low_active	= 0
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

struct map_desc instinctq_iodesc[] __initdata = {
};

static struct platform_device *instinctq_devices[] __initdata = {
#if defined(CONFIG_S3C_DMA_PL080_SOL)
	&s3c_device_dma0,
	&s3c_device_dma1,
	&s3c_device_dma2,
	&s3c_device_dma3,
#endif
	&s3c_device_hsmmc0,
#if (CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
	&s3c_device_hsmmc2,
#endif
	&s3c_device_i2c0,
	&s3c_device_i2c1,
#if defined(CONFIG_I2C_GPIO)
	&s3c_device_i2c2,
	&s3c_device_i2c3,
	//&s3c_device_i2c4,
       &s3c_device_i2c5,
#endif
	&sec_device_ts,
	&s3c_device_adc,
	&s3c_device_lcd,
	&s3c_device_keypad,
	&s3c_device_usbgadget,
	&s3c_device_camif,
	&s3c_device_mfc,
	&s3c_device_g3d,
	&s3c_device_2d,
	&s3c_device_rotator,
	&s3c_device_jpeg,
	&s3c_device_vpp,
	&sec_device_backlight,
	&sec_device_dpram,
	&sec_device_max9877,
	&sec_device_battery,
	&sec_device_rfkill,
	&sec_device_btsleep,  // BT_SLEEP_ENABLER
	&s3c_device_rtc, // by Anubis
#if !defined(CONFIG_JET_OPTION)
//#if (CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
	&sec_device_keyled,
	&sec_device_optjoy,
#endif
	&sec_device_headset,
	&sec_device_opt,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs1[] __initdata = {
};

static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* Support 12-bit resolution */
	.delay		= 0xff,
	.presc 		= 49,
	.resolution	= 12,
};

static void __init instinctq_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = PHYS_UNRESERVED_SIZE;
	mi->bank[0].node = 0;
}

static void __init s3c64xx_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = S3C64XX_KERNEL_PANIC_DUMP_SIZE;
	addr = alloc_bootmem(size);
	S3C64XX_KERNEL_PANIC_DUMP_ADDR = addr;

}
static void __init instinctq_map_io(void)
{
	s3c64xx_init_io(instinctq_iodesc, ARRAY_SIZE(instinctq_iodesc));
	s3c64xx_gpiolib_init();
	s3c_init_clocks(12000000);
	s3c_init_uarts(instinctq_uartcfgs, ARRAY_SIZE(instinctq_uartcfgs));
	s3c64xx_allocate_memory_regions();
}

static void instinctq_set_qos(void) 
{     
	u32 reg;     							 /* AXI sfr */     

	reg = (u32) ioremap((unsigned long) S3C6410_PA_AXI_SYS, SZ_4K); /* QoS override: FIMD min. latency */
	writel(0x2, S3C_VA_SYS + 0x128);  	    			/* AXI QoS */
	writel(0x7, reg + 0x460);   					/* (8 - MFC ch.) */
	writel(0x7ff7, reg + 0x464);      				/* Bus cacheable */
	writel(0x8ff, S3C_VA_SYS + 0x838);

	__raw_writel(0x0, S3C_AHB_CON0);
} 

/*
 *	Power Off Handler
 */

extern int get_usb_cable_state(void);

#define AV					(0x1 << 14)
#define TTY					(0x1 << 13)
#define PPD					(0x1 << 12)
#define JIG_UART_OFF		(0x1 << 11)
#define JIG_UART_ON			(0x1 << 10)
#define JIG_USB_OFF			(0x1 << 9)
#define JIG_USB_ON			(0x1 << 8)
#define USB_OTG				(0x1 << 7)
#define DEDICATED_CHARGER	(0x1 << 6)
#define USB_CHARGER			(0x1 << 5)
#define CAR_KIT				(0x1 << 4)
#define UART				(0x1 << 3)
#define USB					(0x1 << 2)
#define AUDIO_TYPE2			(0x1 << 1)
#define AUDIO_TYPE1			(0x1 << 0)

extern void arch_reset(char mode);

static void instinctq_pm_power_off(void)
{
	int	mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';
	int cnt = 0;

	if (!gpio_get_value(GPIO_TA_CONNECTED_N)) {	/* Reboot Charging */
		mode = REBOOT_MODE_CHARGING;
		if (sec_set_param_value)
			sec_set_param_value(__REBOOT_MODE, &mode);
		/* Watchdog Reset */
		printk(KERN_EMERG "%s: TA is connected, rebooting...\n", __func__);
		arch_reset(reset_mode);
		printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
	}
	else {	/* Power Off or Reboot */
		if (sec_set_param_value)
			sec_set_param_value(__REBOOT_MODE, &mode);
		if (get_usb_cable_state() & (JIG_UART_ON | JIG_UART_OFF | JIG_USB_OFF | JIG_USB_ON)) {
			/* Watchdog Reset */
			printk(KERN_EMERG "%s: JIG is connected, rebooting...\n", __func__);
			arch_reset(reset_mode);
			printk(KERN_EMERG "%s: waiting for reset!\n", __func__);
		}
		else {
			/* POWER_N -> Input */
			gpio_direction_input(GPIO_POWER_N);
			/* PHONE_ACTIVE -> Input */
			gpio_direction_input(GPIO_PHONE_ACTIVE);
			/* Check Power Off Condition */
			if (!gpio_get_value(GPIO_POWER_N) || gpio_get_value(GPIO_PHONE_ACTIVE)) {
				/* Wait Power Button Release */
				printk(KERN_EMERG "%s: waiting for GPIO_POWER_N high.\n", __func__);
				while (!gpio_get_value(GPIO_POWER_N)); 

				/* Wait Phone Power Off */
				printk(KERN_EMERG "%s: waiting for GPIO_PHONE_ACTIVE low.\n", __func__);
				while (gpio_get_value(GPIO_PHONE_ACTIVE)) {
					if (cnt++ < 5) {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE is high(%d)\n", __func__, cnt);
						mdelay(1000);
					} else {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE TIMED OUT!!!\n", __func__);
						break;
					}
				}	
			}
			/* PS_HOLD -> Output Low */
			printk(KERN_EMERG "%s: setting GPIO_PDA_PS_HOLD low.\n", __func__);
			gpio_direction_output(GPIO_PDA_PS_HOLD, GPIO_LEVEL_HIGH);
			s3c_gpio_setpull(GPIO_PDA_PS_HOLD, S3C_GPIO_PULL_NONE);
			gpio_set_value(GPIO_PDA_PS_HOLD, GPIO_LEVEL_LOW);

			printk(KERN_EMERG "%s: should not reach here!\n", __func__);
		}
	}

	while (1);
}

static int uart_current_owner = 0;

static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if ( uart_current_owner )		
		return sprintf(buf, "%s[UART Switch] Current UART owner = PDA \n", buf);	
	else			
		return sprintf(buf, "%s[UART Switch] Current UART owner = MODEM \n", buf);
}

static ssize_t uart_switch_store(		struct device *dev, struct device_attribute *attr,		const char *buf, size_t size)
{	
	int switch_sel;

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

static int instinctq_notifier_call(struct notifier_block *this, unsigned long code, void *_cmd)
{
	int	mode = REBOOT_MODE_NONE;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "arm11_fota"))
			mode = REBOOT_MODE_ARM11_FOTA;
		else if (!strcmp((char *)_cmd, "arm9_fota"))
			mode = REBOOT_MODE_ARM9_FOTA;
		else if (!strcmp((char *)_cmd, "recovery")) 
			mode = REBOOT_MODE_RECOVERY;
		else if (!strcmp((char *)_cmd, "download")) 
			mode = REBOOT_MODE_DOWNLOAD;
	}

	if (sec_set_param_value)
		sec_set_param_value(__REBOOT_MODE, &mode);
	
	return NOTIFY_DONE;
}

static struct notifier_block instinctq_reboot_notifier = {
	.notifier_call = instinctq_notifier_call,
};

static void instinctq_switch_init(void)
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

static void __init instinctq_machine_init(void)
{
	printk("INSTINCTQ Machine INIT : Board REV 0x%x\n", CONFIG_INSTINCTQ_REV);
	
	instinctq_init_gpio();

	//MOTOR and VIBTONZE DISABLE
	s3c_gpio_cfgpin(GPIO_VIB_EN, S3C_GPIO_SFN(GPIO_VIB_EN_AF));
	s3c_gpio_setpull(GPIO_VIB_EN, S3C_GPIO_PULL_DOWN);
	gpio_direction_output(GPIO_VIB_EN, GPIO_LEVEL_LOW);

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

	s3c_adc_set_platdata(&s3c_adc_platform);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	platform_add_devices(instinctq_devices, ARRAY_SIZE(instinctq_devices));
    s3c6410_add_mem_devices (&pmem_setting);

	s3c6410_pm_init();

	instinctq_set_qos();

	pm_power_off = instinctq_pm_power_off;

	register_reboot_notifier(&instinctq_reboot_notifier);

	instinctq_switch_init();

#ifdef CONFIG_SEC_LOG_BUF
	sec_log_buf_init();
#endif
}

#if defined(CONFIG_MACH_VINSQ)
MACHINE_START(INSTINCTQ, "SPH-M910")
#else
MACHINE_START(INSTINCTQ, "SPH-M900")
#endif

	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io		= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,

	.init_irq		= s3c6410_init_irq,
	.fixup			= instinctq_fixup,
	.map_io			= instinctq_map_io,
	.init_machine	= instinctq_machine_init,
#ifndef CONFIG_HIGH_RES_TIMERS
	.timer			= &s3c64xx_timer,
#else
	.timer			= &sec_timer,
#endif /* CONFIG_HIGH_RES_TIMERS */
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

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
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
#endif

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
#if 0  // dgahn.temp
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
#endif
	if (port == 1) {
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_PDA_RXD, S3C_GPIO_SFN(GPIO_PDA_RXD_AF));
		s3c_gpio_setpull(GPIO_PDA_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_PDA_TXD, S3C_GPIO_SFN(GPIO_PDA_TXD_AF));
		s3c_gpio_setpull(GPIO_PDA_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);

void s3c_reset_uart_cfg_gpio(unsigned char port)
{
#if 0  // dgahn.temp
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
#endif
	if (port == 1) {
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_OUTPUT);
		gpio_set_value(GPIO_BT_RTS, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_PDA_RXD, S3C_GPIO_SFN(GPIO_PDA_RXD_AF));
		s3c_gpio_setpull(GPIO_PDA_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_PDA_TXD, S3C_GPIO_SFN(GPIO_PDA_TXD_AF));
		s3c_gpio_setpull(GPIO_PDA_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_reset_uart_cfg_gpio);

#if defined(CONFIG_MACH_VINSQ)
static int instinctq_gpio_table[][6] = {
	/** OFF PART **/
	/* GPA */
	{ GPIO_AP_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_CTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_RTS, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPB */
	{ GPIO_PDA_RXD, GPIO_PDA_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_PDA_TXD, GPIO_PDA_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C0_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_I2C0_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPC */
	{ GPIO_OJ_SPI_MISO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_MOSI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_CS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_SDIO_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_WLAN_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPD */
	{ GPIO_I2S_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_WLAN_REG_ON, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_I2S_LRCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPE */
	{ GPIO_BT_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BOOT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPF */
	{ GPIO_CAM_MCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_HSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_PCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MCAM_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_VSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_4, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_5, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_6, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_7, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MIC_SEL_EN_REV04, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_LUM_PWM, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_PWM1_TOUT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPG */
	{ GPIO_TF_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TF_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_SUB_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
	/* GPH */
	{ GPIO_MAIN_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
	{ GPIO_LCD_BL_EN, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE }, //vinsq.boot : GPIO_LEVEL_LOW > GPIO_LEVEL_HIGH
	{ GPIO_CAM_STANDBY, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_OJ_SHUTDOWN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_TFLASH_EN, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPI */
	{ GPIO_LCD_B_0, GPIO_LCD_B_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_1, GPIO_LCD_B_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_2, GPIO_LCD_B_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_3, GPIO_LCD_B_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_4, GPIO_LCD_B_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_B_5, GPIO_LCD_B_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_B_6, GPIO_LCD_B_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_B_7, GPIO_LCD_B_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_0, GPIO_LCD_G_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_1, GPIO_LCD_G_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_2, GPIO_LCD_G_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_3, GPIO_LCD_G_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_4, GPIO_LCD_G_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_5, GPIO_LCD_G_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_G_6, GPIO_LCD_G_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_G_7, GPIO_LCD_G_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPJ */
	{ GPIO_LCD_R_0, GPIO_LCD_R_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_1, GPIO_LCD_R_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_2, GPIO_LCD_R_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_3, GPIO_LCD_R_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_4, GPIO_LCD_R_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_R_5, GPIO_LCD_R_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_R_6, GPIO_LCD_R_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_R_7, GPIO_LCD_R_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_HSYNC, GPIO_LCD_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_VSYNC, GPIO_LCD_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_DE, GPIO_LCD_DE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_CLK, GPIO_LCD_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/** ALIVE PART **/
	/* GPK */
//.m [VinsQ] [Start] Charger IC Changed to MAX8922    Pineone hgwoo    2010.01.05
	{ GPIO_CHG_EN, GPIO_CHG_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
//.m [VinsQ] [End] Charger IC Changed to MAX8922    hgwoo    2010.01.05
	{ GPIO_AUDIO_EN, GPIO_AUDIO_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_MIC_BIAS, GPIO_EAR_MIC_BIAS_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_MICBIAS_EN, GPIO_MICBIAS_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	/* { GPIO_UART_SEL, GPIO_UART_SEL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, */
	{ GPIO_MONOHEAD_DET, GPIO_MONOHEAD_DET_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_RST_N, GPIO_PHONE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_0, GPIO_KEYSENSE_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_1, GPIO_KEYSENSE_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_2, GPIO_KEYSENSE_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_3, GPIO_KEYSENSE_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_4, GPIO_KEYSENSE_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_5, GPIO_KEYSENSE_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_6, GPIO_KEYSENSE_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_7, GPIO_KEYSENSE_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPL */
	{ GPIO_KEYSCAN_0, GPIO_KEYSCAN_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, GPIO_KEYSCAN_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, GPIO_KEYSCAN_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, GPIO_KEYSCAN_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_4, GPIO_KEYSCAN_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_5, GPIO_KEYSCAN_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_6, GPIO_KEYSCAN_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_7, GPIO_KEYSCAN_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_EN, GPIO_TOUCH_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VIB_EN, GPIO_VIB_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TA_CONNECTED_N, GPIO_TA_CONNECTED_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PS_VOUT, GPIO_PS_VOUT_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BT_HOST_WAKE, GPIO_BT_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	/* GPM */
	{ GPIO_FM_I2C_SCL, GPIO_FM_I2C_SCL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CHG_ING_N, GPIO_CHG_ING_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#if (CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
	{ GPIO_OJ_MOTION, GPIO_OJ_MOTION_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, 
	{ GPIO_MSENSE_RST_N, GPIO_MSENSE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#endif
	/* GPN */
	{ GPIO_ONEDRAM_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	{ GPIO_MSENSE_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_ACC_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_HALL_SW, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_POWER_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TF_DETECT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ACTIVE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_JACK_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_DET_35, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_SEND_END, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_RESOUT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_BOOT_EINT13, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT14, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT15, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/** MEMORY PART **/
	/* GPP */
	{ S3C64XX_GPP(8), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(10), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(14), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
};

#elif defined(CONFIG_MACH_QUATTRO)
static int instinctq_gpio_table[][6] = {
	/** OFF PART **/
	/* GPA */
	{ GPIO_AP_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_CTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_RTS, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPB */
	{ GPIO_PDA_RXD, GPIO_PDA_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_PDA_TXD, GPIO_PDA_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TOUCH_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },	//quattro.tsp
	{ GPIO_TOUCH_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE }, //quattro.tsp
	//{ GPIO_I2C0_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE }, //quattro.tsp
	//{ GPIO_I2C0_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE }, //quattro.tsp
	/* GPC */
	{ GPIO_OJ_SPI_MISO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_MOSI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_BL_EN, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE }, //quattro.boot : GPIO_LEVEL_LOW > GPIO_LEVEL_HIGH
	//{ GPIO_OJ_CS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_SDIO_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_WLAN_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPD */
	{ GPIO_I2S_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_WLAN_REG_ON, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_I2S_LRCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPE */
	{ GPIO_BT_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BOOT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPF */
	{ GPIO_CAM_MCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_HSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_PCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MCAM_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_VSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_4, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_5, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_6, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_7, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MIC_SEL_EN_REV04, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_LUM_PWM, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_PWM1_TOUT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPG */
	{ GPIO_TF_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TF_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
 	{ GPIO_TF_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_SUB_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
	/* GPH */
	{ GPIO_MAIN_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },		
	{ GPIO_CAM_STANDBY, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_OJ_SHUTDOWN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_TFLASH_EN, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPI */
	{ GPIO_LCD_B_0, GPIO_LCD_B_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_1, GPIO_LCD_B_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_2, GPIO_LCD_B_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_3, GPIO_LCD_B_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_4, GPIO_LCD_B_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_5, GPIO_LCD_B_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_B_6, GPIO_LCD_B_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_B_7, GPIO_LCD_B_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_0, GPIO_LCD_G_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_1, GPIO_LCD_G_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_2, GPIO_LCD_G_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_3, GPIO_LCD_G_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_4, GPIO_LCD_G_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_5, GPIO_LCD_G_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_G_6, GPIO_LCD_G_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_G_7, GPIO_LCD_G_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPJ */
	{ GPIO_LCD_R_0, GPIO_LCD_R_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_1, GPIO_LCD_R_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_2, GPIO_LCD_R_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_3, GPIO_LCD_R_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_4, GPIO_LCD_R_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_5, GPIO_LCD_R_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_R_6, GPIO_LCD_R_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	//{ GPIO_LCD_R_7, GPIO_LCD_R_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_HSYNC, GPIO_LCD_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_VSYNC, GPIO_LCD_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_DE, GPIO_LCD_DE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_CLK, GPIO_LCD_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/** ALIVE PART **/
	/* GPK */
	{ GPIO_CHG_EN, GPIO_CHG_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
	{ GPIO_AUDIO_EN, GPIO_AUDIO_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_MIC_BIAS, GPIO_EAR_MIC_BIAS_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_MICBIAS_EN, GPIO_MICBIAS_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_UART_SEL, GPIO_UART_SEL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_RST, GPIO_TOUCH_RST_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, //quattro.tsp
	//{ GPIO_MONOHEAD_DET, GPIO_MONOHEAD_DET_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, //quattro.tsp
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_RST_N, GPIO_PHONE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_0, GPIO_KEYSENSE_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_1, GPIO_KEYSENSE_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_2, GPIO_KEYSENSE_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_3, GPIO_KEYSENSE_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_4, GPIO_KEYSENSE_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_5, GPIO_KEYSENSE_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_6, GPIO_KEYSENSE_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	//{ GPIO_KEYSENSE_7, GPIO_KEYSENSE_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPL */
	{ GPIO_KEYSCAN_0, GPIO_KEYSCAN_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, GPIO_KEYSCAN_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, GPIO_KEYSCAN_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, GPIO_KEYSCAN_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_4, GPIO_KEYSCAN_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_5, GPIO_KEYSCAN_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_6, GPIO_KEYSCAN_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_7, GPIO_KEYSCAN_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_EN, GPIO_TOUCH_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VIB_EN, GPIO_VIB_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TA_CONNECTED_N, GPIO_TA_CONNECTED_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PS_VOUT, GPIO_PS_VOUT_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BT_HOST_WAKE, GPIO_BT_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	/* GPM */
	{ GPIO_FM_I2C_SCL, GPIO_FM_I2C_SCL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CHG_ING_N, GPIO_CHG_ING_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#if (CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
	{ GPIO_OJ_MOTION, GPIO_OJ_MOTION_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, 
	{ GPIO_MSENSE_RST_N, GPIO_MSENSE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#endif
	/* GPN */
	{ GPIO_ONEDRAM_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	{ GPIO_MSENSE_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_ACC_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_HALL_SW, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_POWER_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TF_DETECT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ACTIVE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, //quattro.tsp
	//{ GPIO_TOUCH_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, //quattro.tsp
	{ GPIO_JACK_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_DET_35, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_SEND_END, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_RESOUT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_BOOT_EINT13, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT14, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT15, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/** MEMORY PART **/
	/* GPP */
	{ S3C64XX_GPP(8), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(10), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(14), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
};

#else //CONFIG_MACH_INSTINCTQ
static int instinctq_gpio_table[][6] = {
	/** OFF PART **/
	/* GPA */
	{ GPIO_AP_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_CTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_RTS, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPB */
	{ GPIO_PDA_RXD, GPIO_PDA_RXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_PDA_TXD, GPIO_PDA_TXD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C1_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2C0_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_I2C0_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPC */
	{ GPIO_OJ_SPI_MISO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_SPI_MOSI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_OJ_CS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },	
	{ GPIO_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_SDIO_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_WLAN_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_WAKE, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* GPD */
	{ GPIO_I2S_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_BT_WLAN_REG_ON, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_I2S_LRCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DI, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_I2S_DO, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPE */
	{ GPIO_BT_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BOOT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_PWR_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPF */
	{ GPIO_CAM_MCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_HSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_PCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MCAM_RST_N, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_VSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_4, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_5, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_6, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_CAM_D_7, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_MIC_SEL_EN_REV04, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_LUM_PWM, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_PWM1_TOUT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPG */
	{ GPIO_TF_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_TF_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TF_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
#if !defined(CONFIG_JET_OPTION)
	{ GPIO_SUB_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
	/* GPH */
	{ GPIO_MAIN_KEY_LED_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
#endif
	{ GPIO_CAM_FLASH_SET, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_FLASH_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_STANDBY, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_OJ_SHUTDOWN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_TFLASH_EN, 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* GPI */
	{ GPIO_LCD_B_0, GPIO_LCD_B_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_1, GPIO_LCD_B_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_2, GPIO_LCD_B_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_3, GPIO_LCD_B_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_4, GPIO_LCD_B_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_5, GPIO_LCD_B_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_6, GPIO_LCD_B_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_B_7, GPIO_LCD_B_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_0, GPIO_LCD_G_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_1, GPIO_LCD_G_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_2, GPIO_LCD_G_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_3, GPIO_LCD_G_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_4, GPIO_LCD_G_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_5, GPIO_LCD_G_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_6, GPIO_LCD_G_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_G_7, GPIO_LCD_G_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/* GPJ */
	{ GPIO_LCD_R_0, GPIO_LCD_R_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_1, GPIO_LCD_R_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_2, GPIO_LCD_R_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_3, GPIO_LCD_R_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_4, GPIO_LCD_R_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_5, GPIO_LCD_R_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_6, GPIO_LCD_R_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_R_7, GPIO_LCD_R_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_HSYNC, GPIO_LCD_HSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_VSYNC, GPIO_LCD_VSYNC_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_DE, GPIO_LCD_DE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_LCD_CLK, GPIO_LCD_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	/** ALIVE PART **/
	/* GPK */
	{ GPIO_CHG_EN, GPIO_CHG_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
	{ GPIO_AUDIO_EN, GPIO_AUDIO_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_MIC_BIAS, GPIO_EAR_MIC_BIAS_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_MICBIAS_EN, GPIO_MICBIAS_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	/* { GPIO_UART_SEL, GPIO_UART_SEL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, */
	{ GPIO_MONOHEAD_DET, GPIO_MONOHEAD_DET_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_RST_N, GPIO_PHONE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_0, GPIO_KEYSENSE_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_1, GPIO_KEYSENSE_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_2, GPIO_KEYSENSE_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_3, GPIO_KEYSENSE_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_4, GPIO_KEYSENSE_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_5, GPIO_KEYSENSE_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSENSE_6, GPIO_KEYSENSE_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VMSMP_26V, GPIO_VMSMP_26V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPL */
	{ GPIO_KEYSCAN_0, GPIO_KEYSCAN_0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, GPIO_KEYSCAN_1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, GPIO_KEYSCAN_2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, GPIO_KEYSCAN_3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_4, GPIO_KEYSCAN_4_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_5, GPIO_KEYSCAN_5_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_6, GPIO_KEYSCAN_6_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_7, GPIO_KEYSCAN_7_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
#if !defined(CONFIG_JET_OPTION)
	{ GPIO_TOUCH_EN, GPIO_TOUCH_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
#endif
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VIB_EN, GPIO_VIB_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TA_CONNECTED_N, GPIO_TA_CONNECTED_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PS_VOUT, GPIO_PS_VOUT_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BT_HOST_WAKE, GPIO_BT_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	/* GPM */
	{ GPIO_FM_I2C_SCL, GPIO_FM_I2C_SCL_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_FM_I2C_SDA, GPIO_FM_I2C_SDA_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_CHG_ING_N, GPIO_CHG_ING_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#if (CONFIG_INSTINCTQ_REV >= CONFIG_INSTINCTQ_REV00)
	{ GPIO_OJ_MOTION, GPIO_OJ_MOTION_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 }, 
	{ GPIO_MSENSE_RST_N, GPIO_MSENSE_RST_N_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, 0, 0 }, 
#endif
	/* GPN */
	{ GPIO_ONEDRAM_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, 0, 0 },
	{ GPIO_MSENSE_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_ACC_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_HALL_SW, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_POWER_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TF_DETECT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ACTIVE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_JACK_INT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_DET_35, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_EAR_SEND_END, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_RESOUT_N, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_UP, 0, 0 },
	{ GPIO_BOOT_EINT13, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT14, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_BOOT_EINT15, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/** MEMORY PART **/
	/* GPP */
	{ S3C64XX_GPP(8), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(10), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ S3C64XX_GPP(14), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
};
#endif

void s3c_config_gpio_table(int array_size, int (*gpio_table)[6])
{
	u32 i, gpio;

	pr_debug("%s: ++\n", __func__);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		if (gpio < S3C64XX_GPIO_ALIVE_PART_BASE) { /* Off Part */
			pr_debug("%s: Off gpio=%d,%d\n", __func__, gpio, 
					S3C64XX_GPIO_ALIVE_PART_BASE);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else if (gpio < S3C64XX_GPIO_MEM_PART_BASE) { /* Alive Part */
			pr_debug("%s: Alive gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else { /* Memory Part */
			pr_debug("%s: Memory gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		}
	}
	pr_debug("%s: --\n", __func__);
}
EXPORT_SYMBOL(s3c_config_gpio_table);

void instinctq_init_gpio(void)
{
	s3c_config_gpio_table(ARRAY_SIZE(instinctq_gpio_table),
			instinctq_gpio_table);
}

#if defined(CONFIG_MACH_VINSQ) || defined(CONFIG_MACH_QUATTRO)
static int instinctq_sleep_gpio_table[][6] = {
	/** ALIVE PART **/
	/* GPK */
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPL */
	{ GPIO_KEYSCAN_0, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_4, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_5, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_6, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_7, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_TOUCH_EN, GPIO_TOUCH_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPM */
	{ GPIO_FM_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_FM_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 }, 
	/* GPN */
	{ GPIO_HALL_SW, GPIO_HALL_SW_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/** MEMORY PART **/
	/* GPO */
	{ GPIO_LCD_RST_N, GPIO_LCD_RST_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_CS_N, GPIO_LCD_CS_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_SDI, GPIO_LCD_SDI_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_SCLK, GPIO_LCD_SCLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_ID, GPIO_LCD_ID_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
};

#else

static int instinctq_sleep_gpio_table[][6] = {
	/** ALIVE PART **/
	/* GPK */
	{ GPIO_CAM_EN, GPIO_CAM_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_VMSMP_26V, GPIO_VMSMP_26V_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPL */
	{ GPIO_KEYSCAN_0, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_1, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_2, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_3, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_4, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_5, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_6, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_KEYSCAN_7, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
#if !defined(CONFIG_JET_OPTION)
	{ GPIO_TOUCH_EN, GPIO_TOUCH_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
#endif
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 },
	/* GPM */
	{ GPIO_FM_I2C_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_FM_I2C_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	{ GPIO_PDA_ACTIVE, GPIO_PDA_ACTIVE_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, 0, 0 }, 
	/* GPN */
	{ GPIO_HALL_SW, GPIO_HALL_SW_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, 0, 0 },
	/** MEMORY PART **/
	/* GPO */
	{ GPIO_LCD_RST_N, GPIO_LCD_RST_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_CS_N, GPIO_LCD_CS_N_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_SDI, GPIO_LCD_SDI_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_SCLK, GPIO_LCD_SCLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_ID, GPIO_LCD_ID_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
};
#endif

static void check_pmic(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO3, &reg_buff)) {
		pr_info("%s: OTGI 1.2V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO4, &reg_buff)) {
		pr_info("%s: BT_WL 2.6V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: MMC 3.0V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO6, &reg_buff)) {
		pr_info("%s: VLCD 1.8V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO7, &reg_buff)) {
		pr_info("%s: VLCD 2.8V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO8, &reg_buff)) {
		pr_info("%s: OTG 3.3V (%d)\n", __func__, reg_buff);
	}
}

void s3c_config_sleep_gpio(void)
{	
	int spcon_val;

	check_pmic();
	s3c_config_gpio_table(ARRAY_SIZE(instinctq_sleep_gpio_table),
			instinctq_sleep_gpio_table);

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
}
EXPORT_SYMBOL(s3c_config_wakeup_gpio);

extern unsigned char ftm_sleep;
void s3c_config_wakeup_source(void)
{
	unsigned int eint0pend_val;

	/* Power key (GPN5) */
	s3c_gpio_cfgpin(S3C64XX_GPN(5), S3C64XX_GPN5_EINT5);
	s3c_gpio_setpull(S3C64XX_GPN(5), S3C_GPIO_PULL_NONE);
	__raw_writel((__raw_readl(S3C64XX_EINT0CON0) & ~(0x7 << 8)) |
			(S3C64XX_EXTINT_BOTHEDGE << 8), S3C64XX_EINT0CON0);

	/* Wake-up source 
	 * ONEDRAM_INT(EINT0), WLAN_HOST_WAKE(EINT1) HALL_SW(EINT4), Power key(EINT5),
	 * DET_35(EINT10), EAR_SEND_END(EINT11),
	 * TA_CONNECTED(EINT19),PROXIMITY_SENSOR(EINT20),
	 * BT_HOST_WAKE(EINT22), CHG_ING(EINT25)
	 */

	//SEC_BP_WONSUK_20090811
	//register INTB(EINT9) with wakeup source 
	eint0pend_val = __raw_readl(S3C64XX_EINT0PEND);
	eint0pend_val |= (0x1 << 25) | (0x1 << 22) |(0x1 << 20) |  (0x1 << 19) | 
		(0x1 << 11) | (0x1 << 10) | (0x1 << 9) | (0x1 << 5) | (0x1 << 4) | (0x1 << 1) |  0x1;
	__raw_writel(eint0pend_val, S3C64XX_EINT0PEND);

	eint0pend_val = (0x1 << 25) | (0x1 << 22) | (0x1 << 20) | (0x1 << 19) |
		(0x1 << 11) | (0x1 << 10) | (0x1 << 9) | (0x1 << 5) | (0x1 << 4) | (0x1 << 1) | 0x1;
	__raw_writel(~eint0pend_val, S3C64XX_EINT0MASK);

	__raw_writel((0x0FFFFFFF & ~eint0pend_val), S3C_EINT_MASK);

	/* Alarm Wakeup Enable */
	if (!ftm_sleep)
		__raw_writel((__raw_readl(S3C_PWR_CFG) & ~(0x1 << 10)), S3C_PWR_CFG);
	else {
		pr_info("%s: RTC alarm is disabled\n", __func__);
		__raw_writel((__raw_readl(S3C_PWR_CFG) | (0x1 << 10)), S3C_PWR_CFG);
	}
}
EXPORT_SYMBOL(s3c_config_wakeup_source);
