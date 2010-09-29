#ifndef __GP2A_H__
#define __GP2A_H__

#define I2C_M_WR 0 /* for i2c */
#define I2c_M_RD 1 /* for i2c */



#define IRQ_GP2A_INT IRQ_EINT(20)  /*s3c64xx int number */

#define I2C_DF_NOTIFY			0x01 /* for i2c */

//ADDSEL is LOW
#define GP2A_ADDR		0x88 /* slave addr for i2c */

#define REGS_PROX 	    0x0 // Read  Only
#define REGS_GAIN    	0x1 // Write Only
#define REGS_HYS		0x2 // Write Only
#define REGS_CYCLE		0x3 // Write Only
#define REGS_OPMOD		0x4 // Write Only
#define REGS_CON		0x6 // Write Only

/* sensor type */
#define LIGHT           0
#define PROXIMITY		1
#define ALL				2

/* power control */
#define ON              1
#define OFF				0

/* IOCTL for proximity sensor */
#define SHARP_GP2AP_IOC_MAGIC   'C'                                 
#define SHARP_GP2AP_OPEN    _IO(SHARP_GP2AP_IOC_MAGIC,1)            
#define SHARP_GP2AP_CLOSE   _IO(SHARP_GP2AP_IOC_MAGIC,2)      

/* input device for proximity sensor */
#define USE_INPUT_DEVICE 	0


#define INT_CLEAR    1 /* 0 = normal operation, 1 = INT operation */
#define LIGHT_PERIOD 2 /* sec */
#define ADC_CHANNEL  5 /* s3c6410 channel for adc */

/*for light sensor */
#define STATE_NUM				3   /* number of states */
#define STATE_0_BRIGHTNESS    255   /* brightness of lcd */
#define STATE_1_BRIGHTNESS 	  130    
#define STATE_2_BRIGHTNESS     40

#define ADC_CUT_HIGH 800            /* boundary line between STATE_0 and STATE_1 */
#define ADC_CUT_LOW  300            /* boundary line between STATE_1 and STATE_2 */
#define ADC_CUT_GAP  200            /* in order to prevent chattering condition */




/* state type */
typedef enum t_light_state
{
	STATE_0   = 0,
	STATE_1   = 1,
	STATE_2   = 2,
	

}state_type;


/* initial value for sensor register */
static u8 gp2a_original_image[8] =
{
	0x00,  
	0x08,  
	0x40,  	
	0x04,  
	0x03,   
};


/* for state transition */
struct _light_state {
	state_type type;
	int adc_bottom_limit;
	int adc_top_limit;
	int brightness;

};



static struct _light_state light_state[] = {
	[0] = {
		.type = STATE_0, // 700~20000
		.adc_bottom_limit = ADC_CUT_HIGH - ADC_CUT_GAP/2, 
		.adc_top_limit    = 20000, //unlimited
		.brightness		  = STATE_0_BRIGHTNESS,
		},
	[1] = {
		.type = STATE_1, //200~900
		.adc_bottom_limit = ADC_CUT_LOW  - ADC_CUT_GAP/2, 
		.adc_top_limit    = ADC_CUT_HIGH + ADC_CUT_GAP/2, 
		.brightness		  = STATE_1_BRIGHTNESS,
		},
	
	[2] = {
		.type = STATE_2,   //1~400
		.adc_bottom_limit = 1,
		.adc_top_limit    = ADC_CUT_LOW  + ADC_CUT_GAP/2,
		.brightness		  = STATE_2_BRIGHTNESS,
		},

};




/* driver data */
struct gp2a_data {
	struct input_dev *input_dev;
	struct work_struct work_prox;  /* for proximity sensor */
	struct work_struct work_light; /* for light_sensor     */
	int             irq;
    struct hrtimer timer;

};


struct workqueue_struct *gp2a_wq;

/* prototype */
int opt_i2c_read(u8 reg, u8 *val, unsigned int len );
int opt_i2c_write( u8 reg, u8 *val );
extern int s3c_adc_get_adc_data(int channel);
void backlight_level_ctrl(s32 value);
static int opt_attach_adapter(struct i2c_adapter *adap);
static int proximity_open(struct inode *ip, struct file *fp);
static int proximity_release(struct inode *ip, struct file *fp);
static long proximity_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif
