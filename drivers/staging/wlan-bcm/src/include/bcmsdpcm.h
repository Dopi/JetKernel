/*
 * Broadcom SDIO/PCMCIA
 * Software-specific definitions shared between device and host side
 *
 * Copyright (C) 1999-2009, Broadcom Corporation
 * 
 *         Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: bcmsdpcm.h,v 13.4.26.1 2009/07/06 11:56:22 Exp $
 */

#ifndef	_bcmsdpcm_h_
#define	_bcmsdpcm_h_


/*
 * Shared structure between dongle and the host
 * The structure contains pointers to trap or assert information shared with the host
 */
#define SDPCM_SHARED_VERSION       0x0001
#define SDPCM_SHARED_VERSION_MASK  0x00FF
#define SDPCM_SHARED_ASSERT_BUILT  0x0100
#define SDPCM_SHARED_ASSERT        0x0200
#define SDPCM_SHARED_TRAP          0x0400

typedef struct {
	uint32	flags;
	uint32  trap_addr;
	uint32  assert_exp_addr;
	uint32  assert_file_addr;
	uint32  assert_line;
	uint32	console_addr;		/* Address of hndrte_cons_t */
	uint32  msgtrace_addr;
} sdpcm_shared_t;

extern sdpcm_shared_t sdpcm_shared;

#endif	/* _bcmsdpcm_h_ */
