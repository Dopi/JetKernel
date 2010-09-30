/*
 * include/sec_common/ioctl.h
 *
 * ioctl's defintion.
 *
 */

#ifndef _INCLUDE_SEC_IOCTL_H_
#define _INCLUDE_SEC_IOCTL_H_
#ifndef __ASSEMBLY__

/****************************************************************************
** structures
*****************************************************************************/
typedef struct {	/* led */
	unsigned int index;		/* LED index to control */
	unsigned int stat;		/* control command or current status */
//	unsigned int rate;		/* blinking rate */
	unsigned int color;		/* LED color */
	unsigned int info;		/* capable function */
} LED_RET;

typedef struct {	/* accelerometer */
	int accel_x; /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
	int accel_y; /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
	int accel_z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
} ACCELODATA;

typedef struct {
	int	f_disable;
	int	level;
} VIBRATOR_DATA;

/****************************************************************************
** definitions
*****************************************************************************/
#define MAX_LED_NUM		3
#define	LED_OFF			0x00
#define LED_ON			0x01

#define	LED_KEYPAD_NUM		0x01
#define	LED_KEYPAD_ETC		0x02
#define LED_KEYPAD_OEM1		0x03
#define	LED_KEYPAD_OEM2		0x04
#define	LED_KEYPAD_OEM3		0x05
#define LED_KEYPAD_OEM4		0x06
#define	LED_KEYPAD_OEM5		0x07

#define	LED_HEARTBEAT		0x10

#define	LED_OEM1		0x20
#define	LED_OEM2		0x21
#define	LED_OEM3		0x22
#define	LED_OEM4		0x23
#define	LED_OEM5		0x24

#define VIBRATOR_ENABLE		1
#define VIBRATOR_DISABLE	0


/****************************************************************************
** ioctl
*****************************************************************************/
/*
 * see Documentation/ioctl-number.txt
   F0	
   
   USAGE MAP
	81,82				touchScreen
	83,84				backlight
	85				battery
	86				accesary scanning
	87				battery type scanning
	88				sleep
	8A				HID device wakeup
	8B,8C,8D			LED
	8E				backlight (info)
	8F				battery (raw)
	90,91				touchScreen (on/off)
	92,93				PPP domant
	94,95				mz_mmap
	96				battery control
	B0~B5				UART, BT, WIFI
	C0~CA				DPRAM
	CB~CE				don't USE!!!
	D0~D7				sensor ( acceleration, proximity, light and etc )
	E0~E2				keypad
 */
#define IOC_SEC_MAGIC		(0xF0)

typedef struct {
	int level;	//, voltage, raw;	// -1,0,10,20,30,40,50,60,70,80,90,100
	unsigned char ac;			// on | off | unknown
	unsigned char battery;			// low | mid | full
} BATTERY_RET;
#define IOCTL_GET_BATTERY_STATUS	_IOR(IOC_SEC_MAGIC, 0x85, BATTERY_RET)

/*
 * for apm_bios
 */
/* sleep simply */
#define SEC_SYS_SUSPEND		_IO (IOC_SEC_MAGIC, 0x88)
#define SEC_SUSPEND		SEC_SYS_SUSPEND
/* wakeup HID devices */
#define SEC_RESUME		_IO (IOC_SEC_MAGIC, 0x8A)

/*
 * for LEDs
 */
#define IOCTL_GET_LED_NO		_IOR(IOC_SEC_MAGIC, 0x8B, unsigned int)
#define IOCTL_GET_LED_STATUS		_IOR(IOC_SEC_MAGIC, 0x8C, LED_RET)
#define IOCTL_SET_LED_STATUS		_IOW(IOC_SEC_MAGIC, 0x8D, LED_RET)

/*
 * for Uart, BT, WiFi
 */
#define	IOCTL_UART_PDA			_IO(IOC_SEC_MAGIC, 0xB0)
#define	IOCTL_UART_PHONE		_IO(IOC_SEC_MAGIC, 0xB1)

/*
#define	IOCTL_BTWIFI_ENABLE			_IO(IOC_SEC_MAGIC, 0xB2)
#define	IOCTL_BTWIFI_DISABLE		_IO(IOC_SEC_MAGIC, 0xB3)
*/
#define	IOCTL_BT_ENABLE			_IO(IOC_SEC_MAGIC, 0xB2)
#define	IOCTL_BT_DISABLE		_IO(IOC_SEC_MAGIC, 0xB3)
#define	IOCTL_WIFI_ENABLE		_IO(IOC_SEC_MAGIC, 0xB4)
#define	IOCTL_WIFI_DISABLE		_IO(IOC_SEC_MAGIC, 0xB5)
// minhyo081024 
#define	IOCTL_USB_PDA			_IO(IOC_SEC_MAGIC, 0xB6)
#define	IOCTL_USB_PHONE			_IO(IOC_SEC_MAGIC, 0xB7)
#define	IOCTL_USB_WIBRO			_IO(IOC_SEC_MAGIC, 0xB8)



/*
 * for sensor
 */
#define	IOCTL_SMB_SET_ACC		_IOW(IOC_SEC_MAGIC, 0xD0, unsigned int)
#define	IOCTL_SMB_GET_ACC_VALUE		_IOR(IOC_SEC_MAGIC, 0xD1, ACCELODATA)

/*	minhyo081205 
 *	for vibtonz
 */
#define	IOCTL_SET_VIBTONZ		_IOR(IOC_SEC_MAGIC, 0xD2, VIBRATOR_DATA)


/*
 * for keypad
 */
#define	IOCTL_KEYLOCK_STATUS		_IOR(IOC_SEC_MAGIC, 0xE0, unsigned int)
#define	IOCTL_SLIDE_STATUS		_IOR(IOC_SEC_MAGIC, 0xE1, unsigned int)
#define	IOCTL_FOLDER_STATUS		_IOR(IOC_SEC_MAGIC, 0xE2, unsigned int)







/********************************************************************************/



#endif	/* __ASSEMBLY__ */
#endif /* _INCLUDE_SEC_IOCTL_H_ */

/*
 |
 | -*- End-Of-File -*-
 */
