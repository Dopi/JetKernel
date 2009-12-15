/*
* ==============================================================================
*  Name 	   : MAX9877_Def.h
*  Part of		: audio codec driver
*  Description : max9877 audio amp driver 
*  Version		: 1
*  Author		: Changoh.heo (changoh.heo@samsung.com)
*
*  Copyright (c) 2008 Samsung Electronics Co., Ltd.
*  This material, including documentation and any related 
*  computer programs, is protected by copyright controlled by 
*  Samsung Electronics Co., Ltd. All rights are reserved. Copying, 
*  including reproducing, storing, adapting or translating, any 
*  or all of this material requires the prior written consent of 
*  Samsung Electronics Co., Ltd. This material also contains confidential 
*  information which may not be disclosed to others without the 
*  prior written consent of Electronics Co., Ltd.
* ==============================================================================
*/

#ifndef __MAX9877Reg_H__
#define __MAX9877Reg_H__

#define I2C_MAX9877_SLAVE_ADDR		0x9A // 1001 101R/W

#define MAX9877_INPUT_MODE_CONTROL			0x00
#define MAX9877_SPEAKER_VOLUME 				0x01
#define MAX9877_LEFT_HEADPHONE_VOLUME 		0x02
#define MAX9877_RIGHT_HEADPHONE_VOLUME		0x03
#define MAX9877_OUTPUT_MODE_CONTROL			0x04

/* MAX9877_INPUT_MODE_CONTROL (0x00) */
#define ZCD						(0x1 << 6)
#define INA						(0x1 << 5)
#define INB						(0x1 << 4)
#define PGAINA1					(0x1 << 3)
#define PGAINA0					(0x1 << 2)
#define PGAINB1					(0x1 << 1)
#define PGAINB0					(0x1 << 0)
#define	  PGAINB				(PGAINB0 | PGAINB1)
#define   PGAINA				(PGAINA0 | PGAINA1)


/* MAX9877_SPEAKER_VOLUME (0x01) */
#define SVOL4 					(0x1 << 4)
#define SVOL3 					(0x1 << 3)
#define SVOL2 					(0x1 << 2)
#define SVOL1 					(0x1 << 1)
#define SVOL0 					(0x1 << 0)

/* MAX9877_LEFT_HEADPHONE_VOLUME (0x02) */
#define HPLVOL4					(0x1 << 4)
#define HPLVOL3					(0x1 << 3)
#define HPLVOL2					(0x1 << 2)
#define HPLVOL1					(0x1 << 1)
#define HPLVOL0					(0x1 << 0)


/* MAX9877_LEFT_HEADPHONE_VOLUME (0x03) */
#define HPRVOL4 				(0x1 << 4)
#define HPRVOL3 				(0x1 << 3)
#define HPRVOL2 				(0x1 << 2)
#define HPRVOL1 				(0x1 << 1)
#define HPRVOL0 				(0x1 << 0)
#define	MAX9877_OUTPUT_MUTE		0x0

/* MAX9877_LEFT_HEADPHONE_VOLUME (0x04) */
#define SHDN					(0x1 << 7)
#define BYPASS					(0x1 << 6)
#define OSC1					(0x1 << 5)
#define OSC0 					(0x1 << 4)
#define OUTMODE3 				(0x1 << 3)
#define OUTMODE2 				(0x1 << 2)
#define OUTMODE1 				(0x1 << 1)
#define OUTMODE0 				(0x1 << 0)
#define OUTMODE 				(OUTMODE0 | OUTMODE1 | OUTMODE2 | OUTMODE3)


#if 0
const TUint8 KMAX9877DefaultValueTable[] =
{
	/*00*/0x40,/*01*/0x00,/*02*/0x00,/*03*/0x00,/*04*/0x49
};
#endif

//0:0dB, 1:9dB, 2:20dB, 3:Reserved
#define KMAX9877_Preamp_Gain(a) 		((a == 0) ? 0 : ((a == 9) ? 1 : ((a == 20) ? 2 : 0)))
//24:-7dB, 25:-6dB, 26:-5dB ~ 31:0 dB
#define KMAX9877_Output_Gain_0ToMinus7(a) 		((a  + 7) / 1) + 24
//16:-23dB, 17:-21dB, 18:-19dB ~ 23:-9dB
#define KMAX9877_Output_Gain_Minus9ToMinus23(a)		((a  + 23) / 2) + 16
//8:-47dB, 9:-44dB, 10:-41dB ~ 16:-23dB
#define KMAX9877_Output_Gain_Minus23ToMinus47(a)		((a  + 47) / 3) + 8
//0:mute, 1:-75dB, 2:-71DB ~ 7:-51dB
#define KMAX9877_Output_Gain_Minus51ToMUTE(a)		((a  + 79) / 4)
#endif
