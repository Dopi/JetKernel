/*
 *  Card-specific functions for the Siano SMS1xxx USB dongle
 *
 *  Copyright (c) 2008 Michael Krufky <mkrufky@linuxtv.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  Software distributed under the License is distributed on an "AS IS"
 *  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SMS_CARDS_H__
#define __SMS_CARDS_H__

#include <linux/usb.h>
#include "smscoreapi.h"

#define SMS_BOARD_UNKNOWN 0
#define SMS1XXX_BOARD_SIANO_STELLAR 1
#define SMS1XXX_BOARD_SIANO_NOVA_A  2
#define SMS1XXX_BOARD_SIANO_NOVA_B  3
#define SMS1XXX_BOARD_SIANO_VEGA    4
#define SMS1XXX_BOARD_HAUPPAUGE_CATAMOUNT 5
#define SMS1XXX_BOARD_HAUPPAUGE_OKEMO_A 6
#define SMS1XXX_BOARD_HAUPPAUGE_OKEMO_B 7
#define SMS1XXX_BOARD_HAUPPAUGE_WINDHAM 8
#define SMS1XXX_BOARD_HAUPPAUGE_TIGER_MINICARD 9
#define SMS1XXX_BOARD_HAUPPAUGE_TIGER_MINICARD_R2 10

struct sms_board {
	enum sms_device_type_st type;
	char *name, *fw[DEVICE_MODE_MAX];

	/* gpios */
	int led_power, led_hi, led_lo, lna_ctrl;
};

struct sms_board *sms_get_board(int id);

int sms_board_setup(struct smscore_device_t *coredev);

#define SMS_LED_OFF 0
#define SMS_LED_LO  1
#define SMS_LED_HI  2
int sms_board_led_feedback(struct smscore_device_t *coredev, int led);
int sms_board_power(struct smscore_device_t *coredev, int onoff);

extern struct usb_device_id smsusb_id_table[];

#endif /* __SMS_CARDS_H__ */
