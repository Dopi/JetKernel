#ifndef _LINUX_WL127X_RFKILL_H
#define _LINUX_WL127X_RFKILL_H

#include <linux/rfkill.h>

struct smartq_rfkill_platform_data {
	int nshutdown_gpio;
	struct rfkill *rfkill;  	/* for driver only */
};

#endif

