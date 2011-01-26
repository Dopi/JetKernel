#if defined (CONFIG_PMIC_MAX8906)
#include <linux/i2c/max8906.h>
#elif defined (CONFIG_PMIC_MAX8698)
#include <linux/i2c/max8698.h>
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */
#ifndef NULL  // dgahn
#define NULL   0
#endif

#if !defined(CONFIG_PMIC_MAX8906) && !defined(CONFIG_PMIC_MAX8698)  // dgahn
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
#endif



typedef enum
{
  PMIC_PASS = 0,
  /* Operation was successful */
  PMIC_FAIL
  /* Write operation failed */
} pmic_status_type;


typedef word pmic_err_flag_type;

/*===========================================================================

FUNCTION pmic_write                                

DESCRIPTION
    It does the following: When we need to write a specific register in Power management section, This is used.
INPUT PARAMETERS
    byte slave_addr : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Write operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    slave_addr = 0x8E;
	reg = ONOFF1_ADDR;
	pmic_onoff_buf[i] = (( pmic_onoff_buf[i] & ~(mask))|(mask & data));
	
	if (pmic_write(slave_addr, reg, &pmic_onoff_buf[i], 1) != PMIC_PASS) {
		MSG_HIGH("Write Vreg control failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
extern pmic_status_type pmic_write(byte slave_addr, byte reg, byte *data, byte length);


/*===========================================================================

FUNCTION pmic_read                                

DESCRIPTION
    It does the following: When we need to read a specific register in Power management section, This is used
INPUT PARAMETERS
    byte slave_addr : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Read operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    slave_addr = 0x8E;
	if (pmic_read(slave_addr, IRQ1_ADDR, &irq1_reg, 1) != PMIC_PASS) {
		MSG_HIGH("Write Vreg control failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
extern pmic_status_type pmic_read(byte slave_addr, byte reg, byte *data, byte length);


/*===========================================================================

FUNCTION pmic_tsc_write                                

DESCRIPTION
    It does the following: When we need to write a specific register in Power management section, This is used.
INPUT PARAMETERS
    byte slave_addr : slave address
 	byte cmd : data with cmd bit ( data = [2:0], cmd = [7:3] )
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Write operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	cmd = X_DRIVE | 0x01; // enable CONT
	
	if (pmic_tsc_write(slave_addr, &cmd) != PMIC_PASS) {
		MSG_HIGH("Write TSC command failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
extern pmic_status_type pmic_tsc_write(byte slave_addr, byte *cmd);


/*===========================================================================

FUNCTION pmic_tsc_read                                

DESCRIPTION
    It does the following: When we need to read a specific register in Power management section, This is used
INPUT PARAMETERS
    byte slave_addr : slave address
 	byte cmd : data with cmd bit ( data = [2:0], cmd = [7:3] )
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Read operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	if (pmic_read(X_DRIVE, &cmd) != PMIC_PASS) {
		MSG_HIGH("Read TSC command failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
extern pmic_status_type pmic_tsc_read(byte slave_addr, byte *cmd);



