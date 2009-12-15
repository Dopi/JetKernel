/* ------------------------------------------------------------------------- */
/* 									     */
/* spi.h - definitions for the spi-bus interface			     */
/* 									     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2006 Samsung Electronics Co. ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		     */
/* ------------------------------------------------------------------------- */

#ifndef _LINUX_SPI_H
#define _LINUX_SPI_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>	/* for struct device */
#include <linux/semaphore.h>

/* --- General options ------------------------------------------------	*/

struct spi_msg;
struct spi_dev;
struct spi_algorithm;

/*
 * The master routines are the ones normally used to transmit data to devices
 * on a bus (or/and read from them). Apart from these basic transfer functions
 * to transmit one message at a time, a more complex version can be used to
 * transmit an arbitrary number of messages without interruption.
 */
extern int spi_master_send(struct spi_dev *,const char* ,int);
extern int spi_master_recv(struct spi_dev *,char* ,int);

#define SPI_CHANNEL 0
#define BUFFER_SIZE     65536
#define SPI_MINORS      2

/*
 * A driver is capable of handling one or more physical devices present on
 * SPI adapters. This information is used to inform the driver of adapter
 * events.
 */

struct spi_dev {
	int 			minor;

	dma_addr_t		dmabuf;		/* handle for DMA transfer		*/
	unsigned int 		flags;		/* flags for the SPI operation 		*/
	struct semaphore 	bus_lock;	/* semaphore for bus access 		*/

	struct spi_algorithm 	*algo;		/* the algorithm to access the bus	*/
	void 			*algo_data;	/* the bus control struct		*/

	int 			timeout;
	int 			retries;
	struct device		dev;		/* the adapter device 			*/
};

/*
 * The following structs are for those who like to implement new bus drivers:
 * spi_algorithm is the interface to a class of hardware solutions which can
 * be addressed using the same bus algorithms - i.e. bit-banging or the PCF8584
 * to name two of the most common.
 */
struct spi_algorithm {
	char name[32];				/* textual description 	*/
	unsigned int id;

	/* If an adapter algorithm can't to SPI-level access, set master_xfer
	   to NULL. If an adapter algorithm can do SMBus access, set
	   smbus_xfer. If set to NULL, the SMBus protocol is simulated
	   using common SPI messages */
	int (*master_xfer)(struct spi_dev *spi_dev,struct spi_msg *msgs,
	                   int num);

	/* --- ioctl like call to set div. parameters. */
	int (*algo_control)(struct spi_dev *, unsigned int, unsigned long);

	/* To determine what the adapter supports */
	u32 (*functionality) (struct spi_dev *);
	int (*close)(struct spi_dev *spi_dev);
};

/* ----- functions exported by spi.o */

/* administration...
 */
extern int spi_attach_spidev(struct spi_dev *);
extern int spi_detach_spidev(struct spi_dev *);

/*
 * SPI Message - used for pure spi transaction, also from /dev interface
 */

#define SPI_M_MODE_MASTER	0x001
#define SPI_M_MODE_SLAVE	0x002
#define SPI_M_USE_FIFO		0x004
#define SPI_M_CPOL_ACTHIGH	0x010
#define SPI_M_CPOL_ACTLOW	0x020
#define SPI_M_CPHA_FORMATA	0x040
#define SPI_M_CPHA_FORMATB	0x080
#define SPI_M_DMA_MODE		0x100
#define SPI_M_INT_MODE		0x200
#define SPI_M_POLL_MODE		0x400
#define SPI_M_DEBUG		0x800
#define SPI_M_FIFO_POLL		0x1000


struct spi_msg {
 	__u16 flags;
 	__u16 len;		/* msg length				*/
 	__u8 *wbuf;		/* pointer to msg data to write	*/
 	__u8 *rbuf;		/* pointer to msg data for read */
};

/* ----- commands for the ioctl call:	*/
				/* -> spi-adapter specific ioctls	*/
#define SET_SPI_RETRIES	0x0701	/* number of times a device address      */
				/* should be polled when not            */
                                /* acknowledging 			*/
#define SET_SPI_TIMEOUT	0x0702	/* set timeout - call with int 		*/
#define SET_SPI_FLAGS	0x0704  /* set flags for h/w settings 		*/

#define SPI_MAJOR	153		/* Device major number		*/
					/* minor 0-15 spi0 - spi15 	*/

#endif /* _LINUX_SPI_H */

