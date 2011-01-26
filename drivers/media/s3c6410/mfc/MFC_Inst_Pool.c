/* mfc/MFC_Inst_Pool.c
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung S3C MFC driver
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

#include "MfcConfig.h"
#include "MFC_Inst_Pool.h"

#if !defined(MFC_NUM_INSTANCES_MAX)
#error "MFC_NUM_INSTANCES_MAX should be defined."
#endif
#if ((MFC_NUM_INSTANCES_MAX <= 0) || (MFC_NUM_INSTANCES_MAX > 8))
	#error "MFC_NUM_INSTANCES_MAX should be in the range of 1 ~ 8."
#endif



static int _inst_no = 0;
static int _inst_status[MFC_NUM_INSTANCES_MAX] = {0, };
static int _num_inst_avail = MFC_NUM_INSTANCES_MAX;


int MfcInstPool_NumAvail(void)
{
	return _num_inst_avail;
}


int MfcInstPool_Occupy(void)
{
	int  i;

	if (_num_inst_avail == 0)
		return -1;

	for (i=0; i<MFC_NUM_INSTANCES_MAX; i++) {
		if (_inst_status[_inst_no] == 0) {
			_num_inst_avail--;
			_inst_status[_inst_no] = 1;
			return _inst_no;
		}

		_inst_no = (_inst_no + 1) % MFC_NUM_INSTANCES_MAX;
	}

	return -1;
}


int MfcInstPool_Release(int instance_no)
{
	if (instance_no >= MFC_NUM_INSTANCES_MAX || instance_no < 0) {
		return -1;
	}

	if (_inst_status[instance_no] == 0)
		return -1;

	_num_inst_avail++;
	_inst_status[instance_no] = 0;

	return instance_no;
}


void MfcInstPool_OccupyAll(void)
{
	int  i;

	if (_num_inst_avail == 0)
		return;

	for (i=0; i<MFC_NUM_INSTANCES_MAX; i++) {
		if (_inst_status[i] == 0) {
			_num_inst_avail--;
			_inst_status[i] = 1;
		}
	}
}

void MfcInstPool_ReleaseAll(void)
{
	int  i;

	if (_num_inst_avail == MFC_NUM_INSTANCES_MAX)
		return;

	for (i=0; i<MFC_NUM_INSTANCES_MAX; i++) {
		if (_inst_status[i] == 1) {
			_num_inst_avail++;
			_inst_status[i] = 0;
		}
	}
}
