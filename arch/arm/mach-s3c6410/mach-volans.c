/* linux/arch/arm/mach-s3c6410/mach-volans.c
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
#include <linux/i2c/pmic.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <linux/proc_fs.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/fb.h>

#include <plat/regs-rtc.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/spi.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-f.h>
#include <mach/volans_gpio_table.h>

#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-otg.h>
#include <linux/usb/ch9.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	1

#ifdef USB_HOST_PORT2_EN
#define OTGH_PHY_CLK_VALUE      (0x60)  /* Serial Interface, otg_phy input clk 48Mhz Oscillator */
#else
#define OTGH_PHY_CLK_VALUE      (0x20)  /* UTMI Interface, otg_phy input clk 48Mhz Oscillator */
#endif
#endif

#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

extern struct sys_timer s3c_timer;
extern void s3c64xx_reserve_bootmem(void);


int usbsel = 1;	
EXPORT_SYMBOL(usbsel);

int uartsel = 1;	
EXPORT_SYMBOL(uartsel);

static void __init early_uart_usb_path(char **p)
{
	unsigned int size = strlen(*p);
	unsigned long long ret = simple_strtoull (*p, (p+size), 0);

    uartsel = (ret &0x2)>>1;
	usbsel = ret & 0x1;

	printk(KERN_ERR"[%s] usbsel=%d, uartsel=%d\n", __FUNCTION__, usbsel, uartsel);
	return;
}
__early_param("switch_sel=", early_uart_usb_path);

unsigned int boot_mode = 0; 
EXPORT_SYMBOL(boot_mode); 
static void __init early_get_boot_mode (char **p)
{
    unsigned int size = strlen(*p);
    unsigned long long ret = simple_strtoull(*p, (p+size),0);
    boot_mode = (unsigned int)ret; 

    printk(KERN_ERR"boot_mode:%d\n",boot_mode);
    return; 
}
__early_param("boot_mode=",early_get_boot_mode);
unsigned int factory_cable = 0; 
EXPORT_SYMBOL(factory_cable); 
static void __init early_get_factory_cable (char **p)
{
    unsigned int size = strlen(*p);
    unsigned long long ret = simple_strtoull(*p, (p+size),0);
    factory_cable = (unsigned int)ret; 

    printk(KERN_ERR"factory_cable:%d\n",factory_cable);
    return; 
}
__early_param("factory_cable=",early_get_factory_cable);

static struct s3c2410_uartcfg volans_uartcfgs[] __initdata = {
	[0] = {		/* MODEM */
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[1] = {		/* BT */
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[2] = {		/* AP UART */
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
};

#ifdef CONFIG_I2C_GPIO
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
	.id		= 3,
	.dev.platform_data	= &i2c_common_platdata,
};

static struct i2c_gpio_platform_data i2c_touch_platdata = {
	.sda_pin	= GPIO_TOUCH_SDA,
	.scl_pin	= GPIO_TOUCH_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0
};

static struct platform_device sec_device_i2c_touch = {
	.name	= "i2c-gpio",
	.id		= 4,
	.dev.platform_data	= &i2c_touch_platdata,
};
#endif	/* CONFIG_I2C_GPIO */

static struct platform_device sec_device_dpram = {
	.name	= "dpram",
	.id		= -1,
};

static struct platform_device sec_device_battery = {
	.name	= "volans-battery",
	.id		= -1,
};

static struct platform_device sec_device_usb_ic = {
    .name   = "usb-ic",
    .id     = -1, 
};

struct map_desc volans_iodesc[] = {
	{ VOLANS_VA_DPRAM, __phys_to_pfn(VOLANS_PA_DPRAM), SZ_16M, MT_DEVICE },
};

static struct platform_device *volans_devices[] __initdata = {
#ifdef CONFIG_FB_S3C_V2
	&s3c_device_fb,
#endif
#ifdef CONFIG_SMDK6410_SD_CH0
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_SMDK6410_SD_CH1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_SMDK6410_SD_CH2
	&s3c_device_hsmmc2,
#endif
	&s3c_device_wdt,
	&s3c_device_rtc,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
#if defined(CONFIG_SPI_CNTRLR_0)
//	&s3c_device_spi0,
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
//	&s3c_device_spi1,
#endif
	&sec_device_keypad,
	&sec_device_ts,
//	&s3c_device_smc911x,
	&s3c_device_lcd,
    &s3c_device_vpp,
	&s3c_device_mfc,
//  &s3c_device_tvenc,
//	&s3c_device_tvscaler,
	&s3c_device_rotator,
	&s3c_device_jpeg,
//	&s3c_device_nand,
//	&s3c_device_onenand,
//	&s3c_device_usb,
	&s3c_device_usbgadget,
//	&s3c_device_usb_otghcd,
	&s3c_device_fimc0,
//	&s3c_device_fimc1,
	&s3c_device_g2d,
	&s3c_device_g3d,
	&s3c_device_rp,

#ifdef CONFIG_S3C64XX_ADC
	&s3c_device_adc,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif

#ifdef CONFIG_I2C_GPIO
	&sec_device_i2c_pmic,			/* pmic(max8698) i2c. */
	&sec_device_i2c_common,			/* radio, sound, .. i2c. */
	&sec_device_i2c_touch,			/* touch i2c. */
#endif	/* CONFIG_I2C_GPIO */

	&sec_device_dpram,				/* dpram */
	&sec_device_battery,			/* charger & battery */
	&sec_device_usb_ic,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("KXSD9", 0x18), },		/* accelerator */
	{ I2C_BOARD_INFO("USBIC", 0x25), },		/* uUSB ic */
	{ I2C_BOARD_INFO("max17040", 0x36), },	/* max17040 fuel gauge */
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	/* Samsung S524AD0XD1 */
};

static struct i2c_board_info i2c_devs2[] __initdata = {
	{ I2C_BOARD_INFO("max8698", 0x66), },  	/* Max8698 PMIC */
};

static struct i2c_board_info i2c_devs3[] __initdata = {
	{ I2C_BOARD_INFO("Si4709", 0x10), },  
};

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con	= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	.delay	= 	10000,
	.presc 	= 	49,
	.resolution = 	10,
};

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 78770,
};

static struct platform_device smdk_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
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

static void __init volans_fixup(struct machine_desc *desc,
        struct tag *tags, char **cmdline, struct meminfo *mi)
{
	 unsigned long unreserved_size = 0;

 	 mi->nr_banks = 1;
			 
	 mi->bank[0].start = PHYS_OFFSET;
	 mi->bank[0].size = PHYS_SIZE - unreserved_size;
	 mi->bank[0].node = 0;
}

static void __init volans_map_io(void)
{
	s3c64xx_init_io(volans_iodesc, ARRAY_SIZE(volans_iodesc));
	s3c24xx_init_clocks(XTAL_FREQ);
	s3c24xx_init_uarts(volans_uartcfgs, ARRAY_SIZE(volans_uartcfgs));
	s3c64xx_reserve_bootmem();
}

static void __init volans_smc911x_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S3C64XX_SROM_BW);
	tmp &= ~(S3C64XX_SROM_BW_WAIT_ENABLE1_MASK | S3C64XX_SROM_BW_WAIT_ENABLE1_MASK |
		S3C64XX_SROM_BW_DATA_WIDTH1_MASK);
	tmp |= S3C64XX_SROM_BW_BYTE_ENABLE1_ENABLE | S3C64XX_SROM_BW_WAIT_ENABLE1_ENABLE |
		S3C64XX_SROM_BW_DATA_WIDTH1_16BIT;

	__raw_writel(tmp, S3C64XX_SROM_BW);

	__raw_writel(S3C64XX_SROM_BCn_TACS(0) | S3C64XX_SROM_BCn_TCOS(4) |
			S3C64XX_SROM_BCn_TACC(13) | S3C64XX_SROM_BCn_TCOH(1) |
			S3C64XX_SROM_BCn_TCAH(4) | S3C64XX_SROM_BCn_TACP(6) |
			S3C64XX_SROM_BCn_PMC_NORMAL, S3C64XX_SROM_BC1);
}

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

static void volans_set_drive_strength(void)
{
	unsigned int value;
	value = __raw_readl(S3C64XX_SPC_BASE);

	value &= ~(0x3 << 24);

	__raw_writel(value, S3C64XX_SPC_BASE);
}

static void volans_set_eint_filter(void)
{
	unsigned int value;

	value = __raw_readl(S3C64XX_EINT0FLTCON2);
	value |= (0x1 << 15);	/* FLTEN enable. */
	value &= ~(0x1 << 14);	/* FLTSEL = delay filter. */
	__raw_writel(value, S3C64XX_EINT0FLTCON2);

	value = __raw_readl(S3C64XX_EINT0FLTCON3);
	value |= (0x1 << 7);	/* FLTEN enable. */
	value &= ~(0x1 << 6);	/* FLTSEL = delay filter. */
	__raw_writel(value, S3C64XX_EINT0FLTCON3);
}

static int alarm_boot_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
    char *p = page; 
    int len = 0; 
    len = sprintf(p,"%d",boot_mode); 
    return len; 
}

static int factory_cable_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
    char *p = page; 
    int len = 0; 
    len = sprintf(p,"%d",factory_cable); 
    return len; 
}

static void smdk6410_set_qos(void)
{
    u32 reg;

    reg = (u32) ioremap((unsigned long) 0x7e003000, SZ_4K);

    writel(0x2, S3C_VA_SYS + 0x128);

    writel(0x7, reg + 0x460);   /* (8 - MFC ch.) */
    writel(0x7ff7, reg + 0x464);

    writel(0x8ff, S3C_VA_SYS + 0x838);
}

static void __init volans_machine_init(void)
{
	volans_init_gpio();

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

	s3c_adc_set_platdata(&s3c_adc_platform);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
	i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

#if defined(CONFIG_SPI_CNTRLR_0)
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
#endif

	s3c_fimc0_set_platdata(NULL);
	s3c_fimc1_set_platdata(NULL);

#ifdef CONFIG_FB_S3C_V2
	s3cfb_set_platdata(NULL);
#endif

#ifdef CONFIG_VIDEO_FIMC
#endif

	platform_add_devices(volans_devices, ARRAY_SIZE(volans_devices));
	s3c6410_pm_init();

	smdk_backlight_register();
	create_proc_read_entry("alarm_boot",0,0,alarm_boot_read_proc,NULL);
    create_proc_read_entry("factory_cable",0,0,factory_cable_read_proc,NULL);

	smdk6410_set_qos();

	volans_set_drive_strength();
	volans_set_eint_filter();

	__raw_writel(0xC3F63E7B, S3C_HCLK_GATE);
	__raw_writel(0xFA1F98EF, S3C_PCLK_GATE);
}

MACHINE_START(VOLANS, "VOLANS")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,

	.init_irq	= s3c6410_init_irq,
	.fixup		= volans_fixup,
	.map_io		= volans_map_io,
	.init_machine	= volans_machine_init,
	.timer		= &s3c64xx_timer,
MACHINE_END

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {

	writel(readl(S3C_OTHERS)|S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
	writel(0x0, S3C_USBOTG_PHYPWR);		/* Power up */
        writel(OTGH_PHY_CLK_VALUE, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);

	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(readl(S3C_OTHERS)&~S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_clk_en(void) {
	struct clk *otg_clk;

	switch (S3C_USB_CLKSRC) {
		case 0: /* epll clk */
			writel((readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK)
					|S3C_CLKSRC_EPLL_CLKSEL|S3C_CLKSRC_UHOST_EPLL,
					S3C_CLK_SRC);

			/* USB host colock divider ratio is 2 */
			writel((readl(S3C_CLK_DIV1)& ~S3C6400_CLKDIV1_UHOST_MASK)
					|(1<<20), S3C_CLK_DIV1);
			break;
		case 1: /* oscillator 48M clk */
			otg_clk = clk_get(NULL, "otg");
			clk_enable(otg_clk);
			writel(readl(S3C_CLK_SRC)& ~S3C6400_CLKSRC_UHOST_MASK, S3C_CLK_SRC);
			otg_phy_init();

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
#endif

#if defined(CONFIG_RTC_DRV_S3C)
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
	tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

	if (to)
		tmp |= S3C_RTCCON_TICEN;

	writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN);
	writew(tmp, base + S3C2410_RTCCON);

	s3c_freq = freq;

	tmp = (32768 - 1) * 3600;

	writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev, void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
		writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
	} else {
		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0) {
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp | S3C2410_RTCCON_RTCEN, base + S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)) {
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp& ~S3C2410_RTCCON_CNTSEL, base + S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)) {
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST, base + S3C2410_RTCCON);
		}
	}
}
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE) || defined(CONFIG_MACH_VOLANS)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = S3C64XX_GPK(8 + rows);

	for (gpio = S3C64XX_GPK(8); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S3C64XX_GPL(0 + columns);

	for (gpio = S3C64XX_GPL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);
#endif

#ifdef CONFIG_MMC_SDHCI_S3C
void s3c_setup_hsmmc_clock(void)
{
	struct clk *clk;

	clk = clk_get(NULL, "mmc_bus");
}
EXPORT_SYMBOL(s3c_setup_hsmmc_clock);
#endif

void s3c_setup_bluetooth_cfg(int on)
{
	if (on) {
		if (!gpio_get_value(GPIO_BT_EN)) {
			s3c_gpio_cfgpin(GPIO_HOST_WAKE, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(GPIO_HOST_WAKE, S3C_GPIO_PULL_UP);

			s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT1);
			s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);
			s3c_gpio_slp_cfgpin(GPIO_AP_BT_RTS, S3C_GPIO_SLP_OUT1);

			pmic_power_control(POWER_BT, PMIC_POWER_ON);
			mdelay(10);

			gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_HIGH);
			gpio_set_value(GPIO_BT_WAKE,GPIO_LEVEL_LOW);
			gpio_set_value(GPIO_BT_nRST,GPIO_LEVEL_LOW);
			msleep(200);
			gpio_set_value(GPIO_BT_nRST,GPIO_LEVEL_HIGH);
		}
	}

	else {
		s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);

		s3c_gpio_slp_cfgpin(GPIO_AP_BT_RTS, S3C_GPIO_SLP_INPUT);
		s3c_gpio_slp_setpull_updown(GPIO_AP_BT_RTS, S3C_GPIO_PULL_DOWN);

		gpio_set_value(GPIO_BT_nRST, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW);

		s3c_gpio_cfgpin(GPIO_HOST_WAKE, S3C_GPIO_INPUT);
		s3c_gpio_setpull(GPIO_HOST_WAKE, S3C_GPIO_PULL_DOWN);

		pmic_power_control(POWER_BT,PMIC_POWER_OFF);
	}
}
EXPORT_SYMBOL(s3c_setup_bluetooth_cfg);

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



