/* linux/drivers/media/video/samsung/jpeg_v2/jpg_msg.h
 *
 * Driver header file for Samsung JPEG Encoder/Decoder
 *
 * Peter Oh, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __SAMSUNG_SYSLSI_APDEV_log_msg_H__
#define __SAMSUNG_SYSLSI_APDEV_log_msg_H__


typedef enum {
	LOG_TRACE   = 0,
	LOG_WARNING = 1,
	LOG_ERROR   = 2
} log_level;


#ifdef __cplusplus
extern "C" {
#endif


	void log_msg(log_level level, const char *func_name, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_log_msg_H__ */
