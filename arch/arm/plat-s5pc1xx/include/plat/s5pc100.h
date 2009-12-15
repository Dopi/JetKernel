/* arch/arm/plat-s5pc1xx/include/plat/s5pc100.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Header file for s5pc100 cpu support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

/* Common init code for S5PC100 related SoCs */

extern void s5pc100_common_init_uarts(struct s3c2410_uartcfg *cfg, int no);
extern void s5pc100_register_clocks(void);
extern void s5pc100_setup_clocks(void);

extern  int s5pc100_init(void);
extern void s5pc100_init_irq(void);
extern void s5pc100_map_io(void);
extern void s5pc100_init_clocks(int xtal);

#define s5pc100_init_uarts s5pc100_common_init_uarts

