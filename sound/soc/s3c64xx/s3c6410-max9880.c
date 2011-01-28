/*
 * Samsung S8000 Jet Audio Subsystem
 *
 * (c) 2010 Vaclav Peroutka - vaclavpe@gmail.com
 *
 * Licensed under the GPLv2.
 *
 *  Revision history
 *    2010/11/11   Initial version
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <asm/hardware/scoop.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/audio.h>
#include <mach/map.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-iic.h>
#include <plat/regs-clock.h>
#include <plat/regs-iis.h>

#include "s3c-pcm.h"
#include "s3c6410-i2s.h"
#include "../codecs/max9880.h"

extern int is_pmic_initialized(void);
extern unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length);
extern unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length);
#define MAX8906_RTC_ID (0xD0)
#define MAX8906_ADC_ID (0x8E)
#define MAX8906_GPM_ID (0x78)

#define SUBJECT "Jet_max9880"

static struct max9880_setup_data max9880_setup = {
   .i2c_address = (0x20 >> 1),
};

static struct snd_soc_dai_link s3c6410_max9880_dai = {
   .name = "MAX9880",
   .stream_name = "MAX9880 Codec",
   .cpu_dai = &s3c_i2s_dai,
   .codec_dai = &max9880_dai,
   .init = NULL,
   .ops = NULL,
};

static struct snd_soc_card s3c6410_max9880_soc_card  = {
   .name = "S3C6410 MAX9880",
   .platform = &s3c24xx_soc_platform,
   .dai_link = &s3c6410_max9880_dai,
   .num_links = 1,
};

static struct snd_soc_device s3c6410_max9880_snd_devdata = {
   .card = &s3c6410_max9880_soc_card,
   .codec_dev = &soc_codec_dev_max9880,
   .codec_data = &max9880_setup,
};

static struct platform_device *s3c6410_max9880_snd_device;

static int __init s3c6410_max9880_init(void)
{
   int ret;

   printk("[%s]", __FUNCTION__);

   s3c6410_max9880_snd_device = platform_device_alloc("soc-audio", -1);
   if (!s3c6410_max9880_snd_device)
   {   
      printk(" - soc-audio create fail \n");
      return -ENOMEM;
   }

   printk("[%s] platform_set_drvdata \n",__FUNCTION__);
   platform_set_drvdata( s3c6410_max9880_snd_device, &s3c6410_max9880_snd_devdata);

   s3c6410_max9880_snd_devdata.dev = &s3c6410_max9880_snd_device->dev;

   ret = platform_device_add( s3c6410_max9880_snd_device);
   if (ret) {
      platform_device_put( s3c6410_max9880_snd_device);
      printk("[s3c6410_max9880] platform_device_add fail \n");
   } else {
     unsigned char tscbuff;

     pmic_read( MAX8906_ADC_ID, 0x60, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x60=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x61, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x61=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x62, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x62=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x63, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x63=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x64, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x64=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x65, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x65=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x66, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x66=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x67, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x67=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x68, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x68=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x69, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x69=0x%02X\n",tscbuff);
     pmic_read( MAX8906_ADC_ID, 0x6A, &tscbuff, 1); printk("[MAX8906 dbg]: ADC 0x6a=0x%02X\n",tscbuff);
     /* audio subsystem*/
     pmic_read( MAX8906_GPM_ID, 0x84, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x84=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x85, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x85=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x86, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x86=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x87, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x87=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x88, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x88=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x89, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x89=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x8a, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x8a=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x8b, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x8B=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x8c, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x8C=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x8d, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x8D=0x%02X\n",tscbuff);
     pmic_read( MAX8906_GPM_ID, 0x8e, &tscbuff, 1); printk("[MAX8906 dbg]: AUD 0x8E=0x%02X\n",tscbuff);
   }

   return ret;
}

static void __exit s3c6410_max9880_exit(void)
{
   platform_device_unregister(s3c6410_max9880_snd_device);
}

module_init(s3c6410_max9880_init);
module_exit(s3c6410_max9880_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung S8000 Jet Audio Subsystem");
MODULE_AUTHOR("Vaclav Peroutka <vaclavpe@gmail.com>");
