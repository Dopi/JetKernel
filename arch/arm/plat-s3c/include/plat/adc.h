/* arch/arm/plat-s3c/include/plat/adc.h
 *
 * Copyright (c) 2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_ADC_H
#define __ASM_ARCH_ADC_H __FILE__


struct s3c_adc_request
{
	/* for linked list */
	struct list_head *list;
	/* after finish ADC sampling, s3c_adc_request function call this function with three parameter */
	void (*callback)( int channel, unsigned long int param, unsigned short sample);
	/* for private data */
	unsigned long int param;
	/* selected channel for ADC sampling */
	int channel;
};

struct s3c_adc_mach_info
{
	/* if you need to use some platform data, add in here*/
	int delay;
	int presc;
	int resolution;
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd);
int s3c_adc_get(struct s3c_adc_request *req);

#endif /* __ASM_ARCH_ADC_H */
