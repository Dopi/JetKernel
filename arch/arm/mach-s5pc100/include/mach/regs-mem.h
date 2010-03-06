/* arch/arm/mach-s3c6400/include/mach/regs-mem.h
 *
 * Copyright (c) 2004 Simtec Electronics <linux@simtec.co.uk>
 *		http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2410 Memory Control register definitions
*/

#ifndef __ASM_ARM_MEMREGS_H
#define __ASM_ARM_MEMREGS_H

#ifndef S5PC1XX_MEMREG
#define S5PC1XX_MEMREG(x) (S5PC1XX_VA_SROMC + (x))
#endif


/* Bank Idle Cycle Control Registers 0-5 */
#define S5PC1XX_SROM_BW		S5PC1XX_MEMREG(0x00)

#define S5PC1XX_SROM_BC0	S5PC1XX_MEMREG(0x04)
#define S5PC1XX_SROM_BC1	S5PC1XX_MEMREG(0x08)
#define S5PC1XX_SROM_BC2	S5PC1XX_MEMREG(0x0C)
#define S5PC1XX_SROM_BC3	S5PC1XX_MEMREG(0x10)
#define S5PC1XX_SROM_BC4	S5PC1XX_MEMREG(0x14)
#define S5PC1XX_SROM_BC5	S5PC1XX_MEMREG(0x18)

#endif /* __ASM_ARM_MEMREGS_H */
