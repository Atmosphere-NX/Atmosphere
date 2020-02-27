/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "../preprocessor.h"
#include "../defines.hpp"

#define THERMOSPHERE_GET_SYSREG(r) ({\
    u64 __val;                                                                      \
    __asm__ __volatile__("mrs %0, " STRINGIZE(r) : "=r" (__val) :: "memory");       \
    __val;                                                                          \
})

#define THERMOSPHERE_SET_SYSREG(reg, val)\
    do {\
        u64 temp_reg = (val);\
        __asm__ __volatile__ ("msr " STRINGIZE(reg) ", %0" :: "r"(temp_reg) : "memory");\
    } while(false)

#define THERMOSPHERE_SET_SYSREG_IMM(reg, imm)\
    do {\
        __asm__ __volatile__ ("msr " STRINGIZE(reg) ", %0" :: "I"(imm) : "memory", "cc");\
    } while(false)


namespace ams::hvisor::cpu {

    using SysregEncoding = std::array<u8, 5>;

    constexpr u32 EncodeSysregIss(SysregEncoding reg)
    {
        auto [op0, op1, crn, crm, op2] = reg;
        return op0 << 20 | op2 << 17 | op1 << 14 | crn << 10 | crm << 1;
    }

    constexpr u32 MakeMsrFromEncoding(SysregEncoding reg, u32 Rt)
    {
        auto [op0, op1, crn, crm, op2] = reg;
        u32 enc = op0 << 19 | op1 << 16 | crn << 12 | crm << 8 | op2 << 5;
        return 0xD5000000u | enc | (Rt & 0x1Fu);
    }

    constexpr u32 MakeMrsFromEncoding(SysregEncoding reg, u32 Rt)
    {
        auto [op0, op1, crn, crm, op2] = reg;
        u32 enc = op0 << 19 | op1 << 16 | crn << 12 | crm << 8 | op2 << 5;
        return 0xD5200000u | enc | (Rt & 0x1Fu);
    }

    // The list mostly includes EL1 registers as these are the one we're trapping

    constexpr SysregEncoding dbgbvrN_el1(u8 n)           { return {2, 0, 0, n, 4}; }
    constexpr SysregEncoding dbgbcrN_el1(u8 n)           { return {2, 0, 0, n, 5}; }
    constexpr SysregEncoding dbgwvrN_el1(u8 n)           { return {2, 0, 0, n, 6}; }
    constexpr SysregEncoding dbgwcrN_el1(u8 n)           { return {2, 0, 0, n, 7}; }

    constexpr inline SysregEncoding dc_isw               = {1, 0, 7, 6, 2};
    constexpr inline SysregEncoding dc_csw               = {1, 0, 7, 10, 2};
    constexpr inline SysregEncoding dc_cisw              = {1, 0, 7, 14, 2};

    constexpr inline SysregEncoding osdtrrx_el1          = {2, 0, 0, 0, 2};
    constexpr inline SysregEncoding mdccint_el1          = {2, 0, 0, 2, 0};
    constexpr inline SysregEncoding mdscr_el1            = {2, 0, 0, 2, 2};
    constexpr inline SysregEncoding osdtrtx_el1          = {2, 0, 0, 3, 2};
    constexpr inline SysregEncoding oseccr_el1           = {2, 0, 0, 6, 2};

    constexpr inline SysregEncoding mdrar_el1            = {2, 0, 1, 0, 0};
    constexpr inline SysregEncoding oslar_el1            = {2, 0, 1, 0, 4};
    constexpr inline SysregEncoding oslsr_el1            = {2, 0, 1, 1, 4};
    constexpr inline SysregEncoding osdlr_el1            = {2, 0, 1, 3, 4};
    constexpr inline SysregEncoding dbgprcr_el1          = {2, 0, 1, 4, 4};
    constexpr inline SysregEncoding dbgclaimset_el1      = {2, 0, 7, 8, 6};
    constexpr inline SysregEncoding dbgclaimclr_el1      = {2, 0, 7, 9, 6};
    constexpr inline SysregEncoding dbgauthstatus_el1    = {2, 0, 7, 14, 6};
    constexpr inline SysregEncoding mdccsr_el0           = {2, 3, 0, 1, 0};
    constexpr inline SysregEncoding dbgdtr_el0           = {2, 3, 0, 4, 0};
    constexpr inline SysregEncoding dbgdtrrx_el0         = {2, 3, 0, 5, 0};
    constexpr inline SysregEncoding dbgdtrtx_el0         = {2, 3, 0, 5, 0};
    constexpr inline SysregEncoding dbgvcr32_el2         = {2, 4, 0, 7, 0};

    constexpr inline SysregEncoding midr_el1             = {3, 0, 0, 0, 0};
    constexpr inline SysregEncoding mpidr_el1            = {3, 0, 0, 0, 5};
    constexpr inline SysregEncoding revidr_el1           = {3, 0, 0, 0, 6};

    constexpr inline SysregEncoding id_pfr0_el1          = {3, 0, 0, 1, 0};
    constexpr inline SysregEncoding id_pfr1_el1          = {3, 0, 0, 1, 1};
    constexpr inline SysregEncoding id_dfr0_el1          = {3, 0, 0, 1, 2};
    constexpr inline SysregEncoding id_afr0_el1          = {3, 0, 0, 1, 3};
    constexpr inline SysregEncoding id_mmfr0_el1         = {3, 0, 0, 1, 4};
    constexpr inline SysregEncoding id_mmfr1_el1         = {3, 0, 0, 1, 5};
    constexpr inline SysregEncoding id_mmfr2_el1         = {3, 0, 0, 1, 6};
    constexpr inline SysregEncoding id_mmfr3_el1         = {3, 0, 0, 1, 7};

    constexpr inline SysregEncoding id_isar0_el1         = {3, 0, 0, 2, 0};
    constexpr inline SysregEncoding id_isar1_el1         = {3, 0, 0, 2, 1};
    constexpr inline SysregEncoding id_isar2_el1         = {3, 0, 0, 2, 2};
    constexpr inline SysregEncoding id_isar3_el1         = {3, 0, 0, 2, 3};
    constexpr inline SysregEncoding id_isar4_el1         = {3, 0, 0, 2, 4};
    constexpr inline SysregEncoding id_isar5_el1         = {3, 0, 0, 2, 5};
    constexpr inline SysregEncoding id_mmfr4_el1         = {3, 0, 0, 2, 6};

    constexpr inline SysregEncoding mvfr0_el1            = {3, 0, 0, 3, 0};
    constexpr inline SysregEncoding mvfr1_el1            = {3, 0, 0, 3, 1};
    constexpr inline SysregEncoding mvfr2_el1            = {3, 0, 0, 3, 2};

    constexpr inline SysregEncoding id_aa64pfr0_el1      = {3, 0, 0, 4, 0};
    constexpr inline SysregEncoding id_aa64pfr1_el1      = {3, 0, 0, 4, 1};
    constexpr inline SysregEncoding id_aa64zfr0_el1      = {3, 0, 0, 4, 4};

    constexpr inline SysregEncoding id_aa64dfr0_el1      = {3, 0, 0, 5, 0};
    constexpr inline SysregEncoding id_aa64dfr1_el1      = {3, 0, 0, 5, 1};

    constexpr inline SysregEncoding id_aa64afr0_el1      = {3, 0, 0, 5, 4};
    constexpr inline SysregEncoding id_aa64afr1_el1      = {3, 0, 0, 5, 5};

    constexpr inline SysregEncoding id_aa64isar0_el1     = {3, 0, 0, 6, 0};
    constexpr inline SysregEncoding id_aa64isar1_el1     = {3, 0, 0, 6, 1};

    constexpr inline SysregEncoding id_aa64mmfr0_el1     = {3, 0, 0, 7, 0};
    constexpr inline SysregEncoding id_aa64mmfr1_el1     = {3, 0, 0, 7, 1};
    constexpr inline SysregEncoding id_aa64mmfr2_el1     = {3, 0, 0, 7, 2};

    constexpr inline SysregEncoding sctlr_el1            = {3, 0, 1, 0, 0};
    constexpr inline SysregEncoding actlr_el1            = {3, 0, 1, 0, 1};
    constexpr inline SysregEncoding cpacr_el1            = {3, 0, 1, 0, 2};

    constexpr inline SysregEncoding zcr_el1              = {3, 0, 1, 2, 0};

    constexpr inline SysregEncoding ttbr0_el1            = {3, 0, 2, 0, 0};
    constexpr inline SysregEncoding ttbr1_el1            = {3, 0, 2, 0, 1};
    constexpr inline SysregEncoding tcr_el1              = {3, 0, 2, 0, 2};

    constexpr inline SysregEncoding apiakeylo_el1        = {3, 0, 2, 1, 0};
    constexpr inline SysregEncoding apiakeyhi_el1        = {3, 0, 2, 1, 1};
    constexpr inline SysregEncoding apibkeylo_el1        = {3, 0, 2, 1, 2};
    constexpr inline SysregEncoding apibkeyhi_el1        = {3, 0, 2, 1, 3};

    constexpr inline SysregEncoding apdakeylo_el1        = {3, 0, 2, 2, 0};
    constexpr inline SysregEncoding apdakeyhi_el1        = {3, 0, 2, 2, 1};
    constexpr inline SysregEncoding apdbkeylo_el1        = {3, 0, 2, 2, 2};
    constexpr inline SysregEncoding apdbkeyhi_el1        = {3, 0, 2, 2, 3};

    constexpr inline SysregEncoding apgakeylo_el1        = {3, 0, 2, 3, 0};
    constexpr inline SysregEncoding apgakeyhi_el1        = {3, 0, 2, 3, 1};

    constexpr inline SysregEncoding afsr0_el1            = {3, 0, 5, 1, 0};
    constexpr inline SysregEncoding afsr1_el1            = {3, 0, 5, 1, 1};
    constexpr inline SysregEncoding esr_el1              = {3, 0, 5, 2, 0};

    constexpr inline SysregEncoding erridr_el1           = {3, 0, 5, 3, 0};
    constexpr inline SysregEncoding errselr_el1          = {3, 0, 5, 3, 1};
    constexpr inline SysregEncoding erxfr_el1            = {3, 0, 5, 4, 0};
    constexpr inline SysregEncoding erxctlr_el1          = {3, 0, 5, 4, 1};
    constexpr inline SysregEncoding erxstatus_el1        = {3, 0, 5, 4, 2};
    constexpr inline SysregEncoding erxaddr_el1          = {3, 0, 5, 4, 3};
    constexpr inline SysregEncoding erxmisc0_el1         = {3, 0, 5, 5, 0};
    constexpr inline SysregEncoding erxmisc1_el1         = {3, 0, 5, 5, 1};

    constexpr inline SysregEncoding far_el1              = {3, 0, 6, 0, 0};
    constexpr inline SysregEncoding par_el1              = {3, 0, 7, 4, 0};


    constexpr inline SysregEncoding pmsidr_el1           = {3, 0, 9, 9, 7};
    constexpr inline SysregEncoding pmbidr_el1           = {3, 0, 9, 10, 7};
    constexpr inline SysregEncoding pmscr_el1            = {3, 0, 9, 9, 0};
    constexpr inline SysregEncoding pmscr_el2            = {3, 4, 9, 9, 0};
    constexpr inline SysregEncoding pmsicr_el1           = {3, 0, 9, 9, 2};
    constexpr inline SysregEncoding pmsirr_el1           = {3, 0, 9, 9, 3};
    constexpr inline SysregEncoding pmsfcr_el1           = {3, 0, 9, 9, 4};


    constexpr inline SysregEncoding pmsevfr_el1          = {3, 0, 9, 9, 5};
    constexpr inline SysregEncoding pmslatfr_el1         = {3, 0, 9, 9, 6};
    constexpr inline SysregEncoding pmblimitr_el1        = {3, 0, 9, 10, 0};


    constexpr inline SysregEncoding pmbptr_el1           = {3, 0, 9, 10, 1};

    constexpr inline SysregEncoding pmbsr_el1            = {3, 0, 9, 10, 3};


    constexpr inline SysregEncoding pmintenset_el1       = {3, 0, 9, 14, 1};
    constexpr inline SysregEncoding pmintenclr_el1       = {3, 0, 9, 14, 2};

    constexpr inline SysregEncoding mair_el1             = {3, 0, 10, 2, 0};
    constexpr inline SysregEncoding amair_el1            = {3, 0, 10, 3, 0};

    constexpr inline SysregEncoding lorsa_el1            = {3, 0, 10, 4, 0};
    constexpr inline SysregEncoding lorea_el1            = {3, 0, 10, 4, 1};
    constexpr inline SysregEncoding lorn_el1             = {3, 0, 10, 4, 2};
    constexpr inline SysregEncoding lorc_el1             = {3, 0, 10, 4, 3};
    constexpr inline SysregEncoding lorid_el1            = {3, 0, 10, 4, 7};

    constexpr inline SysregEncoding vbar_el1             = {3, 0, 12, 0, 0};
    constexpr inline SysregEncoding disr_el1             = {3, 0, 12, 1, 1};

    constexpr inline SysregEncoding contextidr_el1       = {3, 0, 13, 0, 1};
    constexpr inline SysregEncoding tpidr_el1            = {3, 0, 13, 0, 4};

    constexpr inline SysregEncoding cntkctl_el1          = {3, 0, 14, 1, 0};

    constexpr inline SysregEncoding ccsidr_el1           = {3, 1, 0, 0, 0};
    constexpr inline SysregEncoding clidr_el1            = {3, 1, 0, 0, 1};
    constexpr inline SysregEncoding aidr_el1             = {3, 1, 0, 0, 7};

    constexpr inline SysregEncoding csselr_el1           = {3, 2, 0, 0, 0};

    constexpr inline SysregEncoding ctr_el0              = {3, 3, 0, 0, 1};
    constexpr inline SysregEncoding dczid_el0            = {3, 3, 0, 0, 7};

    constexpr inline SysregEncoding pmcr_el0             = {3, 3, 9, 12, 0};
    constexpr inline SysregEncoding pmcntenset_el0       = {3, 3, 9, 12, 1};
    constexpr inline SysregEncoding pmcntenclr_el0       = {3, 3, 9, 12, 2};
    constexpr inline SysregEncoding pmovsclr_el0         = {3, 3, 9, 12, 3};
    constexpr inline SysregEncoding pmswinc_el0          = {3, 3, 9, 12, 4};
    constexpr inline SysregEncoding pmselr_el0           = {3, 3, 9, 12, 5};
    constexpr inline SysregEncoding pmceid0_el0          = {3, 3, 9, 12, 6};
    constexpr inline SysregEncoding pmceid1_el0          = {3, 3, 9, 12, 7};
    constexpr inline SysregEncoding pmccntr_el0          = {3, 3, 9, 13, 0};
    constexpr inline SysregEncoding pmxevtyper_el0       = {3, 3, 9, 13, 1};
    constexpr inline SysregEncoding pmxevcntr_el0        = {3, 3, 9, 13, 2};
    constexpr inline SysregEncoding pmuserenr_el0        = {3, 3, 9, 14, 0};
    constexpr inline SysregEncoding pmovsset_el0         = {3, 3, 9, 14, 3};

    constexpr inline SysregEncoding tpidr_el0            = {3, 3, 13, 0, 2};
    constexpr inline SysregEncoding tpidrro_el0          = {3, 3, 13, 0, 3};

    constexpr inline SysregEncoding cntfrq_el0           = {3, 3, 14, 0, 0};
    constexpr inline SysregEncoding cntpct_el0           = {3, 3, 14, 0, 1};
    constexpr inline SysregEncoding cntvct_el0           = {3, 3, 14, 0, 2};

    constexpr inline SysregEncoding cntp_tval_el0        = {3, 3, 14, 2, 0};
    constexpr inline SysregEncoding cntp_ctl_el0         = {3, 3, 14, 2, 1};
    constexpr inline SysregEncoding cntp_cval_el0        = {3, 3, 14, 2, 2};

    constexpr inline SysregEncoding cntv_tval_el0        = {3, 3, 14, 3, 0};
    constexpr inline SysregEncoding cntv_ctl_el0         = {3, 3, 14, 3, 1};
    constexpr inline SysregEncoding cntv_cval_el0        = {3, 3, 14, 3, 2};

    constexpr inline SysregEncoding cntvoff_el2          = {3, 4, 14, 0, 3};
    constexpr inline SysregEncoding cnthctl_el2          = {3, 4, 14, 1, 0};
    constexpr inline SysregEncoding cnthp_cval_el2       = {3, 4, 14, 2, 2};

    constexpr inline SysregEncoding pmccfiltr_el0        = {3, 3, 14, 15, 7};

    constexpr inline SysregEncoding zcr_el2              = {3, 4, 1, 2, 0};

    constexpr inline SysregEncoding dacr32_el2           = {3, 4, 3, 0, 0};
    constexpr inline SysregEncoding ifsr32_el2           = {3, 4, 5, 0, 1};
    constexpr inline SysregEncoding vsesr_el2            = {3, 4, 5, 2, 3};
    constexpr inline SysregEncoding fpexc32_el2          = {3, 4, 5, 3, 0};

    constexpr inline SysregEncoding zcr_el12             = {3, 5, 1, 2, 0};

    enum SctlrFlags {
        SCTLR_ELx_DSSBS         = BITL(44),
        SCTLR_ELx_ENIA          = BITL(31),
        SCTLR_ELx_ENIB          = BITL(30),
        SCTLR_ELx_ENDA          = BITL(27),
        SCTLR_ELx_EE            = BITL(25),
        SCTLR_ELx_IESB          = BITL(21),
        SCTLR_ELx_WXN           = BITL(19),
        SCTLR_ELx_ENDB          = BITL(13),
        SCTLR_ELx_I             = BITL(12),
        SCTLR_ELx_SA            = BITL(3),
        SCTLR_ELx_C             = BITL(2),
        SCTLR_ELx_A             = BITL(1),
        SCTLR_ELx_M             = BITL(0),

        SCTLR_EL1_UCI           = BITL(26),
        SCTLR_EL1_E0E           = BITL(24),
        SCTLR_EL1_SPAN          = BITL(23),
        SCTLR_EL1_NTWE          = BITL(18),
        SCTLR_EL1_NTWI          = BITL(16),
        SCTLR_EL1_UCT           = BITL(15),
        SCTLR_EL1_DZE           = BITL(14),
        SCTLR_EL1_UMA           = BITL(9),
        SCTLR_EL1_SED           = BITL(8),
        SCTLR_EL1_ITD           = BITL(7),
        SCTLR_EL1_CP15BEN       = BITL(5),
        SCTLR_EL1_SA0           = BITL(4),

        SCTLR_EL2_RES1          = util::CombineBits<u64>(29, 28, 23, 22, 18, 16, 11, 5, 4),
        SCTLR_EL2_RES0          = (0xFFFFEFFFull << 32) | util::CombineBits<u64>(
            31, 30, 27, 26, 24, 20, 17, 15, 14, 13, 10, 9, 8, 7, 6
        ),

        SCTLR_EL1_RES1          = util::CombineBits<u64>(29, 28, 22, 20, 11),
        SCTLR_EL1_RES0          = (0xFFFFEFFFull << 32) | util::CombineBits<u64>(31, 30, 27, 17, 13, 10, 6),
    };


    // HCR Flags
    enum HcrFlags {
        HCR_FWB     = BITL(46),
        HCR_API     = BITL(41),
        HCR_APK     = BITL(40),
        HCR_TEA     = BITL(37),
        HCR_TERR    = BITL(36),
        HCR_TLOR    = BITL(35),
        HCR_E2H     = BITL(34),
        HCR_ID      = BITL(33),
        HCR_CD      = BITL(32),
        HCR_RW      = BITL(31),
        HCR_TRVM    = BITL(30),
        HCR_HCD     = BITL(29),
        HCR_TDZ     = BITL(28),
        HCR_TGE     = BITL(27),
        HCR_TVM     = BITL(26),
        HCR_TTLB    = BITL(25),
        HCR_TPU     = BITL(24),
        HCR_TPC     = BITL(23),
        HCR_TSW     = BITL(22),
        HCR_TAC     = BITL(21),
        HCR_TIDCP   = BITL(20),
        HCR_TSC     = BITL(19),
        HCR_TID3    = BITL(18),
        HCR_TID2    = BITL(17),
        HCR_TID1    = BITL(16),
        HCR_TID0    = BITL(15),
        HCR_TWE     = BITL(14),
        HCR_TWI     = BITL(13),
        HCR_DC      = BITL(12),
        HCR_BSU     = (3ul << 10),
        HCR_BSU_IS  = BITL(10),
        HCR_FB      = BITL(9),
        HCR_VSE     = BITL(8),
        HCR_VI      = BITL(7),
        HCR_VF      = BITL(6),
        HCR_AMO     = BITL(5),
        HCR_IMO     = BITL(4),
        HCR_FMO     = BITL(3),
        HCR_PTW     = BITL(2),
        HCR_SWI     = BITL(1),
        HCR_VM      = BITL(0),
    };

    // CPTR flags
    enum CptrFlags {
        CPTR_TCPAC      = BITL(31),
        CPTR_TAM        = BITL(30),
        CPTR_TTA        = BITL(20),
        CPTR_TFP        = BITL(10),
        CPTR_TZ         = BITL(8), // (EL2)
        CPTR_EZ         = BITL(8), // (EL3)
        CPTR_RES1       = 0x000032FFul,
    };

    // MDCR flags (EL2)
    enum MdcrEl2Flags {
        MDCR_EL2_TPMS           = BITL(14),
        MDCR_EL2_E2PB_MASK      = 3ul,
        MDCR_EL2_E2PB_SHIFT     = 12,
        MDCR_EL2_TDRA           = BITL(11),
        MDCR_EL2_TDOSA          = BITL(10),
        MDCR_EL2_TDA            = BITL(9),
        MDCR_EL2_TDE            = BITL(8),
        MDCR_EL2_HPME           = BITL(7),
        MDCR_EL2_TPM            = BITL(6),
        MDCR_EL2_TPMCR          = BITL(5),
        MDCR_EL2_HPMN_MASK      = 0x1Ful,
    };

    // Some MDSCR flags
    enum MdscrFlags {
        MDSCR_MDE   = BITL(15),
        MDSCR_KDE   = BITL(13),
        MDSCR_SS    = BITL(0),
    };

    // Some CNTHCTL flags + shifts
    enum CnthctlFlags {
        CNTHCTL_EVNTI_MASK      = 0xFul,
        CNTHCTL_EVNTI_SHIFT     = 4,

        CNTHCTL_EVNTDIR         = BITL(3),
        CNTHCTL_EVNTEN          = BITL(2),
        CNTHCTL_EL1PCEN         = BITL(1),
        CNTHCTL_EL1PCTEN        = BITL(0),
    };

    // PAR_EL1 flags, shifts, masks
    enum ParFlags {
        PAR_F               = BITL(0),

        // Successful translation:
        PAR_ATTR_SHIFT      = 56,
        PAR_ATTR_MASK       = 0xFFul,
        PAR_PA_MASK         = MASK2L(51, 12),// bits 51-48 RES0 if not implemented
        PAR_NS              = BITL(9),
        PAR_SH_SHIFT        = 7,
        PAR_SH_MASK         = 3ul,

        // Faulting translation:
        PAR_S               = BITL(9),
        PAR_PTW             = BITL(8),
        PAR_FST_SHIFT       = 1,
        PAR_FST_MASK        = 0x3Ful,
    };

    // Some (S)PSR flags, masks, shifts
    enum PsrFlags {
        PSR_AA32_IT10_SHIFT = 25,
        PSR_AA32_IT10_MASK  = 3ul,

        PSR_SS              = BITL(21),

        PSR_AA32_IT72_SHIFT = 10,
        PSR_AA32_IT72_MASK  = 0x3Ful,

        PSR_DAIF_SHIFT      = 6,
        PSR_D               = BITL(9),
        PSR_A               = BITL(8),
        PSR_I               = BITL(7),
        PSR_F               = BITL(6),

        PSR_AA32_THUMB      = BITL(5),
        PSR_MODE32          = BITL(4),

        PSR_EL_SHIFT        = 2,
        PSR_EL_MASK         = 3ul,

        PSR_SP_ELX          = BITL(0),
    };

    // cnt*_ctl flags
    enum CntCtlFlags {
        CNTCTL_ISTATUS      = BITL(2),
        CNTCTL_IMASK        = BITL(1),
        CNTCTL_ENABLE       = BITL(0),
    };

 }
