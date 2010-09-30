/* arch/arm/plat-s3c64xx/pwm-s3c6410.h
 *
 * Copyright (C) 2003,2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Samsung S3C PWM support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Changelog:
 *  ??-May-2003 BJD   Created file
 *  ??-Jun-2003 BJD   Added more dma functionality to go with arch
 *  10-Nov-2004 BJD   Added sys_device support
*/

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H __FILE__

#include <linux/sysdev.h>
#include <plat/regs-timer.h>


#define pwmch_t int

/* we have 4 pwm channels */
#define S3C_PWM_CHANNELS        5
#define PRESCALER 		((4-1)/2)

struct s3c_pwm_client {
	char                *name;
};

typedef struct s3c_pwm_client s3c_pwm_client_t;


typedef struct s3c_pwm_chan_s s3c6410_pwm_chan_t;

/* s3c_pwm_cbfn_t
 *
 * buffer callback routine type
*/

typedef void (*s3c_pwm_cbfn_t)(void *buf);



/* struct s3c_pwm_chan_s
 *
 * full state information for each DMA channel
*/

struct s3c_pwm_chan_s {
	/* channel state flags and information */
	unsigned char          number;      /* number of this dma channel */
	unsigned char          in_use;      /* channel allocated */
	unsigned char          irq_claimed; /* irq claimed for channel */
	unsigned char          irq_enabled; /* irq enabled for channel */

	/* channel state */

	s3c_pwm_client_t  *client;
	void 	*dev;
	/* channel configuration */
	unsigned int           flags;        /* channel flags */

	/* channel's hardware position and configuration */
	unsigned int           irq;          /* channel irq */

	/* driver handles */
	s3c_pwm_cbfn_t     callback_fn;  /* buffer done callback */

	/* system device */
	struct sys_device	sysdev;
};

#endif /* __ASM_ARCH_DMA_H */
