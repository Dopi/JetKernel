/*
 * qmath functions used in complex arithmetic/
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: qmath_complex.h,v 1.1.2.1 2009/09/09 20:58:00 Exp $
 *
 */


#ifndef __QMATH_COMPLEX_H__
#define __QMATH_COMPLEX_H__

#include <typedefs.h>
#include <qmath.h>

ComplexShort qcm_conj(ComplexShort op1);
int32 qcm_sqmag16(ComplexShort op1);
ComplexShort qcm_add16(ComplexShort op1, ComplexShort op2);
ComplexShort qcm_sub16(ComplexShort op1, ComplexShort op2);
ComplexShort qcm_mul16(ComplexShort op1, ComplexShort op2);
ComplexInt qcm_muls321616(ComplexShort op1, ComplexShort op2);
ComplexShort qcm_div16(ComplexShort op1, ComplexShort op2, int16* qQuotient);
ComplexInt qcm_sub32(ComplexInt op1, ComplexInt op2);

#endif  
