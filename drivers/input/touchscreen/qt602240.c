/* drivers/input/touchscreen/qt602240.c
 *
 * Quantum TSP driver.
 *
 * Copyright (C) 2009 Samsung Electronics Co. Ltd.
 *
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <asm/io.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#ifdef CONFIG_CPU_FREQ
#include <plat/s3c64xx-dvfs.h>
#endif

/* for i2c */
#define I2C_M_WR 0
#define I2C_MAX_SEND_LENGTH     300
#define QT602240_I2C_ADDR 0x4A

#define IRQ_TOUCH_INT IRQ_EINT(20)

#define DEBUG printk("[QT] %s/%d\n",__func__,__LINE__)

/* CHJ Define start  */
#define DEBUG_CHJ 0
#define READ_MESSAGE_LENGTH 6
#define QT602240_DEBUG

#define REPORT( touch, width, x, y) \
{	input_report_abs(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, touch ); \
	input_report_abs(qt602240->input_dev, ABS_MT_WIDTH_MAJOR, width ); \
	input_report_abs(qt602240->input_dev, ABS_MT_POSITION_X, x); \
	input_report_abs(qt602240->input_dev, ABS_MT_POSITION_Y, y); \
	input_mt_sync(qt602240->input_dev); }
/* CHJ Define end */

// print message
#if defined(QT602240_DEBUG) || defined(DEBUG)
#define DEBUG_MSG(p, x...)			printk("[QT602240]:[%s] ", __func__); printk(p, ## x);		
#else
#define DEBUG_MSG(p, x...)
#endif

#define PRINT_MSG(p, x...)		{ printk("[QT602240]:[%s] ", __func__); printk(p, ## x); }

#ifdef QT602240_DEBUG
#define ENTER_FUNC	{ printk("[TSP] +%s\n", __func__); }
#define LEAVE_FUNC	{ printk("[TSP] -%s\n", __func__); }
#else
#define ENTER_FUNC
#define LEAVE_FUNC
#endif

/* firmware 2009.09.24 CHJ - start 1/2 */
#define QT602240_I2C_BOOT_ADDR 0x24
#define QT_WAITING_BOOTLOAD_COMMAND 0xC0
#define QT_WAITING_FRAME_DATA       0x80
#define QT_FRAME_CRC_CHECK          0x02
#define QT_FRAME_CRC_PASS           0x04
#define QT_FRAME_CRC_FAIL           0x03

uint8_t QT_Boot(void);
static int firmware_ret_val = -1;

unsigned char firmware_602240[] = {
#include "Firmware16_EE.h"
};
/* firmware 2009.09.24 CHJ - end 1/2 */

struct i2c_driver qt602240_i2c_driver;
struct workqueue_struct *qt602240_wq;

struct i2c_ts_driver {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend	early_suspend;
};
struct i2c_ts_driver *qt602240 = NULL;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void qt602240_early_suspend(struct early_suspend *);
static void qt602240_late_resume(struct early_suspend *);
#endif	/* CONFIG_HAS_EARLYSUSPEND */


/*---------------------------------------------------------/
 *
 * Quantum Code Block
 *
 * -------------------------------------------------------*/
#define  U16    unsigned short int 
#define U8      __u8
#define u8      __u8
#define S16     signed short int
#define U16     unsigned short int
#define S32     signed long int
#define U32     unsigned long int
#define S64     signed long long int
#define U64     unsigned long long int
#define F32     float
#define F64     double

/*------------------------------ definition block -----------------------------------*/

/* \brief Defines CHANGE line active mode. */
#define CHANGELINE_ASSERTED 0

/* This sets the I2C frequency to 400kHz (it's a feature in I2C driver that the
   actual value needs to be double that). */
#define I2C_SPEED                   800000u

#define CONNECT_OK                  1u
#define CONNECT_ERROR               2u

#define READ_MEM_OK                 1u
#define READ_MEM_FAILED             2u

#define MESSAGE_READ_OK             1u
#define MESSAGE_READ_FAILED         2u

#define WRITE_MEM_OK                1u
#define WRITE_MEM_FAILED            2u

#define CFG_WRITE_OK                1u
#define CFG_WRITE_FAILED            2u

#define I2C_INIT_OK                 1u
#define I2C_INIT_FAIL               2u

#define CRC_CALCULATION_OK          1u
#define CRC_CALCULATION_FAILED      2u

#define ID_MAPPING_OK               1u
#define ID_MAPPING_FAILED           2u

#define ID_DATA_OK                  1u
#define ID_DATA_NOT_AVAILABLE       2u

/*! \brief Returned by get_object_address() if object is not found. */
#define OBJECT_NOT_FOUND   0u

/*! Address where object table starts at touch IC memory map. */
#define OBJECT_TABLE_START_ADDRESS      7U

/*! Size of one object table element in touch IC memory map. */
#define OBJECT_TABLE_ELEMENT_SIZE       6U

/*! Offset to RESET register from the beginning of command processor. */
#define RESET_OFFSET                        0u

/*! Offset to BACKUP register from the beginning of command processor. */
#define BACKUP_OFFSET           1u

/*! Offset to CALIBRATE register from the beginning of command processor. */
#define CALIBRATE_OFFSET        2u

/*! Offset to REPORTALL register from the beginning of command processor. */
#define REPORTATLL_OFFSET       3u

/*! Offset to DEBUG_CTRL register from the beginning of command processor. */
#define DEBUG_CTRL_OFFSET       4u

/*! Offset to DIAGNOSTIC register from the beginning of command processor. */
#define DIAGNOSTIC_OFFSET       5u


enum driver_setup_t {DRIVER_SETUP_OK, DRIVER_SETUP_INCOMPLETE};



#define OBJECT_LIST__VERSION_NUMBER     0x10

#define RESERVED_T0                               0u
#define RESERVED_T1                               1u
#define DEBUG_DELTAS_T2                           2u
#define DEBUG_REFERENCES_T3                       3u
#define DEBUG_SIGNALS_T4                          4u
#define GEN_MESSAGEPROCESSOR_T5                   5u
#define GEN_COMMANDPROCESSOR_T6                   6u
#define GEN_POWERCONFIG_T7                        7u
#define GEN_ACQUISITIONCONFIG_T8                  8u
#define TOUCH_MULTITOUCHSCREEN_T9                 9u
#define TOUCH_SINGLETOUCHSCREEN_T10               10u
#define TOUCH_XSLIDER_T11                         11u
#define TOUCH_YSLIDER_T12                         12u
#define TOUCH_XWHEEL_T13                          13u
#define TOUCH_YWHEEL_T14                          14u
#define TOUCH_KEYARRAY_T15                        15u
#define PROCG_SIGNALFILTER_T16                    16u
#define PROCI_LINEARIZATIONTABLE_T17              17u
#define SPT_COMCONFIG_T18                         18u
#define SPT_GPIOPWM_T19                           19u
#define PROCI_GRIPFACESUPPRESSION_T20             20u
#define RESERVED_T21                              21u
#define PROCG_NOISESUPPRESSION_T22                22u
#define TOUCH_PROXIMITY_T23                           23u
#define PROCI_ONETOUCHGESTUREPROCESSOR_T24        24u
#define SPT_SELFTEST_T25                          25u
#define DEBUG_CTERANGE_T26                        26u
#define PROCI_TWOTOUCHGESTUREPROCESSOR_T27        27u
#define SPT_CTECONFIG_T28                         28u
#define SPT_GPI_T29                               29u
#define SPT_GATE_T30                              30u
#define TOUCH_KEYSET_T31                          31u
#define TOUCH_XSLIDERSET_T32                      32u
#define RESERVED_T33                              33u
#define GEN_MESSAGEBLOCK_T34                      34u
#define SPT_GENERICDATA_T35                       35u
#define RESERVED_T36                              36u
#define DEBUG_DIAGNOSTIC_T37                      37u
#define SPT_USERDATA_T38                          38u

#define Enable_global_interrupt()        
#define Disable_global_interrupt()
/* -------------------- End of definition block --------------------------*/




/* -------------------- Type define block --------------------------------*/
typedef struct
{
	uint8_t reset;       /*!< Force chip reset             */
	uint8_t backupnv;    /*!< Force backup to eeprom/flash */
	uint8_t calibrate;   /*!< Force recalibration          */
	uint8_t reportall;   /*!< Force all objects to report  */
	uint8_t debugctrl;   /*!< Turn on output of debug data */
	uint8_t diagnostic;  /*!< Controls the diagnostic object */
} gen_commandprocessor_t6_config_t;

typedef struct
{
	uint8_t idleacqint;    /*!< Idle power mode sleep length in ms           */
	uint8_t actvacqint;    /*!< Active power mode sleep length in ms         */
	uint8_t actv2idleto;   /*!< Active to idle power mode delay length in ms */
} gen_powerconfig_t7_config_t;

typedef struct
{
	uint8_t chrgtime;          /*!< Burst charge time                      */
	uint8_t Reserved;  
	uint8_t tchdrift;          /*!< Touch drift compensation period        */
	uint8_t driftst;           /*!< Drift suspend time                     */
	uint8_t tchautocal;        /*!< Touch automatic calibration delay in ms*/
	uint8_t sync;              /*!< Measurement synchronisation control    */
	uint8_t atchcalst;
	uint8_t atchcalsthr;      
} gen_acquisitionconfig_t8_config_t;

typedef struct
{
	/* Screen Configuration */
	uint8_t ctrl;            /*!< Main configuration field           */

	/* Physical Configuration */
	uint8_t xorigin;         /*!< Object x start position on matrix  */
	uint8_t yorigin;         /*!< Object y start position on matrix  */
	uint8_t xsize;           /*!< Object x size (i.e. width)         */
	uint8_t ysize;           /*!< Object y size (i.e. height)        */

	/* Detection Configuration */
	uint8_t akscfg;          /*!< Adjacent key suppression config     */
	uint8_t blen;            /*!< Burst length for all object channels*/
	uint8_t tchthr;          /*!< Threshold for all object channels   */
	uint8_t tchdi;           /*!< Detect integration config           */

	uint8_t orientate;  /*!< Controls flipping and rotating of touchscreen
						 *   object */
	uint8_t mrgtimeout; /*!< Timeout on how long a touch might ever stay
						 *   merged - units of 0.2s, used to tradeoff power
						 *                           *   consumption against being able to detect a touch
						 *                                                   *   de-merging early */

	/* Position Filter Configuration */
	uint8_t movhysti;   /*!< Movement hysteresis setting used after touchdown */
	uint8_t movhystn;   /*!< Movement hysteresis setting used once dragging   */
	uint8_t movfilter;  /*!< Position filter setting controlling the rate of  */

	/* Multitouch Configuration */
	uint8_t numtouch;   /*!< The number of touches that the screen will attempt
						 *   to track */
	uint8_t mrghyst;    /*!< The hystersis applied on top of the merge threshold
						 *   to stop oscillation */
	uint8_t mrgthr;     /*!< The threshold for the point when two peaks are
						 *   considered one touch */

	uint8_t tchamphyst;          /*!< TBD */

	/* Resolution Controls */
	uint16_t xres;
	uint16_t yres;
	uint8_t xloclip;
	uint8_t xhiclip;
	uint8_t yloclip;
	uint8_t yhiclip;
	uint8_t xedgectrl;
	uint8_t xedgedist;
	uint8_t yedgectrl;
	uint8_t yedgedist;

} touch_multitouchscreen_t9_config_t;

typedef struct
{
	/* Key Array Configuration */
	uint8_t ctrl;               /*!< Main configuration field           */

	/* Physical Configuration */
	uint8_t xorigin;           /*!< Object x start position on matrix  */
	uint8_t yorigin;           /*!< Object y start position on matrix  */
	uint8_t xsize;             /*!< Object x size (i.e. width)         */
	uint8_t ysize;             /*!< Object y size (i.e. height)        */

	/* Detection Configuration */
	uint8_t akscfg;             /*!< Adjacent key suppression config     */
	uint8_t blen;               /*!< Burst length for all object channels*/
	uint8_t tchthr;             /*!< Threshold for all object channels   */
	uint8_t tchdi;              /*!< Detect integration config           */
	uint8_t reserved[2];        /*!< Spare x2 */

} touch_keyarray_t15_config_t;

typedef struct
{
	uint8_t  ctrl;
	uint16_t xoffset;
	uint8_t  xsegment[16];
	uint16_t yoffset;
	uint8_t  ysegment[16];

} proci_linearizationtable_t17_config_t;

typedef struct
{
	/* GPIOPWM Configuration */
	uint8_t ctrl;             /*!< Main configuration field           */
	uint8_t reportmask;       /*!< Event mask for generating messages to
							   *   the host */
	uint8_t dir;              /*!< Port DIR register   */
	uint8_t pullup;           /*!< Port pull-up per pin enable register */
	uint8_t out;              /*!< Port OUT register*/
	uint8_t wake;             /*!< Port wake on change enable register  */
	uint8_t pwm;              /*!< Port pwm enable register    */
	uint8_t per;              /*!< PWM period (min-max) percentage*/
	uint8_t duty[4];          /*!< PWM duty cycles percentage */

} spt_gpiopwm_t19_config_t;

typedef struct
{
	uint8_t ctrl;
	uint8_t xlogrip;
	uint8_t xhigrip;
	uint8_t ylogrip;
	uint8_t yhigrip;
	uint8_t maxtchs;
	uint8_t RESERVED2;
	uint8_t szthr1;
	uint8_t szthr2;
	uint8_t shpthr1;
	uint8_t shpthr2;
	uint8_t supextto;
} proci_gripfacesuppression_t20_config_t;

/*! ==PROCG_NOISESUPPRESSION_T22==
  The T22 NOISESUPPRESSION object provides frequency hopping acquire control,
  outlier filtering and grass cut filtering.
 */
/*! \brief 
  This structure is used to configure the NOISESUPPRESSION object and
  should be made accessible to the host controller.
 */

typedef struct
{
	uint8_t ctrl;                 /* LCMASK ACMASK */
	uint8_t reserved;
	uint8_t reserved1;
	int16_t gcaful;               /* LCMASK */
	int16_t gcafll;               /* LCMASK */
	uint8_t actvgcafvalid;        /* LCMASK */
	uint8_t noisethr;             /* LCMASK */
	uint8_t reserved2;
	uint8_t freqhopscale;
	uint8_t freq[5u];             /* LCMASK ACMASK */
	uint8_t idlegcafvalid;        /* LCMASK */
} procg_noisesuppression_t22_config_t;

/*! ==TOUCH_PROXIMITY_T23==
  The T23 Proximity is a proximity key made of upto 16 channels
 */
/*! \brief
  This structure is used to configure the prox object and should be made
  accessible to the host controller.
 */
typedef struct
{
	/* Prox Configuration */
	uint8_t ctrl;               /*!< ACENABLE LCENABLE Main configuration field           */

	/* Physical Configuration */
	uint8_t xorigin;           /*!< ACMASK LCMASK Object x start position on matrix  */
	uint8_t yorigin;           /*!< ACMASK LCMASK Object y start position on matrix  */
	uint8_t xsize;             /*!< ACMASK LCMASK Object x size (i.e. width)         */
	uint8_t ysize;             /*!< ACMASK LCMASK Object y size (i.e. height)        */
	uint8_t reserved_for_future_aks_usage;
	/* Detection Configuration */
	uint8_t blen;               /*!< ACMASK Burst length for all object channels*/
	uint16_t tchthr;             /*!< LCMASK Threshold    */
	uint8_t tchdi;              /*!< Detect integration config           */
	uint8_t average;            /*!< LCMASK Sets the filter length on the prox signal */
	uint16_t rate;               /*!< Sets the rate that prox signal must exceed */

} touch_proximity_t23_config_t;

typedef struct
{
	uint8_t  ctrl;
	uint8_t  numgest;
	uint16_t gesten;
	uint8_t  pressproc;
	uint8_t  tapto;
	uint8_t  flickto;
	uint8_t  dragto;
	uint8_t  spressto;
	uint8_t  lpressto;
	uint8_t  rptpressto;
	uint16_t flickthr;
	uint16_t dragthr;
	uint16_t tapthr;
	uint16_t throwthr;
} proci_onetouchgestureprocessor_t24_config_t;

typedef struct
{
	uint8_t  ctrl;
	uint8_t  cmd;
	uint16_t upsiglim;
	uint16_t losiglim;
} spt_selftest_t25_config_t;

#ifdef MULTITOUCH_ENABLE_CHJ   // for multitouch enable
typedef struct
{
	uint16_t upsiglim;              /* LCMASK */
	uint16_t losiglim;              /* LCMASK */

} siglim_t;

/*! = Config Structure = */

typedef struct
{
	uint8_t  ctrl;                 /* LCENABLE */
	uint8_t  cmd;
#if(NUM_OF_TOUCH_OBJECTS)
	siglim_t siglim[NUM_OF_TOUCH_OBJECTS];   /* LCMASK */
#endif

} spt_selftest_t25_config_t;
#endif

typedef struct
{
	uint8_t  ctrl;          /*!< Bit 0 = object enable, bit 1 = report enable */
	uint8_t  numgest;       /*!< Runtime control for how many two touch gestures
							 *   to process */
	uint8_t reserved;
	uint8_t gesten;        /*!< Control for turning particular gestures on or
							*  off */
	uint8_t  rotatethr;
	uint16_t zoomthr;

} proci_twotouchgestureprocessor_t27_config_t;

/*! ==SPT_CTECONFIG_T28==
  The T28 CTECONFIG object provides controls for the CTE.
 */

/*! \brief 
  This structure is used to configure the CTECONFIG object and
  should be made accessible to the host controller.
 */

typedef struct
{
	uint8_t ctrl;          /*!< Ctrl field reserved for future expansion */
	uint8_t cmd;           /*!< Cmd field for sending CTE commands */
	uint8_t mode;          /*!< LCMASK CTE mode configuration field */
	uint8_t idlegcafdepth; /*!< LCMASK The global gcaf number of averages when idle */
	uint8_t actvgcafdepth; /*!< LCMASK The global gcaf number of averages when active */
	uint8_t temp; 
} spt_cteconfig_t28_config_t;

typedef struct
{
	char * config_name;
	gen_powerconfig_t7_config_t power_config;
	gen_acquisitionconfig_t8_config_t acquisition_config;
	touch_multitouchscreen_t9_config_t touchscreen_config;
	touch_keyarray_t15_config_t keyarray_config;
	proci_gripfacesuppression_t20_config_t gripfacesuppression_config;
	proci_linearizationtable_t17_config_t linearization_config;
	spt_selftest_t25_config_t selftest_config;
	procg_noisesuppression_t22_config_t noise_suppression_config;
	proci_onetouchgestureprocessor_t24_config_t onetouch_gesture_config;
	proci_twotouchgestureprocessor_t27_config_t twotouch_gesture_config;
	spt_cteconfig_t28_config_t cte_config;
	touch_proximity_t23_config_t proximity_config;
} all_config_setting;

typedef struct
{
	int x;
	int y;
	int press;
} dec_input;

/*! \brief Object table element struct. */
typedef struct
{
	uint8_t object_type;     /*!< Object type ID. */
	uint16_t i2c_address;    /*!< Start address of the obj config structure. */
	uint8_t size;            /*!< Byte length of the obj config structure -1.*/
	uint8_t instances;       /*!< Number of objects of this obj. type -1. */
	uint8_t num_report_ids;  /*!< The max number of touches in a screen,
							  *  max number of sliders in a slider array, etc.*/
} object_t;

typedef struct
{
	uint8_t family_id;            /* address 0 */
	uint8_t variant_id;           /* address 1 */

	uint8_t version;              /* address 2 */
	uint8_t build;                /* address 3 */

	uint8_t matrix_x_size;        /* address 4 */
	uint8_t matrix_y_size;        /* address 5 */

	uint8_t num_declared_objects; /* address 6 */
} info_id_t;

typedef struct
{
	/*! Info ID struct. */
	info_id_t info_id;

	/*! Pointer to an array of objects. */
	object_t *objects;

	/*! CRC field, low bytes. */
	uint16_t CRC;

	/*! CRC field, high byte. */
	uint8_t CRC_hi;
} info_block_t;


typedef struct
{
	uint8_t object_type;         /*!< Object type. */
	uint8_t instance;        /*!< Instance number. */
} report_id_map_t;




static info_block_t *info_block;

static report_id_map_t *report_id_map;

volatile uint8_t read_pending;

static int max_report_id = 0;

uint8_t max_message_length;

uint16_t message_processor_address;

/*! Command processor address. */
static uint16_t command_processor_address;

/*! Flag indicating if driver setup is OK. */
static enum driver_setup_t driver_setup = DRIVER_SETUP_INCOMPLETE;

/*! Pointer to message handler provided by main app. */
static void (*application_message_handler)();

/*! Message buffer holding the latest message received. */
uint8_t *quantum_msg;

/*! \brief The current address pointer. */
static U16 address_pointer;

/* flag to indicate if last calibration has been confirmed good */
static uint8_t cal_check_flag = 0u;

/*------------------------------ functions prototype -----------------------------------*/
uint8_t init_touch_driver(uint8_t I2C_address, void (*handler)());
void message_handler(U8 *msg, U8 length);
uint8_t get_version(uint8_t *version);
uint8_t get_family_id(uint8_t *family_id);
uint8_t get_build_number(uint8_t *build);
uint8_t get_variant_id(uint8_t *variant);
uint8_t calculate_infoblock_crc(uint32_t *crc_pointer);
uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance);
uint8_t type_to_report_id(uint8_t object_type, uint8_t instance);
uint8_t write_power_config(gen_powerconfig_t7_config_t cfg);
uint8_t get_max_report_id();
uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t cfg);
uint8_t write_multitouchscreen_config(uint8_t instance, touch_multitouchscreen_t9_config_t cfg);
uint16_t get_object_address(uint8_t object_type, uint8_t instance);
uint8_t write_linearization_config(uint8_t instance, proci_linearizationtable_t17_config_t cfg);
uint8_t write_twotouchgesture_config(uint8_t instance, proci_twotouchgestureprocessor_t27_config_t cfg);
uint8_t write_onetouchgesture_config(uint8_t instance, proci_onetouchgestureprocessor_t24_config_t cfg);
uint8_t write_gripsuppression_config(uint8_t instance, proci_gripfacesuppression_t20_config_t cfg);
uint8_t write_CTE_config(spt_cteconfig_t28_config_t cfg);
uint8_t write_noisesuppression_config(uint8_t instance, procg_noisesuppression_t22_config_t cfg);
uint8_t write_selftest_config(uint8_t instance, spt_selftest_t25_config_t cfg);
uint8_t backup_config(void);
uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg);
uint8_t calibrate_chip(void);
int set_all_config( all_config_setting config );
uint8_t write_keyarray_config(uint8_t instance, touch_keyarray_t15_config_t cfg);
void get_message(void);
U8 init_I2C(U8 I2C_address_arg);
uint8_t read_id_block(info_id_t *id);
U8 read_mem(U16 start, U8 size, U8 *mem);
U8 read_U16(U16 start, U16 *mem);
U8 write_mem(U16 start, U8 size, U8 *mem);
uint8_t get_object_size(uint8_t object_type);
uint8_t write_simple_config(uint8_t object_type, uint8_t instance, void *cfg);
uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2);
int qt602240_i2c_read(u16 reg,unsigned char *rbuf, int buf_size);
void touch_hw_rst( int );
void read_all_register();
uint8_t report_all(void);
//uint8_t selftest(void);
void init_hw_setting(void);
void check_chip_calibration(void);

/*------------------------------ for tunning ATmel - start ----------------------------*/
struct class *touch_class;
EXPORT_SYMBOL(touch_class);

struct device *switch_test;
EXPORT_SYMBOL(switch_test);

static int config_set_enable = 0;  // 0 for disable, 1 for enable
//all_config_setting config_normal = { "config_normal" , 0 };
all_config_setting config_normal = { "config_normal" , 0 };
static dec_input id2 = { 0 };
static dec_input id3 = { 0 };
static dec_input tmp2 = { 0 };
static dec_input tmp3 = { 0 };
static rst_cnt[12] = { 0 };

static int CAL_THR = 10;
static int cal_depth = 0;
static int result_cal[100]={0};	
static int diag_err = 0;

/*------------------------------ for tunning ATmel - end ----------------------------*/


/*------------------------------ function block -----------------------------------*/
/*------------------------------ main block -----------------------------------*/
void quantum_touch_probe(void)
{
	U8 touch_chip_found = 0;
	U8 report_id, MAX_report_ID;
	U8 object_type, instance;
	U32 crc;//, stored_crc;
	int tmp = -1;
	U8 version, family_id, variant, build;

	DEBUG;

	if (init_touch_driver( QT602240_I2C_ADDR , (void *) &message_handler) == DRIVER_SETUP_OK)
	{
		/* "\nTouch device found\n" */
		printk("\n[TSP] Touch device found\n");
	}
	else
	{
		/* "\nTouch device NOT found\n" */
		printk("\n[TSP][ERROR] Touch device NOT found\n");
		return ;
	}

	/* Get and show the version information. */
	get_family_id(&family_id);
	get_variant_id(&variant);
	get_version(&version);
	get_build_number(&build);

	/* Disable interrupts from touch chip during config. We might miss initial
	 *     * calibration/reset messages, but the text we are ouputting here doesn't
	 *         * get cluttered with touch chip messages. */

	if(calculate_infoblock_crc(&crc) != CRC_CALCULATION_OK)
	{
		/* "Calculating CRC failed, skipping check!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		return;
	}



	/* Test the report id to object type / instance mapping: get the maximum
	 *     * report id and print the report id map. */
	MAX_report_ID = get_max_report_id();
	for (report_id = 1; report_id <= MAX_report_ID; report_id++)
	{
		object_type = report_id_to_type(report_id, &instance);
	}

	/* Find the report ID of the first key array (instance 0). */
	report_id = type_to_report_id(TOUCH_KEYARRAY_T15, 0);

	/* config set table */	
	/* Set power config. */

	config_normal.power_config.idleacqint = 100;
	config_normal.power_config.actvacqint = 14;
	config_normal.power_config.actv2idleto = 50; 

	config_normal.acquisition_config.chrgtime = 6;
	config_normal.acquisition_config.Reserved = 0; 
	config_normal.acquisition_config.tchdrift = 5;
	config_normal.acquisition_config.driftst = 0;
	config_normal.acquisition_config.tchautocal = 0;
	config_normal.acquisition_config.sync = 0;
	// normal set 9, 15
	config_normal.acquisition_config.atchcalst = 9;//5;
	config_normal.acquisition_config.atchcalsthr = 15;//0;

	config_normal.touchscreen_config.ctrl = 15;   // former value => 143;
	config_normal.touchscreen_config.xorigin = 0;
	config_normal.touchscreen_config.yorigin = 0;
	config_normal.touchscreen_config.xsize = 15;
	config_normal.touchscreen_config.ysize = 11;
	config_normal.touchscreen_config.akscfg = 0;
	config_normal.touchscreen_config.blen = 33;
	config_normal.touchscreen_config.tchthr = 40;
	config_normal.touchscreen_config.tchdi = 3;  //former value => 2;
	config_normal.touchscreen_config.orientate = 6;
	config_normal.touchscreen_config.mrgtimeout = 0;
	config_normal.touchscreen_config.movhysti = 5;
	config_normal.touchscreen_config.movhystn = 1;
	config_normal.touchscreen_config.movfilter = 15;
	config_normal.touchscreen_config.numtouch= 2;
	config_normal.touchscreen_config.mrghyst = 5;
	config_normal.touchscreen_config.mrgthr = 40;
	config_normal.touchscreen_config.tchamphyst = 5; //former value => 10;
	config_normal.touchscreen_config.xres = 800;
	config_normal.touchscreen_config.yres = 480;
	config_normal.touchscreen_config.xloclip = 0;
	config_normal.touchscreen_config.xhiclip = 0;
	config_normal.touchscreen_config.yloclip = 0;
	config_normal.touchscreen_config.yhiclip = 0; 
	config_normal.touchscreen_config.xedgectrl= 0;
	config_normal.touchscreen_config.xedgedist= 0;
	config_normal.touchscreen_config.yedgectrl= 0;
	config_normal.touchscreen_config.yedgedist= 0;

	config_normal.keyarray_config.ctrl = 0;
	config_normal.keyarray_config.xorigin = 0;
	config_normal.keyarray_config.xsize = 0;
	config_normal.keyarray_config.yorigin = 0;
	config_normal.keyarray_config.ysize = 0;
	config_normal.keyarray_config.akscfg = 0;
	config_normal.keyarray_config.blen = 0;
	config_normal.keyarray_config.tchthr = 0;
	config_normal.keyarray_config.tchdi = 0;

	config_normal.linearization_config.ctrl = 0;
	config_normal.twotouch_gesture_config.ctrl = 0;
	config_normal.onetouch_gesture_config.ctrl = 0;

	config_normal.noise_suppression_config.ctrl = 5;
	config_normal.noise_suppression_config.gcaful = 25;             
	config_normal.noise_suppression_config.gcafll = 65511;             
	config_normal.noise_suppression_config.actvgcafvalid = 3;      
	config_normal.noise_suppression_config.noisethr = 30;           	
	config_normal.noise_suppression_config.freqhopscale = 0;
	config_normal.noise_suppression_config.freq[0] = 0;            
	config_normal.noise_suppression_config.freq[1] = 255;
	config_normal.noise_suppression_config.freq[2] = 12;
	config_normal.noise_suppression_config.freq[3] = 15;
	config_normal.noise_suppression_config.freq[4] = 20;
	config_normal.noise_suppression_config.idlegcafvalid = 3;       

	config_normal.selftest_config.ctrl = 3;

	config_normal.gripfacesuppression_config.ctrl = 7;  // disable grip, enable face
	config_normal.gripfacesuppression_config.xlogrip = 0 ;
	config_normal.gripfacesuppression_config.xhigrip = 0 ;
	config_normal.gripfacesuppression_config.ylogrip = 0 ;
	config_normal.gripfacesuppression_config.yhigrip = 0 ;
	config_normal.gripfacesuppression_config.maxtchs = 0 ;
	config_normal.gripfacesuppression_config.RESERVED2 = 0 ;
	config_normal.gripfacesuppression_config.szthr1 = 85 ;
	config_normal.gripfacesuppression_config.szthr2 = 40 ;
	config_normal.gripfacesuppression_config.shpthr1 = 4 ;
	config_normal.gripfacesuppression_config.shpthr2 = 15 ;
	config_normal.gripfacesuppression_config.supextto = 10 ;

	config_normal.cte_config.ctrl = 0;
	config_normal.cte_config.cmd = 0;
	config_normal.cte_config.mode = 0;
	config_normal.cte_config.idlegcafdepth = 16;
	config_normal.cte_config.actvgcafdepth = 63;
	config_normal.cte_config.temp = 30;   // This value will be not set at Ver 1.4

	/* This is settings for Ver 1.5 (21) and 1.6 (22) */
	if ( info_block->info_id.version >= 21 )
	{
		printk("[TSP] This is settings for Firmware Ver 1.5 & 1.6\n" );
		config_normal.touchscreen_config.blen = 33;
		config_normal.touchscreen_config.tchthr = 55;
	}

	/* set all configuration */
	tmp = set_all_config( config_normal );

	if( tmp == 1 )
		printk("[TSP] set all configs success : %d\n", __LINE__);
	else
	{
		printk("[TSP] set all configs fail : error : %d\n", __LINE__);
		touch_hw_rst( 1 );  // TOUCH HW RESET No.1
		tmp = set_all_config( config_normal );

		if( tmp == 1 )
			printk("[TSP] set all configs success : %d\n", __LINE__);
		else
			printk("[TSP] 2nd set all configs fail : error : %d\n", __LINE__);
	}	
}

int INT_clear( void )
{
	int ret = -1;

	ret = qt602240_i2c_read( message_processor_address, quantum_msg , 1);

	if ( ret < 0 )
		printk("[TSP] %s : i2c fail : %d\n", __func__ , __LINE__ );

	return ret;
}

/*------------------------------ Sub functions -----------------------------------*/
/*!
  \brief Initializes touch driver.

  This function initializes the touch driver: tries to connect to given 
  address, sets the message handler pointer, reads the info block and object
  table, sets the message processor address etc.

  @param I2C_address is the address where to connect.
  @param (*handler) is a pointer to message handler function.
  @return DRIVER_SETUP_OK if init successful, DRIVER_SETUP_FAILED 
  otherwise.
 */
uint8_t init_touch_driver(uint8_t I2C_address, void (*handler)())
{
	int i;

	int current_report_id = 0;

	uint8_t tmp;
	uint16_t current_address;
	uint16_t crc_address;
	object_t *object_table;
	info_id_t *id;

	uint32_t *CRC;

	uint8_t status;


	printk("[QT] %s start\n",__func__);
	/* Set the message handler function pointer. */ 
	application_message_handler = handler;

	/* Try to connect. */
	if(init_I2C(I2C_address) != CONNECT_OK)
	{
		printk("[TSP][ERROR] 1\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}


	/* Read the info block data. */

	id = (info_id_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (id == NULL)
	{
		return(DRIVER_SETUP_INCOMPLETE);
	}



	if (read_id_block(id) != 1)
	{
		printk("[TSP][ERROR] 2\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}  

	/* Read object table. */

	object_table = (object_t *) kmalloc(id->num_declared_objects * sizeof(object_t), GFP_KERNEL | GFP_ATOMIC);
	if (object_table == NULL)
	{
		printk("[TSP][ERROR] 3\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}


	/* Reading the whole object table block to memory directly doesn't work cause sizeof object_t
	   isn't necessarily the same on every compiler/platform due to alignment issues. Endianness
	   can also cause trouble. */

	current_address = OBJECT_TABLE_START_ADDRESS;

	for (i = 0; i < id->num_declared_objects; i++)
	{
		status = read_mem(current_address, 1, &(object_table[i]).object_type);
		if (status != READ_MEM_OK)
		{
			printk("[TSP][ERROR] 4\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_U16(current_address, &object_table[i].i2c_address);
		if (status != READ_MEM_OK)
		{
			printk("[TSP][ERROR] 5\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address += 2;

		status = read_mem(current_address, 1, (U8*)&object_table[i].size);
		if (status != READ_MEM_OK)
		{
			printk("[TSP][ERROR] 6\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].instances);
		if (status != READ_MEM_OK)
		{
			printk("[TSP][ERROR] 7\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].num_report_ids);
		if (status != READ_MEM_OK)
		{
			printk("[TSP][ERROR] 8\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		max_report_id += object_table[i].num_report_ids;

		/* Find out the maximum message length. */
		if (object_table[i].object_type == GEN_MESSAGEPROCESSOR_T5)
		{
			max_message_length = object_table[i].size + 1;
		}
	}

	/* Check that message processor was found. */
	if (max_message_length == 0)
	{
		printk("[TSP][ERROR] 9\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Read CRC. */

	CRC = (uint32_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (CRC == NULL)
	{
		printk("[TSP][ERROR] 10\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}



	info_block = kmalloc(sizeof(info_block_t), GFP_KERNEL | GFP_ATOMIC);
	if (info_block == NULL)
	{
		printk("err\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}


	info_block->info_id = *id;

	info_block->objects = object_table;

	crc_address = OBJECT_TABLE_START_ADDRESS + 
		id->num_declared_objects * OBJECT_TABLE_ELEMENT_SIZE;

	status = read_mem(crc_address, 1u, &tmp);
	if (status != READ_MEM_OK)
	{
		printk("[TSP][ERROR] 11\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC = tmp;

	status = read_mem(crc_address + 1u, 1u, &tmp);
	if (status != READ_MEM_OK)
	{
		printk("[TSP][ERROR] 12\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC |= (tmp << 8u);

	status = read_mem(crc_address + 2, 1, &info_block->CRC_hi);
	if (status != READ_MEM_OK)
	{
		printk("[TSP][ERROR] 13\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store message processor address, it is needed often on message reads. */
	message_processor_address = get_object_address(GEN_MESSAGEPROCESSOR_T5, 0);
	//   printk("%s message_processor_address = %d\n",__FUNCTION__, message_processor_address );

	if (message_processor_address == 0)
	{
		printk("[TSP][ERROR] 14 !!\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store command processor address. */
	command_processor_address = get_object_address(GEN_COMMANDPROCESSOR_T6, 0);
	if (command_processor_address == 0)
	{
		printk("[TSP][ERROR] 15\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	quantum_msg = kmalloc(max_message_length, GFP_KERNEL | GFP_ATOMIC);
	if (quantum_msg == NULL)
	{
		printk("[TSP][ERROR] 16\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Allocate memory for report id map now that the number of report id's 
	 * is known. */

	report_id_map = kmalloc(sizeof(report_id_map_t) * max_report_id, GFP_KERNEL | GFP_ATOMIC);

	if (report_id_map == NULL)
	{
		printk("[TSP][ERROR] 17\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}


	/* Report ID 0 is reserved, so start from 1. */

	report_id_map[0].instance = 0;
	report_id_map[0].object_type = 0;
	current_report_id = 1;

	for (i = 0; i < id->num_declared_objects; i++)
	{
		if (object_table[i].num_report_ids != 0)
		{
			int instance;
			for (instance = 0; 
					instance <= object_table[i].instances; 
					instance++)
			{
				int start_report_id = current_report_id;
				for (; 
						current_report_id < 
						(start_report_id + object_table[i].num_report_ids);
						current_report_id++)
				{
					report_id_map[current_report_id].instance = instance;
					report_id_map[current_report_id].object_type = 
						object_table[i].object_type;
				}
			}
		}
	}
	driver_setup = DRIVER_SETUP_OK;

	return(DRIVER_SETUP_OK);
}

int set_all_config( all_config_setting config )
{
	int ret=-1;
	static int one_time_flag = 1;

	printk("[TSP] %s set!, set condition = %s\n", __func__, config.config_name );

	/* Write power config to chip. */
	if (write_power_config(config.power_config) != CFG_WRITE_OK)
	{
		/* "Power config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		return ret;
	}

	/* Write acquisition config to chip. */
	if (write_acquisition_config(config.acquisition_config) != CFG_WRITE_OK)
	{
		/* "Acquisition config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		return ret;
	}

	/* Write touchscreen (1st instance) config to chip. */
	if (write_multitouchscreen_config(0, config.touchscreen_config) != CFG_WRITE_OK)
	{
		/* "Multitouch screen config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		return ret;
	}

	/* Write grip suppression config to chip. */
	if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND)
	{
		if (write_gripsuppression_config(0, config.gripfacesuppression_config) != CFG_WRITE_OK)
		{
			/* "Grip suppression config write failed!\n" */
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return ret;
		}
	}

	/* Write CTE config to chip. */
	if (get_object_address(SPT_CTECONFIG_T28, 0) != OBJECT_NOT_FOUND)
	{
		if (write_CTE_config(config.cte_config) != CFG_WRITE_OK)
		{
			/* "CTE config write failed!\n" */
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return ret;
		}
	}

	/* Write Noise suppression config to chip. */
	if (get_object_address(PROCG_NOISESUPPRESSION_T22, 0) != OBJECT_NOT_FOUND)
	{
		if (write_noisesuppression_config(0,config.noise_suppression_config) != CFG_WRITE_OK)
		{
			/* "CTE config write failed!\n" */
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return ret;
		}
	}

	/* Backup settings to NVM. */
	if ( one_time_flag )	
	{
		one_time_flag--;
		if ( backup_config() != WRITE_MEM_OK) 
		{
			/* "Failed to backup, exiting...\n" */
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			return ret;
		}
		printk("[TSP] backup one time\n");
		msleep(500);
	}

	/* Calibrate the touch IC. */
	if (calibrate_chip() != WRITE_MEM_OK)
	{
		printk("Failed to calibrate, exiting...\n");
		return ret;
	}

	return 1;

}

void read_all_register()
{
	U16 addr = 0;
	U8 msg;
	U16 calc_addr = 0;

	for(addr = 0 ; addr < 1273 ; addr++)
	{
		calc_addr = addr;


		if(read_mem(addr, 1, &msg) == READ_MEM_OK)
		{
			printk("(0x%2x)", msg);
			if( (addr+1) % 10 == 0)
			{
				printk("\n");
				printk("%2d : ", addr+1);
			}

		}else
		{
			printk("\n\n[TSP][ERROR] %s() read fail !! \n", __FUNCTION__);
		}
	}
}

void print_msg(void)
{
	printk("[TSP] msg = ");
	if ((quantum_msg[1] & 0x80) == 0x80 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x40) == 0x40 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x20) == 0x20 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x10) == 0x10 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x08) == 0x08 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x04) == 0x04 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x02) == 0x02 )
		printk("1");	
	else
		printk("0");	
	if ((quantum_msg[1] & 0x01) == 0x01 )
		printk("1");	
	else
		printk("0");	
}

/*!
 * \brief Reads message from the message processor.
 * 
 * This function reads a message from the message processor and calls
 * the message handler function provided by application earlier.
 *
 * @return MESSAGE_READ_OK if driver setup correctly and message can be read 
 * without problems, or MESSAGE_READ_FAILED if setup not done or incorrect, 
 * or if there's a previous message read still ongoing (this can happen if
 * we are both polling the CHANGE line and monitoring it with interrupt that
 * is triggered by falling edge).
 * 
 */
void get_message(void)
{
#ifdef CONFIG_CPU_FREQ
	//bss set_dvfs_perf_level();
#endif

	unsigned int x, y;
	int size;
	static int check_flag=0;
	uint8_t touch_message_flag = 0;

	/* 
	 * quantum_msg[0] : message ID 
	 * quantum_msg[1] : status 
	 * quantum_msg[2] : x position
	 * quantum_msg[3] : y position
	 * quantum_msg[4] : x & y position
	 * quantum_msg[5] : size
	 * quantum_msg[6] : Touch amplitude
	 * quantum_msg[7] : Touch vector
	 */


	//	if( qt602240_i2c_read( message_processor_address, quantum_msg , READ_MESSAGE_LENGTH) <= 0 )

	quantum_msg[0]= 0;  // clear

	/**********************************************************/	
	/*************** i2c read function - start ****************/	
	struct i2c_msg rmsg;
	size=-1;   // size is used as result value temporarily
	unsigned char data[2];

	rmsg.addr = qt602240->client->addr;
	rmsg.flags = I2C_M_WR;
	rmsg.len = 2;
	rmsg.buf = data;
	data[0] = message_processor_address & 0x00ff;
	data[1] = message_processor_address >> 8;
	size = i2c_transfer(qt602240->client->adapter, &rmsg, 1);

	if( size > 0 ) {
		rmsg.flags = I2C_M_RD;
		rmsg.len = READ_MESSAGE_LENGTH;
		rmsg.buf = quantum_msg;
		size = i2c_transfer(qt602240->client->adapter, &rmsg, 1);

		if ( size <= 0 )
		{
			printk("[TSP] Error code : %d\n", __LINE__ );
			touch_hw_rst( 2 );  // TOUCH HW RESET No.2
			enable_irq(qt602240->client->irq);
			return ;
		}
	}
	else
	{
		printk("[TSP] Error code : %d\n", __LINE__ );
		touch_hw_rst( 3 );  // TOUCH HW RESET No.3
		enable_irq(qt602240->client->irq);
		return ;
	}

	/*************** i2c read function - end ******************/	
	/**********************************************************/	

	/* Call the main application to handle the message. */
	/* x is real y, y is real x. Should change two of them */	
	y = quantum_msg[2]; 
	y = y << 2;
	y = y | (quantum_msg[4] >> 6);

	x = quantum_msg[3];
	x = x << 2;
	x = x | ((quantum_msg[4] & 0xC)  >> 2);

	size = quantum_msg[5];

	//	printk("[TSP] Dump state, msg[0]=%d, [1]=%d\n", quantum_msg[0],  quantum_msg[1] );		

	/* 
	 * quantum_msg[1] & 0x80 : 10000000 -> DETECT 
	 * quantum_msg[1] & 0x40 : 01000000 -> PRESS
	 * quantum_msg[1] & 0x20 : 00100000 -> RELEASE
	 * quantum_msg[1] & 0x10 : 00010000 -> MOVE
	 * quantum_msg[1] & 0x08 : 00001000 -> VECTOR
	 * quantum_msg[1] & 0x04 : 00000100 -> AMP
	 * quantum_msg[1] & 0x02 : 00000010 -> SUPPRESS
	 */

	/* for ID=2 & 3, these are valid inputs. */
	if ((quantum_msg[0]==2)||(quantum_msg[0]==3))
	{
		/* for Single Touch input */
		if((id2.press+id3.press)!= 2)
		{
			/* case.1 - case 10010000 -> DETECT & MOVE */
			if( ( quantum_msg[1] & 0x90 ) == 0x90 )
			{
				REPORT( 1, size , x, y);
			}
			/* case.2 - 11000000 -> DETECT & PRESS */
			else if( ( quantum_msg[1] & 0xC0 ) == 0xC0  ) 
			{
				touch_message_flag = 1;

				if (quantum_msg[0] % 2)
					id3.press = 1;   // for ID=3
				else
					id2.press = 1;	 // for ID=2

				if ( (id2.press + id3.press) == 2 )
				{
					if ( quantum_msg[0] % 2)
					{
						REPORT( 1, size, tmp2.x, tmp2.y );
					}  // for ID=3
					else 
					{
						REPORT( 1, size, tmp3.x, tmp3.y );
					}  // for ID=2
					REPORT( 1, size, x, y );

					if (quantum_msg[0] % 2)
					{
						id2.x = tmp2.x;	// for ID=3
						id2.y = tmp2.y;
						id3.x = x;
						id3.y = y;
					}
					else
					{
						id2.x = x; 		// for ID=2
						id2.y = y;
						id3.x = tmp3.x;
						id3.y = tmp3.y;
					}
				}
				else
				{
					REPORT( 1, size, x, y );
				}
			}
			/* case.3 - case 00100000 -> RELEASE */
			else if( ((quantum_msg[1] & 0x20 ) == 0x20))   
			{
				REPORT( 0, 1, x, y );

				if (quantum_msg[0] % 2)
					id3.press = 0; 	// for ID=3
				else
					id2.press = 0;	// for ID=2
			}

			input_sync(qt602240->input_dev);

			if (quantum_msg[0] % 2)
			{
				tmp3.x = x; 	// for ID=3
				tmp3.y = y;
			}
			else
			{
				tmp2.x = x;		// for ID=2
				tmp2.y = y;
			}
		}
		/* for Two Touch input */
		else if( id2.press && id3.press )
		{
			if ( quantum_msg[0] % 2 )
			{
				id3.x = x; // for x, ID=3
				id3.y = y; // for y
			}
			else
			{
				id2.x = x; // for x, ID=2
				id2.y = y; // for y
			}

			if( ((quantum_msg[1] & 0x20 ) == 0x20))   // release
			{
				if (quantum_msg[0] % 2)
					id3.press = 0;	// for ID=3
				else
					id2.press = 0;	// for ID=2
			}	

			if ( id2.y && id3.y ) // report
			{
				REPORT( id2.press, size, id2.x, id2.y );
				REPORT( id3.press, size, id3.x, id3.y );

				input_sync(qt602240->input_dev);

				id2.y = 0;  // clear
				id3.y = 0;
			}
			else if ((quantum_msg[0]==3)  // report one real value and one dummy value
					|| (quantum_msg[0]==2 && check_flag) // id2 is reported twice
					|| ((quantum_msg[1] & 0x20) == 0x20 )) // any one is released
			{
				if( id3.y == 0 )
				{
					REPORT( id2.press , size , id2.x, id2.y);
					REPORT( id3.press , size , tmp3.x, tmp3.y);
					input_sync(qt602240->input_dev);
					id2.y = 0;  // clear
				}
				else if( id2.y == 0 )
				{
					REPORT( id2.press , size , tmp2.x, tmp2.y);
					REPORT( id3.press , size , id3.x, id3.y);
					input_sync(qt602240->input_dev);
					id3.y = 0; // clear
				}
			}
			else if ( quantum_msg[0] == 2 ) // for ID=2
			{
				check_flag=1;
			}

			if ( quantum_msg[0] % 2 ) 
			{
				tmp3.x = x;	 	// for ID=3
				tmp3.y = y;	
				check_flag=0; 
			}
			else
			{
				tmp2.x = x;	   	// for ID=2
				tmp2.y = y;
			}
		}
	}
	/* case.4 - Palm Touch & Unknow sate */
	else if ( quantum_msg[0] == 14 )
	{
		if((quantum_msg[1]&0x01) == 0x00)   
		{ 
			printk("[TSP] Palm Touch! - %d released, Ver. %x\n", quantum_msg[1], info_block->info_id.version );	

			id2.press=0;  // touch history clear
			id3.press=0;

		}
		else
		{
			printk("[TSP] Palm Touch! - %d suppressed\n", quantum_msg[1]);	
			touch_message_flag = 1;
		}
	}	
	else if ( quantum_msg[0] == 1 )
	{
		if ( quantum_msg[1] & 0x10 )
			printk("[TSP] Calibrating : msg[0] = %d, msg[1] = %d\n", quantum_msg[0], quantum_msg[1] );
		else	
			printk("[TSP] msg[0] = %d, msg[1] = %d\n", quantum_msg[0], quantum_msg[1] );

	}
	else if ( quantum_msg[0] == 0 )
	{
		printk("[TSP] Error : %d - What happen to TSP chip?\n", __LINE__ );

		touch_hw_rst( 4 );  // TOUCH HW RESET No.4
	}
	else
	{
		printk("[TSP] Dump state, msg[0]=%d, [1]=%d\n", quantum_msg[0],  quantum_msg[1] );		
	}

	/* if chip has recently calibrated we must confirm if the cal is good
	 * in case of calibration with a palm on the screen */
	if(touch_message_flag && cal_check_flag)
	{ 
		/* This function is for Ver 1.5 (21) and 1.6 (22) */
		if ( info_block->info_id.version >= 21 )
			check_chip_calibration();
	}

	memset( quantum_msg ,0x00, READ_MESSAGE_LENGTH); // clear
	enable_irq(qt602240->client->irq);
}

/*!
 * \brief Resets the chip.
 * 
 *  This function will send a reset command to touch chip.
 *
 * @return WRITE_MEM_OK if writing the command to touch chip was successful.
 * 
 */
uint8_t reset_chip(void)
{
	uint8_t data = 1u;
	int ret = write_mem(command_processor_address + RESET_OFFSET, 1, &data);
	//		msleep(65);
	return ret;
}

/*!
 * \brief Backup the chip.
 * 
 *  This function will send a backup command to touch chip.
 *
 * @return WRITE_MEM_OK if writing the command to touch chip was successful.
 * 
 */
uint8_t backup_chip(void)
{
	uint8_t data = 0x55u;
	return(write_mem(command_processor_address + BACKUP_OFFSET, 1, &data));
}

/*!
 * \brief Calibrates the chip.
 * 
 * This function will send a calibrate command to touch chip.
 * Whilst calibration has not been confirmed as good, this function will set
 * the ATCHCALST and ATCHCALSTHR to zero to allow a bad cal to always recover
 * 
 * @return WRITE_MEM_OK if writing the command to touch chip was successful.
 * 
 */
uint8_t calibrate_chip(void)
{
	uint8_t data = 1u;
	int ret = WRITE_MEM_OK;
	uint8_t atchcalst, atchcalsthr;


	/* change calibration suspend settings to zero until calibration confirmed good */
	/* store normal settings */
	atchcalst = config_normal.acquisition_config.atchcalst;
	atchcalsthr = config_normal.acquisition_config.atchcalsthr;

	/* resume calibration must be performed with zero settings */
	config_normal.acquisition_config.atchcalst = 0;
	config_normal.acquisition_config.atchcalsthr = 0; 

	printk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n", config_normal.acquisition_config.atchcalst, config_normal.acquisition_config.atchcalsthr );

	/* Write temporary acquisition config to chip. */
	if (write_acquisition_config(config_normal.acquisition_config) != CFG_WRITE_OK)
	{
		/* "Acquisition config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		ret = WRITE_MEM_FAILED; /* calling function should retry calibration call */
	}

	/* restore settings to the local structure so that when we confirm the 
	 * cal is good we can correct them in the chip */
	/* this must be done before returning */
	config_normal.acquisition_config.atchcalst = atchcalst;
	config_normal.acquisition_config.atchcalsthr = atchcalsthr;


	/* send calibration command to the chip */
	if(ret == WRITE_MEM_OK)
	{
		/* change calibration suspend settings to zero until calibration confirmed good */
		ret = write_mem(command_processor_address + CALIBRATE_OFFSET, 1, &data);

		/* set flag for calibration lockup recovery if cal command was successful */
		if(ret == WRITE_MEM_OK)
		{ 
			/* set flag to show we must still confirm if calibration was good or bad */
			cal_check_flag = 1u;
		}
	}



	msleep(120);

	return ret;
}


/*!
 * \brief Used to ensure that calibration was good
 *
 * This function will check if the last calibration was good.
 * 
 * It should be called on every touch message whilst 'cal_check_flag' is set,
 * it will recalibrate the chip if the calibration is bad. If the calibration
 * is good it will restore the ATCHCALST and ATCHCALSTHR settings in the chip 
 * so that we do not get water issues.
 *
 *
 */

void check_chip_calibration(void)
{
	uint8_t data_buffer[100] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0xF3; /* dianostic command to get touch flags */
	uint16_t diag_address;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t check_mask;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit;


	/* we have had the first touchscreen or face suppression message 
	 * after a calibration - check the sensor state and try to confirm if
	 * cal was good or bad */

	/* get touch flags from the chip using the diagnostic object */
	/* write command to command processor to get touch flags - 0xF3 Command required to do this */
	write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);
	/* get the address of the diagnostic object so we can get the data we need */
	diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0);

	msleep(5); 

	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );

	/* wait for diagnostic object to update */
	while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)))
	{
		/* wait for data to be valid  */
		if(try_ctr > 100)
		{
			/* Failed! */
			printk("[TSP] Diagnostic Data did not update!!\n");
			diag_err++;

			if ( diag_err > 5 )	
			{
				touch_hw_rst( 11 );  // TOUCH HW RESET No.11
				diag_err=0;	
			}	
			break;
		}
		msleep(2); 
		try_ctr++; /* timeout counter */
		read_mem(diag_address, 2,data_buffer);
//		printk("[TSP] Waiting for diagnostic data to update, try %d\n", try_ctr);
	}


	/* data is ready - read the detection flags */
	read_mem(diag_address, 82,data_buffer);

	/* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/

	/* count up the channels/bits if we recived the data properly */
	if((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))
	{

		/* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4.*/
		x_line_limit = 16 + config_normal.cte_config.mode;
		if(x_line_limit > 20)
		{
			/* hard limit at 20 so we don't over-index the array */
			x_line_limit = 20;
		}

		/* double the limit as the array is in bytes not words */
		x_line_limit = x_line_limit << 1;

		/* count the channels and print the flags to the log */
		for(i = 0; i < x_line_limit; i+=2) /* check X lines - data is in words so increment 2 at a time */
		{
			/* print the flags to the log - only really needed for debugging */
			//printk("[TSP] Detect Flags X%d, %x%x, %x%x \n", i>>1,data_buffer[3+i],data_buffer[2+i],data_buffer[43+i],data_buffer[42+i]);

			/* count how many bits set for this row */
			for(j = 0; j < 8; j++)
			{
				/* create a bit mask to check against */
				check_mask = 1 << j;

				/* check detect flags */
				if(data_buffer[2+i] & check_mask)
				{
					tch_ch++;
				}
				if(data_buffer[3+i] & check_mask)
				{
					tch_ch++;
				}

				/* check anti-detect flags */
				if(data_buffer[42+i] & check_mask)
				{
					atch_ch++;
				}
				if(data_buffer[43+i] & check_mask)
				{
					atch_ch++;
				}
			}
		}


		/* print how many channels we counted */
//		printk("[TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

		/* send page up command so we can detect when data updates next time,
		 * page byte will sit at 1 until we next send F3 command */
		data_byte = 0x01;
		write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);

		cal_depth++;
//		printk("[TSP] cal try : %d\n", cal_depth );

		/* process counters and decide if we must re-calibrate or if cal was good */      
		if((tch_ch) && (atch_ch == 0))
		{
			printk("[TSP] calibration was good, saved : %d\n", cal_depth );
			/* cal was good - don't need to check any more */
			cal_check_flag = 0;

//			printk("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n", config_normal.acquisition_config.atchcalst, config_normal.acquisition_config.atchcalsthr );
			/* Write normal acquisition config back to the chip. */
			if (write_acquisition_config(config_normal.acquisition_config) != CFG_WRITE_OK)
			{
				/* "Acquisition config write failed!\n" */
				printk("\n[TSP][ERROR] line : %d\n", __LINE__);
				// MUST be fixed
			}

			if ( cal_depth >= 100 )
				result_cal[0]++;	
			else
				result_cal[cal_depth]++;	

		}
		else if((tch_ch + CAL_THR /*10*/ ) <= atch_ch)
		{
			printk("[TSP] calibration was bad\n");
			/* cal was bad - must recalibrate and check afterwards */
			calibrate_chip();
		}
		else
		{
			printk("[TSP] calibration was not decided yet\n");
			/* we cannot confirm if good or bad - we must wait for next touch 
			 * message to confirm */
		}
	}

}

/*!
 * \brief Backups config area.
 * 
 * This function will send a command to backup the configuration to
 * non-volatile memory.
 * 
 * @return WRITE_MEM_OK if writing the command to touch chip was successful.
 * 
 */
uint8_t backup_config(void)
{
	/* Write 0x55 to BACKUPNV register to initiate the backup. */
	uint8_t data = 0x55u;

	return(write_mem(command_processor_address + BACKUP_OFFSET, 1, &data));
}

uint8_t report_all(void)
{
	uint8_t data = 1u;
	int ret = write_mem(command_processor_address + REPORTATLL_OFFSET, 1, &data);

	msleep(20);
	return ret;
}

/*!
 * \brief Reads the id part of info block.
 * 
 * Reads the id part of the info block (7 bytes) from touch IC to 
 * info_block struct.
 *
 */
uint8_t read_id_block(info_id_t *id)
{
	uint8_t status;

	printk("[QT] before read id->family_id : 0x%x\n",id->family_id);
	id->family_id = 0x0;
	status = read_mem(0, 1, (void *) &id->family_id);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] family_id = 0x%x\n\n", id->family_id);
	}

	status = read_mem(1, 1, (void *) &id->variant_id);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] variant_id = 0x%x\n\n", id->variant_id);
	}

	status = read_mem(2, 1, (void *) &id->version);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] version = 0x%x\n\n", id->version);
	}

	status = read_mem(3, 1, (void *) &id->build);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] build = 0x%x\n\n", id->build);
	}

	status = read_mem(4, 1, (void *) &id->matrix_x_size);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] matrix_x_size = 0x%x\n\n", id->matrix_x_size);
	}

	status = read_mem(5, 1, (void *) &id->matrix_y_size);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] matrix_y_size = 0x%x\n\n", id->matrix_y_size);
	}

	status = read_mem(6, 1, (void *) &id->num_declared_objects);
	if (status != READ_MEM_OK)
	{
		return(status);
	}else
	{
		printk("[TSP] num_declared_objects = 0x%x\n\n", id->num_declared_objects);
	}

	return(status);
}

/*!
 * \brief  This function retrieves the FW version number.
 * 
 * This function retrieves the FW version number.
 * 
 * @param *version pointer where to save version data.
 * @return ID_DATA_OK if driver is correctly initialized and 
 * id data can be read, ID_DATA_NOT_AVAILABLE otherwise.
 * 
 */
uint8_t get_version(uint8_t *version)
{
	if (info_block)
	{
		*version = info_block->info_id.version;
	}
	else
	{
		return(ID_DATA_NOT_AVAILABLE);
	}

	return (ID_DATA_OK);
}

/*!
 * \brief  This function retrieves the FW family id number.
 * 
 * This function retrieves the FW family id number.
 * 
 * @param *family_id pointer where to save family id data.
 * @return ID_DATA_OK if driver is correctly initialized and 
 * id data can be read, ID_DATA_NOT_AVAILABLE otherwise.
 * 
 */
uint8_t get_family_id(uint8_t *family_id)
{
	if (info_block)
	{
		*family_id = info_block->info_id.family_id;
	}
	else
	{
		return(ID_DATA_NOT_AVAILABLE);
	}

	return (ID_DATA_OK);
}

/*!
 * \brief  This function retrieves the FW build number.
 * 
 * This function retrieves the FW build number.
 * 
 * @param *build pointer where to save the build number data.
 * @return ID_DATA_OK if driver is correctly initialized and 
 * id data can be read, ID_DATA_NOT_AVAILABLE otherwise.
 * 
 */
uint8_t get_build_number(uint8_t *build)
{
	if (info_block)
	{
		*build = info_block->info_id.build;
	}
	else
	{
		return(ID_DATA_NOT_AVAILABLE);
	}

	return (ID_DATA_OK);
}

/*!
 * \brief  This function retrieves the FW variant number.
 * 
 * This function retrieves the FW variant id number.
 * 
 * @param *variant pointer where to save the variant id number data.
 * @return ID_DATA_OK if driver is correctly initialized and 
 * id data can be read, ID_DATA_NOT_AVAILABLE otherwise.
 * 
 */
uint8_t get_variant_id(uint8_t *variant)
{
	if (info_block)
	{
		*variant = info_block->info_id.variant_id;
	}
	else
	{
		return(ID_DATA_NOT_AVAILABLE);
	}

	return (ID_DATA_OK);
}

/*!
 * \brief Writes multitouchscreen config. 
 * 
 * 
 * This function will write the given configuration to given multitouchscreen
 * instance number.
 * 
 * @param instance the instance number of the multitouchscreen.
 * @param cfg multitouchscreen config struct.
 * @return 1 if successful.
 * 
 */
uint8_t write_multitouchscreen_config(uint8_t instance, 
		touch_multitouchscreen_t9_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;
	object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}


	/* 18 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 18);

	/* Next two are 2 bytes. */

	*(tmp + 18) = (uint8_t) (cfg.xres &  0xFF);
	*(tmp + 19) = (uint8_t) (cfg.xres >> 8);

	*(tmp + 20) = (uint8_t) (cfg.yres &  0xFF);
	*(tmp + 21) = (uint8_t) (cfg.yres >> 8);

	/* And the last 4 1 bytes each again. */

	*(tmp + 22) = cfg.xloclip;
	*(tmp + 23) = cfg.xhiclip;
	*(tmp + 24) = cfg.yloclip;
	*(tmp + 25) = cfg.yhiclip;

	*(tmp + 26) = cfg.xedgectrl;
	*(tmp + 27) = cfg.xedgedist;
	*(tmp + 28) = cfg.yedgectrl;
	*(tmp + 29) = cfg.yedgedist;

	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}


/*!
 * \brief Writes GPIO/PWM config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg GPIOPWM config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_gpio_config(uint8_t instance, 
		spt_gpiopwm_t19_config_t cfg)
{
	return(write_simple_config(SPT_GPIOPWM_T19, instance, (void *) &cfg));
}

/*!
 * \brief Writes GPIO/PWM config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg GPIOPWM config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_linearization_config(uint8_t instance, 
		proci_linearizationtable_t17_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;   
	uint8_t object_size;

	object_size = get_object_size(PROCI_LINEARIZATIONTABLE_T17);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);

	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}


	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = (uint8_t) (cfg.xoffset & 0x00FF);
	*(tmp + 2) = (uint8_t) (cfg.xoffset >> 8);

	memcpy((tmp+3), &cfg.xsegment, 16);

	*(tmp + 19) = (uint8_t) (cfg.yoffset & 0x00FF);
	*(tmp + 20) = (uint8_t) (cfg.yoffset >> 8);

	memcpy((tmp+21), &cfg.ysegment, 16);

	object_address = get_object_address(PROCI_LINEARIZATIONTABLE_T17, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}

/*!
 * \brief Writes two touch gesture processor config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg two touch gesture config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_twotouchgesture_config(uint8_t instance, 
		proci_twotouchgestureprocessor_t27_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;   
	uint8_t object_size;

	object_size = get_object_size(PROCI_TWOTOUCHGESTUREPROCESSOR_T27);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);

	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = 0;

	*(tmp + 2) = 0;
	*(tmp + 3) = cfg.gesten; 

	*(tmp + 4) = cfg.rotatethr;

	*(tmp + 5) = (uint8_t) (cfg.zoomthr & 0x00FF);
	*(tmp + 6) = (uint8_t) (cfg.zoomthr >> 8);

	object_address = get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}


/*!
 * \brief Writes one touch gesture processor config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg one touch gesture config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_onetouchgesture_config(uint8_t instance, 
		proci_onetouchgestureprocessor_t24_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;   
	uint8_t object_size;

	object_size = get_object_size(PROCI_ONETOUCHGESTUREPROCESSOR_T24);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}


	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.numgest;

	*(tmp + 2) = (uint8_t) (cfg.gesten & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.gesten >> 8);

	memcpy((tmp+4), &cfg.pressproc, 7);

	*(tmp + 11) = (uint8_t) (cfg.flickthr & 0x00FF);
	*(tmp + 12) = (uint8_t) (cfg.flickthr >> 8);

	*(tmp + 13) = (uint8_t) (cfg.dragthr & 0x00FF);
	*(tmp + 14) = (uint8_t) (cfg.dragthr >> 8);

	*(tmp + 15) = (uint8_t) (cfg.tapthr & 0x00FF);
	*(tmp + 16) = (uint8_t) (cfg.tapthr >> 8);

	*(tmp + 17) = (uint8_t) (cfg.throwthr & 0x00FF);
	*(tmp + 18) = (uint8_t) (cfg.throwthr >> 8);

	object_address = get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);
	//   kfree(tmp);
	return(status);
}

/*!
 * \brief Writes selftest config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg selftest config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_selftest_config(uint8_t instance, 
		spt_selftest_t25_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;   
	uint8_t object_size;

	object_size = get_object_size(SPT_SELFTEST_T25);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);


	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.cmd;

	*(tmp + 2) = (uint8_t) (cfg.upsiglim & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.upsiglim >> 8);

	*(tmp + 2) = (uint8_t) (cfg.losiglim & 0x00FF);
	*(tmp + 3) = (uint8_t) (cfg.losiglim >> 8);

	printk("[TSP] %s, %d\n", __func__, cfg.ctrl );
	printk("[TSP] %s, %d\n", __func__, cfg.cmd);
	printk("[TSP] %s, %d\n", __func__, cfg.upsiglim);
	printk("[TSP] %s, %d\n", __func__, cfg.losiglim);

	object_address = get_object_address(SPT_SELFTEST_T25, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}

/*!
 * \brief Writes key array config. 
 * 
 * @param instance the instance number which config to write.
 * @param cfg key array config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_keyarray_config(uint8_t instance, 
		touch_keyarray_t15_config_t cfg)
{
	return(write_simple_config(TOUCH_KEYARRAY_T15, instance, (void *) &cfg));
}

/*!
 * \brief Writes grip suppression config. 
 * 
 * @param instance the instance number indicating which config to write.
 * @param cfg grip suppression config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_gripsuppression_config(uint8_t instance, 
		proci_gripfacesuppression_t20_config_t cfg)
{
	return(write_simple_config(PROCI_GRIPFACESUPPRESSION_T20, instance, 
				(void *) &cfg));
}

/*!
 * \brief Writes a simple config struct to touch chip.
 *
 * Writes a simple config struct to touch chip. Does not necessarily 
 * (depending on how structs are stored on host platform) work for 
 * configs that have multibyte variables, so those are handled separately.
 * 
 * @param object_type object id number.
 * @param instance object instance number.
 * @param cfg pointer to config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_simple_config(uint8_t object_type, 
		uint8_t instance, void *cfg)
{
	uint16_t object_address;
	uint8_t object_size;

	object_address = get_object_address(object_type, instance);
	object_size = get_object_size(object_type);

	if ((object_size == 0) || (object_address == 0))
	{
		return(CFG_WRITE_FAILED);
	}

	return (write_mem(object_address, object_size, cfg));
}


/*!
 * \brief Writes power config. 
 * 
 * @param cfg power config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_power_config(gen_powerconfig_t7_config_t cfg)
{
	return(write_simple_config(GEN_POWERCONFIG_T7, 0, (void *) &cfg));
}


/*!
 * \brief Writes noise suppression config.
 *
 * @param instance the instance number indicating which config to write.
 * @param cfg noise suppression config struct.
 *
 * @return 1 if successful.
 *
 */

uint8_t write_noisesuppression_config(uint8_t instance,
		procg_noisesuppression_t22_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;
	object_size = get_object_size(PROCG_NOISESUPPRESSION_T22);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}

	*(tmp + 0) = (uint8_t) cfg.ctrl;
	*(tmp + 1) = (uint8_t) cfg.reserved;
	*(tmp + 2) = (uint8_t) cfg.reserved1;
	*(tmp + 3) = (uint8_t) (cfg.gcaful &  0xFF);
	*(tmp + 4) = (uint8_t) (cfg.gcaful >> 8);

	*(tmp + 5) = (uint8_t) (cfg.gcafll &  0xFF);
	*(tmp + 6) = (uint8_t) (cfg.gcafll >> 8);

	*(tmp + 7) = cfg.actvgcafvalid;
	*(tmp + 8) = cfg.noisethr;
	*(tmp + 9) = cfg.reserved2;
	*(tmp + 10) = cfg.freqhopscale;

	*(tmp + 11) = cfg.freq[0];
	*(tmp + 12) = cfg.freq[1];
	*(tmp + 13) = cfg.freq[2];
	*(tmp + 14) = cfg.freq[3];
	*(tmp + 15) = cfg.freq[4];

	*(tmp + 16) = cfg.idlegcafvalid;

	object_address = get_object_address(PROCG_NOISESUPPRESSION_T22, 
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}

/*!
 * \brief Writes CTE config. 
 * 
 * Writes CTE config, assumes that there is only one instance of 
 * CTE config objects.
 *
 * @param cfg CTE config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_CTE_config(spt_cteconfig_t28_config_t cfg)
{
	return(write_simple_config(SPT_CTECONFIG_T28, 0, (void *) &cfg));
}


/*!
 * \brief Writes acquisition config. 
 * 
 * @param cfg acquisition config struct.
 * 
 * @return 1 if successful.
 * 
 */
uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t cfg)
{
	return(write_simple_config(GEN_ACQUISITIONCONFIG_T8, 0, (void *) &cfg));
}



uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(TOUCH_PROXIMITY_T23);
	if (object_size == 0)
	{
		return(CFG_WRITE_FAILED);
	}
	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
	{
		return(CFG_WRITE_FAILED);
	}

	memset(tmp,0,object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.xorigin;
	*(tmp + 2) = cfg.yorigin;
	*(tmp + 3) = cfg.xsize;
	*(tmp + 4) = cfg.ysize;
	*(tmp + 5) = cfg.reserved_for_future_aks_usage;
	*(tmp + 6) = cfg.blen;

	*(tmp + 7) = (uint8_t) (cfg.tchthr & 0x00FF);
	*(tmp + 8) = (uint8_t) (cfg.tchthr >> 8);

	*(tmp + 9) = cfg.tchdi;
	*(tmp + 10) = cfg.average;

	*(tmp + 11) = (uint8_t) (cfg.rate & 0x00FF);
	*(tmp + 12) = (uint8_t) (cfg.rate >> 8);

	object_address = get_object_address(TOUCH_PROXIMITY_T23,
			instance);

	if (object_address == 0)
	{
		return(CFG_WRITE_FAILED);
	}

	status = write_mem(object_address, object_size, tmp);

	return(status);
}


/*!
 * \brief Returns the start address of the selected object.
 * 
 * Returns the start address of the selected object and instance 
 * number in the touch chip memory map.  
 *
 * @param object_type the object ID number.
 * @param instance the instance number of the object.
 * 
 * @return object address, or OBJECT_NOT_FOUND if object/instance not found.
 * 
 */
uint16_t get_object_address(uint8_t object_type, uint8_t instance)
{
	object_t obj;
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = OBJECT_NOT_FOUND;

	object_t *object_table;
	object_table = info_block->objects;

	while ((object_table_index < info_block->info_id.num_declared_objects) && !address_found)
	{
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type)
		{

			address_found = 1;

			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance)
			{
				address = obj.i2c_address + (obj.size + 1) * instance;
			}
		}
		object_table_index++;
	}

	return(address);
}


/*!
 * \brief Returns the size of the selected object in the touch chip memory map.
 * 
 * Returns the size of the selected object in the touch chip memory map.
 *
 * @param object_type the object ID number.
 * 
 * @return object size, or OBJECT_NOT_FOUND if object not found.
 * 
 */
uint8_t get_object_size(uint8_t object_type)
{
	object_t obj;
	uint8_t object_table_index = 0;
	uint8_t object_found = 0;
	//uint16_t 
	uint8_t size = OBJECT_NOT_FOUND;

	object_t *object_table;
	object_table = info_block->objects;

	while ((object_table_index < info_block->info_id.num_declared_objects) &&
			!object_found)
	{
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type)
		{
			object_found = 1;
			size = obj.size + 1;
		}
		object_table_index++;
	}

	return(size);
}


/*!
 * \brief Return report id of given object/instance.
 * 
 *  This function will return a report id corresponding to given object type
 *  and instance, or 
 * 
 * @param object_type the object type identifier.
 * @param instance the instance of object.
 * 
 * @return report id, or 255 if the given object type does not have report id,
 * of if the firmware does not support given object type / instance.
 * 
 */
uint8_t type_to_report_id(uint8_t object_type, uint8_t instance)
{
	uint8_t report_id = 1;
	int8_t report_id_found = 0;

	while((report_id <= max_report_id) && (report_id_found == 0))
	{
		if((report_id_map[report_id].object_type == object_type) &&
				(report_id_map[report_id].instance == instance))
		{
			report_id_found = 1;
		}
		else
		{
			report_id++;	
		}
	}
	if (report_id_found)
	{
		return(report_id);
	}
	else
	{
		return(ID_MAPPING_FAILED);
	}
}

/*!
 * \brief Maps report id to object type and instance.
 * 
 *  This function will return an object type id and instance corresponding
 *  to given report id.
 * 
 * @param report_id the report id.
 * @param *instance pointer to instance variable. This function will set this
 *        to instance number corresponding to given report id.
 * @return the object type id, or 255 if the given report id is not used
 * at all.
 * 
 */
uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance)
{
	if (report_id <= max_report_id)
	{
		*instance = report_id_map[report_id].instance;
		return(report_id_map[report_id].object_type);
	}
	else
	{
		return(ID_MAPPING_FAILED);
	}
}

/*! 
 * \brief Return the maximum report id in use in the touch chip.
 * 
 * @return maximum_report_id 
 * 
 */
uint8_t get_max_report_id()
{
	return(max_report_id);
}


/*------------------------------ I2C Driver block -----------------------------------*/



int qt602240_i2c_write(u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret,i;

	if(len+2 > I2C_MAX_SEND_LENGTH)
	{
		printk("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = qt602240->client->addr;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len + 2;
	wmsg.buf = data;

	data[0] = reg & 0x00ff;
	data[1] = reg >> 8;

	for (i = 0; i < len; i++)
	{
		data[i+2] = *(read_val+i);
	}

	ret = i2c_transfer(qt602240->client->adapter, &wmsg, 1);
	if ( ret < 0 )
	{
		printk("[TSP] Error code : %d\n", __LINE__ );
		touch_hw_rst( 5 );  // TOUCH HW RESET No.5
		return -1;
	}

	return ret;
}

int qt602240_i2c_read(u16 reg,unsigned char *rbuf, int buf_size)
{
	struct i2c_msg rmsg;
	int ret=-1;
	unsigned char data[2];



	rmsg.addr = qt602240->client->addr;
	rmsg.flags = I2C_M_WR;
	rmsg.len = 2;
	rmsg.buf = data;
	data[0] = reg & 0x00ff;
	data[1] = reg >> 8;
	ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);

	//	printk("%s, %d, %d = ",__func__,data[0], data[1]);

	if(ret>=0) {
		rmsg.flags = I2C_M_RD;
		rmsg.len = buf_size;
		rmsg.buf = rbuf;
		ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);
	}
	else 
	{
		printk("[TSP] Error code : %d\n", __LINE__ );
		touch_hw_rst( 6 );  // TOUCH HW RESET No.6
		return -1;
	}

	return ret;
}
/* ------------------------- ????????????? -----------------*/
/*!
 * \brief Initializes the I2C interface.
 *
 * @param I2C_address_arg the touch chip I2C address.
 *
 */
U8 init_I2C(U8 I2C_address_arg)
{
	int status = 0;


	printk("[QT] %s start\n",__func__);   

	return (I2C_INIT_OK);
}

/*! \brief Maxtouch Memory read by I2C bus */
U8 read_mem(U16 start, U8 size, U8 *mem)
{
	//	char *read_buf;
	int ret;

	//   read_buf = (char *)kmalloc(size, GFP_KERNEL | GFP_ATOMIC);
	memset(mem,0xFF,size);
	ret = qt602240_i2c_read(start,mem,size);
	if(ret < 0) {
		printk("%s, %d : i2c read failed\n",__func__, __LINE__);
		return(READ_MEM_FAILED);
	} 

	return(READ_MEM_OK);
}

U8 read_U16(U16 start, U16 *mem)
{
	U8 tmp;
	U8 status;

	status = read_mem(start, 2, (U8 *) mem);

	return (status);
}

U8 write_mem(U16 start, U8 size, U8 *mem)
{
	int ret;

	ret = qt602240_i2c_write(start,mem,size);
	if(ret < 0) 
		return(WRITE_MEM_FAILED);
	else
		return(WRITE_MEM_OK);
}


/*
 * Message handler that is called by the touch chip driver when messages
 * are received.
 * 
 * This example message handler simply prints the messages as they are
 * received to USART1 port of EVK1011 board.
 */
void message_handler(U8 *msg, U8 length)
{  
	//usart_write_line(&AVR32_USART1, "Touch IC message: ");
	//write_message_to_usart(msg, length);
	//usart_write_line(&AVR32_USART1, "\n");
}

/*------------------------------ CRC calculation  block -----------------------------------*/
/*!
 * \brief Returns the stored CRC sum for the info block & object table area.
 *
 * @return the stored CRC sum for the info block & object table.
 *
 */
uint32_t get_stored_infoblock_crc()
{
	uint32_t crc;

	crc = (uint32_t) (((uint32_t) info_block->CRC_hi) << 16);
	crc = crc | info_block->CRC;
	return(crc);
}

/*!
 * \brief Calculates the CRC sum for the info block & object table area,
 * and checks it matches the stored CRC.
 *
 * Global interrupts need to be on when this function is called
 * since it reads the info block & object table area from the touch chip.
 *
 * @param *crc_pointer Pointer to memory location where
 *        the calculated CRC sum for the info block & object
 *        will be stored.
 * @return the CRC_CALCULATION_FAILED if calculation doesn't succeed, of
 *         CRC_CALCULATION_OK otherwise.
 *
 */
uint8_t calculate_infoblock_crc(uint32_t *crc_pointer)
{
	uint32_t crc = 0;
	//uint16_t 
	uint8_t crc_area_size;
	uint8_t *mem;
	uint8_t i;
	uint8_t status;
	/* 7 bytes of version data, 6 * NUM_OF_OBJECTS bytes of object table. */
	crc_area_size = 7 + info_block->info_id.num_declared_objects * 6;

	mem = (uint8_t *) kmalloc(crc_area_size, GFP_KERNEL | GFP_ATOMIC);
	if (mem == NULL)
	{
		return(CRC_CALCULATION_FAILED);
	}

	status = read_mem(0, crc_area_size, mem);

	if (status != READ_MEM_OK)
	{
		return(CRC_CALCULATION_FAILED);
	}

	i = 0;
	while (i < (crc_area_size - 1))
	{
		crc = CRC_24(crc, *(mem + i), *(mem + i + 1));
		i += 2;
	}

	crc = CRC_24(crc, *(mem + i), 0);

	/* Return only 24 bit CRC. */
	*crc_pointer = (crc & 0x00FFFFFF);
	return(CRC_CALCULATION_OK);
}

/*!
 * \brief CRC calculation routine.
 *
 * @param crc the current CRC sum.
 * @param byte1 1st byte of new data to add to CRC sum.
 * @param byte2 2nd byte of new data to add to CRC sum.
 * @return crc the new CRC sum.
 *
 */
uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2)
{
	static const uint32_t crcpoly = 0x80001B;
	uint32_t result;
	uint16_t data_word;

	data_word = (uint16_t) ((uint16_t) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (uint32_t) data_word);

	if (result & 0x1000000)
	{
		result ^= crcpoly;
	}

	return(result);
}

irqreturn_t qt602240_irq_handler(int irq, void *dev_id)
{
	disable_irq(qt602240->client->irq);

#if 0 //def  CONFIG_CPU_FREQ
	set_dvfs_perf_level();
#endif

	queue_work(qt602240_wq, &qt602240->work);
	return IRQ_HANDLED;
}

void touch_hw_rst( int point)
{
	printk("[TSP] %s! : %d\n", __func__, __LINE__);
	uint8_t status;
	uint8_t build;   
	static int not_one_time=0;

	id2.press=0;  // touch history clear
	id3.press=0;

	rst_cnt[point]++;
	rst_cnt[0]=point;

	printk("[TSP] Dump state, last rst=%d\n#1=%d,#2=%d,#3=%d,#4=%d,#5=%d,#6=%d,#7=%d,#8=%d,#9=%d,#10=%d,#11=%d\n",
			rst_cnt[0],rst_cnt[1],rst_cnt[2],rst_cnt[3],rst_cnt[4],rst_cnt[5],
			rst_cnt[6],rst_cnt[7],rst_cnt[8],rst_cnt[9],rst_cnt[10],rst_cnt[11] );

	mdelay(20);
	gpio_set_value(GPIO_TOUCH_LDO_EN, 0);  // TOUCH EN
	gpio_set_value(GPIO_TOUCH_SCL , 0); 
	gpio_set_value(GPIO_TOUCH_SDA , 0); 
	gpio_set_value(GPIO_TOUCH_IRQ , 0); 
	gpio_set_value(GPIO_TOUCH_RST, 0); 

	mdelay(20);
	gpio_set_value(GPIO_TOUCH_LDO_EN, 1);  // TOUCH EN
	gpio_set_value(GPIO_TOUCH_IRQ , 1); 
	gpio_set_value(GPIO_TOUCH_RST, 1); 
	msleep(100);	// at least 40 msec recommended by manual Docu. 

	if ( not_one_time )
	{
		status = read_mem(3, 1, (void *) &build);
		if (status != READ_MEM_OK)
		{
			printk("[TSP] Error, TSP fail hw_rst : %d\n", __LINE__ );
		}
		else
		{
			printk("[TSP] build = 0x%x\n", build);
		}
	}
	not_one_time=1;
}

int qt602240_probe(void)
{
	int ret = 0;
	DEBUG;
	printk("+-----------------------------------------+\n");
	printk("|  Quantrm Touch Driver Probe!            |\n");
	printk("+-----------------------------------------+\n");

	INIT_WORK(&qt602240->work, get_message );

	qt602240->input_dev = input_allocate_device();
	if (qt602240->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "qt602240_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	qt602240->input_dev->name = "qt602240_ts_input";
	set_bit(EV_SYN, qt602240->input_dev->evbit);
	set_bit(EV_KEY, qt602240->input_dev->evbit);
	set_bit(EV_ABS, qt602240->input_dev->evbit);
	input_set_abs_params(qt602240->input_dev, ABS_MT_POSITION_X, 0, 480, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_POSITION_Y, 0, 800, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(qt602240->input_dev, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);


	ret = input_register_device(qt602240->input_dev);
	if (ret) {
		printk(KERN_ERR "qt602240_probe: Unable to register %s input device\n", qt602240->input_dev->name);
		goto err_input_register_device_failed;
	}

	quantum_touch_probe();

	if (driver_setup != DRIVER_SETUP_OK)
	{
		printk("[TSP] %s, %d - Error:DRIVER_SETUP fail!!!\n");
		quantum_touch_probe();
		if (driver_setup != DRIVER_SETUP_OK)
			return -1;
		printk("[TSP] %s, %d DRIVER_SETUP success!!!\n");
	}

	ret = request_irq(qt602240->client->irq, qt602240_irq_handler, IRQF_DISABLED, "qt602240 irq", 0);
	if (ret == 0) {
		printk("[TSP] request touchscreen irq i- %s\n", qt602240->input_dev->name);
	}
	else {
		printk("request_irq failed\n");
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	qt602240->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	qt602240->early_suspend.suspend = qt602240_early_suspend;
	qt602240->early_suspend.resume = qt602240_late_resume;
	register_early_suspend(&qt602240->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	return 0;

err_input_register_device_failed:
	input_free_device(qt602240->input_dev);

err_input_dev_alloc_failed:
	kfree(qt602240);
	return ret;
}

//int qt602240_remove(struct i2c_client *client)
static int qt602240_remove(struct platform_device *aDevice)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&qt602240->early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
	free_irq(qt602240->client->irq, 0);
	input_unregister_device(qt602240->input_dev);
	kfree(qt602240);

	return 0;
}

static int qt602240_suspend(pm_message_t mesg)
{

	ENTER_FUNC;
	disable_irq(qt602240->client->irq);

	config_set_enable = 0; // 0 for disable, 1 for enable

	id2.press=0;  // touch history clear
	id3.press=0;

	gen_powerconfig_t7_config_t power_config_sleep = {0};

	/* Write power config to chip. */
	if (write_power_config(power_config_sleep) != CFG_WRITE_OK)
	{
		/* "Power config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
	}

	LEAVE_FUNC;
	return 0;
}

static int qt602240_resume(void)
{
	ENTER_FUNC;

	gen_powerconfig_t7_config_t power_config_sleep = {0};

	config_set_enable = 1; // 0 for disable, 1 for enable
	power_config_sleep.idleacqint = 100;
	power_config_sleep.actvacqint = 14;
	power_config_sleep.actv2idleto = 50;

	cal_depth=0;

	/* Write power config to chip. */
	if (write_power_config(power_config_sleep) != CFG_WRITE_OK)
	{
		/* "Power config write failed!\n" */
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		msleep(50);
		if (write_power_config(power_config_sleep) != CFG_WRITE_OK)
		{
			/* "Power config write failed!\n" */
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			touch_hw_rst( 7 );  // TOUCH HW RESET No.7
		}
	}

	/* Calibrate the touch IC. */
	if (calibrate_chip() != WRITE_MEM_OK)
	{
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		/* Calibrate the touch IC. */
		if (calibrate_chip() != WRITE_MEM_OK)
		{
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			touch_hw_rst( 8 );  // TOUCH HW RESET No.8
		}
	}

	if ( report_all() != WRITE_MEM_OK)
	{
		printk("\n[TSP][ERROR] line : %d\n", __LINE__);
		msleep(10);
		if (report_all() != WRITE_MEM_OK)
		{
			printk("\n[TSP][ERROR] line : %d\n", __LINE__);
			touch_hw_rst( 9 );  // TOUCH HW RESET No.9
		}
	}

	mdelay(20);
	if ( gpio_get_value(GPIO_TOUCH_IRQ) )
	{
		printk("[TSP] %d - INT is High!!\n", __LINE__ );
		mdelay(20);
		if ( gpio_get_value(GPIO_TOUCH_IRQ) )
		{	
			printk("[TSP] %d - INT is High!! Twice!\n", __LINE__ );
			mdelay(30);
			if ( gpio_get_value(GPIO_TOUCH_IRQ) )
			{	
				printk("[TSP] Error : %d - INT is High!! Three!\n", __LINE__ );
				touch_hw_rst( 10 );  // TOUCH HW RESET No.10
			}
		}
	}

	printk("[TSP] Dump state, last rst=%d\n#1=%d,#2=%d,#3=%d,#4=%d,#5=%d,#6=%d,#7=%d,#8=%d,#9=%d,#10=%d,#11=%d\n",
			rst_cnt[0],rst_cnt[1],rst_cnt[2],rst_cnt[3],rst_cnt[4],rst_cnt[5],
			rst_cnt[6],rst_cnt[7],rst_cnt[8],rst_cnt[9],rst_cnt[10],rst_cnt[11] );

	enable_irq(qt602240->client->irq);
	INT_clear( );

	LEAVE_FUNC;
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void qt602240_early_suspend(struct early_suspend *h)
{
	qt602240_suspend(PMSG_SUSPEND);
}

static void qt602240_late_resume(struct early_suspend *h)
{
	qt602240_resume();
}
#endif	// End of CONFIG_HAS_EARLYSUSPEND

unsigned short qt602240_i2c_normal[] = { I2C_CLIENT_END};
unsigned short qt602240_i2c_ignore[] = { I2C_CLIENT_END};
unsigned short qt602240_i2c_probe[] = {3, QT602240_I2C_ADDR, I2C_CLIENT_END};


struct i2c_client_address_data qt602240_addr_data = {
	.normal_i2c = qt602240_i2c_normal,
	.ignore = qt602240_i2c_ignore,
	.probe = qt602240_i2c_probe,
};

int qt602240_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if(!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));

	strcpy(c->name, "qt602240");
	c->addr = addr;
	c->adapter = adap;
	c->irq = IRQ_TOUCH_INT;
	c->driver = &qt602240_i2c_driver;

	if((ret = i2c_attach_client(c)) < 0)
		goto error;

	qt602240->client = c;
	printk("qt602240_i2c is attached..\n");
error:
	return ret;
}

int qt602240_attach_adapter(struct i2c_adapter *adap)
{
	int ret;
	ret = i2c_probe(adap, &qt602240_addr_data, qt602240_attach);

	if(ret) {
		printk("i2c_probe ERROR : %d\n",ret);
		return ret;
	}

	return 0;
}

struct i2c_driver qt602240_i2c_driver = {
	.driver = {
		.name	= "qt602240_i2c",
		.owner  = THIS_MODULE,
	},
	.attach_adapter = qt602240_attach_adapter,
};

void init_hw_setting(void)
{
	s3c_gpio_cfgpin(GPIO_TOUCH_IRQ, S3C_GPIO_SFN(GPIO_TOUCH_IRQ));
	s3c_gpio_setpull(GPIO_TOUCH_IRQ, S3C_GPIO_PULL_NONE); 

	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_EDGE_FALLING);
	//	set_irq_type(IRQ_TOUCH_INT, IRQ_TYPE_LEVEL_LOW);

	if (gpio_is_valid(GPIO_TOUCH_LDO_EN)) {
		if (gpio_request(GPIO_TOUCH_LDO_EN, S3C_GPIO_LAVEL(GPIO_TOUCH_LDO_EN)))
			printk(KERN_ERR "Filed to request GPIO_TOUCH_LDO_EN!\n");
		gpio_direction_output(GPIO_TOUCH_LDO_EN, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_TOUCH_LDO_EN, S3C_GPIO_PULL_NONE); 

	if (gpio_is_valid(GPIO_TOUCH_RST)) {
		if (gpio_request(GPIO_TOUCH_RST, S3C_GPIO_LAVEL(GPIO_TOUCH_RST)))
			printk(KERN_ERR "Filed to request GPIO_TOUCH_LDO_EN!\n");
		gpio_direction_output(GPIO_TOUCH_RST, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_TOUCH_RST, S3C_GPIO_PULL_NONE); 

}

struct platform_driver qt602240_driver =  {
	.probe	= qt602240_probe,
	.remove = qt602240_remove,
	.driver = {
		.name = "qt602240-ts",
		.owner	= THIS_MODULE,
	},
};

extern struct class *sec_class;
struct device *ts_dev;

static ssize_t i2c_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	int ret=-1;
	unsigned char read_buf[5];
	memset( read_buf ,0xFF, 5 );

	ret = qt602240_i2c_read(0,read_buf, 5);
	if (ret < 0) {
		printk("qt602240 i2c read failed.\n");
	}
	printk("qt602240 read: %x, %x, %x, %x, %x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3], read_buf[4]);

	return sprintf(buf, "%s\n", buf);
}

static ssize_t i2c_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("qt602240 GPIO Status\n");
	printk("TOUCH_EN  : %s\n", gpio_get_value(GPIO_TOUCH_LDO_EN)? "High":"Low");
	printk("TOUCH_RST : %s\n", gpio_get_value(GPIO_TOUCH_RST)? "High":"Low");
	printk("TOUCH_INT : %s\n", gpio_get_value(GPIO_TOUCH_IRQ)? "High":"Low");

	return sprintf(buf, "%s\n", buf);
}

static ssize_t gpio_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	if(strncmp(buf, "ENHIGH", 6) == 0 || strncmp(buf, "enhigh", 6) == 0) {
		gpio_set_value(GPIO_TOUCH_LDO_EN, GPIO_LEVEL_HIGH);
		printk("set TOUCH_EN High.\n");
		mdelay(100);
	}
	if(strncmp(buf, "ENLOW", 5) == 0 || strncmp(buf, "enlow", 5) == 0) {
		gpio_set_value(GPIO_TOUCH_LDO_EN, GPIO_LEVEL_LOW);
		printk("set TOUCH_EN Low.\n");
		mdelay(100);
	}

	if(strncmp(buf, "RSTHIGH", 7) == 0 || strncmp(buf, "rsthigh", 7) == 0) {
		gpio_set_value(GPIO_TOUCH_RST, GPIO_LEVEL_HIGH);
		printk("set TOUCH_RST High.\n");
		mdelay(100);
	}
	if(strncmp(buf, "RSTLOW", 6) == 0 || strncmp(buf, "rstlow", 6) == 0) {
		gpio_set_value(GPIO_TOUCH_RST, GPIO_LEVEL_LOW);
		printk("set TOUCH_RST Low.\n");
		mdelay(100);
	}

	return size;
}

static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{	// v1.2 = 18 , v1.4 = 20 , v1.5 = 21
	printk("[TSP] QT602240 Firmware Ver. %d\n", info_block->info_id.version);
	printk("[TSP] build = %d\n", info_block->info_id.build );

	sprintf(buf, "QT602240 Firmware Ver. %x\n", info_block->info_id.version);

	int tmp=0;

	for ( tmp=0; tmp<100 ; tmp++)
	{	
		printk("[TSP] result_cal[%d]=%d\n", tmp, result_cal[tmp] );	
	}
	printk("[TSP] diag_err=%d\n", diag_err );	

	return sprintf(buf, "%s", buf );
}

static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);

	if ( value == 1 )
	{
		printk("[TSP] Firmware update start!!\n" );
		printk("[TSP] version = %d\n", info_block->info_id.version);

		if (( info_block->info_id.version == 21 ) || ( info_block->info_id.version == 22 )) 
		{
			printk("[TSP] version = %d\n", firmware_ret_val );
			firmware_ret_val = QT_Boot();
			printk("[TSP] Firmware result = %d\n", firmware_ret_val );
			printk("[TSP] version = %d\n", info_block->info_id.version);
			printk("[TSP] build = %d\n", info_block->info_id.build );
		}	
		else
		{	
			printk("[TSP] version = %x\n", info_block->info_id.version );
			firmware_ret_val = 1; 
			printk("[TSP] Firmware result = %d\n", firmware_ret_val );
		}		

		return size;
	}
}

static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(i2c, S_IRUGO | S_IWUSR, i2c_show, i2c_store);
static DEVICE_ATTR(firmware	, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, firmware_show, firmware_store);

static ssize_t firmware1_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	printk("[TSP] QT602240 Firmware Ver. %d\n", info_block->info_id.version);
	printk("[TSP] build = %d\n", info_block->info_id.build );

	sprintf(buf, "QT602240 Build Ver. %x\n", info_block->info_id.build );

	return sprintf(buf, "%s", buf );
}

static ssize_t firmware1_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	char *after;

	printk("[TSP] %s - operate nothing\n", __FUNCTION__);

	return size;
}

static DEVICE_ATTR(firmware1, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, firmware1_show, firmware1_store);

static ssize_t firmware_ret_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	printk("QT602240 %s!\n", __func__);
	//	sprintf(buf, "%d", firmware_ret_val );

	return sprintf(buf, "%d", firmware_ret_val );
}

static ssize_t firmware_ret_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	printk("QT602240 %s, operate nothing!\n", __func__);

	return size;
}

static DEVICE_ATTR(firmware_ret	, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, firmware_ret_show, firmware_ret_store);



/*------------------------------ for tunning ATmel - start ----------------------------*/
static ssize_t set_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , power_config.idleacqint = %d\n", __func__,config_value );
		config_normal.power_config.idleacqint = config_value;
	}		
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , power_config.actvacqint = %d\n", __func__, config_value);
		config_normal.power_config.actvacqint = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , power_config.actv2idleto= %d\n", __func__, config_value);
		config_normal.power_config.actv2idleto = config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_power, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_power_show, set_power_store);

static ssize_t set_acquisition_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_acquisition_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , acquisition_config.chrgtime = %d\n", __func__, config_value );
		config_normal.acquisition_config.chrgtime = config_value;
	}		
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , acquisition_config.Reserved = %d\n", __func__, config_value );
		config_normal.acquisition_config.Reserved = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , acquisition_config.tchdrift = %d\n", __func__, config_value );
		config_normal.acquisition_config.tchdrift = config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , acquisition_config.driftst= %d\n", __func__, config_value );
		config_normal.acquisition_config.driftst = config_value;
	}else if(cmd_no == 4)
	{
		printk("[%s] CMD 4 , acquisition_config.tchautocal = %d\n", __func__, config_value );
		config_normal.acquisition_config.tchautocal= config_value;
	}
	else if(cmd_no == 5)
	{
		printk("[%s] CMD 5 , acquisition_config.sync = %d\n", __func__, config_value );
		config_normal.acquisition_config.sync = config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , acquisition_config.atchcalst = %d\n", __func__, config_value );
		config_normal.acquisition_config.atchcalst = config_value;
	}
	else if(cmd_no == 7)
	{
		printk("[%s] CMD 7 , acquisition_config.atchcalsthr = %d\n", __func__, config_value );
		config_normal.acquisition_config.atchcalsthr= config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}


	return size;
}
static DEVICE_ATTR(set_acquisition, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_acquisition_show, set_acquisition_store);

static ssize_t set_touchscreen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_touchscreen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , touchscreen_config.ctrl = %d\n", __func__, config_value );
		config_normal.touchscreen_config.ctrl = config_value;
	}		
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , touchscreen_config.xorigin  = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xorigin = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , touchscreen_config.yorigin = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yorigin  = config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , touchscreen_config.xsize = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xsize =  config_value;
	}
	else if(cmd_no == 4)
	{
		printk("[%s] CMD 4 , touchscreen_config.ysize = %d\n", __func__, config_value );
		config_normal.touchscreen_config.ysize =  config_value;
	}
	else if(cmd_no == 5)
	{
		printk("[%s] CMD 5 , touchscreen_config.akscfg = %d\n", __func__, config_value );
		config_normal.touchscreen_config.akscfg = config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , touchscreen_config.blen = %d\n", __func__, config_value );
		config_normal.touchscreen_config.blen = config_value;
	}
	else if(cmd_no == 7)
	{
		printk("[%s] CMD 7 , touchscreen_config.tchthr = %d\n", __func__, config_value );
		config_normal.touchscreen_config.tchthr = config_value;
	}
	else if(cmd_no == 8)
	{
		printk("[%s] CMD 8 , touchscreen_config.tchdi = %d\n", __func__, config_value );
		config_normal.touchscreen_config.tchdi= config_value;
	}
	else if(cmd_no == 9)
	{
		printk("[%s] CMD 9 , touchscreen_config.orientate = %d\n", __func__, config_value );
		config_normal.touchscreen_config.orientate = config_value;
	}
	else if(cmd_no == 10)
	{
		printk("[%s] CMD 10 , touchscreen_config.mrgtimeout = %d\n", __func__, config_value );
		config_normal.touchscreen_config.mrgtimeout = config_value;
	}
	else if(cmd_no == 11)
	{
		printk("[%s] CMD 11 , touchscreen_config.movhysti = %d\n", __func__, config_value );
		config_normal.touchscreen_config.movhysti = config_value;
	}
	else if(cmd_no == 12)
	{
		printk("[%s] CMD 12 , touchscreen_config.movhystn = %d\n", __func__, config_value );
		config_normal.touchscreen_config.movhystn = config_value;
	}
	else if(cmd_no == 13)
	{
		printk("[%s] CMD 13 , touchscreen_config.movfilter = %d\n", __func__, config_value );
		config_normal.touchscreen_config.movfilter = config_value;
	}
	else if(cmd_no == 14)
	{
		printk("[%s] CMD 14 , touchscreen_config.numtouch = %d\n", __func__, config_value );
		config_normal.touchscreen_config.numtouch = config_value;
	}
	else if(cmd_no == 15)
	{
		printk("[%s] CMD 15 , touchscreen_config.mrghyst = %d\n", __func__, config_value );
		config_normal.touchscreen_config.mrghyst = config_value;
	}
	else if(cmd_no == 16)
	{
		printk("[%s] CMD 16 , touchscreen_config.mrgthr = %d\n", __func__, config_value );
		config_normal.touchscreen_config.mrgthr = config_value;
	}
	else if(cmd_no == 17)
	{
		printk("[%s] CMD 17 , touchscreen_config.tchamphyst = %d\n", __func__, config_value );
		config_normal.touchscreen_config.tchamphyst = config_value;
	}
	else if(cmd_no == 18)
	{
		printk("[%s] CMD 18 , touchscreen_config.xres = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xres= config_value;
	}
	else if(cmd_no == 19)
	{
		printk("[%s] CMD 19 , touchscreen_config.yres = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yres = config_value;
	}
	else if(cmd_no == 20)
	{
		printk("[%s] CMD 20 , touchscreen_config.xloclip = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xloclip = config_value;
	}
	else if(cmd_no == 21)
	{
		printk("[%s] CMD 21 , touchscreen_config.xhiclip = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xhiclip = config_value;
	}
	else if(cmd_no == 22)
	{
		printk("[%s] CMD 22 , touchscreen_config.yloclip = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yloclip = config_value;
	}
	else if(cmd_no == 23)
	{
		printk("[%s] CMD 23 , touchscreen_config.yhiclip = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yhiclip = config_value;
	}
	else if(cmd_no == 24)
	{
		printk("[%s] CMD 24 , touchscreen_config.xedgectrl = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xedgectrl = config_value;
	}
	else if(cmd_no == 25)
	{
		printk("[%s] CMD 25 , touchscreen_config.xedgedist = %d\n", __func__, config_value );
		config_normal.touchscreen_config.xedgedist = config_value;
	}
	else if(cmd_no == 26)
	{
		printk("[%s] CMD 26 , touchscreen_config.yedgectrl = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yedgectrl = config_value;
	}
	else if(cmd_no == 27)
	{
		printk("[%s] CMD 27 , touchscreen_config.yedgedist = %d\n", __func__, config_value );
		config_normal.touchscreen_config.yedgedist = config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_touchscreen, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_touchscreen_show, set_touchscreen_store);

static ssize_t set_keyarray_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_keyarray_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , keyarray_config.ctrl = %d\n", __func__, config_value );
		config_normal.keyarray_config.ctrl = config_value;
	}		
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , keyarray_config.xorigin = %d\n", __func__, config_value );
		config_normal.keyarray_config.xorigin = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , keyarray_config.yorigin = %d\n", __func__, config_value );
		config_normal.keyarray_config.yorigin = config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , keyarray_config.xsize = %d\n", __func__, config_value );
		config_normal.keyarray_config.xsize = config_value;
	}
	else if(cmd_no == 4)
	{
		printk("[%s] CMD 4 , keyarray_config.ysize = %d\n", __func__, config_value );
		config_normal.keyarray_config.ysize  = config_value;
	}
	else if(cmd_no == 5)
	{
		printk("[%s] CMD 5 , keyarray_config.akscfg = %d\n", __func__, config_value );
		config_normal.keyarray_config.akscfg  = config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , keyarray_config.blen = %d\n", __func__, config_value );
		config_normal.keyarray_config.blen = config_value;
	}
	else if(cmd_no == 7)
	{
		printk("[%s] CMD 7 , keyarray_config.tchthr = %d\n", __func__, config_value );
		config_normal.keyarray_config.tchthr = config_value;
	}
	else if(cmd_no == 8)
	{
		printk("[%s] CMD 8 , keyarray_config.tchdi = %d\n", __func__, config_value );
		config_normal.keyarray_config.tchdi = config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_keyarray, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_keyarray_show, set_keyarray_store);

static ssize_t set_grip_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_grip_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , gripfacesuppression_config.ctrl = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.ctrl =  config_value;
	}
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , gripfacesuppression_config.xlogrip = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.xlogrip = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , gripfacesuppression_config.xhigrip = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.xhigrip =  config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , gripfacesuppression_config.ylogrip = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.ylogrip =  config_value;
	}
	else if(cmd_no == 4 )
	{
		printk("[%s] CMD 4 , gripfacesuppression_config.yhigrip = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.yhigrip =  config_value;
	}
	else if(cmd_no == 5 )
	{
		printk("[%s] CMD 5 , gripfacesuppression_config.maxtchs= %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.maxtchs =  config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , gripfacesuppression_config.RESERVED2 = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.RESERVED2=  config_value;
	}
	else if(cmd_no == 7)
	{
		printk("[%s] CMD 7 , gripfacesuppression_config.szthr1 = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.szthr1=  config_value;
	}
	else if(cmd_no == 8)
	{
		printk("[%s] CMD 8 , gripfacesuppression_config.szthr2 = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.szthr2=  config_value;
	}
	else if(cmd_no == 9)
	{
		printk("[%s] CMD 9 , gripfacesuppression_config.shpthr1 = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.shpthr1=  config_value;
	}
	else if(cmd_no == 10)
	{
		printk("[%s] CMD 10 , gripfacesuppression_config.shpthr2 = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.shpthr2 =  config_value;
	}
	else if(cmd_no == 11)
	{
		printk("[%s] CMD 11 , gripfacesuppression_config.supextto = %d\n", __func__, config_value );
		config_normal.gripfacesuppression_config.supextto =  config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_grip , S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_grip_show, set_grip_store);

static ssize_t set_noise_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_noise_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);

	if ( value > 100000 ){ 
		cmd_no = (int) (value / 100000 );
		config_value = ( int ) (value % 100000 );

		if( !( (cmd_no==1) || (cmd_no==2) ) ){   // this can be only gcaful, gcafll // CMD1, CMD2
			printk("[%s] unknown CMD\n", __func__);
			return size;	
		}
	}	
	else 
	{
		cmd_no = (int) (value / 1000);
		config_value = ( int ) (value % 1000 );

		if( ( (cmd_no==1) || (cmd_no==2) ) ){   // this can't be only gcaful, gcafll // CMD1, CMD2
			printk("[%s] unknown CMD\n", __func__);
			return size;	
		}
	}


	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , noise_suppression_config.ctrl = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.ctrl = config_value;
	}
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , noise_suppression_config.gcaful = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.gcaful = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , noise_suppression_config.gcafll= %d\n", __func__, config_value );
		config_normal.noise_suppression_config.gcafll = config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , noise_suppression_config.actvgcafvalid = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.actvgcafvalid = config_value;
	}
	else if(cmd_no == 4)
	{
		printk("[%s] CMD 4 , noise_suppression_config.noisethr = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.noisethr = config_value;
	}
	else if(cmd_no == 5)
	{
		printk("[%s] CMD 5 , noise_suppression_config.freqhopscale = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freqhopscale = config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , noise_suppression_config.freq[0] = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freq[0] = config_value;
	}
	else if(cmd_no == 7)
	{	printk("[%s] CMD 7 , noise_suppression_config.freq[1] = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freq[1] = config_value;
	}
	else if(cmd_no == 8)
	{
		printk("[%s] CMD 8 , noise_suppression_config.freq[2] = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freq[2] = config_value;
	}
	else if(cmd_no == 9)
	{		
		printk("[%s] CMD 9 , noise_suppression_config.freq[3] = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freq[3] = config_value;
	}
	else if(cmd_no == 10)
	{		
		printk("[%s] CMD 10 , noise_suppression_config.freq[4] = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.freq[4] = config_value;
	}
	else if(cmd_no == 11)
	{
		printk("[%s] CMD 11 , noise_suppression_config.idlegcafvalid = %d\n", __func__, config_value );
		config_normal.noise_suppression_config.idlegcafvalid = config_value;
	}
	else 
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_noise, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_noise_show, set_noise_store);

static ssize_t set_total_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;
	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t set_total_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);
	cmd_no = (int) (value / 1000);
	config_value = ( int ) (value % 1000 );

	if (cmd_no == 0)
	{
		printk("[%s] CMD 0 , linearization_config.ctrl = %d\n", __func__, config_value );
		config_normal.linearization_config.ctrl = config_value;
	}		
	else if(cmd_no == 1)
	{
		printk("[%s] CMD 1 , twotouch_gesture_config.ctrl = %d\n", __func__, config_value );
		config_normal.twotouch_gesture_config.ctrl = config_value;
	}
	else if(cmd_no == 2)
	{
		printk("[%s] CMD 2 , onetouch_gesture_config.ctrl = %d\n", __func__, config_value );
		config_normal.onetouch_gesture_config.ctrl = config_value;
	}
	else if(cmd_no == 3)
	{
		printk("[%s] CMD 3 , selftest_config.ctrl = %d\n", __func__, config_value );
		config_normal.selftest_config.ctrl = config_value;
	}
	else if(cmd_no == 4)
	{
		printk("[%s] CMD 4 , cte_config.ctrl = %d\n", __func__, config_value );
		config_normal.cte_config.ctrl = config_value;
	}
	else if(cmd_no == 5)
	{
		printk("[%s] CMD 5 , cte_config.cmd = %d\n", __func__, config_value );
		config_normal.cte_config.cmd = config_value;
	}
	else if(cmd_no == 6)
	{
		printk("[%s] CMD 6 , cte_config.mode = %d\n", __func__, config_value );
		config_normal.cte_config.mode = config_value;
	}
	else if(cmd_no == 7)
	{
		printk("[%s] CMD 7 , cte_config.idlegcafdepth = %d\n", __func__, config_value );
		config_normal.cte_config.idlegcafdepth = config_value;
	}
	else if(cmd_no == 8)
	{
		printk("[%s] CMD 8 , cte_config.actvgcafdepth = %d\n", __func__, config_value );
		config_normal.cte_config.actvgcafdepth = config_value;
	}
	else if(cmd_no == 9)
	{
		printk("[%s] CMD 9 , cte_config.temp = %d\n", __func__, config_value );
		config_normal.cte_config.temp = config_value;
	}
	else if(cmd_no == 10)
	{
		printk("[%s] CMD 10 , CAL_THR = %d\n", __func__, CAL_THR );
		CAL_THR = config_value;
	}
	else
	{
		printk("[%s] unknown CMD\n", __func__);
	}

	return size;
}
static DEVICE_ATTR(set_total, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_total_show, set_total_store);

static ssize_t set_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value,tmp=-1;

	printk("\n=========== [TSP] Configure SET for normal ============\n");
	printk("=== set_power - GEN_POWERCONFIG_T7 ===\n");
	printk("0. idleacqint=%3d, 1. actvacqint=%3d, 2. actv2idleto=%3d\n", config_normal.power_config.idleacqint, config_normal.power_config.actvacqint, config_normal.power_config.actv2idleto );  

	printk("=== set_acquisition - GEN_ACQUIRECONFIG_T8 ===\n");
	printk("0. chrgtime=%3d,   1. Reserved=%3d,   2. tchdrift=%3d\n",config_normal.acquisition_config.chrgtime,config_normal.acquisition_config.Reserved,config_normal.acquisition_config.tchdrift); 
	printk("3. driftst=%3d,    4. tchautocal=%3d, 5. sync=%3d\n", config_normal.acquisition_config.driftst,config_normal.acquisition_config.tchautocal,config_normal.acquisition_config.sync);
	printk("6. atchcalst=%3d,  7. atchcalsthr=%3d\n", config_normal.acquisition_config.atchcalst , config_normal.acquisition_config.atchcalsthr); 

	printk("=== set_touchscreen - TOUCH_MULTITOUCHSCREEN_T9 ===\n");
	printk("0. ctrl=%3d,       1. xorigin=%3d,    2. yorigin=%3d\n",  config_normal.touchscreen_config.ctrl, config_normal.touchscreen_config.xorigin, config_normal.touchscreen_config.yorigin  );
	printk("3. xsize=%3d,      4. ysize=%3d,      5. akscfg=%3d\n", config_normal.touchscreen_config.xsize, config_normal.touchscreen_config.ysize , config_normal.touchscreen_config.akscfg  );
	printk("6. blen=%3d,       7. tchthr=%3d,     8. tchdi=%3d\n", config_normal.touchscreen_config.blen, config_normal.touchscreen_config.tchthr, config_normal.touchscreen_config.tchdi );
	printk("9. orientate=%3d,  10.mrgtimeout=%3d, 11.movhysti=%3d\n",config_normal.touchscreen_config.orientate,config_normal.touchscreen_config.mrgtimeout, config_normal.touchscreen_config.movhysti);
	printk("12.movhystn=%3d,   13.movfilter=%3d,  14.numtouch=%3d\n",config_normal.touchscreen_config.movhystn, config_normal.touchscreen_config.movfilter ,config_normal.touchscreen_config.numtouch );
	printk("15.mrghyst=%3d,    16.mrgthr=%3d,     17.tchamphyst=%3d\n",config_normal.touchscreen_config.mrghyst,config_normal.touchscreen_config.mrgthr,config_normal.touchscreen_config.tchamphyst );
	printk("18.xres=%3d,       19.yres=%3d,       20.xloclip=%3d\n",config_normal.touchscreen_config.xres, config_normal.touchscreen_config.yres, config_normal.touchscreen_config.xloclip );
	printk("21.xhiclip=%3d,    22.yloclip=%3d,    23.yhiclip=%3d\n", config_normal.touchscreen_config.xhiclip, config_normal.touchscreen_config.yloclip ,config_normal.touchscreen_config.yhiclip );
	printk("24.xedgectrl=%3d,  25.xedgedist=%3d,  26.yedgectrl=%3d\n",config_normal.touchscreen_config.xedgectrl,config_normal.touchscreen_config.xedgedist,config_normal.touchscreen_config.yedgectrl);
	printk("27.yedgedist=%3d\n", config_normal.touchscreen_config.yedgedist );

	printk("=== set_keyarray - TOUCH_KEYARRAY_T15 ===\n");
	printk("0. ctrl=%3d,       1. xorigin=%3d,    2. yorigin=%3d\n", config_normal.keyarray_config.ctrl, config_normal.keyarray_config.xorigin, config_normal.keyarray_config.yorigin); 
	printk("3. xsize=%3d,      4. ysize=%3d,      5. akscfg=%3d\n", config_normal.keyarray_config.xsize, config_normal.keyarray_config.ysize, config_normal.keyarray_config.akscfg );
	printk("6. blen=%3d,       7. tchthr=%3d,     8. tchdi=%3d\n", config_normal.keyarray_config.blen, config_normal.keyarray_config.tchthr, config_normal.keyarray_config.tchdi  );

	printk("=== set_grip - PROCI_GRIPFACESUPRESSION_T20 ===\n");
	printk("0. ctrl=%3d,       1. xlogrip=%3d,    2. xhigrip=%3d\n",config_normal.gripfacesuppression_config.ctrl, config_normal.gripfacesuppression_config.xlogrip, config_normal.gripfacesuppression_config.xhigrip );
	printk("3. ylogrip=%3d     4. yhigrip=%3d,    5. maxtchs=%3d\n",config_normal.gripfacesuppression_config.ylogrip,config_normal.gripfacesuppression_config.yhigrip , config_normal.gripfacesuppression_config.maxtchs );
	printk("6. RESERVED2=%3d,  7. szthr1=%3d,     8. szthr2=%3d\n"  , config_normal.gripfacesuppression_config.RESERVED2 , config_normal.gripfacesuppression_config.szthr1, config_normal.gripfacesuppression_config.szthr2 );
	printk("9. shpthr1=%3d     10.shpthr2=%3d,    11.supextto=%3d\n",config_normal.gripfacesuppression_config.shpthr1 , config_normal.gripfacesuppression_config.shpthr2 , config_normal.gripfacesuppression_config.supextto );

	printk("=== set_noise ===\n");
	printk("0. ctrl = %3d,         1. gcaful(2bts)=%d\n", config_normal.noise_suppression_config.ctrl, config_normal.noise_suppression_config.gcaful); 
	printk("2. gcafll(2bts)=%5d, 3. actvgcafvalid =%d\n", config_normal.noise_suppression_config.gcafll ,  config_normal.noise_suppression_config.actvgcafvalid); 
	printk("4. noisethr=%3d,   5.freqhopscale=%3d,6. freq[0]=%3d\n", config_normal.noise_suppression_config.noisethr, config_normal.noise_suppression_config.freqhopscale, config_normal.noise_suppression_config.freq[0] ); 
	printk("7. freq[1]=%3d,    8. freq[2]=%3d,    9. freq[3]=%3d\n", config_normal.noise_suppression_config.freq[1],  config_normal.noise_suppression_config.freq[2] , config_normal.noise_suppression_config.freq[3]); 
	printk("10.freq[4]=%3d,    11.idlegcafvalid=%3d\n", config_normal.noise_suppression_config.freq[4] , config_normal.noise_suppression_config.idlegcafvalid); 

	printk("=== set_total ===\n");
	printk("0 , linearization_config.ctrl = %d\n", config_normal.linearization_config.ctrl );
	printk("1 , twotouch_gesture_config.ctrl = %d\n", config_normal.twotouch_gesture_config.ctrl );
	printk("2 , onetouch_gesture_config.ctrl = %d\n",config_normal.onetouch_gesture_config.ctrl );
	printk("3 , selftest_config.ctrl = %d\n", config_normal.selftest_config.ctrl);
	printk("4. cte_config.ctrl=%3d,  5. cte_config.cmd=%3d\n", config_normal.cte_config.ctrl, config_normal.cte_config.cmd );
	printk("6. cte_config.mode=%3d,  7. cte_config.idlegcafdepth=%3d\n", config_normal.cte_config.mode,  config_normal.cte_config.idlegcafdepth );
	printk("8. cte_config.actvgcafdepth=%3d,  9.cte_config.temp=%3d\n", config_normal.cte_config.actvgcafdepth, config_normal.cte_config.temp );
	printk("10.CAL_THR=%3d\n", CAL_THR );
	printk("================= end ======================\n");

	/* set all configuration */
	tmp = set_all_config( config_normal );

	if( tmp == 1 )
		printk("[TSP] set all configs success : %d\n", __LINE__);
	else
		printk("[TSP] set all configs fail : error : %d\n", __LINE__);

	return 0;
}
static ssize_t set_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret, cmd_no,config_value = 0;
	char *after;

	printk(KERN_INFO "[TSP] %s : operate nothing\n", __FUNCTION__);

	return size;
}
static DEVICE_ATTR(set_write, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_write_show, set_write_store);
/*------------------------------ for tunning ATmel - end ----------------------------*/


int __init qt602240_init(void)
{
	int ret;

	DEBUG;

	init_hw_setting();
	touch_hw_rst( 0 );  // TOUCH HW RESET

	qt602240 = kzalloc(sizeof(struct i2c_ts_driver), GFP_KERNEL);
	if (qt602240 == NULL) {
		return -ENOMEM;
	}

	ret = i2c_add_driver(&qt602240_i2c_driver);
	if(ret) 
		printk("[%s], i2c_add_driver failed...(%d)\n", __func__, ret);

	printk("[QT] ret : %d, qt602240->client name : %s\n",ret,qt602240->client->name);

	if(!(qt602240->client)) {
		printk("###################################################\n");
		printk("##                                               ##\n");
		printk("##    WARNING! TOUCHSCREEN DRIVER CAN'T WORK.    ##\n");
		printk("##    PLEASE CHECK YOUR TOUCHSCREEN CONNECTOR!   ##\n");
		printk("##                                               ##\n");
		printk("###################################################\n");
		i2c_del_driver(&qt602240_i2c_driver);
		return 0;
	}
	printk("[QT] %s/%d\n",__func__,__LINE__);
	qt602240_wq = create_singlethread_workqueue("qt602240_wq");
	if (!qt602240_wq)
		return -ENOMEM;

	ts_dev = device_create(sec_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_i2c) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_i2c.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware1) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware1.attr.name);
	if (device_create_file(ts_dev, &dev_attr_firmware_ret) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_firmware_ret.attr.name);
	printk("[QT] %s/%d, platform_driver_register!!\n",__func__,__LINE__);

	/*------------------------------ for tunning ATmel - start ----------------------------*/
	touch_class = class_create(THIS_MODULE, "touch");
	if (IS_ERR(touch_class))
		pr_err("Failed to create class(sec)!\n");

	switch_test = device_create(touch_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_test))
		pr_err("Failed to create device(switch)!\n");

	if (device_create_file(switch_test, &dev_attr_set_power) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_power.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_acquisition) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_acquisition.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_touchscreen) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_touchscreen.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_keyarray) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_keyarray.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_grip ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_grip.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_noise) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_noise.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_total ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_total.attr.name);
	if (device_create_file(switch_test, &dev_attr_set_write ) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_set_write.attr.name);
	/*------------------------------ for tunning ATmel - end ----------------------------*/

	return platform_driver_register(&qt602240_driver);

}

void __exit qt602240_exit(void)
{
	i2c_del_driver(&qt602240_i2c_driver);
	if (qt602240_wq)
		destroy_workqueue(qt602240_wq);
}

/* firmware 2009.09.24 CHJ - start 2/2 */

uint8_t boot_reset(void)
{
	uint8_t data = 0xA5u;
	return(write_mem(command_processor_address + RESET_OFFSET, 1, &data));
}

uint8_t read_boot_state(u8 *data)
{
	struct i2c_msg rmsg;
	int ret;

	rmsg.addr = QT602240_I2C_BOOT_ADDR ;
	rmsg.flags = I2C_M_RD;
	rmsg.len = 1;
	rmsg.buf = data;
	ret = i2c_transfer(qt602240->client->adapter, &rmsg, 1);
	printk("[TSP] %s, %d\n", __func__, __LINE__ );

	if ( ret < 0 )
	{
		printk("[CHJ] %s,%d - Fail!!!! ret = %d\n", __func__, __LINE__, ret );
		return READ_MEM_FAILED; 
	}	
	else 
	{
		return READ_MEM_OK;
	}
}

uint8_t boot_unlock(void)
{
	printk("[CHJ] %s - start, %d\n", __func__, __LINE__ );
	struct i2c_msg wmsg;
	int ret,i;
	unsigned char data[2];

	data[0] = 0xDC;
	data[1] = 0xAA;

	wmsg.addr = QT602240_I2C_BOOT_ADDR ;
	wmsg.flags = I2C_M_WR;
	wmsg.len = 2;
	wmsg.buf = data;

	ret = i2c_transfer(qt602240->client->adapter, &wmsg, 1);

	if ( ret >= 0 )
		return WRITE_MEM_OK;
	else 
		return WRITE_MEM_FAILED; 
}

uint8_t boot_write_mem(uint16_t ByteCount, unsigned char * Data )
{
	struct i2c_msg wmsg;
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret,i;

	wmsg.addr = QT602240_I2C_BOOT_ADDR ;
	wmsg.flags = I2C_M_WR;
	wmsg.len = ByteCount;
	wmsg.buf = Data;

	ret = i2c_transfer(qt602240->client->adapter, &wmsg, 1);
	if ( ret >= 0 )
	{
		return WRITE_MEM_OK;
	}	
	else 
	{
		printk("[CHJ] %s,%d - Fail!!!!\n", __func__, __LINE__ );
		return WRITE_MEM_FAILED; 
	}
}

uint8_t QT_Boot(void)
{
	unsigned char boot_status;
	unsigned char boot_ver;
	unsigned long f_size;
	unsigned long int character_position;
	unsigned int frame_size;
	unsigned int next_frame;
	unsigned int crc_error_count;
	unsigned char rd_msg;
	unsigned int retry_cnt;
	unsigned int size1,size2;
	unsigned int i;

	firmware_ret_val = -1;

	// disable_irq for firmware update
	disable_irq(qt602240->client->irq);

	// pointer 
	unsigned char *firmware_data ;

	firmware_data = firmware_602240;
	//    firmware_data = &my_code;

	f_size = sizeof(firmware_602240);
	//f_size = sizeof(my_code);

	//Step 1 : Boot Reset
	if(boot_reset() == WRITE_MEM_OK)
		printk("[TSP] boot_reset is complete!\n");
	else 	
	{
		printk("[TSP] boot_reset is not complete!\n");
		return 0;	
	}

	i=0;

	for(retry_cnt = 0; retry_cnt < 30; retry_cnt++)
	{
		printk("[TSP] %s,retry = %d\n", __func__, retry_cnt );

		mdelay(60);

		if( (read_boot_state(&boot_status) == READ_MEM_OK) && (boot_status & 0xC0) == 0xC0) 
		{
			boot_ver = boot_status & 0x3F;
			crc_error_count = 0;
			character_position = 0;
			next_frame = 0;

			while(1)
			{
				mdelay(60);
				if(read_boot_state(&boot_status) == READ_MEM_OK)
				{
					if((boot_status & QT_WAITING_BOOTLOAD_COMMAND) == QT_WAITING_BOOTLOAD_COMMAND)
					{
						mdelay(60);
						if(boot_unlock() == WRITE_MEM_OK)
						{
							mdelay(60);
						}
						else
						{
							printk("[TSP] %s - boot_unlock fail!!%d\n", __func__, __LINE__ );
						}
					}
					else if((boot_status & 0xC0) == QT_WAITING_FRAME_DATA)
					{
						printk("[TSP] %s, sending packet %dth\n", __func__,i);
						/* Add 2 to frame size, as the CRC bytes are not included */
						size1 =  *(firmware_data+character_position);
						size2 =  *(firmware_data+character_position+1)+2;
						frame_size = (size1<<8) + size2;
						msleep(200);

						/* Exit if frame data size is zero */
						next_frame = 1;
						boot_write_mem(frame_size, (firmware_data +character_position));
						i++;
						msleep(100);

					}
					else if(boot_status == QT_FRAME_CRC_CHECK)
					{
						mdelay(60);
					}
					else if(boot_status == QT_FRAME_CRC_PASS)
					{
						if( next_frame == 1)
						{
							/* Ensure that frames aren't skipped */
							character_position += frame_size;
							next_frame = 0;
						}
					}
					else if(boot_status  == QT_FRAME_CRC_FAIL)
					{
						printk("[TSP] %s, crc fail %d times\n", __func__, crc_error_count);
						crc_error_count++;
					}
					if(crc_error_count > 10)
					{
						printk("[TSP] %s, crc fail %d times! firmware update fail!!!!\n", __func__, crc_error_count );
						return 0;
						//return QT_FRAME_CRC_FAIL;
					}
				}
				else
				{
					/* Complete Firmware Update!!!!! */
					printk("[TSP] %s, Complete update, Check the new Version!\n", __func__);
					/* save new slave address */
					//QT_i2c_address = I2C_address;
					mdelay(60);

					/* Poll device */
					if(read_mem(0, 1, &rd_msg) == READ_MEM_OK)
					{
						quantum_touch_probe();

						if (driver_setup != DRIVER_SETUP_OK)
						{
							printk("[TSP] Error:after firmware update, DRIVER_SETUP fail!!!\n");

						}
						else
						{
							printk("[TSP] after firmware update, DRIVER_SETUP success!!!\n");

							printk("[TSP] QT602240 Firmware Ver.\n");
							printk("[TSP] version = %x\n", info_block->info_id.version);

							enable_irq(qt602240->client->irq);

							return 1;
						}	
					}
					printk("[TSP] %s, After firmware update, read_mem fail - line %d\n", __func__, __LINE__ );
					return 0;
				}
			}
		}
		else
		{
			printk("[TSP] read_boot_state() or (boot_status & 0xC0) == 0xC0) is fail!!!\n");
			return 0;
		}

	}
	return 0;
}

/* firmware 2009.09.24 CHJ - end 2/2 */

late_initcall(qt602240_init);
module_exit(qt602240_exit);

MODULE_DESCRIPTION("Quantum Touchscreen Driver");
MODULE_LICENSE("GPL");
