/*****************************************************************************
** COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2011 ALL RIGHTS RESERVED
** AUTHOR		: KyoungHOON Kim (khoonk)
******************************************************************************
**
** VERSION&DATE	: Version 1.00	2006/06/02 (khoonk)
** VERSION&DATE	: Version 2.00	2009/08/11 (mizzibi)
** 
*****************************************************************************/
#ifndef	DEBUGPRINTF_H
#define	DEBUGPRINTF_H

//extern unsigned long long	iPrintFlag;
extern int 					debug_check(unsigned long long flag);

#ifdef CONFIG_SEC_DPRINTK
#define	dprintk(x, y...)	debug_check(x) ? printk("["#x"] "y) : 0
#else
#define dprintk(x,y...) do { } while(0)
#endif

/* flag defintion for MODULE (can be ORed) */
#define ODR_WR			0x0000000000000001
#define ODR_RD			0x0000000000000002
#define ODR_IRQ			0x0000000000000004
#define ODR_MAP			0x0000000000000008

#define TSP_KEY			0x0000000000000010
#define TSP_ABS			0x0000000000000020
#define KPD_PRS			0x0000000000000040
#define KPD_RLS			0x0000000000000080

#endif
