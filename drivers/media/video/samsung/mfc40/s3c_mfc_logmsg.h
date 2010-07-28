/* 
 * drivers/media/video/samsung/mfc40/s3c_mfc_logmsg.h
 *
 * Header file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _S3C_MFC_LOGMSG_H_
#define _S3C_MFC_LOGMSG_H_

/* debug macros */
#define MFC_DEBUG(fmt, ...)					\
	do {							\
		printk(KERN_DEBUG				\
			"%s: " fmt, __func__, ##__VA_ARGS__);	\
	} while(0)

#define MFC_ERROR(fmt, ...)					\
	do {							\
		printk(KERN_ERR					\
			"%s: " fmt, __func__, ##__VA_ARGS__);	\
	} while (0)

#define MFC_NOTICE(fmt, ...)					\
	do {							\
		printk(KERN_NOTICE				\
			fmt, ##__VA_ARGS__);			\
	} while (0)

#define MFC_INFO(fmt, ...)					\
	do {							\
		printk(KERN_INFO				\
			fmt, ##__VA_ARGS__);			\
	} while (0)

#define MFC_WARN(fmt, ...)					\
	do {							\
		printk(KERN_WARNING				\
			fmt, ##__VA_ARGS__);			\
	} while (0)


#ifdef CONFIG_VIDEO_MFC40_DEBUG
#define mfc_debug(fmt, ...)		MFC_DEBUG(fmt, ##__VA_ARGS__)
#else
#define mfc_debug(fmt, ...)
#endif

#define mfc_err(fmt, ...)		MFC_ERROR(fmt, ##__VA_ARGS__)
#define mfc_notice(fmt, ...)		MFC_NOTICE(fmt, ##__VA_ARGS__)
#define mfc_info(fmt, ...)		MFC_INFO(fmt, ##__VA_ARGS__)
#define mfc_warn(fmt, ...)		MFC_WARN(fmt, ##__VA_ARGS__)

#endif /* _S3C_MFC_LOGMSG_H_ */
