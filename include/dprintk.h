/*****************************************************************************
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2011 ALL RIGHTS RESERVED
** AUTHOR		: KyoungHOON Kim (khoonk)
******************************************************************************
**
** VERSION&DATE	: Version 1.00	2006/06/02 (khoonk)
** 
*****************************************************************************/
#ifndef	DEBUGPRINTF_H
#define	DEBUGPRINTF_H



//extern unsigned int		iPrintFlag;
extern unsigned long long	iPrintFlag;
extern int 					debug_check(unsigned int flag);

#define	dprintk(x, y...)	debug_check(x) ? printk("["#x"] "y) : 0
/* flag defintion for MODULE (can be ORed) */
#define DCM_ERR			0x0000000000000001
#define DCM_INP			0x0000000000000002
#define DCM_OUT			0x0000000000000004
#define DCM_DET			0x0000000000000008

#define PWR_ERR			0x0000000000000010
#define PWR_WRN			0x0000000000000020
#define PWR_DBG			0x0000000000000040
#define PWR_INF			0x0000000000000080

#define CAM_ERR			0x0000000000000100
#define CAM_WRN   		0x0000000000000200
#define CAM_DBG   		0x0000000000000400
#define CAM_INF   		0x0000000000000800

#define LCD_ERR			0x0000000000001000
#define LCD_WRN			0x0000000000002000
#define LCD_DBG			0x0000000000004000
#define LCD_INF			0x0000000000008000

#define SND_ERR			0x0000000000010000
#define SND_WRN			0x0000000000020000
#define SND_DBG			0x0000000000040000
#define SND_INF			0x0000000000080000

#define DVFS_ERR		0x0000000000100000
#define DVFS_WRN		0x0000000000200000
#define DVFS_DBG		0x0000000000400000
#define DVFS_INF		0x0000000000800000

#define BAT_ERR			0x0000000001000000
#define BAT_WRN			0x0000000002000000
#define BAT_DBG			0x0000000004000000
#define BAT_INF			0x0000000008000000

#define USB_ERR			0x0000000010000000
#define USB_WRN			0x0000000020000000
#define USB_DBG			0x0000000040000000
#define USB_INF			0x0000000080000000

#define BLZ_ERR			0x0000000100000000
#define BLZ_WRN			0x0000000200000000
#define BLZ_DBG			0x0000000400000000
#define BLZ_INF			0x0000000800000000

#define WLN_ERR			0x0000001000000000
#define WLN_WRN			0x0000002000000000
#define WLN_DBG			0x0000004000000000
#define WLN_INF			0x0000008000000000

#define MMC_ERR			0x0000010000000000
#define MMC_WRN			0x0000020000000000
#define MMC_DBG			0x0000040000000000
#define MMC_INF			0x0000080000000000

#define FS_ERR			0x0000100000000000
#define FS_UFD			0x0000200000000000
#define FS_ZFTL			0x0000400000000000
#define FS_EXT3			0x0000800000000000

#define OOM_ERR			0x0001000000000000
#define OOM_WRN			0x0002000000000000
#define OOM_DBG			0x0004000000000000
#define OOM_INF			0x0008000000000000
#endif


