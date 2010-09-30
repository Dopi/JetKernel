/****************************************************************************
**
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
** AUTHOR		: KyoungHOON Kim (khoonk)
**
*****************************************************************************
**
*****************************************************************************/
#ifndef _MESSAGE_
#define _MESSAGE_

/****************************************************************************
** structures
*****************************************************************************/
typedef struct {
	unsigned short	s_vid;			/* source task virtual id */
	unsigned short	d_vid;			/* destination task virtual id*/
	unsigned int	msgid;			/* message id */
	unsigned int	size;			/* size of message */
    struct timeval 	time;
}__attribute__ ((packed)) SYS_MSG_HDR_t;

typedef	struct {
	SYS_MSG_HDR_t	hdr;
	unsigned int	para1;
	unsigned int	para2;
	unsigned int	para3;
	unsigned int	para4;
}__attribute__ ((packed)) t_callMsg;

typedef struct
{
	unsigned char	state;
	unsigned char	prev_state;
	unsigned int	high_count;
	unsigned int	low_count;
} DCM_DETECT_STATE;

typedef struct
{
	unsigned char	state;			/* On/Off/Blink */
	unsigned char	blinkOnOffState;/* Blink On/Off state */
	unsigned char	blinkCurState;	/* During Blink On, Current On/Off state */
	unsigned short	count;			/* current count */
	unsigned short	onCount;		/* count of remaining ON state */
	unsigned short	offCount;		/* count of remaining OFF state */
	unsigned short	onTimeout;		/* count of on Timeout counter */
} LED_STATE;

typedef struct led {
	unsigned int	ledNum;
	unsigned int	mode;
	unsigned int	onCount;
	unsigned int	offCount;
} LED_INFO;

/* [LINUSYS] added by jhmin for battery monitoring on 20060913 */
struct power_info {
	int	ac_line_status;
	int	battery_level;
	int	phone_battery_level;
	int	battery_life;
	int	battery_voltage;
};

typedef struct {
	int pid;
	int oom_adj;
	int task_size;
	int ofree;
} strUserOOMData;


/* [LINUSYS] added by jhmin for battery monitoring on 20060913 */

/* [LINUSYS] added by khoonk & young for test on 20060823 */

#define SOUND_PATH_TEST

#ifdef SOUND_PATH_TEST
/* Input Definitions */
#define AUDIO_IN_PDA		(1 << 0)
#define AUDIO_IN_BT			(1 << 1)
#define AUDIO_IN_MODEM		(1 << 2)
#define AUDIO_IN_MIC		(1 << 3)

/* Output Definitions */
#define AUDIO_OUT_SPEAKER	(1 << 0)
#define AUDIO_OUT_RECEIVER	(1 << 1)
#define AUDIO_OUT_BT		(1 << 2)
#define AUDIO_OUT_MODEM		(1 << 3)
#define AUDIO_OUT_EARPHONE	(1 << 4)
#endif
/* [LINUSYS] added by khoonk & young for test on 20060823 */

/****************************************************************************
** definitions
*****************************************************************************/
#define TASK_INTERVAL				1

#define BASE_SOCKET					12000

#define DCM_QUEUE_SIZE				10
#define DCM_QUEUE_TYPE 				t_callMsg


#if 0
#define KEY_EXT_SAMSUNG				  0x1d0		//0x1f0                    //will change vlaue of define to 0x1d0.
#define KEY_EXT_SOFTKEY_LEFT		  (KEY_EXT_SAMSUNG)
#define KEY_EXT_SOFTKEY_RIGHT		  (KEY_EXT_SAMSUNG + 1)
#define KEY_EXT_CALL				  (KEY_EXT_SAMSUNG + 2)
#define KEY_EXT_END					  (KEY_EXT_SAMSUNG + 3)
#define KEY_EXT_MULTI_TASK			  (KEY_EXT_SAMSUNG + 4)
#define KEY_EXT_CAMERA				  (KEY_EXT_SAMSUNG + 5)
#define KEY_EXT_FLIP_OPEN             (KEY_EXT_SAMSUNG + 6)
#define KEY_EXT_FLIP_CLOSE            (KEY_EXT_SAMSUNG + 7)
#define KEY_EXT_HOME				  (KEY_EXT_SAMSUNG + 8)
#define KEY_EXT_TS_CALIBRATION_START  (KEY_EXT_SAMSUNG + 9)
#define KEY_EXT_TS_CALIBRATION_END    (KEY_EXT_SAMSUNG + 10)
#define KEY_EXT_ROTATE_0              (KEY_EXT_SAMSUNG + 11)
#define KEY_EXT_ROTATE_90             (KEY_EXT_SAMSUNG + 12)
#define KEY_EXT_ROTATE_180            (KEY_EXT_SAMSUNG + 13)
#define KEY_EXT_ROTATE_270            (KEY_EXT_SAMSUNG + 14)
#define KEY_EXT_PREPARE_CAMERA        (KEY_EXT_SAMSUNG + 15)
#define KEY_EXT_POWER_LONG            (KEY_EXT_SAMSUNG + 16) 
#define KEY_EXT_POWER_SHORT           (KEY_EXT_SAMSUNG + 17) 
#define KEY_EXT_EARKEY_STATUS         (KEY_EXT_SAMSUNG + 18)
#define KEY_EXT_LIFE_DRIVE        (KEY_EXT_SAMSUNG + 19)		//SEND(=CALL)
#define KEY_EXT_PEOPLE_VIEW       (KEY_EXT_SAMSUNG + 20)		//HOME(=QUBE)
#define KEY_EXT_ACTIVITY_VIEW     (KEY_EXT_SAMSUNG + 21)		//END
#endif

#if 1	//key code will be changed at the 20090408
#define	KEY_POWER2					0x164
#undef  KEY_POWER
#define KEY_POWER					0x074
#undef  KEY_VOLUMEUP
#define	KEY_VOLUMEUP				0x073
#undef  KEY_VOLUMEDOWN
#define	KEY_VOLUMEDOWN				0x072

#undef  KEY_SEARCH
#define KEY_SEARCH					0x0D9
#undef  KEY_CAMERA
#define	KEY_CAMERA					0x0D4
#undef  KEY_CONFIG
#define KEY_CONFIG					0x0AB
#undef  KEY_PHONE
#define	KEY_PHONE					0x0A9
#undef  KEY_FRONT
#define	KEY_FRONT					0x084
#undef  KEY_EXIT
#define	KEY_EXIT					0x0AE

#define KEY_EXT_ROTATE_0			0x1DB
#define KEY_EXT_ROTATE_90			0x1DC
#define KEY_EXT_ROTATE_180			0x1DD
#define KEY_EXT_ROTATE_270			0x1DE

#endif
#if 0
#define KEY_DEVMGR_3DOME1				KEY_EXT_LIFE_DRIVE		//KEY_EXT_CALL
#define KEY_DEVMGR_3DOME2				KEY_EXT_PEOPLE_VIEW		//KEY_EXT_HOME
#define KEY_DEVMGR_3DOME3				KEY_EXT_ACTIVITY_VIEW	//KEY_EXT_END
#define KEY_TEST_CALL				KEY_EXT_LIFE_DRIVE		//KEY_EXT_CALL
#define KEY_TEST_HOME				KEY_EXT_PEOPLE_VIEW		//KEY_EXT_HOME
#define KEY_TEST_END				KEY_EXT_ACTIVITY_VIEW	//KEY_EXT_END
#endif

#define KEY_DEVMGR_3DOME1			KEY_PHONE	//KEY_EXT_CALL
#define KEY_DEVMGR_3DOME2			KEY_FRONT	//KEY_EXT_HOME
#define KEY_DEVMGR_3DOME3			KEY_EXIT	//KEY_EXT_END
#define KEY_TEST_CALL				KEY_PHONE	//KEY_EXT_CALL
#define KEY_TEST_HOME				KEY_FRONT	//KEY_EXT_HOME
#define KEY_TEST_END				KEY_EXIT	//KEY_EXT_END

/* macros for LED */
#define MAX_LED_NUM				3
#define LED_OFF						0x00
#define LED_ON						0x01
#define	LED_BLINK					0x02
#define	LED_BLINK2					0x03
#define	LED_ON_TIMEOUT				0x04

#define LED_KEY						0x00
#if defined(CONFIG_MACH_ZYLONITE) || defined(CONFIG_MACH_MIRAGE) || defined(CONFIG_MACH_RIEDEL) || defined(CONFIG_MACH_GRANDPRIX) || defined(CONFIG_MACH_FIORE) || defined(CONFIG_TC_TI)
#define LED_RED						0x03
#define LED_GREEN					0x06
#define LED_BLUE					0x09

#define LED_RED_ON                  LED_ON    + LED_RED
#define LED_RED_OFF                 LED_OFF   + LED_RED
#define LED_RED_BLINK  		        LED_BLINK + LED_RED

#define LED_GREEN_ON                LED_ON    + LED_GREEN
#define LED_GREEN_OFF               LED_OFF   + LED_GREEN
#define LED_GREEN_BLINK             LED_BLINK + LED_GREEN

#define LED_BLUE_ON                 LED_ON    + LED_BLUE
#define LED_BLUE_OFF                LED_OFF   + LED_BLUE
#define LED_BLUE_BLINK              LED_BLINK + LED_BLUE

#endif
#define LEDSTATE(X) 				gLedState[X].state
#define LED_BLINK_STATE(X)			gLedState[X].blinkOnOffState
#define LED_BLINK_CUR_STATE(X)		gLedState[X].blinkCurState
#define LED_COUNT(X)				gLedState[X].count
#define LED_ON_COUNT(X) 			gLedState[X].onCount
#define LED_OFF_COUNT(X)			gLedState[X].offCount
#define LED_TIMEOUT(X)				gLedState[X].onTimeout

#define KEY_LED_ON                  LED_ON+LED_KEY
#define KEY_LED_OFF                 LED_OFF+LED_KEY
#define KEY_LED_BLINK               LED_BLINK+LED_KEY


/* task id field */
#define TASKID_DCM					0x0001
#define TASKID_MMC					0x0002
#define TASKID_DEVMGR				0x0010
#define TASKID_DIRECTFB				0x0020
#define TASKID_PHONESVR				0x0040
#define TASKID_NOTI_ADAPTOR			0x0080

/* message id field */
#define SYSTEM_EVENT				0x00000001
#define KEYPAD_EVENT				0x00000002
#define TOUCH_EVENT					0x00000004
#define FOLDER_EVENT				0x00000008
#define EARPHONE_EVENT				0x00000010
#define USB_EVENT					0x00000020
#define APM_EVENT					0x00000040
#define CALL_EVENT					0x00000080
#define BATTERY_EVENT				0x00000100
#define MMC_EVENT					0x00000200
#define MISC_EVENT					0x00000400
#define CHARGER_EVENT				0x00000800
#define OOM_EVENT                   0x00001000

/* new event id for power manager */
#define PWRMGR_EVENT                0x00002000
#define WAKEUP_EVENT                0x00004000

/* parameter filed of BATTERY_EVENT */
#define BATTERY_LEVEL_CHANGE		1
#define BATTERY_POWER_OFF			2

#define AC_OFFLINE					0
#define AC_ONLINE					1
#define AC_UNKNOWN					2
#define AC_FULL						3
#define AC_COUNT_FULL               4
#define AC_ABNORMAL_TEMP            5
#define AC_RECOVER_TEMP             6

#define BATTERY_LEVEL0				0
#define BATTERY_LEVEL1				1
#define BATTERY_LEVEL2				2
#define BATTERY_LEVEL3				3
#define BATTERY_CHARGING			4	
#define BATTERY_UNKNOWN				5

/* parameter field of CHARGER EVENT */
#define CHARGER_DISCONNECTED		0
#define CHARGER_CONNECTED			1	

/* parameter filed of MMC EVENT */
#define MMC_INSERTED				0
#define MMC_REMOVED					1

/* parameter filed of KEY EVENT */
#define KEY_PRESSED					1
#define KEY_RELEASED				0

/* parameter filed of FOLDER EVENT */
#define FOLDER_CLOSED				0
#define FOLDER_OPENED				1

/* parameter filed of EARJACK EVENT */
#define EARJACK_STATUS				0
#define EARPHONE_KEY				1

#define EARJACK_DISCONNECTED		0
#define EARJACK_CONNECTED			1

/* parameter filed of CALL EVENT */
#define REQUEST_STATUS				0
#define STATUS_REPORT				1

#define CALL_IDLE					0
#define CALL_ALERTING				1
#define CALL_INCOMING				2
#define CALL_CONV					3
#define SMS_IDLE					0
#define SMS_TX						1
#define SMS_RX						2

/* parameter field of MISC EVENT */
#define SET_SUSPEND_TIMEOUT			0

/* parameter filed of KEY EVENT */
#define USB_DISCONNECTED			0
#define USB_CONNECTED				1


#define CHARGER_OUT		0
#define CHARGER_IN		1

#define UNKNOWN_STATUS				2

/* parameter field of PWRMGR_EVENT */
#define PWRMGR_EVENT_KEY            0x00
#define PWRMGR_EVENT_TOUCH          0x01
#define PWRMGR_EVENT_POWERKEY       0x02

/* parameter field of PWRMGR_EVENT */
#define PWRMGR_EVENT_KEY_RELEASED       0x0
#define PWRMGR_EVENT_KEY_PRESSED        0x1

#define PWRMGR_EVENT_TOUCH_RELEASED     0x0
#define PWRMGR_EVENT_TOUCH_PRESSED      0x1

#define PWRMGR_EVENT_POWERKEY_RELEASED  0x0 
#define PWRMGR_EVENT_POWERKEY_PRESSED   0x1



/* parameter : get accleration data */ 
#define CONVERT_X_COMMAND 0x00
#define CONVERT_Y_COMMAND 0x02
#define CONVERT_Z_COMMAND 0x01



/* macros for event parsing */
#define	CALL_MSG_STID(p)			(((t_callMsg *) p)->hdr.s_vid )
#define	CALL_MSG_DTID(p)			(((t_callMsg *) p)->hdr.d_vid )
#define	CALL_MSG_MSGID(p)			(((t_callMsg *) p)->hdr.msgid )
#define	CALL_MSG_SIZE(p)			(((t_callMsg *) p)->hdr.size )
#define	CALL_MSG_TIME(p)			(((t_callMsg *) p)->hdr.time )
#define	CALL_MSG_PARA1(p)			(((t_callMsg *) p)->para1 )
#define	CALL_MSG_PARA2(p)			(((t_callMsg *) p)->para2 )
#define	CALL_MSG_PARA3(p)			(((t_callMsg *) p)->para3 )
#define	CALL_MSG_PARA4(p)			(((t_callMsg *) p)->para4 )

#define R_STID						CALL_MSG_STID (p_rcv_msg)
#define R_DTID                      CALL_MSG_DTID (p_rcv_msg)
#define R_MSGID                     CALL_MSG_MSGID(p_rcv_msg)
#define R_SIZE                      CALL_MSG_SIZE (p_rcv_msg)
#define R_TIME                      CALL_MSG_TIME (p_rcv_msg)
#define	R_PARA1						CALL_MSG_PARA1(p_rcv_msg)
#define	R_PARA2						CALL_MSG_PARA2(p_rcv_msg)
#define	R_PARA3						CALL_MSG_PARA3(p_rcv_msg)
#define	R_PARA4						CALL_MSG_PARA4(p_rcv_msg)

#define S_STID						CALL_MSG_STID (p_snd_msg)
#define S_DTID                      CALL_MSG_DTID (p_snd_msg)
#define S_MSGID                     CALL_MSG_MSGID(p_snd_msg)
#define S_SIZE                      CALL_MSG_SIZE (p_snd_msg)
#define S_TIME                      CALL_MSG_TIME (p_snd_msg)
#define	S_PARA1						CALL_MSG_PARA1(p_snd_msg)
#define	S_PARA2						CALL_MSG_PARA2(p_snd_msg)
#define	S_PARA3						CALL_MSG_PARA3(p_snd_msg)
#define	S_PARA4						CALL_MSG_PARA4(p_snd_msg)

#if 1
/* [LINUSYS] added by jhmin for getting wakeup source info on 20060912 */
#define UNKNOWN_WAKEUP				0x00
#define POWER_BUTTON_WAKEUP			0x01
#define	DPRAM_WAKEUP				0x02
#define KEYPAD_WAKEUP				0x03	/* except power button */
#define	GPIO_RESET_WAKEUP			0x04
#define	FOLDER_WAKEUP				0x05
#define	JACK_INT_WAKEUP				0x06
#define	JACK_KEY_WAKEUP				0x07
/* [LINUSYS] added by jhmin for getting wakeup source info on 20060912 */
#endif

/****************************************************************************
** ioctl
*****************************************************************************/
#define DCM_IOC_MAGIC           	's'

#if 1
/* [LINUSYS] added by khoonk for sync problem on 20060718 */
#define IOC_DCM_START				_IO (DCM_IOC_MAGIC, 0x00)
/* [LINUSYS] added by khoonk for sync problem on 20060718 */
#endif

#define IOC_LED_SET					_IO (DCM_IOC_MAGIC, 0x10)

#define IOC_TCH_GET_CAL				_IO (DCM_IOC_MAGIC, 0x20)
#define IOC_TCH_SET_CAL				_IO (DCM_IOC_MAGIC, 0x21)
#define IOC_TCH_ENABLE 				_IO (DCM_IOC_MAGIC, 0x22)
#define IOC_TCH_DISABLE				_IO (DCM_IOC_MAGIC, 0x23)

#define IOC_LCD_GET_BRT				_IO (DCM_IOC_MAGIC, 0x30)
#define IOC_LCD_SET_BRT				_IO (DCM_IOC_MAGIC, 0x31)

#define IOC_LCD_GET_ALC				_IO (DCM_IOC_MAGIC, 0x32)
#define IOC_LCD_SET_ALC				_IO (DCM_IOC_MAGIC, 0x33)

#define IOC_LCD_POW_ON				_IO (DCM_IOC_MAGIC, 0x35)
#define IOC_LCD_POW_OFF				_IO (DCM_IOC_MAGIC, 0x36)

#define IOC_LCD_GET_CABC			_IO (DCM_IOC_MAGIC, 0x37)
#define IOC_LCD_SET_CABC			_IO (DCM_IOC_MAGIC, 0x38)

#define IOC_BAT_GET_STATUS			_IO (DCM_IOC_MAGIC, 0x40)
#define IOC_BAT_SET_PHONE_CRITICAL	_IO (DCM_IOC_MAGIC, 0x41)
#define IOC_BAT_GET_SOC             _IO (DCM_IOC_MAGIC, 0x42)

#define IOC_ACC_GET_DATA            _IO (DCM_IOC_MAGIC, 0x90)

/*@ LDK added by sheom : 20081022*/      
#define IOC_SET_SLEEP_MODE            _IO (DCM_IOC_MAGIC, 0xA0)
#define IOC_SET_WAKEUP_SRC            _IO (DCM_IOC_MAGIC, 0xA1)
#define IOC_GET_WAKEUP_SRC            _IO (DCM_IOC_MAGIC, 0xA2)
#define IOC_GET_WAKEUP_DONE_SRC             _IO (DCM_IOC_MAGIC, 0xA3)



#if 1
/* [LINUSYS] added by jhmin for setting vibrator on/off on 20060731 */
#define IOC_VIB_SET					_IO (DCM_IOC_MAGIC, 0x50)
/* [LINUSYS] added by jhmin for setting vibrator on/off on 20060731 */
#endif

#if 1
/* hkchoi for vibetonz */
#define VIBETONZ_IOC_MAGIC	'v'

#define HN_MOTOR_ON			_IO (VIBETONZ_IOC_MAGIC, 0x00)
#define HN_MOTOR_OFF		_IO (VIBETONZ_IOC_MAGIC, 0x01)
#define HN_MOTOR_MAG		_IOW (VIBETONZ_IOC_MAGIC, 0x02, int)
#endif

#define PARAM_MAGIC			0x72726624
#define PARAM_VERSION		0x10	/* Rev 1.0 */
#define PARAM_STRING_SIZE	1024	/* 1024 Characters */

#define MAX_PARAM			20
#define MAX_STRING_PARAM	5

/* Default Parameter Values */
#define TERMINAL_SPEED		7	/* 115200 */
#define LCD_LEVEL			6	/* lcd level */
#define BOOT_DELAY			0	/* wait 0 second */
#define LOAD_RAMDISK		0	/* do not load ramdisk into ram */
#define SWITCH_SEL			1	/* set uart and usb path to pda */
#define PHONE_DEBUG_ON		0	/* set phone debug mode */
#define BL_DIM_TIME			6	/* set backlight dimming time */
#define MELODY_MODE			1	/* set melody mode (sound -> 1, slient -> 0) */
#define DOWNLOAD_MODE		0	/* set download mode */
#define NATION_SEL			0	/* set nation specific configuration */
#define VERSION_LINE		"I8315BUHE6"	/* set image version info */
#if defined(CONFIG_CPU_MONAHANS_POP_ONENAND)
#define COMMAND_LINE		"root=139:4 rootfstype=cramfs console=ttyS0,115200 mem=128M"
#elif defined(CONFIG_CPU_MONAHANS_MCP_NAND)
#define COMMAND_LINE		"root=137:4 rootfstype=cramfs console=ttyS0,115200 mem=128M"
#endif

typedef enum {
	__SERIAL_SPEED,
	__LOAD_RAMDISK,
	__BOOT_DELAY,
	__LCD_LEVEL,
	__SWITCH_SEL,
	__PHONE_DEBUG_ON,
	__BL_DIM_TIME,
	__MELODY_MODE,
	__DOWNLOAD_MODE,
	__NATION_SEL,
	__PARAM_INT_10,
	__PARAM_INT_11,
	__PARAM_INT_12,
	__PARAM_INT_13,
	__PARAM_INT_14,
	__VERSION,
	__CMDLINE,
	__PARAM_STR_2,
	__PARAM_STR_3,
	__PARAM_STR_4
} param_idx;

typedef struct _param_int_t {
	param_idx ident;
	int  value;
} param_int_t;

typedef struct _param_str_t {
	param_idx ident;
	char value[PARAM_STRING_SIZE];
} param_str_t;

typedef struct {
	int param_magic;
	int param_version;
	param_int_t param_list[MAX_PARAM - MAX_STRING_PARAM];
	param_str_t param_str_list[MAX_STRING_PARAM];
} status_t;

/* @LDK added by sheom[20080917] : store wakeup event source*/
typedef struct {
	unsigned long value;
	struct {
		unsigned uart1 : 1;
		unsigned uart2 : 1;
		unsigned uart3 : 1;
		unsigned dpram : 1;
		unsigned earkey : 1;
		unsigned earjack : 1;
		unsigned earjack35 : 1;
		unsigned mmc : 1;
		unsigned ta : 1;
		unsigned ta_on : 1;
		unsigned low_vol : 1;
		unsigned mkey : 1;
		unsigned usb : 1;
		unsigned gptimer : 1;
		unsigned rtc : 1;
		unsigned pwrkey : 1;
	} bits;
} pm_wakeup_src_t;




#define IOC_SAVE_PARAM				_IO (DCM_IOC_MAGIC, 0x51)
#define IOC_SET_PARAM				_IO (DCM_IOC_MAGIC, 0x52)
#define IOC_READ_PARAM				_IO (DCM_IOC_MAGIC, 0x53)

#ifdef SOUND_PATH_TEST
/* [LINUSYS] added by khoonk & young for test on 20060823 */
#define IOC_SND_PATH				_IO (DCM_IOC_MAGIC, 0x60)
/* [LINUSYS] added by khoonk & young for test on 20060823 */
#endif

#if 1
/* [LINUSYS] added by jhmin for supporting UART selection on 20060725 */
#define IOC_UART_SET				_IO (DCM_IOC_MAGIC, 0x80)
#define IOC_UART_GET				_IO (DCM_IOC_MAGIC, 0x82)
/* [LINUSYS] added by jhmin for supporting UART selection on 20060725 */
#endif

#if 1
/* [LINUSYS] added by jhmin for supporting USB selection on 20060913 */
#define IOC_USB_SET					_IO (DCM_IOC_MAGIC, 0x81)
#define IOC_USB_GET					_IO (DCM_IOC_MAGIC, 0x83)
/* [LINUSYS] added by jhmin for supporting USB selection on 20060913 */
#endif

#if 1
/* @LDK@ added by jtjang for mount when booting on 20060816 */
#define MMC_IOC_MAGIC           	'm'

#define IOC_MMC_SLOT_IS_NOK		_IO (DCM_IOC_MAGIC, 0x00)
/* @LDK@ added by jtjang for mount when booting on 20060816 */
#endif


#endif

#if 1 
/* @LDK@ added by sbkang for tvout on 060905 */
#define IOC_TVOUT_POWER_ON			_IO (DCM_IOC_MAGIC, 0x70)
#define IOC_TVOUT_POWER_OFF			_IO (DCM_IOC_MAGIC, 0x71)
#define IOC_TVOUT_START_REFRESH		_IO (DCM_IOC_MAGIC, 0x72)
#define IOC_TVOUT_STOP_REFRESH		_IO (DCM_IOC_MAGIC, 0x73)

#define IOC_TVOUT_SET_PROPERTY		_IO (DCM_IOC_MAGIC, 0x74)
#define IOC_TVOUT_GET_PROPERTY		_IO (DCM_IOC_MAGIC, 0x75)

#define IOC_TVOUT_SET_REGISTER		_IO (DCM_IOC_MAGIC, 0x76)
#define IOC_TVOUT_GET_REGISTER		_IO (DCM_IOC_MAGIC, 0x77)
#define IOC_TVOUT_TEST_PATTERN		_IO (DCM_IOC_MAGIC, 0x78)
#endif

#if 1
/* [LINUSYS] added by jhmin for getting wakeup source info on 20060912 */
//#define IOC_GET_WAKEUP_SOURCE		_IO (DCM_IOC_MAGIC, 0x61)
/* [LINUSYS] added by jhmin for getting wakeup source info on 20060912 */
#define IOC_SET_TOUCH_INTERRUPT		_IO (DCM_IOC_MAGIC, 0x62)
#define IOC_SET_KEYPAD_INTERRUPT    _IO (DCM_IOC_MAGIC, 0x63)
#define IOC_SET_ACCELEROMETER       _IO (DCM_IOC_MAGIC, 0x64)
#define IOC_SET_IPM                 _IO (DCM_IOC_MAGIC, 0x65)
#define IOC_SET_TOUCH_DELAY         _IO (DCM_IOC_MAGIC, 0x66)
#endif


#define IOC_EARPHONE_STATUS		_IO (DCM_IOC_MAGIC, 0xB0)
#define IOC_EARKEY_STATUS		_IO (DCM_IOC_MAGIC, 0xB1)

//hosun
#define IOC_COMBO_EN_SET			_IO (DCM_IOC_MAGIC, 0x84)

#define IOC_SET_USER_OOM            _IO (DCM_IOC_MAGIC, 0x85)
#define IOC_SEND_CHARGER_STATUS     _IO (DCM_IOC_MAGIC, 0x86)

