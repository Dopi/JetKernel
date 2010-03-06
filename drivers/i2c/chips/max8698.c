/*
 * linux/drivers/i2c/max8698.c
 *
 * Battery measurement code for SEC
 *
 * based on max8906.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c/maximi2c.h>
#include <linux/i2c/pmic.h>

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "MAX8698: "

max8698_register_type max8698reg[ENDOFREG] =
{
	/* Slave addr 		  Reg addr */
	{ I2C_SLAVE_ADDR_MAX8698, REG_ONOFF1 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_ONOFF2 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_ADISCHG_EN1 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_ADISCHG_EN2 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_DVSARM1_2 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_DVSARM3_4 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_DVSINT1_2 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_BUCK3 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO2_3 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO4 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO5 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO6 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO7 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO8 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LDO9 },
	{ I2C_SLAVE_ADDR_MAX8698, REG_LBCNFG },
};

max8698_function_type max8698pm[ENDOFPM] =
{
	/* ONOFF1 register */
	/* slave_addr              addr   mask   clear  shift */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x80,  0x7F,  0x07 }, /* EN1 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x40,  0xBF,  0x06 }, /* EN2 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x20,  0xDF,  0x05 }, /* EN3 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x10,  0xEF,  0x04 }, /* ELDO2 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x08,  0xF7,  0x03 }, /* ELDO3 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x04,  0xFB,  0x02 }, /* ELDO4 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x00,  0x02,  0xFD,  0x01 }, /* ELDO5 */
	/* ONOFF2 register */
	{ I2C_SLAVE_ADDR_MAX8698,  0x01,  0x80,  0x7F,  0x07 }, /* ELDO6 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x01,  0x40,  0xBF,  0x06 }, /* ELDO7 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x01,  0x20,  0xDF,  0x05 }, /* ELDO8 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x01,  0x10,  0xEF,  0x04 }, /* ELDO9 */
	{ I2C_SLAVE_ADDR_MAX8698,  0x01,  0x01,  0xFE,  0x00 }, /* ELBCNFG */
	/* ADISCHG_EN1 register */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x80,  0x7F,  0x07 }, /* BUCK1_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x40,  0xBF,  0x06 }, /* BUCK2_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x20,  0xDF,  0x05 }, /* BUCK3_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x10,  0xEF,  0x04 }, /* LDO2_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x08,  0xF7,  0x03 }, /* LDO3_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x04,  0xFB,  0x02 }, /* LDO4_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x02,  0xFD,  0x01 }, /* LDO5_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x02,  0x01,  0xFE,  0x00 }, /* LDO6_ADEN */
	/* ADISCHG_EN2 register */
	{ I2C_SLAVE_ADDR_MAX8698,  0x03,  0x80,  0x7F,  0x07 }, /* LDO7_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x03,  0x40,  0xBF,  0x06 }, /* LDO8_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x03,  0x20,  0xDF,  0x05 }, /* LDO9_ADEN */
	{ I2C_SLAVE_ADDR_MAX8698,  0x03,  0x0F,  0xF0,  0x00 }, /* RAMP */
	/* DVSARM1_2 register */
	{ I2C_SLAVE_ADDR_MAX8698,  0x04,  0xF0,  0x0F,  0x04 }, /* DVSARM2 */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x04,  0x0F,  0xF0,  0x00 }, /* DVSARM1 */
	/* DVSARM3_4 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x05,  0xF0,  0x0F,  0x04 }, /* DVSARM4 */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x05,  0x0F,  0xF0,  0x00 }, /* DVSARM3 */
	/* DVSINT1_2 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x06,  0xF0,  0x0F,  0x04 }, /* DVSINT2 */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x06,  0x0F,  0xF0,  0x00 }, /* DVSINT1 */
	/* BUCK3 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x07,  0xFF,  0x00,  0x00 }, /* BUCK3 */
	/* LDO2_3 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x08,  0xF0,  0x0F,  0x04 }, /* LDO3 */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x08,  0x0F,  0xF0,  0x00 }, /* LDO2 */
	/* LDO4 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x09,  0xFF,  0x00,  0x00 }, /* LDO4 */
	/* LDO5 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0A,  0xFF,  0x00,  0x00 }, /* LDO5 */
	/* LDO6 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0B,  0xFF,  0x00,  0x00 }, /* LDO6 */
    	/* LDO7 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0C,  0xFF,  0x00,  0x00 }, /* LDO7 */
	/* LDO8 & BKCHR register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0D,  0xF0,  0x0F,  0x04 }, /* LDO8 */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0D,  0x0F,  0xF0,  0x00 }, /* BKCHR */
	/* LDO9 register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0E,  0xFF,  0x00,  0x00 }, /* LDO9 */
	/* LBCNFG register */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0F,  0x30,  0x0E,  0x04 }, /* LBHYST */
  	{ I2C_SLAVE_ADDR_MAX8698,  0x0F,  0x0E,  0xF1,  0x01 }, /* LBTH */
};

/* MAX8698 voltage table */
#define ARM_VCC_TABLE_MAX		0x10
static const unsigned int arm_voltage_table[ARM_VCC_TABLE_MAX] = {
	750, 800, 850, 900,	/* 0x0 ~ 0x3 */
	950, 1000, 1050, 1100,	/* 0x4 ~ 0x7 */
	1150, 1200, 1250, 1300,	/* 0x8 ~ 0xB */
	1350, 1400, 1450, 1500,	/* 0xC ~ 0xF */
};



/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/

/*===========================================================================

FUNCTION Set_MAX8698_PM_REG                                

DESCRIPTION
    This function write the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    value   :   the value for reg_num.
                This is aligned to the right side of the return value

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8698_PM_REG(CHGENB, onoff);

===========================================================================*/
boolean Set_MAX8698_PM_REG(max8698_pm_function_type reg_num, byte value)
{
    byte reg_buff = 0;

    if(pmic_read(max8698pm[reg_num].slave_addr, max8698pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read command failed
        return FALSE;
    }

    reg_buff = (reg_buff & max8698pm[reg_num].clear) | (value << max8698pm[reg_num].shift);
    if(pmic_write(max8698pm[reg_num].slave_addr, max8698pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;
}
EXPORT_SYMBOL(Set_MAX8698_PM_REG);

/*===========================================================================

FUNCTION Get_MAX8698_PM_REG                                

DESCRIPTION
    This function read the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8698_PM_REG(max8698_pm_function_type reg_num, byte *reg_buff)
{
    byte temp_buff;

    if(pmic_read(max8698pm[reg_num].slave_addr, max8698pm[reg_num].addr, &temp_buff, (byte)1) != PMIC_PASS)
    {
        // Register Read Command failed
        return FALSE;
    }

    *reg_buff = (byte)((temp_buff & max8698pm[reg_num].mask) >> max8698pm[reg_num].shift);

    return TRUE;
}
EXPORT_SYMBOL(Get_MAX8698_PM_REG);

/*===========================================================================

FUNCTION Set_MAX8698_PM_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8698_pm_register_type reg_addr    : the register address.
    byte *reg_buff   : the array for data of register to write.
 	byte length      : the number of the register 

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8698_PM_ADDR(max8698_pm_register_type reg_addr, byte *reg_buff, byte length)
{

    if(pmic_write(max8698reg[reg_addr].slave_addr, max8698reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Write command failed
        return FALSE;
    }
    return TRUE;

}


/*===========================================================================

FUNCTION Get_MAX8698_PM_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    max8698_pm_register_type reg_addr   : the register address.
    byte *reg_buff  : the array for data of register to write.
 	byte length     : the number of the register 

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8698_PM_ADDR(max8698_pm_register_type reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ENDOFREG)
    {
        // Invalid Read Register
        return FALSE; // return error;
    }
    if(pmic_read(max8698reg[reg_addr].slave_addr, max8698reg[reg_addr].addr, reg_buff, length) != PMIC_PASS)
    {
        // Register Read command failed
        return FALSE;
    }
    return TRUE;
}

/*===========================================================================*/
boolean change_vcc_arm(int voltage)
{
	byte reg_value = 0;

	pr_debug(PREFIX "%s:I: voltage: %d\n", __func__, voltage);

	if (voltage < DVSARM_MIN_VCC || voltage > DVSARM_MAX_VCC) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 50) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	for (reg_value = 0; reg_value <  ARM_VCC_TABLE_MAX; reg_value++) {
		if ( arm_voltage_table[reg_value] == voltage )
			break;
	}

	if (!Set_MAX8698_PM_REG(DVSARM1, reg_value)) {
		pr_err(PREFIX "%s:E: set pmic reg fail(%d)\n", __func__, reg_value);
		return FALSE;
	}
 
	return TRUE;
}

boolean change_vcc_internal(int voltage)
{
	byte reg_value = 0;

	pr_debug(PREFIX "%s:I: voltage: %d\n", __func__, voltage);

	if (voltage < DVSINT_MIN_VCC || voltage > DVSINT_MAX_VCC) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 50) {
		pr_err(PREFIX "%s:E: invalid voltage: %d\n", __func__, voltage);
		return FALSE;
	}

	for (reg_value = 0; reg_value <  ARM_VCC_TABLE_MAX; reg_value++) {
		if ( arm_voltage_table[reg_value] == voltage )
			break;
	}

	if (!Set_MAX8698_PM_REG(DVSINT1, reg_value))
	{
		pr_err(PREFIX "%s:E: set pmic reg fail(%d)\n", __func__, reg_value);
		return FALSE;
	}
 
	return TRUE;
}

boolean set_pmic(pmic_pm_type pm_type, int value)
{
	boolean rc = FALSE;
	switch (pm_type) {
	case VCC_ARM:
		rc = change_vcc_arm(value);
		break;
	case VCC_INT:
		rc = change_vcc_internal(value);
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
	return rc;
}

boolean get_pmic(pmic_pm_type pm_type, int *value)
{
	boolean rc = FALSE;
	byte reg_buff;
	*value = 0;

	switch (pm_type) {
	case VCC_ARM:
		if(! Get_MAX8698_PM_REG(DVSARM1, &reg_buff)) {
			pr_err(PREFIX "%s:VCC_ARM: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		if((reg_buff) < ARM_VCC_TABLE_MAX)
			*value = arm_voltage_table[reg_buff];
		break;
	case VCC_INT:
		if(!Get_MAX8698_PM_REG(DVSINT1, &reg_buff))
		{
			pr_err(PREFIX "%s:VCC_INT: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		if((reg_buff) < ARM_VCC_TABLE_MAX)
			*value = arm_voltage_table[reg_buff];
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
        return rc;
}

/*============================================================================*/
/* MAX8698 I2C Interface                                                      */
/*============================================================================*/

#include <linux/i2c.h>

#define I2C_GPIO3_DEVICE_ID	3 /* mach-($DEVICE_NAME).c */

static struct i2c_driver max8698_driver;
static struct i2c_client *max8698_i2c_client = NULL;

static unsigned short max8698_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short max8698_ignore[] = { I2C_CLIENT_END };
static unsigned short max8698_probe[] = { I2C_GPIO3_DEVICE_ID,
	(I2C_SLAVE_ADDR_MAX8698 >> 1), I2C_CLIENT_END };

static struct i2c_client_address_data max8698_addr_data = {
	.normal_i2c = max8698_normal_i2c,
	.ignore		= max8698_ignore,
	.probe		= max8698_probe,
};

static int max8698_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int max8698_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = data;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#if 0
	pr_info("%s -> reg 0x%02x, data 0x%02x\n", __func__, reg, *data);
#endif	
	if (slaveaddr == I2C_SLAVE_ADDR_MAX8698)
		client = max8698_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8698_read(client, reg, data) < 0) { 
		pr_err(KERN_ERR "%s -> Failed! (reg 0x%02x, data 0x%02x)\n",
					__func__, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
	struct i2c_client *client;
#if 0
	pr_info("%s -> reg 0x%02x, data 0x%02x\n", __func__, reg, *data);
#endif	
	if (slaveaddr == I2C_SLAVE_ADDR_MAX8698)
		client = max8698_i2c_client;
	else 
		return PMIC_FAIL;

	if (max8698_write(client, reg, *data) < 0) { 
		pr_err(KERN_ERR "%s -> Failed! (reg 0x%02x, data 0x%02x)\n",
					__func__, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}

unsigned int pmic_tsc_write(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

unsigned int pmic_tsc_read(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

static int max8698_attach(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;

	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strncpy(c->name, max8698_driver.driver.name, I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &max8698_driver;

	if ((ret = i2c_attach_client(c)))
		goto error;

	max8698_i2c_client = c;

error:
	return ret;
}

static int max8698_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &max8698_addr_data, max8698_attach);
}

static int max8698_detach_client(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int max8698_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

static struct i2c_driver max8698_driver = {
	.driver = {
		.name = "max8698",
	},
	.attach_adapter = max8698_attach_adapter,
	.detach_client = max8698_detach_client,
	.command = max8698_command
};

static int pmic_init_status = 0;
int is_pmic_initialized(void)
{
	return pmic_init_status;
}

static int __init max8698_init(void)
{
	int ret;
	ret = i2c_add_driver(&max8698_driver);
	pmic_init_status = 1;
	return ret;
}

static void __exit max8698_exit(void)
{
	i2c_del_driver(&max8698_driver);
	pmic_init_status = 0;
}

module_init(max8698_init);
module_exit(max8698_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("MAX8698 Driver");
MODULE_LICENSE("GPL");
