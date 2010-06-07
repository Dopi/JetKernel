#ifndef _ASM_X86_DMI_H
#define _ASM_X86_DMI_H

#include <asm/io.h>

#define DMI_MAX_DATA 2048

extern int dmi_alloc_index;
extern char dmi_alloc_data[DMI_MAX_DATA];

/* This is so early that there is no good way to allocate dynamic memory.
   Allocate data in an BSS array. */
static inline void *dmi_alloc(unsigned len)
{
	int idx = dmi_alloc_index;
	if ((dmi_alloc_index + len) > DMI_MAX_DATA)
		return NULL;
	dmi_alloc_index += len;
	return dmi_alloc_data + idx;
}

/* Use early IO mappings for DMI because it's initialized early */
#define dmi_ioremap early_ioremap
#define dmi_iounmap early_iounmap

#endif /* _ASM_X86_DMI_H */
