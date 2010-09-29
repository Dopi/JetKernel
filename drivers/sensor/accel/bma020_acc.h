#ifndef __BMA020_ACC_HEADER__
#define __BMA020_ACC_HEADER__

#include "bma020_i2c.h"
#include "bma020.h"

#define ACC_DEV_NAME "accelerometer"
#define ACC_DEV_MAJOR 241

#define BMA150_MAJOR 	100

/* bma020 ioctl command label */
#define IOCTL_BMA020_GET_ACC_VALUE		0
#define DCM_IOC_MAGIC			's'
#define IOC_SET_ACCELEROMETER	_IO (DCM_IOC_MAGIC, 0x64)


#define BMA020_POWER_OFF               0
#define BMA020_POWER_ON                1

#endif
