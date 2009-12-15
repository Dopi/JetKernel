
/**************************************************************************

Module Name:  max8698.h

Abstract:


**************************************************************************/


#ifndef __LINUX_MAX8698_H
#define __LINUX_MAX8698_H

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */
#ifndef NULL
#define NULL   0
#endif

typedef  unsigned char      boolean;     /* Boolean value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsinged 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */


/* MAX8698 PMIC slave address (8-bit aligned) */
#define I2C_SLAVE_ADDR_MAX8698		0xCC

/* ADISCHG_EN2 Reg bits */
#define RAMP_1MV_US		(0x0)
#define RAMP_2MV_US		(0x1)
#define RAMP_3MV_US		(0x2)
#define RAMP_4MV_US		(0x3)
#define RAMP_5MV_US		(0x4)
#define RAMP_6MV_US		(0x5)
#define RAMP_7MV_US		(0x6)
#define RAMP_8MV_US		(0x7)
#define RAMP_9MV_US		(0x8)
#define RAMP_10MV_US		(0x9)
#define RAMP_11MV_US		(0xA)
#define RAMP_12MV_US		(0xB)

/* DVSARM 1~4 */
#define DVSARM_MAX_VCC		1500
#define DVSARM_MIN_VCC		 750
#define DVSARM_0_75		(0x0)
#define DVSARM_0_80		(0x1)
#define DVSARM_0_85		(0x2)
#define DVSARM_0_90		(0x3)
#define DVSARM_0_95		(0x4)
#define DVSARM_1_00		(0x5)
#define DVSARM_1_05		(0x6)
#define DVSARM_1_10		(0x7)
#define DVSARM_1_15		(0x8)
#define DVSARM_1_20		(0x9)
#define DVSARM_1_25		(0xA)
#define DVSARM_1_30		(0xB)
#define DVSARM_1_35		(0xC)
#define DVSARM_1_40		(0xD)
#define DVSARM_1_45		(0xE)
#define DVSARM_1_50		(0xF)

/* DVSINT 1~2 */
#define DVSINT_MAX_VCC		1500
#define DVSINT_MIN_VCC		 750
#define DVSINT_0_75		(0x0)
#define DVSINT_0_80		(0x1)
#define DVSINT_0_85		(0x2)
#define DVSINT_0_90		(0x3)
#define DVSINT_0_95		(0x4)
#define DVSINT_1_00		(0x5)
#define DVSINT_1_05		(0x6)
#define DVSINT_1_10		(0x7)
#define DVSINT_1_15		(0x8)
#define DVSINT_1_20		(0x9)
#define DVSINT_1_25		(0xA)
#define DVSINT_1_30		(0xB)
#define DVSINT_1_35		(0xC)
#define DVSINT_1_40		(0xD)
#define DVSINT_1_45		(0xE)
#define DVSINT_1_50		(0xF)

/* BUCK3 */
#define BUCK3_1_60		(0x00)
#define BUCK3_1_70		(0x01)
#define BUCK3_1_80		(0x02)
#define BUCK3_1_90		(0x03)
#define BUCK3_2_00		(0x04)
#define BUCK3_2_10		(0x05)
#define BUCK3_2_20		(0x06)
#define BUCK3_2_30		(0x07)
#define BUCK3_2_40		(0x08)
#define BUCK3_2_50		(0x09)
#define BUCK3_2_60		(0x0A)
#define BUCK3_2_70		(0x0B)
#define BUCK3_2_80		(0x0C)
#define BUCK3_2_90		(0x0D)
#define BUCK3_3_00		(0x0E)
#define BUCK3_3_10		(0x0F)
#define BUCK3_3_20		(0x10)
#define BUCK3_3_30		(0x11)
#define BUCK3_3_40		(0x12)
#define BUCK3_3_50		(0x13)
#define BUCK3_3_60		(0x14)

/* LDO 2~3 */
#define LDO2TO3_0_80		(0x0)
#define LDO2TO3_0_85		(0x1)
#define LDO2TO3_0_90		(0x2)
#define LDO2TO3_0_95		(0x3)
#define LDO2TO3_1_00		(0x4)
#define LDO2TO3_1_05		(0x5)
#define LDO2TO3_1_10		(0x6)
#define LDO2TO3_1_15		(0x7)
#define LDO2TO3_1_20		(0x8)
#define LDO2TO3_1_25		(0x9)
#define LDO2TO3_1_30		(0xA)

/* LDO 4~7 */
#define LDO4TO7_1_60		(0x00)
#define LDO4TO7_1_70		(0x01)
#define LDO4TO7_1_80		(0x02)
#define LDO4TO7_1_90		(0x03)
#define LDO4TO7_2_00		(0x04)
#define LDO4TO7_2_10		(0x05)
#define LDO4TO7_2_20		(0x06)
#define LDO4TO7_2_30		(0x07)
#define LDO4TO7_2_40		(0x08)
#define LDO4TO7_2_50		(0x09)
#define LDO4TO7_2_60		(0x0A)
#define LDO4TO7_2_70		(0x0B)
#define LDO4TO7_2_80		(0x0C)
#define LDO4TO7_2_90		(0x0D)
#define LDO4TO7_3_00		(0x0E)
#define LDO4TO7_3_10		(0x0F)
#define LDO4TO7_3_20		(0x10)
#define LDO4TO7_3_30		(0x11)
#define LDO4TO7_3_40		(0x12)
#define LDO4TO7_3_50		(0x13)
#define LDO4TO7_3_60		(0x14)

/* LDO8 */
#define LDO8_3_00		(0x0)
#define LDO8_3_10		(0x1)
#define LDO8_3_20		(0x2)
#define LDO8_3_30		(0x3)
#define LDO8_3_40		(0x4)
#define LDO8_3_50		(0x5)
#define LDO8_3_60		(0x6)
#define BKCHR_2_90		(0x0)
#define BKCHR_3_00		(0x1)
#define BKCHR_3_10		(0x2)
#define BKCHR_3_20		(0x3)
#define BKCHR_3_30		(0x4)

/* LDO9 */
#define LDO9_1_60		(0x00)
#define LDO9_1_70		(0x01)
#define LDO9_1_80		(0x02)
#define LDO9_1_90		(0x03)
#define LDO9_2_00		(0x04)
#define LDO9_2_10		(0x05)
#define LDO9_2_20		(0x06)
#define LDO9_2_30		(0x07)
#define LDO9_2_40		(0x08)
#define LDO9_2_50		(0x09)
#define LDO9_2_60		(0x0A)
#define LDO9_2_70		(0x0B)
#define LDO9_2_80		(0x0C)
#define LDO9_2_90		(0x0D)
#define LDO9_3_00		(0x0E)
#define LDO9_3_10		(0x0F)
#define LDO9_3_20		(0x10)
#define LDO9_3_30		(0x11)
#define LDO9_3_40		(0x12)
#define LDO9_3_50		(0x13)
#define LDO9_3_60		(0x14)

/* LBCNFG */
#define LBHYST_100MV		(0x0)
#define LBHYST_200MV		(0x1)
#define LBHYST_300MV		(0x2)
#define LBHYST_400MV		(0x3)
#define LBTH_2_80		(0x0)
#define LBTH_2_90		(0x1)
#define LBTH_3_00		(0x2)
#define LBTH_3_10		(0x3)
#define LBTH_3_20		(0x4)
#define LBTH_3_30		(0x5)
#define LBTH_3_40		(0x6)
#define LBTH_3_50		(0x7)

/* I2C Control Register Address */
typedef enum {
	REG_ONOFF1 = 0,
	REG_ONOFF2,
	REG_ADISCHG_EN1,
	REG_ADISCHG_EN2,
	REG_DVSARM1_2,
	REG_DVSARM3_4,
	REG_DVSINT1_2,
	REG_BUCK3,
	REG_LDO2_3,
	REG_LDO4,
	REG_LDO5,
	REG_LDO6,
	REG_LDO7,
	REG_LDO8,
	REG_LDO9,
	REG_LBCNFG,

	ENDOFREG
} max8698_pm_register_type;

typedef enum {
	/* ONOFF1 register */
	EN1,
	EN2,
	EN3,
	ELDO2,
	ELDO3,
	ELDO4,
	ELDO5,
	/* ONOFF2 register */
	ELDO6,
	ELDO7,
	ELDO8,
	ELDO9,
	ELBCNFG,
	// ADISCHG_EN1 register
	BUCK1_ADEN,
	BUCK2_ADEN,
	BUCK3_ADEN,
	LDO2_ADEN,
	LDO3_ADEN,
	LDO4_ADEN,
	LDO5_ADEN,
	LDO6_ADEN,
	// ADISCHG_EN2 register
	LDO7_ADEN,
	LDO8_ADEN,
	LDO9_ADEN,
	RAMP,
	// DVSARM1_2 register
	DVSARM2,
	DVSARM1,
	// DVSARM3_4 register
	DVSARM4,
	DVSARM3,
	// DVSINT1_2 register
	DVSINT2,
	DVSINT1,
	// BUCK3 register
	BUCK3,
	// LDO2_3 register
	LDO3,
	LDO2,
	// LDO4 register
	LDO4,
	// LDO5 register
	LDO5,
	// LDO6 register
	LDO6,
	// LDO7 register
	LDO7,
	// LDO8 & BKCHR register
	LDO8,
	BKCHR,
	// LDO9 register
	LDO,
	// LBCNFG register
	LBHYST,
	LBTH,

	ENDOFPM
} max8698_pm_function_type;

/* MAX8698 each register info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
} max8698_register_type;

/* MAX8698 each function info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
	const byte  mask;
	const byte  clear;
	const byte  shift;
} max8698_function_type;


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
extern boolean Set_MAX8698_PM_REG(max8698_pm_function_type reg_num, byte value);

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
extern boolean Get_MAX8698_PM_REG(max8698_pm_function_type reg_num, byte *reg_buff);

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
extern boolean Set_MAX8698_PM_ADDR(max8698_pm_register_type reg_addr, byte *reg_buff, byte length);

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
extern boolean Get_MAX8698_PM_ADDR(max8698_pm_register_type reg_addr, byte *reg_buff, byte length);

#endif /* __LINUX_MAX8698_H */

