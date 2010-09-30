/****************************************************************
 * $ID: hhtech_gpio.h     三, 18  2月 2009 10:16:56 +0800  wk $ *
 *                                                              *
 * Description:                                                 *
 *                                                              *
 * Maintainer:  wk@hhcn.com            *
 *                                                              *
 * CopyRight (c)  2009  HHTech                                  *
 *   www.hhcn.com, www.hhcn.org                                 *
 *   All rights reserved.                                       *
 *                                                              *
 * This file is free software;                                  *
 *   you are free to modify and/or redistribute it   	        *
 *   under the terms of the GNU General Public Licence (GPL).   *
 *                                                              *
 * Last modified: Fri, 24 Apr 2009 09:25:41 +0800       by root #
 ****************************************************************/


/*====================================================================== 
 * GPIO
 */

//SD
#define GPIO_SD_WP				S3C64XX_GPK(0)     /* GPK0 ,SD write protect detect,*/

//USB
#define GPIO_USB_EN			    	S3C64XX_GPL(0)     /* GPL0 USB Improving voltage Enable, 1:open 0:off */
#define GPIO_USB_HOSTPWR_EN	S3C64XX_GPL(1)     /* GPL1 1:5V on 0:5Voff */
#define GPIO_USB_HOST_STATUS	S3C64XX_GPL(10)    /* GPL10 USB protect status ,0:error*/
#define GPIO_USB_OTG_STATUS	S3C64XX_GPL(11)    /* GPL11 USB otg Over-current protection status ,0:error*/
#define GPIO_USB_OTGDRV_EN		S3C64XX_GPL(8)     /* GPL0 USB otg drv Enable, 1:open 0:off */

//Headphone Sperker
#define GPIO_HEADPHONE_S		S3C64XX_GPL(12)    /* GPL12  headphone audio detect,0:insert */
#define GPIO_SPEAKER_EN			S3C64XX_GPK(12)    /* GPK12 Speaker 0:off 1:open */

//Backlight
#define GPIO_LCD_BLIGHT_EN		S3C64XX_GPM(3)     /* GPM3,MP1530 LCD backlight,1:enable 0:off */
#define GPIO_LCD_BLIGHT_S		S3C64XX_GPM(4)     /* GPM4,MP1530 status */

//Charging
#define GPIO_DC_DETE			S3C64XX_GPL(13)    /* GPL13 DC insert Detect */
#define GPIO_CHARG_S1			S3C64XX_GPK(4)     /* GPK4 ,Charging status 1,*/
#define GPIO_CHARG_S2			S3C64XX_GPK(5)     /* GPK5 ,Charging status 2,*/
#define GPIO_CHARGER_EN			S3C64XX_GPK(6)     /* GPK6 DC 0:200ma 1:860ma */

// System Power 
#define GPIO_PWR_EN				S3C64XX_GPK(15)    /* GPK15 System power control 0:off 1:open */
#define GPIO_PWR_HOLD			S3C64XX_GPL(14)    /* GPL14 System power hold over 5 second  time,pull up GPK15  */

//Vidoe amplifier
#define GPIO_VIDEOAMP_EN		S3C64XX_GPK(13)    /* GPK13,Vidoe amplifier output control,0:off*/

#define GPIO_WIFI_EN				S3C64XX_GPK(1)    /* Wifi  switch*/
#define GPIO_WIFI_RESET			S3C64XX_GPK(2)    /* Wifi  reset*/

#if defined(CONFIG_LCD_4)
#define GPIO_LED1_EN				S3C64XX_GPN(8)
#define GPIO_LED2_EN				S3C64XX_GPN(9)
#else
#define GPIO_LED1_EN				S3C64XX_GPN(9)
#define GPIO_LED2_EN				S3C64XX_GPN(8)
#endif
