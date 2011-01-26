#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <plat/regs-clock.h> 
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>



#include <linux/interrupt.h>
#include <linux/irq.h>

#include <plat/pm.h>
#include <plat/s3c64xx-dvfs.h>
#include <linux/i2c/pmic.h>
  
#include <mach/param.h>
#include <mach/hardware.h>

#include <linux/random.h>
#include "fsa9480_i2c.h"

#include <linux/wakelock.h>

#define NO_USING_USB_SWITCH 0

static struct wake_lock fsa9480_wake_lock;

extern int is_pmic_initialized(void);


extern struct device *switch_dev;
extern ftm_sleep;

#define FSA9480_UART 	1
#define FSA9480_USB 	2

#define FSA9480UCX		0x4A
static unsigned short fsa9480_normal_i2c[] = {I2C_CLIENT_END };
static unsigned short fsa9480_ignore[] = { I2C_CLIENT_END };
static unsigned short fsa9480_i2c_probe[] = { 5, FSA9480UCX >> 1, I2C_CLIENT_END };

static struct i2c_client_address_data fsa9480_addr_data = {
	.normal_i2c = fsa9480_normal_i2c,
	.ignore     = fsa9480_ignore,
	.probe      = fsa9480_i2c_probe,
};


struct i2c_driver fsa9480_i2c_driver;
static struct i2c_client *fsa9480_i2c_client = NULL;

struct fsa9480_state {
	struct i2c_client *client;
};

static struct i2c_device_id fsa9480_id[] = {
	{"fsa9480", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, fsa9480_id);


static int usb_path = 0;
static int usb_power = 2;
static int usb_state = 0;

static struct timer_list fsa9480_init_timer;
static wait_queue_head_t usb_detect_waitq;
static struct workqueue_struct *fsa9480_workqueue;
static struct work_struct fsa9480_work;


/********************************************************************/
/* function definitions                                                                             */
/********************************************************************/

//by ss1
//refer to drivers/usb/gadget/s3c-udc-otg-hs.c	
#if !NO_USING_USB_SWITCH
void s3c_udc_power_up(void);
void s3c_udc_power_down(void);
#endif

void fsa9480_SetAutoSWMode(void);


int wait_condition = 0;
static bool ta_connection = false;



void get_usb_serial(char *usb_serial_number)
{
	char temp_serial_number[13] = {0};

	u32 serial_number=0;
	
	serial_number = (system_serial_high << 16) + (system_serial_low >> 16);

#if defined(CONFIG_MACH_VINSQ)
	sprintf(temp_serial_number,"M910%08x",serial_number);
#elif defined(CONFIG_MACH_VITAL)
	sprintf(temp_serial_number,"M920%08x",serial_number);
#elif defined(CONFIG_MACH_QUATTRO)
	sprintf(temp_serial_number,"S500%08x",serial_number);
#elif defined(CONFIG_MACH_MAX)	
	sprintf(temp_serial_number,"I100%08x",serial_number);
#elif defined(CONFIG_MACH_INFOBOWLQ)
  sprintf(temp_serial_number,"R880%08x",serial_number); //not defined  
#else //CONFIG_MACH_INSTINCTQ
	sprintf(temp_serial_number,"M900%08x",serial_number);
#endif
	strcpy(usb_serial_number,temp_serial_number);
}


int available_PM_Set(void)
{
	DEBUG_FSA9480("[FSA9480]%s ", __func__);

	if(is_pmic_initialized())
	{
		printk("[FSA9480] find max8698 \n");
		return 1;
	}
	return 0;
}

int get_usb_power_state(void)
{
	DEBUG_FSA9480("[FSA9480]%s : usb_power = %d \n ", __func__,usb_power);	

	if(usb_power !=1)
	{		
		wait_condition = 0;
		wait_event_interruptible_timeout(usb_detect_waitq, wait_condition , 2 * HZ); 
	}

	if(usb_power==2 && ta_connection)
	{
		printk("[FSA9480] usb_power = 2 & taconnection is true \n");
		return 0;
	}

	return usb_power;
}

int get_usb_cable_state(void)
{
	//DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	return usb_state;
}

#if !NO_USING_USB_SWITCH
void fsa9480_s3c_udc_on(void)
{	
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	usb_power = 1;

	//SEC_BSP_WONSUK_20090806 
	//resolve Power ON/OFF panic issue. do not wake_up function before FSA9480 probe.
	if(&usb_detect_waitq == NULL)
		{
		printk("[FSA9480] fsa9480_s3c_udc_on : usb_detect_waitq is NULL\n");
		}
	else
		{
		wait_condition = 1;
		wake_up_interruptible(&usb_detect_waitq);
		}

    /*LDO control*/
	if(!Set_MAX8698_PM_REG(ELDO3, 1) || !Set_MAX8698_PM_REG(ELDO8, 1))
		printk("[FSA9480]%s : Fail to LDO ON\n ", __func__);

}

void fsa9480_s3c_udc_off(void)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);

	usb_power = 0;

    /*LDO control*/
	if(!Set_MAX8698_PM_REG(ELDO3, 0) || !Set_MAX8698_PM_REG(ELDO8, 0))
		printk("[FSA9480]%s : Fail to LDO OFF\n ", __func__);

}
EXPORT_SYMBOL(fsa9480_s3c_udc_on);
EXPORT_SYMBOL(fsa9480_s3c_udc_off);
#endif

static int fsa9480_read(u8 reg, u8 *data)
{
	struct i2c_client *client = fsa9480_i2c_client;
	int ret;

	if(client == NULL)
		return -ENODEV;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	*data = ret & 0xff;
	return 0;
}

static int fsa9480_write(u8 reg, u8 data)
{
	struct i2c_client *client = fsa9480_i2c_client;
	if(client == NULL)
		return -ENODEV;

	return i2c_smbus_write_byte_data(client, reg, data);
}

/**********************************************************************
*    Name         : fsa9480_modify()
*    Description : used when modify fsa9480 register value via i2c
*                        
*    Parameter   : None
*                       @ reg : fsa9480 register's address
*                       @ data : modified data
*                       @ mask : mask bit
*    Return        : None
*
***********************************************************************/
static int fsa9480_modify(u8 reg, u8 data, u8 mask)
{
   u8 original_value, modified_value;

   fsa9480_read(reg, &original_value);
   DEBUG_FSA9480("[FSA9480] %s Original value is 0x%02x\n ",__func__, original_value);
   modified_value = ((original_value&~mask) | data);
   DEBUG_FSA9480("[FSA9480] %s modified value is 0x%02x\n ",__func__, modified_value);
   fsa9480_write(reg, modified_value);

   return 0;
}


/* MODEM USB_SEL Pin control */
/* 1 : PDA, 2 : MODEM, 3 : WIMAX */
#define SWITCH_PDA			1
#define SWITCH_MODEM		2

//called by udc 
/* UART <-> USB switch (for UART/USB JIG) */
void fsa9480_check_usb_connection(void)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	u8 control, int1, deviceType1, deviceType2, manual1, manual2, pData,adc, carkitint1;
	u8 oldmanual2;
	bool bInitConnect = false;

	fsa9480_read( REGISTER_INTERRUPT1, &int1); // interrupt clear
	fsa9480_read( REGISTER_DEVICETYPE1, &deviceType1);
	fsa9480_read( REGISTER_ADC, &adc);

	//in case of carkit we should process the extra interrupt. 1: attach interrupt, 2: carkit interrupt
	//  (interrupt_cr3 & 0x0) case is when power on after TA is inserted.
	if (((int1 & ATTACH) /*|| !(int1 & 0x0)*/) && 
		               (( deviceType1 & CRA_CARKIT) || adc == CEA936A_TYPE_1_CHARGER ))
	{
		DEBUG_FSA9480("[FSA9480] %s : Carkit is inserted! 1'st resolve insert interrupt\n ",__func__);
		fsa9480_write( REGISTER_CARKITSTATUS, 0x02);  //use only carkit charger

		fsa9480_read( REGISTER_CARKITINT1, &carkitint1);    // make INTB to high
		DEBUG_FSA9480("[FSA9480] %s : Carkit int1 is 0x%02x\n ",__func__, carkitint1);
		return;
	}

	fsa9480_read( REGISTER_CONTROL, &control);
	fsa9480_read( REGISTER_INTERRUPT2, &pData); // interrupt clear
	fsa9480_read( REGISTER_DEVICETYPE2, &deviceType2);
	fsa9480_read( REGISTER_MANUALSW1, &manual1);
	fsa9480_read( REGISTER_MANUALSW2, &manual2);

	printk("[FSA9480] control(0x%x) int1(0x%x) adc(0x%x) dev1(0x%x) dev2(0x%x) man1(0x%x) man2(0x%x) pwr(%d)\n", 
		control, int1, adc, deviceType1, deviceType2, manual1, manual2, usb_power);

	/* TA Connection */
	if(deviceType1 ==0x40)
	{
		wait_condition = 1;
		ta_connection = true;
		wake_up_interruptible(&usb_detect_waitq);
		printk("[FSA9480] TA is connected \n");
	}
	
	usb_state = (deviceType2 << 8) | (deviceType1 << 0);
	
	/* Disconnect cable */
	if (deviceType1 == DEVICE_TYPE_NC && deviceType2 == DEVICE_TYPE_NC) {
		ta_connection = false;
		//DEBUG_FSA9480("[FSA9480] Cable is not connected\n ");
		if (usb_power == 1) 
		{
			/* reset manual2 s/w register */
			fsa9480_write( REGISTER_MANUALSW2, 0x00);

			/* auto mode settings */
			fsa9480_write( REGISTER_CONTROL, 0x1E); 

#if !NO_USING_USB_SWITCH
			/* power down mode */
			s3c_udc_power_down();
#endif

			return ;
		}
	}
	
	/* USB Detected */
	if (deviceType1 == CRA_USB|| (deviceType2 & CRB_JIG_USB)) 
	{
		fsa9480_write(REGISTER_MANUALSW1, (usb_path==SWITCH_PDA) ? 0x24:0x90);
#if 1
		/* Manual Mode for USB */
		//DEBUG_FSA9480("[FSA9480] MANUAL MODE.. d1:0x%02x, d2:0x%02x\n", deviceType1, deviceType2);
		if (control != 0x1A) 
		{ 	
			fsa9480_write(REGISTER_CONTROL, 0x1A); 
		}
#endif
		/* reset manual2 s/w register */
		manual2 &= ~(0x1f);

		if(deviceType1 == CRA_USB)
		{
			/* ID Switching : ID connected to bypass port */
			manual2 |= 0x2;
		}
		else if(deviceType2 & CRB_JIG_USB_ON)
		{	
			/* BOOT_SW : High , JIG_ON : GND */
			manual2 |= (0x1 << 3) | (0x1 << 2);
		}
		else if(deviceType2 & CRB_JIG_USB_OFF)
		{
			/* BOOT_SW : LOW , JIG_ON : GND */
			manual2 |= (0x1 << 2);
		}

		fsa9480_write(REGISTER_MANUALSW2, manual2);

		/* connect cable */
		if((((manual1 == 0x24) || (manual1 == 0x90)) && usb_power !=1)
		|| ((deviceType1 == CRA_USB) && (int1 == ATTACH)))
		{
			bInitConnect = true;
#if !NO_USING_USB_SWITCH
			s3c_udc_power_up();
#endif
		}
	}
	/* Auto mode settings */
	else
	{
		/* Auto Mode except usb */
		DEBUG_FSA9480("[FSA9480] AUTO MODE.. d1:0x%02x, d2:0x%02x\n", deviceType1, deviceType2);
		if (control != 0x1E) 
		{ 
			fsa9480_write( REGISTER_CONTROL, 0x1E);
		}
	}

#if !NO_USING_USB_SWITCH
	/* initialization driver */
	if(usb_power == 2 && bInitConnect ==false )
	{
		fsa9480_s3c_udc_off();

	}	
#endif
}
EXPORT_SYMBOL(fsa9480_check_usb_connection);

static void usb_sel(int sel)
{  
	usb_path = sel;
}

/* for sysfs control (/sys/class/sec/switch/usb_sel) */
static ssize_t usb_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 i, pData;

	sprintf(buf, "USB Switch : %s\n", usb_path==SWITCH_PDA?"PDA":"MODEM");

//	sprintf(buf, "[USB Switch] fsa9480 register\n");
    for(i = 0; i <= 0x14; i++) {
		fsa9480_read( i, &pData);
//		sprintf(buf, "%s0x%02x = 0x%02x\n", buf, i, pData);
	}
//	sprintf(buf, "%s[USB Switch] fsa9480 register done\n", buf);
  
	return sprintf(buf, "%s\n", buf);
}

void usb_switch_mode(int sel)
{
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	if (sel == SWITCH_PDA)
	{
		DEBUG_FSA9480("[FSA9480] Path : PDA\n");
		usb_sel(SWITCH_PDA);
		fsa9480_write(0x13, 0x24); // PDA Port
	} else if (sel == SWITCH_MODEM) 
	{
		DEBUG_FSA9480("[FSA9480] Path : MODEM\n");
		usb_sel(SWITCH_MODEM);
		fsa9480_write(0x13, 0x90); // V_Audio port (Modem USB)
	} else
		DEBUG_FSA9480("[FSA9480] Invalid mode...\n");
}
EXPORT_SYMBOL(usb_switch_mode);

static ssize_t usb_sel_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{	
    DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	int switch_sel;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);
  
	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		usb_switch_mode(SWITCH_PDA);
		switch_sel |= USB_SEL_MASK;
	}

	if(strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		usb_switch_mode(SWITCH_MODEM);
		switch_sel &= ~USB_SEL_MASK;
	}

	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);

	return size;
}

static DEVICE_ATTR(usb_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_sel_show, usb_sel_store);



/**********************************************************************
*    Name         : usb_state_show()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        return usb state using fsa9480's device1 and device2 register
*                        this function is used only when NPS want to check the usb cable's state.
*    Parameter   :
*                       
*                       
*    Return        : USB cable state's string
*                        USB_STATE_CONFIGURED is returned if usb cable is connected
***********************************************************************/
static ssize_t usb_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int cable_state;

	cable_state = get_usb_cable_state();

	sprintf(buf, "%s\n", (cable_state & (CRB_JIG_USB<<8 | CRA_USB<<0 ))?"USB_STATE_CONFIGURED":"USB_STATE_NOTCONFIGURED");

	return sprintf(buf, "%s\n", buf);
} 


/**********************************************************************
*    Name         : usb_state_store()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        noting to do.
*    Parameter   :
*                       
*                       
*    Return        : None
*
***********************************************************************/
static ssize_t usb_state_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);

	return 0;
}

/*sysfs for usb cable's state.*/
static DEVICE_ATTR(usb_state, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_state_show, usb_state_store);


/* UART <-> USB switch (for UART/USB JIG) */
static void mode_switch(struct work_struct *ignored)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	fsa9480_check_usb_connection();
}

static irqreturn_t fsa9480_interrupt(int irq, void *ptr)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
#if 1
	DEBUG_FSA9480("### FSA9480 interrupt(%d) happened! ###\n", irq);
	DEBUG_FSA9480("GPIO_JACK_INT_N value : %d\n", gpio_get_value(GPIO_JACK_INT_N));
#endif
	queue_work(fsa9480_workqueue, &fsa9480_work);

	return IRQ_HANDLED; 
}

/* pm init check for MAX_Set*/
void pm_check(void)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	if (available_PM_Set()) {
		set_irq_type(IRQ_EINT(9), IRQ_TYPE_EDGE_FALLING);
		if (request_irq(IRQ_EINT(9), fsa9480_interrupt, IRQF_DISABLED, "FSA9480 Detected", NULL)) 
		{
			DEBUG_FSA9480("[FSA9480]fail to register IRQ[%d] for FSA9480 USB Switch \n", IRQ_EINT(9));
		}
		queue_work(fsa9480_workqueue, &fsa9480_work);
	} else {
		fsa9480_init_timer.expires = get_jiffies_64()+10;
		add_timer(&fsa9480_init_timer);
	}
}

static int __devinit fsa9480_codec_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fsa9480_state *state;
	struct device *dev = &client->dev;
	u8 pData;

	DEBUG_FSA9480("[FSA9480] %s\n", __func__);

	/* FSA9480 Interrupt */
	s3c_gpio_cfgpin(GPIO_JACK_INT_N, S3C_GPIO_SFN(GPIO_JACK_INT_N_AF));
	s3c_gpio_setpull(GPIO_JACK_INT_N, S3C_GPIO_PULL_NONE);

	init_waitqueue_head(&usb_detect_waitq); 
	INIT_WORK(&fsa9480_work, mode_switch);
	fsa9480_workqueue = create_singlethread_workqueue("fsa9480_workqueue");

     state = kzalloc(sizeof(struct fsa9480_state), GFP_KERNEL);
	 if(!state) {
		 dev_err(dev, "%s: failed to create fsa9480_state\n", __func__);
		 return -ENOMEM;
	 }

	 
	 state->client = client;
	 fsa9480_i2c_client = client;
	 
	 i2c_set_clientdata(client, state);
	 if(!fsa9480_i2c_client)
	 {
		 dev_err(dev, "%s: failed to create fsa9480_i2c_client\n", __func__);
		 return -ENODEV;
	 }

	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		DEBUG_FSA9480("[FSA9480]Failed to create device file(%s)!\n", dev_attr_usb_sel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_usb_state) < 0)
		DEBUG_FSA9480("[FSA9480]Failed to create device file(%s)!\n", dev_attr_usb_state.attr.name);
	

	/*init wakelock*/
	wake_lock_init(&fsa9480_wake_lock, WAKE_LOCK_SUSPEND, "fsa9480_wakelock");

	/*clear interrupt mask register*/
	fsa9480_modify(REGISTER_CONTROL,~INT_MASK, INT_MASK);
	
	fsa9480_read(0x13, &pData);
	
	if (pData == 0x24) /* PDA */
		usb_path = SWITCH_PDA;
	else /* PHONE */
		usb_path = SWITCH_MODEM;	

	usb_sel(usb_path);

	init_timer(&fsa9480_init_timer);
	fsa9480_init_timer.function = (void*) pm_check;

	pm_check();

	return 0;
}

static int __devexit fsa9480_remove(struct i2c_client *client)
{
	struct fsa9480_state *state = i2c_get_clientdata(client);
	kfree(state);
	return 0;
}

static int fsa9480_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    DEBUG_FSA9480("%s\n", __func__);
	strlcpy(info->type, "fsa9480", I2C_NAME_SIZE);
	return 0;
}


struct i2c_driver fsa9480_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "fsa9480",
	},
	.class		= I2C_CLASS_HWMON,
	.id_table	= fsa9480_id,
	.probe	= fsa9480_codec_probe,
	.remove	= __devexit_p(fsa9480_remove),
	.detect = fsa9480_detect,
	.command = NULL,
	.address_data = &fsa9480_addr_data
};


static int __init fsa9480_driver_init(void)
{
	int ret;
	DEBUG_FSA9480("%s\n", __func__);

	if((ret = i2c_add_driver(&fsa9480_i2c_driver)))
		DEBUG_FSA9480("%s: Can't add fsa9480 i2c driver\n", __func__);

	return ret;
}

static void __exit fsa9480_driver_exit(void)
{
	DEBUG_FSA9480("%s\n", __func__);
	i2c_del_driver(&fsa9480_i2c_driver);
}



subsys_initcall(fsa9480_driver_init);
module_exit(fsa9480_driver_exit);

MODULE_AUTHOR("Samsung eletronics");
MODULE_DESCRIPTION("fsa9480 driver");
MODULE_LICENSE("GPL");



//SEC_BSP_WONSUK_20090810 : Add the codes related SLEEP CMD in factory process
/*================================================
	When DIAG SLEEP command arrived, UART RXD, TXD port make disable
	because CP could not enter the sleep mode due to the UART floating voltage.
================================================*/


/**********************************************************************
*    Name         : fsa9480_SetManualSW()
*    Description : Control FSA9480's Manual SW1 and SW2
*                        
*    Parameter   :
*                       @ valManualSw1 : the value to set SW1
*                       @ valManualSw2 : the value to set SW2
*    Return        : None
*
***********************************************************************/
void fsa9480_SetManualSW(unsigned char valManualSw1, unsigned char valManualSw2)
{
	DEBUG_FSA9480("[FSA9480]%s \n", __func__);
	unsigned char cont_reg, man_sw1, man_sw2;

	/*Set Manual switch*/
	fsa9480_write( REGISTER_MANUALSW1, valManualSw1);
	mdelay(20);
	
	fsa9480_write( REGISTER_MANUALSW2, valManualSw2);
	mdelay(20);


	/*when detached the cable, Control register automatically be restored.*/
	fsa9480_read( REGISTER_CONTROL, &cont_reg);
	mdelay(20);
	DEBUG_FSA9480("[FSA9480] fsa9480_SetManualSW : [Before]Control Register's value is %s\n",&cont_reg);

	/*set switching mode to MANUAL*/
	fsa9480_write( REGISTER_CONTROL, 0x1A);


	/* Read current setting value , manual sw1, manual sw2, control register.*/
	fsa9480_read( REGISTER_MANUALSW1, &man_sw1);
	mdelay(20);
	DEBUG_FSA9480("[FSA9480] fsa9480_SetManualSW : Manual SW1 Register's value is %s\n",&man_sw1);

	fsa9480_read( REGISTER_MANUALSW2, &man_sw2);
	mdelay(20);
	DEBUG_FSA9480("[FSA9480] fsa9480_SetManualSW : Manual SW2 Register's value is %s\n",&man_sw2);

	fsa9480_read( REGISTER_CONTROL, &cont_reg);
	DEBUG_FSA9480("[FSA9480] fsa9480_SetManualSW : [After]Control Register's value is %s\n",&cont_reg);
}



/**********************************************************************
*    Name         : fsa9480_SetAutoSWMode()
*    Description : Set FSA9480 with Auto Switching Mode.
*                        
*    Parameter   : None
*                       @ 
*                       @ 
*    Return        : None
*
***********************************************************************/
void fsa9480_SetAutoSWMode(void)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	unsigned char cont_reg=0xff;

	/*set Auto Swithing mode */
	fsa9480_write( REGISTER_CONTROL, 0x1E);
}


/**********************************************************************
*    Name         : fsa9480_MakeRxdLow()
*    Description : Make UART port to OPEN state.
*                        
*    Parameter   : None
*                       @ 
*                       @ 
*    Return        : None
*
***********************************************************************/
void fsa9480_MakeRxdLow(void)
{
	DEBUG_FSA9480("[FSA9480]%s\n ", __func__);
	unsigned char hidden_reg;
	
	fsa9480_write( HIDDEN_REGISTER_MANUAL_OVERRDES1, 0x0a); 
	mdelay(20);
	fsa9480_read( HIDDEN_REGISTER_MANUAL_OVERRDES1, &hidden_reg);
	fsa9480_SetManualSW(0x00, 0x00);
}


EXPORT_SYMBOL(fsa9480_SetManualSW);
EXPORT_SYMBOL(fsa9480_SetAutoSWMode);
EXPORT_SYMBOL(fsa9480_MakeRxdLow);


