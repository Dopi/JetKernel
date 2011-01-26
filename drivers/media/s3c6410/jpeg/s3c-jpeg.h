/* jpeg/s3c-jpeg.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C JPEG driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __JPEG_DRIVER_H__
#define __JPEG_DRIVER_H__

#define MAX_INSTANCE_NUM	10

#define IOCTL_JPG_DECODE				0x00000002
#define IOCTL_JPG_ENCODE				0x00000003
#define IOCTL_JPG_SET_STRBUF			0x00000004
#define IOCTL_JPG_SET_FRMBUF			0x00000005
#define IOCTL_JPG_SET_THUMB_STRBUF		0x0000000A
#define IOCTL_JPG_SET_THUMB_FRMBUF		0x0000000B

#endif /*__JPEG_DRIVER_H__*/
