/* mfc/MFC_HW_Init.c
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

#include "Mfc.h"
#include "MfcSfr.h"
#include "BitProcBuf.h"
#include "DataBuf.h"
#include "MfcTypes.h"
#include "LogMsg.h"
#include "MfcConfig.h"
#include "FramBufMgr.h"


BOOL MFC_MemorySetup(void)
{
	//MfcMemMapping();
	BOOL ret_sfr, ret_bit, ret_dat;
	unsigned char *pDataBuf;


	// MFC SFR(Special Function Registers), Bitprocessor buffer, Data buffer의 
	// physical address 를 virtual address로 mapping 한다 
	ret_sfr	= MfcSfrMemMapping();
	if (ret_sfr == FALSE) {
		LOG_MSG(LOG_ERROR, "MfcMemorySetup", "MFC SFR Memory mapping fail!\r\n");
		return FALSE;
	}

	ret_bit = MfcBitProcBufMemMapping();
	if (ret_bit == FALSE) {
		LOG_MSG(LOG_ERROR, "MfcMemorySetup", "Bitprocessor buffer Memory mapping fail!\r\n");
		return FALSE;
	}

	ret_dat	= MfcDataBufMemMapping();
	if (ret_dat == FALSE) {
		LOG_MSG(LOG_ERROR, "MfcMemorySetup", "Data buffer Memory mapping fail!\r\n");
		return FALSE;
	}


	// FramBufMgr Module Initialization
	pDataBuf = (unsigned char *)GetDataBufVirAddr();
	FramBufMgrInit(pDataBuf + MFC_STRM_BUF_SIZE, MFC_FRAM_BUF_SIZE);


	return TRUE;
}


BOOL MFC_HW_Init(void)
{
	/////////////////////////
	//                     //
	// 1. Reset the MFC IP //
	//                     //
	/////////////////////////
	MfcReset();


	////////////////////////////////////////
	//                                    //
	// 2. Download Firmware code into MFC //
	//                                    //
	////////////////////////////////////////
	MfcFirmwareIntoCodeBuf();
	MfcFirmwareIntoCodeDownReg();
	LOG_MSG(LOG_TRACE, "MFC_HW_Init", "Download  FirmwareIntoBitProcessor  OK.\n");


	////////////////////////////
	//                        //
	// 3. Start Bit Processor //
	//                        //
	////////////////////////////
	MfcStartBitProcessor();


	////////////////////////////////////////////////////////////////////
	//                                                                //
	// 4. Set the Base Address Registers for the following 3 buffers  //
	//        (CODE_BUF, WORKING_BUF, PARAMETER_BUF)                  //
	//                                                                //
	////////////////////////////////////////////////////////////////////
	MfcConfigSFR_BITPROC_BUF();


	//////////////////////////////////
	//                              //
	// 5. Set the Control Registers //
	//       - STRM_BUF_CTRL        //
	//       - FRME_BUF_CTRL        //
	//       - DEC_FUNC_CTRL        //
	//       - WORK_BUF_CTRL        //
	//                              //
	//////////////////////////////////
	MfcConfigSFR_CTRL_OPTS();



	GetFirmwareVersion();


	return TRUE;
}

