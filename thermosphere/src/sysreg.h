/*
 * Copyright (c) 2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "preprocessor.h"

// Only for most EL1 regs, etc.

#ifndef BITL
#define BITL(n)               (1ull << (n))
#endif

#define TUP_DC_ISW              (1, 0, 7, 6, 2)
#define TUP_DC_CSW              (1, 0, 7, 10, 2)
#define TUP_DC_CISW             (1, 0, 7, 14, 2)

#define TUP_OSDTRRX_EL1         (2, 0, 0, 0, 2)
#define TUP_MDCCINT_EL1         (2, 0, 0, 2, 0)
#define TUP_MDSCR_EL1           (2, 0, 0, 2, 2)
#define TUP_OSDTRTX_EL1         (2, 0, 0, 3, 2)
#define TUP_OSECCR_EL1          (2, 0, 0, 6, 2)
#define TUP_DBGBVRn_EL1(n)      (2, 0, 0, n, 4)
#define TUP_DBGBCRn_EL1(n)      (2, 0, 0, n, 5)
#define TUP_DBGWVRn_EL1(n)      (2, 0, 0, n, 6)
#define TUP_DBGWCRn_EL1(n)      (2, 0, 0, n, 7)
#define TUP_MDRAR_EL1           (2, 0, 1, 0, 0)
#define TUP_OSLAR_EL1           (2, 0, 1, 0, 4)
#define TUP_OSLSR_EL1           (2, 0, 1, 1, 4)
#define TUP_OSDLR_EL1           (2, 0, 1, 3, 4)
#define TUP_DBGPRCR_EL1         (2, 0, 1, 4, 4)
#define TUP_DBGCLAIMSET_EL1     (2, 0, 7, 8, 6)
#define TUP_DBGCLAIMCLR_EL1     (2, 0, 7, 9, 6)
#define TUP_DBGAUTHSTATUS_EL1   (2, 0, 7, 14, 6)
#define TUP_MDCCSR_EL0          (2, 3, 0, 1, 0)
#define TUP_DBGDTR_EL0          (2, 3, 0, 4, 0)
#define TUP_DBGDTRRX_EL0        (2, 3, 0, 5, 0)
#define TUP_DBGDTRTX_EL0        (2, 3, 0, 5, 0)
#define TUP_DBGVCR32_EL2        (2, 4, 0, 7, 0)

#define TUP_MIDR_EL1            (3, 0, 0, 0, 0)
#define TUP_MPIDR_EL1           (3, 0, 0, 0, 5)
#define TUP_REVIDR_EL1          (3, 0, 0, 0, 6)

#define TUP_ID_PFR0_EL1         (3, 0, 0, 1, 0)
#define TUP_ID_PFR1_EL1         (3, 0, 0, 1, 1)
#define TUP_ID_DFR0_EL1         (3, 0, 0, 1, 2)
#define TUP_ID_AFR0_EL1         (3, 0, 0, 1, 3)
#define TUP_ID_MMFR0_EL1        (3, 0, 0, 1, 4)
#define TUP_ID_MMFR1_EL1        (3, 0, 0, 1, 5)
#define TUP_ID_MMFR2_EL1        (3, 0, 0, 1, 6)
#define TUP_ID_MMFR3_EL1        (3, 0, 0, 1, 7)

#define TUP_ID_ISAR0_EL1        (3, 0, 0, 2, 0)
#define TUP_ID_ISAR1_EL1        (3, 0, 0, 2, 1)
#define TUP_ID_ISAR2_EL1        (3, 0, 0, 2, 2)
#define TUP_ID_ISAR3_EL1        (3, 0, 0, 2, 3)
#define TUP_ID_ISAR4_EL1        (3, 0, 0, 2, 4)
#define TUP_ID_ISAR5_EL1        (3, 0, 0, 2, 5)
#define TUP_ID_MMFR4_EL1        (3, 0, 0, 2, 6)

#define TUP_MVFR0_EL1           (3, 0, 0, 3, 0)
#define TUP_MVFR1_EL1           (3, 0, 0, 3, 1)
#define TUP_MVFR2_EL1           (3, 0, 0, 3, 2)

#define TUP_ID_AA64PFR0_EL1     (3, 0, 0, 4, 0)
#define TUP_ID_AA64PFR1_EL1     (3, 0, 0, 4, 1)
#define TUP_ID_AA64ZFR0_EL1     (3, 0, 0, 4, 4)

#define TUP_ID_AA64DFR0_EL1     (3, 0, 0, 5, 0)
#define TUP_ID_AA64DFR1_EL1     (3, 0, 0, 5, 1)

#define TUP_ID_AA64AFR0_EL1     (3, 0, 0, 5, 4)
#define TUP_ID_AA64AFR1_EL1     (3, 0, 0, 5, 5)

#define TUP_ID_AA64ISAR0_EL1    (3, 0, 0, 6, 0)
#define TUP_ID_AA64ISAR1_EL1    (3, 0, 0, 6, 1)

#define TUP_ID_AA64MMFR0_EL1    (3, 0, 0, 7, 0)
#define TUP_ID_AA64MMFR1_EL1    (3, 0, 0, 7, 1)
#define TUP_ID_AA64MMFR2_EL1    (3, 0, 0, 7, 2)

#define TUP_SCTLR_EL1           (3, 0, 1, 0, 0)
#define TUP_ACTLR_EL1           (3, 0, 1, 0, 1)
#define TUP_CPACR_EL1           (3, 0, 1, 0, 2)

#define TUP_ZCR_EL1             (3, 0, 1, 2, 0)

#define TUP_TTBR0_EL1           (3, 0, 2, 0, 0)
#define TUP_TTBR1_EL1           (3, 0, 2, 0, 1)
#define TUP_TCR_EL1             (3, 0, 2, 0, 2)

#define TUP_APIAKEYLO_EL1       (3, 0, 2, 1, 0)
#define TUP_APIAKEYHI_EL1       (3, 0, 2, 1, 1)
#define TUP_APIBKEYLO_EL1       (3, 0, 2, 1, 2)
#define TUP_APIBKEYHI_EL1       (3, 0, 2, 1, 3)

#define TUP_APDAKEYLO_EL1       (3, 0, 2, 2, 0)
#define TUP_APDAKEYHI_EL1       (3, 0, 2, 2, 1)
#define TUP_APDBKEYLO_EL1       (3, 0, 2, 2, 2)
#define TUP_APDBKEYHI_EL1       (3, 0, 2, 2, 3)

#define TUP_APGAKEYLO_EL1       (3, 0, 2, 3, 0)
#define TUP_APGAKEYHI_EL1       (3, 0, 2, 3, 1)

#define TUP_ICC_PMR_EL1         (3, 0, 4, 6, 0)

#define TUP_AFSR0_EL1           (3, 0, 5, 1, 0)
#define TUP_AFSR1_EL1           (3, 0, 5, 1, 1)
#define TUP_ESR_EL1             (3, 0, 5, 2, 0)

#define TUP_ERRIDR_EL1          (3, 0, 5, 3, 0)
#define TUP_ERRSELR_EL1         (3, 0, 5, 3, 1)
#define TUP_ERXFR_EL1           (3, 0, 5, 4, 0)
#define TUP_ERXCTLR_EL1         (3, 0, 5, 4, 1)
#define TUP_ERXSTATUS_EL1       (3, 0, 5, 4, 2)
#define TUP_ERXADDR_EL1         (3, 0, 5, 4, 3)
#define TUP_ERXMISC0_EL1        (3, 0, 5, 5, 0)
#define TUP_ERXMISC1_EL1        (3, 0, 5, 5, 1)

#define TUP_FAR_EL1             (3, 0, 6, 0, 0)
#define TUP_PAR_EL1             (3, 0, 7, 4, 0)


#define TUP_PMSIDR_EL1          (3, 0, 9, 9, 7)
#define TUP_PMBIDR_EL1          (3, 0, 9, 10, 7)
#define TUP_PMSCR_EL1           (3, 0, 9, 9, 0)
#define TUP_PMSCR_EL2           (3, 4, 9, 9, 0)
#define TUP_PMSICR_EL1          (3, 0, 9, 9, 2)
#define TUP_PMSIRR_EL1          (3, 0, 9, 9, 3)
#define TUP_PMSFCR_EL1          (3, 0, 9, 9, 4)


#define TUP_PMSEVFR_EL1         (3, 0, 9, 9, 5)
#define TUP_PMSLATFR_EL1        (3, 0, 9, 9, 6)
#define TUP_PMBLIMITR_EL1       (3, 0, 9, 10, 0)


#define TUP_PMBPTR_EL1          (3, 0, 9, 10, 1)

#define TUP_PMBSR_EL1           (3, 0, 9, 10, 3)


#define TUP_PMINTENSET_EL1      (3, 0, 9, 14, 1)
#define TUP_PMINTENCLR_EL1      (3, 0, 9, 14, 2)

#define TUP_MAIR_EL1            (3, 0, 10, 2, 0)
#define TUP_AMAIR_EL1           (3, 0, 10, 3, 0)

#define TUP_LORSA_EL1           (3, 0, 10, 4, 0)
#define TUP_LOREA_EL1           (3, 0, 10, 4, 1)
#define TUP_LORN_EL1            (3, 0, 10, 4, 2)
#define TUP_LORC_EL1            (3, 0, 10, 4, 3)
#define TUP_LORID_EL1           (3, 0, 10, 4, 7)

#define TUP_VBAR_EL1            (3, 0, 12, 0, 0)
#define TUP_DISR_EL1            (3, 0, 12, 1, 1)

#define TUP_ICC_IAR0_EL1        (3, 0, 12, 8, 0)
#define TUP_ICC_EOIR0_EL1       (3, 0, 12, 8, 1)
#define TUP_ICC_HPPIR0_EL1      (3, 0, 12, 8, 2)
#define TUP_ICC_BPR0_EL1        (3, 0, 12, 8, 3)
#define TUP_ICC_AP0Rn_EL1(n)    (3, 0, 12, 8, 4 | n)
#define TUP_ICC_AP0R0_EL1       TUP_ICC_AP0Rn_EL1(0)
#define TUP_ICC_AP0R1_EL1       TUP_ICC_AP0Rn_EL1(1)
#define TUP_ICC_AP0R2_EL1       TUP_ICC_AP0Rn_EL1(2)
#define TUP_ICC_AP0R3_EL1       TUP_ICC_AP0Rn_EL1(3)
#define TUP_ICC_AP1Rn_EL1(n)    (3, 0, 12, 9, n)
#define TUP_ICC_AP1R0_EL1       TUP_ICC_AP1Rn_EL1(0)
#define TUP_ICC_AP1R1_EL1       TUP_ICC_AP1Rn_EL1(1)
#define TUP_ICC_AP1R2_EL1       TUP_ICC_AP1Rn_EL1(2)
#define TUP_ICC_AP1R3_EL1       TUP_ICC_AP1Rn_EL1(3)
#define TUP_ICC_DIR_EL1	        (3, 0, 12, 11, 1)
#define TUP_ICC_RPR_EL1         (3, 0, 12, 11, 3)
#define TUP_ICC_SGI1R_EL1       (3, 0, 12, 11, 5)
#define TUP_ICC_ASGI1R_EL1      (3, 0, 12, 11, 6)
#define TUP_ICC_SGI0R_EL1       (3, 0, 12, 11, 7)
#define TUP_ICC_IAR1_EL1        (3, 0, 12, 12, 0)
#define TUP_ICC_EOIR1_EL1       (3, 0, 12, 12, 1)
#define TUP_ICC_HPPIR1_EL1      (3, 0, 12, 12, 2)
#define TUP_ICC_BPR1_EL1        (3, 0, 12, 12, 3)
#define TUP_ICC_CTLR_EL1        (3, 0, 12, 12, 4)
#define TUP_ICC_SRE_EL1         (3, 0, 12, 12, 5)
#define TUP_ICC_IGRPEN0_EL1     (3, 0, 12, 12, 6)
#define TUP_ICC_IGRPEN1_EL1     (3, 0, 12, 12, 7)

#define TUP_CONTEXTIDR_EL1      (3, 0, 13, 0, 1)
#define TUP_TPIDR_EL1           (3, 0, 13, 0, 4)

#define TUP_CNTKCTL_EL1         (3, 0, 14, 1, 0)

#define TUP_CCSIDR_EL1          (3, 1, 0, 0, 0)
#define TUP_CLIDR_EL1           (3, 1, 0, 0, 1)
#define TUP_AIDR_EL1            (3, 1, 0, 0, 7)

#define TUP_CSSELR_EL1          (3, 2, 0, 0, 0)

#define TUP_CTR_EL0             (3, 3, 0, 0, 1)
#define TUP_DCZID_EL0           (3, 3, 0, 0, 7)

#define TUP_PMCR_EL0            (3, 3, 9, 12, 0)
#define TUP_PMCNTENSET_EL0      (3, 3, 9, 12, 1)
#define TUP_PMCNTENCLR_EL0      (3, 3, 9, 12, 2)
#define TUP_PMOVSCLR_EL0        (3, 3, 9, 12, 3)
#define TUP_PMSWINC_EL0         (3, 3, 9, 12, 4)
#define TUP_PMSELR_EL0          (3, 3, 9, 12, 5)
#define TUP_PMCEID0_EL0         (3, 3, 9, 12, 6)
#define TUP_PMCEID1_EL0         (3, 3, 9, 12, 7)
#define TUP_PMCCNTR_EL0         (3, 3, 9, 13, 0)
#define TUP_PMXEVTYPER_EL0      (3, 3, 9, 13, 1)
#define TUP_PMXEVCNTR_EL0       (3, 3, 9, 13, 2)
#define TUP_PMUSERENR_EL0       (3, 3, 9, 14, 0)
#define TUP_PMOVSSET_EL0        (3, 3, 9, 14, 3)

#define TUP_TPIDR_EL0           (3, 3, 13, 0, 2)
#define TUP_TPIDRRO_EL0         (3, 3, 13, 0, 3)

#define TUP_CNTFRQ_EL0          (3, 3, 14, 0, 0)
#define TUP_CNTPCT_EL0          (3, 3, 14, 0, 1)
#define TUP_CNTVCT_EL0          (3, 3, 14, 0, 2)

#define TUP_CNTP_TVAL_EL0       (3, 3, 14, 2, 0)
#define TUP_CNTP_CTL_EL0        (3, 3, 14, 2, 1)
#define TUP_CNTP_CVAL_EL0       (3, 3, 14, 2, 2)

#define TUP_CNTV_TVAL_EL0       (3, 3, 14, 3, 0)
#define TUP_CNTV_CTL_EL0        (3, 3, 14, 3, 1)
#define TUP_CNTV_CVAL_EL0       (3, 3, 14, 3, 2)

#define TUP_CNTVOFF_EL2         (3, 4, 14, 0, 3)
#define TUP_CNTHCTL_EL2         (3, 4, 14, 1, 0)
#define TUP_CNTHP_CVAL_EL2      (3, 4, 14, 2, 2)

#define __field_PMEV_op2(n)     ((n) & 0x7)
#define __field__CNTR_CRm(n)    (0x8 | (((n) >> 3) & 0x3))
#define __field__TYPER_CRm(n)   (0xC | (((n) >> 3) & 0x3))
#define TUP_PMEVCNTRn_EL0(n)    (3, 3, 14, __field_CNTR_CRm(n), __field_PMEV_op2(n))
#define TUP_PMEVTYPERn_EL0(n)   (3, 3, 14, __field_TYPER_CRm(n), __field_PMEV_op2(n))

#define TUP_PMCCFILTR_EL0        (3, 3, 14, 15, 7)

#define TUP_ZCR_EL2             (3, 4, 1, 2, 0)

#define TUP_DACR32_EL2          (3, 4, 3, 0, 0)
#define TUP_IFSR32_EL2          (3, 4, 5, 0, 1)
#define TUP_VSESR_EL2           (3, 4, 5, 2, 3)
#define TUP_FPEXC32_EL2         (3, 4, 5, 3, 0)

#define TUP_VDISR_EL2           (3, 4, 12, 1,  1)
#define TUP_ICH_AP0Rn_EL2(n)    (3, 4, 12, 8, n)
#define TUP_ICH_AP0R0_EL2       TUP_ICH_AP0Rn_EL2(0)
#define TUP_ICH_AP0R1_EL2       TUP_ICH_AP0Rn_EL2(1)
#define TUP_ICH_AP0R2_EL2       TUP_ICH_AP0Rn_EL2(2)
#define TUP_ICH_AP0R3_EL2       TUP_ICH_AP0Rn_EL2(3)

#define TUP_AP1Rx_EL2(n)        (3, 4, 12, 9, n)
#define TUP_ICH_AP1R0_EL2       TUP_ICH_AP1Rn_EL2(0)
#define TUP_ICH_AP1R1_EL2       TUP_ICH_AP1Rn_EL2(1)
#define TUP_ICH_AP1R2_EL2       TUP_ICH_AP1Rn_EL2(2)
#define TUP_ICH_AP1R3_EL2       TUP_ICH_AP1Rn_EL2(3)

#define TUP_ICH_VSEIR_EL2       (3, 4, 12, 9, 4)
#define TUP_ICC_SRE_EL2         (3, 4, 12, 9, 5)
#define TUP_ICH_HCR_EL2         (3, 4, 12, 11, 0)
#define TUP_ICH_VTR_EL2         (3, 4, 12, 11, 1)
#define TUP_ICH_MISR_EL2        (3, 4, 12, 11, 2)
#define TUP_ICH_EISR_EL2        (3, 4, 12, 11, 3)
#define TUP_ICH_ELRSR_EL2       (3, 4, 12, 11, 5)
#define TUP_ICH_VMCR_EL2        (3, 4, 12, 11, 7)

#define TUP_ICH_LRn_EL2(n)      (3, 4, 12, 12 + (n)/8, n)
#define TUP_ICH_LR0_EL2         TUP_ICH_LRn_EL2(0)
#define TUP_ICH_LR1_EL2         TUP_ICH_LRn_EL2(1)
#define TUP_ICH_LR2_EL2         TUP_ICH_LRn_EL2(2)
#define TUP_ICH_LR3_EL2         TUP_ICH_LRn_EL2(3)
#define TUP_ICH_LR4_EL2         TUP_ICH_LRn_EL2(4)
#define TUP_ICH_LR5_EL2         TUP_ICH_LRn_EL2(5)
#define TUP_ICH_LR6_EL2         TUP_ICH_LRn_EL2(6)
#define TUP_ICH_LR7_EL2         TUP_ICH_LRn_EL2(7)
#define TUP_ICH_LR8_EL2         TUP_ICH_LRn_EL2(8)
#define TUP_ICH_LR9_EL2         TUP_ICH_LRn_EL2(9)
#define TUP_ICH_LR10_EL2        TUP_ICH_LRn_EL2(10)
#define TUP_ICH_LR11_EL2        TUP_ICH_LRn_EL2(11)
#define TUP_ICH_LR12_EL2        TUP_ICH_LRn_EL2(12)
#define TUP_ICH_LR13_EL2        TUP_ICH_LRn_EL2(13)
#define TUP_ICH_LR14_EL2        TUP_ICH_LRn_EL2(14)
#define TUP_ICH_LR15_EL2        TUP_ICH_LRn_EL2(15)

#define TUP_ZCR_EL12            (3, 5, 1, 2, 0)


#define SCTLR_ELx_DSSBS         BITL(44)
#define SCTLR_ELx_ENIA          BITL(31)
#define SCTLR_ELx_ENIB          BITL(30)
#define SCTLR_ELx_ENDA          BITL(27)
#define SCTLR_ELx_EE            BITL(25)
#define SCTLR_ELx_IESB          BITL(21)
#define SCTLR_ELx_WXN           BITL(19)
#define SCTLR_ELx_ENDB          BITL(13)
#define SCTLR_ELx_I             BITL(12)
#define SCTLR_ELx_SA            BITL(3)
#define SCTLR_ELx_C             BITL(2)
#define SCTLR_ELx_A             BITL(1)
#define SCTLR_ELx_M             BITL(0)

#define SCTLR_ELx_FLAGS         (SCTLR_ELx_M  | SCTLR_ELx_A | SCTLR_ELx_C | SCTLR_ELx_SA | SCTLR_ELx_I | SCTLR_ELx_IESB)

#define SCTLR_EL2_RES1          (BITL(4) | BITL(5)  | BITL(11) | BITL(16) | \
                                BITL(18) | BITL(22) | BITL(23) | BITL(28) | \
                                BITL(29))
#define SCTLR_EL2_RES0          (BITL(6) | BITL(7)  | BITL(8)  | BITL(9)  | \
                                BITL(10) | BITL(13) | BITL(14) | BITL(15) | \
                                BITL(17) | BITL(20) | BITL(24) | BITL(26) | \
                                BITL(27) | BITL(30) | BITL(31) | \
                                (0xFFFFEFFFull << 32))

#define SCTLR_EL1_UCI           BITL(26)
#define SCTLR_EL1_E0E           BITL(24)
#define SCTLR_EL1_SPAN          BITL(23)
#define SCTLR_EL1_NTWE          BITL(18)
#define SCTLR_EL1_NTWI          BITL(16)
#define SCTLR_EL1_UCT           BITL(15)
#define SCTLR_EL1_DZE           BITL(14)
#define SCTLR_EL1_UMA           BITL(9)
#define SCTLR_EL1_SED           BITL(8)
#define SCTLR_EL1_ITD           BITL(7)
#define SCTLR_EL1_CP15BEN       BITL(5)
#define SCTLR_EL1_SA0           BITL(4)

#define SCTLR_EL1_RES1  (BITL(11) | BITL(20) | BITL(22) | BITL(28) | BITL(29))
#define SCTLR_EL1_RES0  (BITL(6)  | BITL(10) | BITL(13) | BITL(17) | BITL(27) | BITL(30) | BITL(31) | (0xFFFFEFFFull << 32))


// HCR Flags

#define HCR_FWB     BITL(46)
#define HCR_API     BITL(41)
#define HCR_APK     BITL(40)
#define HCR_TEA     BITL(37)
#define HCR_TERR    BITL(36)
#define HCR_TLOR    BITL(35)
#define HCR_E2H     BITL(34)
#define HCR_ID      BITL(33)
#define HCR_CD      BITL(32)
#define HCR_RW      BITL(31)
#define HCR_TRVM    BITL(30)
#define HCR_HCD     BITL(29)
#define HCR_TDZ     BITL(28)
#define HCR_TGE     BITL(27)
#define HCR_TVM     BITL(26)
#define HCR_TTLB    BITL(25)
#define HCR_TPU     BITL(24)
#define HCR_TPC     BITL(23)
#define HCR_TSW     BITL(22)
#define HCR_TAC     BITL(21)
#define HCR_TIDCP   BITL(20)
#define HCR_TSC     BITL(19)
#define HCR_TID3    BITL(18)
#define HCR_TID2    BITL(17)
#define HCR_TID1    BITL(16)
#define HCR_TID0    BITL(15)
#define HCR_TWE     BITL(14)
#define HCR_TWI     BITL(13)
#define HCR_DC      BITL(12)
#define HCR_BSU     (3ull  << 10)
#define HCR_BSU_IS  BITL(10)
#define HCR_FB      BITL(9)
#define HCR_VSE     BITL(8)
#define HCR_VI      BITL(7)
#define HCR_VF      BITL(6)
#define HCR_AMO     BITL(5)
#define HCR_IMO     BITL(4)
#define HCR_FMO     BITL(3)
#define HCR_PTW     BITL(2)
#define HCR_SWI     BITL(1)
#define HCR_VM      BITL(0)

#define HSTR_EL2_T(x)       BITL(x)

#define CPTR_EL2_TCPAC      BITL(31)
#define CPTR_EL2_TTA        BITL(20)
#define CPTR_EL2_TFP        BITL(10)
#define CPTR_EL2_TZ         BITL(8)
#define CPTR_EL2_RES1       0x000032FF

#define MDCR_EL2_TPMS       BITL(14)
#define MDCR_EL2_E2PB_MASK  3ull
#define MDCR_EL2_E2PB_SHIFT 12
#define MDCR_EL2_TDRA       BITL(11)
#define MDCR_EL2_TDOSA      BITL(10)
#define MDCR_EL2_TDA        BITL(9)
#define MDCR_EL2_TDE        BITL(8)
#define MDCR_EL2_HPME       BITL(7)
#define MDCR_EL2_TPM        BITL(6)
#define MDCR_EL2_TPMCR      BITL(5)
#define MDCR_EL2_HPMN_MASK  0x1Full

#define MDSCR_MDE           BITL(15)
#define MDSCR_SS            BITL(0)

// Common CNTHCTL_EL2 flags
#define CNTHCTL_EVNTI_MASK  0xFul
#define CNTHCTL_EVNTI_SHIFT 4
#define CNTHCTL_EVNTDIR     BITL(3)
#define CNTHCTL_EVNTEN      BITL(2)
#define CNTHCTL_EL1PCEN     BITL(1)
#define CNTHCTL_EL1PCTEN    BITL(0)

// PAR_EL1
#define PAR_F               BITL(0)
// Successful translation:
#define PAR_ATTR_SHIFT      56
#define PAR_ATTR_MASK       0xFFul
#define PAR_PA_MASK         MASK2L(51, 12)      // bits 51-48 RES0 if not implemented
#define PAR_NS              BITL(9)
#define PAR_SH_SHIFT        7
#define PAR_SH_MASK         3ul
// Faulting translation:
#define PAR_S               BITL(9)
#define PAR_PTW             BITL(8)
#define PAR_FST_SHIFT       1
#define PAR_FST_MASK        0x3Ful

#define ENCODE_SYSREG_FIELDS_MOV(op0, op1, crn, crm, op2) (((op0) << 19) | ((op1) << 16) | ((crn) << 12) | ((crm) << 8) | ((op2) << 5))
#define ENCODE_SYSREG_MOV(name) EVAL(ENCODE_SYSREG_FIELDS_MOV CAT(TUP_, name))
#define MAKE_MSR(name, Rt)      (0xD5000000 | ENCODE_SYSREG_MOV(name) | ((Rt) & 0x1F))
#define MAKE_MRS(name, Rt)      (0xD5200000 | ENCODE_SYSREG_MOV(name) | ((Rt) & 0x1F))

#define MAKE_MSR_FROM_FIELDS(op0, op1, crn, crm, op2, Rt)    (0xD5000000 | ENCODE_SYSREG_FIELDS_MOV(op0, op1, crn, crm, op2) | ((Rt) & 0x1F))
#define MAKE_MRS_FROM_FIELDS(op0, op1, crn, crm, op2, Rt)    (0xD5200000 | ENCODE_SYSREG_FIELDS_MOV(op0, op1, crn, crm, op2) | ((Rt) & 0x1F))

#define ENCODE_SYSREG_FIELDS_ISS(op0, op1, crn, crm, op2) (((op0) << 20) | ((op2) << 17) | ((op1) << 14) | ((crn) << 10) | ((crm) << 1))
#define ENCODE_SYSREG_ISS(name) EVAL(ENCODE_SYSREG_FIELDS_ISS CAT(TUP_, name))

#define GET_SYSREG(r) ({\
    u64 __val;                                                      \
    asm volatile("mrs %0, " STRINGIZE(r) : "=r" (__val));     \
    __val;                                                          \
})

#define SET_SYSREG(reg, val)        do { u64 temp_reg = (val); __asm__ __volatile__ ("msr " STRINGIZE(reg) ", %0" :: "r"(temp_reg) : "memory"); } while(false)
#define SET_SYSREG_IMM(reg, imm)    do { __asm__ __volatile__ ("msr " STRINGIZE(reg) ", %0" :: "I"(imm) : "memory"); } while(false)

#define SYSREG_OP1_AARCH32_AUTO 0
#define SYSREG_OP1_AARCH64_EL1  0
#define SYSREG_OP1_CACHE        1
#define SYSREG_OP1_CACHESZSEL   2
#define SYSREG_OP1_EL0          3
#define SYSREG_OP1_EL2          4
#define SYSREG_OP1_EL3          6
#define SYSREG_OP1_AARCH32_JZL  7
#define SYSREG_OP1_AARCH64_SEL1 7

#define PSTATE_SS               BITL(21)
