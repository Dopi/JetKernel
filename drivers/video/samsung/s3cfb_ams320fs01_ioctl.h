#ifndef _S3CFB_AMS320FS01_IOCTL_H
#define _S3CFB_AMS320FS01_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#include "s3cfb.h"

/*****************IOCTLS******************/
/*magic no*/
#define AMS320FS01_IOC_MAGIC  0xFB
/*max seq no*/
#define AMS320FS01_IOC_NR_MAX 3

/*commands*/

#define AMS320FS01_IOC_GAMMA22                        _IO(AMS320FS01_IOC_MAGIC, 0)

#define AMS320FS01_IOC_GAMMA19                        _IO(AMS320FS01_IOC_MAGIC, 1)

#define AMS320FS01_IOC_GAMMA17                        _IO(AMS320FS01_IOC_MAGIC, 2)

#endif


