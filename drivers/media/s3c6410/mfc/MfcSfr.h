/* mfc/MfcSfr.h
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
 
#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__

#include "MfcTypes.h"
#include "Mfc.h"

#ifdef __cplusplus
extern "C" {
#endif

int MFC_Sleep(void);
int MFC_Wakeup(void);
BOOL MfcIssueCmd(int inst_no, MFC_CODECMODE codec_mode, MFC_COMMAND mfc_cmd);
int  GetFirmwareVersion(void);
BOOL MfcSfrMemMapping(void);
volatile S3C6400_MFC_SFR *GetMfcSfrVirAddr(void);
void *MfcGetCmdParamRegion(void);
	
void MfcReset(void);
void MfcClearIntr(void);
unsigned int MfcIntrReason(void);
void MfcSetEos(int buffer_mode);
void MfcStreamEnd(void);
void MfcFirmwareIntoCodeDownReg(void);
void MfcStartBitProcessor(void);
void MfcStopBitProcessor(void);
void MfcConfigSFR_BITPROC_BUF(void);
void MfcConfigSFR_CTRL_OPTS(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__ */
