/* mfc/DataBuf.c
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
#include "MfcTypes.h"
#include "MfcMemory.h"
#include "LogMsg.h"
#include "DataBuf.h"
#include "MfcConfig.h"

static volatile unsigned char     *vir_pDATA_BUF      = NULL;

static unsigned int                phyDATA_BUF     = 0;


BOOL MfcDataBufMemMapping()
{
	BOOL	ret = FALSE;

	// STREAM BUFFER, FRAME BUFFER  <-- virtual data buffer address mapping
	vir_pDATA_BUF = (volatile unsigned char *)Phy2Vir_AddrMapping(S3C6400_BASEADDR_MFC_DATA_BUF, MFC_DATA_BUF_SIZE);
	if (vir_pDATA_BUF == NULL)
	{
		LOG_MSG(LOG_ERROR, "MfcDataBufMapping", "For DATA_BUF: VirtualAlloc failed!\r\n");
		return ret;
	}
	LOG_MSG(LOG_TRACE, "MfcDataBufMapping", "VIRTUAL ADDR DATA BUF : 0x%X\n", vir_pDATA_BUF);

	// Physical register address mapping
	phyDATA_BUF	= S3C6400_BASEADDR_MFC_DATA_BUF;


	ret = TRUE;

	return ret;
}

volatile unsigned char *GetDataBufVirAddr()
{
	volatile unsigned char	*pDataBuf;

	pDataBuf	= vir_pDATA_BUF;

	return pDataBuf;	
}

volatile unsigned char *GetFramBufVirAddr()
{
	volatile unsigned char	*pFramBuf;

	pFramBuf	= vir_pDATA_BUF + MFC_STRM_BUF_SIZE;

	return pFramBuf;	
}

/*
  * virtual address of DBK_BUF is returned
  *
  * 2009.5.12 by yj (yunji.kim@samsung.com)
  */
volatile unsigned char *GetDbkBufVirAddr()
{
	volatile unsigned char	*pDbkBuf;

	pDbkBuf	= vir_pDATA_BUF + MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE;

	return pDbkBuf;	
}

unsigned int GetDataBufPhyAddr()
{
	unsigned int	phyDataBuf;

	phyDataBuf	= phyDATA_BUF;

	return phyDataBuf;
}

unsigned int GetFramBufPhyAddr()
{
	unsigned int	phyFramBuf;

	phyFramBuf	= phyDATA_BUF + MFC_STRM_BUF_SIZE;

	return phyFramBuf;
}

/*
  * physical address of DBK_BUF is returned
  *
  * 2009.5.12 by yj (yunji.kim@samsung.com)
  */
unsigned int GetDbkBufPhyAddr()
{
	unsigned int	phyDbkBuf;

	phyDbkBuf	= phyDATA_BUF + MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE;

	return phyDbkBuf;	
}
