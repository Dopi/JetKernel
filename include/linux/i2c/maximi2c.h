#if defined (CONFIG_PMIC_MAX8906)
#include <linux/i2c/max8906.h>
#elif defined (CONFIG_PMIC_MAX8698)
#include <linux/i2c/max8698.h>
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



