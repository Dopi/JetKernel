/*
 * USB Driver for ALi m5602 based webcams
 *
 * Copyright (C) 2008 Erik Andrén
 * Copyright (C) 2007 Ilyes Gouta. Based on the m5603x Linux Driver Project.
 * Copyright (C) 2005 m5603x Linux Driver Project <m5602@x3ng.com.br>
 *
 * Portions of code to USB interface and ALi driver software,
 * Copyright (c) 2006 Willem Duinker
 * v4l2 interface modeled after the V4L2 driver
 * for SN9C10x PC Camera Controllers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 *
 */

#ifndef M5602_SENSOR_H_
#define M5602_SENSOR_H_

#include "m5602_bridge.h"

#define M5602_DEFAULT_FRAME_WIDTH  640
#define M5602_DEFAULT_FRAME_HEIGHT 480

#define M5602_MAX_CTRLS		(V4L2_CID_LASTP1 - V4L2_CID_BASE + 10)

/* Enumerates all supported sensors */
enum sensors {
	OV9650_SENSOR	= 1,
	S5K83A_SENSOR	= 2,
	S5K4AA_SENSOR	= 3,
	MT9M111_SENSOR	= 4,
	PO1030_SENSOR	= 5
};

/* Enumerates all possible instruction types */
enum instruction {
	BRIDGE,
	SENSOR,
	SENSOR_LONG
};

struct m5602_sensor {
	/* Defines the name of a sensor */
	char name[32];

	/* What i2c address the sensor is connected to */
	u8 i2c_slave_id;

	/* Width of each i2c register (in bytes) */
	u8 i2c_regW;

	/* Probes if the sensor is connected */
	int (*probe)(struct sd *sd);

	/* Performs a initialization sequence */
	int (*init)(struct sd *sd);

	/* Executed when the camera starts to send data */
	int (*start)(struct sd *sd);

	/* Performs a power down sequence */
	int (*power_down)(struct sd *sd);

	int nctrls;
	struct ctrl ctrls[M5602_MAX_CTRLS];

	char nmodes;
	struct v4l2_pix_format modes[];
};

#endif
