#ifndef __LINUX_PMIC_H
#define __LINUX_PMIC_H

#if defined (CONFIG_PMIC_MAX8906)
#include <linux/i2c/max8906.h>
#elif defined (CONFIG_PMIC_MAX8698)
#include <linux/i2c/max8698.h>
#endif

/*===========================================================================

FUNCTION set_pmic

DESCRIPTION
    This function turn on / off block power or change voltage
    in the PM section.

INPUT PARAMETERS
    max8698_pm_type pm_type   : type of power control.
    int value	: value for changing state

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/

typedef enum {
	VCC_ARM,
	VCC_INT,
	ENDOFPMTYPE
} pmic_pm_type;

extern boolean set_pmic(pmic_pm_type pm_type, int value);
extern boolean get_pmic(pmic_pm_type pm_type, int *value);

#endif /* __LINUX_PMIC_H */
