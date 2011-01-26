/*
 * HND SiliconBackplane PMU support.
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
 * $Id: hndpmu.h,v 13.14.4.3.4.3.22.2 2009/01/21 23:54:44 Exp $
 */

#ifndef _hndpmu_h_
#define _hndpmu_h_

#if !defined(BCMDONGLEHOST)
#define SET_LDO_VOLTAGE_LDO1	1
#define SET_LDO_VOLTAGE_LDO2	2
#define SET_LDO_VOLTAGE_LDO3	3
#define SET_LDO_VOLTAGE_PAREF	4
#define SET_LDO_VOLTAGE_CLDO_PWM	5
#define SET_LDO_VOLTAGE_CLDO_BURST	6
#define SET_LDO_VOLTAGE_CBUCK_PWM	7
#define SET_LDO_VOLTAGE_CBUCK_BURST	8
#define SET_LDO_VOLTAGE_LNLDO1	9
#define SET_LDO_VOLTAGE_LNLDO2_SEL	10

extern void si_pmu_init(si_t *sih, osl_t *osh);
extern void si_pmu_chip_init(si_t *sih, osl_t *osh);
extern void si_pmu_pll_init(si_t *sih, osl_t *osh, uint32 xtalfreq);
extern void si_pmu_res_init(si_t *sih, osl_t *osh);
extern void si_pmu_swreg_init(si_t *sih, osl_t *osh);

extern uint32 si_pmu_force_ilp(si_t *sih, osl_t *osh, bool force);

extern uint32 si_pmu_si_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_cpu_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_alp_clock(si_t *sih, osl_t *osh);
extern uint32 si_pmu_ilp_clock(si_t *sih, osl_t *osh);

extern void si_pmu_set_switcher_voltage(si_t *sih, osl_t *osh, uint8 bb_voltage, uint8 rf_voltage);
extern void si_pmu_set_ldo_voltage(si_t *sih, osl_t *osh, uint8 ldo, uint8 voltage);
extern void si_pmu_paref_ldo_enable(si_t *sih, osl_t *osh, bool enable);
extern uint16 si_pmu_fast_pwrup_delay(si_t *sih, osl_t *osh);
extern void si_pmu_rcal(si_t *sih, osl_t *osh);

extern void si_pmu_spuravoid(si_t *sih, osl_t *osh, bool spuravoid);

extern bool si_pmu_is_otp_powered(si_t *sih, osl_t *osh);
extern void si_pmu_chipcontrol(si_t *sih, uint reg, uint32 mask, uint32 val);
extern bool si_pmu_is_sprom_enabled(si_t *sih, osl_t *osh);
extern void si_pmu_sprom_enable(si_t *sih, osl_t *osh, bool enable);

extern void si_pmu_radio_enable(si_t *sih, bool enable);
extern uint32 si_pmu_waitforclk_on_backplane(si_t *sih, osl_t *osh, uint32 clk, uint32 delay);
#endif /* !defined(BCMDONGLEHOST) */

extern void si_pmu_otp_power(si_t *sih, osl_t *osh, bool on);
extern void si_sdiod_drive_strength_init(si_t *sih, osl_t *osh, uint32 drivestrength);

#endif /* _hndpmu_h_ */
