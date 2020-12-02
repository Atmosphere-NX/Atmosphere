/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>

namespace ams::hw::arch::arm64 {

    #define HW_CPU_GET_SYSREG(name, value) __asm__ __volatile__("mrs %0, " #name "" : "=&r"(value) :: "memory");

    #define HW_CPU_SET_SYSREG(name, value) __asm__ __volatile__("msr " #name ", %0" :: "r"(value) : "memory", "cc")

    #define HW_CPU_GET_SCTLR_EL3(value) HW_CPU_GET_SYSREG(sctlr_el3, value)
    #define HW_CPU_SET_SCTLR_EL3(value) HW_CPU_SET_SYSREG(sctlr_el3, value)

    #define HW_CPU_GET_SCR_EL3(value) HW_CPU_GET_SYSREG(scr_el3, value)
    #define HW_CPU_SET_SCR_EL3(value) HW_CPU_SET_SYSREG(scr_el3, value)

    #define HW_CPU_GET_CPTR_EL3(value) HW_CPU_GET_SYSREG(cptr_el3, value)
    #define HW_CPU_SET_CPTR_EL3(value) HW_CPU_SET_SYSREG(cptr_el3, value)

    #define HW_CPU_GET_TTBR0_EL3(value) HW_CPU_GET_SYSREG(ttbr0_el3, value)
    #define HW_CPU_SET_TTBR0_EL3(value) HW_CPU_SET_SYSREG(ttbr0_el3, value)

    #define HW_CPU_GET_TCR_EL3(value) HW_CPU_GET_SYSREG(tcr_el3, value)
    #define HW_CPU_SET_TCR_EL3(value) HW_CPU_SET_SYSREG(tcr_el3, value)

    #define HW_CPU_GET_MAIR_EL3(value) HW_CPU_GET_SYSREG(mair_el3, value)
    #define HW_CPU_SET_MAIR_EL3(value) HW_CPU_SET_SYSREG(mair_el3, value)

    #define HW_CPU_GET_VBAR_EL3(value) HW_CPU_GET_SYSREG(vbar_el3, value)
    #define HW_CPU_SET_VBAR_EL3(value) HW_CPU_SET_SYSREG(vbar_el3, value)

    #define HW_CPU_GET_ELR_EL3(value) HW_CPU_GET_SYSREG(elr_el3, value)
    #define HW_CPU_SET_ELR_EL3(value) HW_CPU_SET_SYSREG(elr_el3, value)

    #define HW_CPU_GET_FAR_EL3(value) HW_CPU_GET_SYSREG(far_el3, value)
    #define HW_CPU_SET_FAR_EL3(value) HW_CPU_SET_SYSREG(far_el3, value)

    #define HW_CPU_GET_ESR_EL3(value) HW_CPU_GET_SYSREG(esr_el3, value)
    #define HW_CPU_SET_ESR_EL3(value) HW_CPU_SET_SYSREG(esr_el3, value)

    #define HW_CPU_GET_CLIDR_EL1(value) HW_CPU_GET_SYSREG(clidr_el1, value)
    #define HW_CPU_SET_CLIDR_EL1(value) HW_CPU_SET_SYSREG(clidr_el1, value)

    #define HW_CPU_GET_CCSIDR_EL1(value) HW_CPU_GET_SYSREG(ccsidr_el1, value)
    #define HW_CPU_SET_CCSIDR_EL1(value) HW_CPU_SET_SYSREG(ccsidr_el1, value)

    #define HW_CPU_GET_CSSELR_EL1(value) HW_CPU_GET_SYSREG(csselr_el1, value)
    #define HW_CPU_SET_CSSELR_EL1(value) HW_CPU_SET_SYSREG(csselr_el1, value)

    #define HW_CPU_GET_CPUACTLR_EL1(value) HW_CPU_GET_SYSREG(s3_1_c15_c2_0, value)
    #define HW_CPU_SET_CPUACTLR_EL1(value) HW_CPU_SET_SYSREG(s3_1_c15_c2_0, value)

    #define HW_CPU_GET_CPUECTLR_EL1(value) HW_CPU_GET_SYSREG(s3_1_c15_c2_1, value)
    #define HW_CPU_SET_CPUECTLR_EL1(value) HW_CPU_SET_SYSREG(s3_1_c15_c2_1, value)

    #define HW_CPU_GET_DBGAUTHSTATUS_EL1(value) HW_CPU_GET_SYSREG(dbgauthstatus_el1, value)
    #define HW_CPU_SET_DBGAUTHSTATUS_EL1(value) HW_CPU_SET_SYSREG(dbgauthstatus_el1, value)

    #define HW_CPU_GET_MPIDR_EL1(value) HW_CPU_GET_SYSREG(mpidr_el1, value)

    #define HW_CPU_GET_OSLAR_EL1(value) HW_CPU_GET_SYSREG(oslar_el1, value)
    #define HW_CPU_SET_OSLAR_EL1(value) HW_CPU_SET_SYSREG(oslar_el1, value)

    #define HW_CPU_GET_OSDTRRX_EL1(value) HW_CPU_GET_SYSREG(osdtrrx_el1, value)
    #define HW_CPU_SET_OSDTRRX_EL1(value) HW_CPU_SET_SYSREG(osdtrrx_el1, value)

    #define HW_CPU_GET_OSDTRTX_EL1(value) HW_CPU_GET_SYSREG(osdtrtx_el1, value)
    #define HW_CPU_SET_OSDTRTX_EL1(value) HW_CPU_SET_SYSREG(osdtrtx_el1, value)

    #define HW_CPU_GET_MDSCR_EL1(value) HW_CPU_GET_SYSREG(mdscr_el1, value)
    #define HW_CPU_SET_MDSCR_EL1(value) HW_CPU_SET_SYSREG(mdscr_el1, value)

    #define HW_CPU_GET_OSECCR_EL1(value) HW_CPU_GET_SYSREG(oseccr_el1, value)
    #define HW_CPU_SET_OSECCR_EL1(value) HW_CPU_SET_SYSREG(oseccr_el1, value)

    #define HW_CPU_GET_MDCCINT_EL1(value) HW_CPU_GET_SYSREG(mdccint_el1, value)
    #define HW_CPU_SET_MDCCINT_EL1(value) HW_CPU_SET_SYSREG(mdccint_el1, value)

    #define HW_CPU_GET_DBGCLAIMCLR_EL1(value) HW_CPU_GET_SYSREG(dbgclaimclr_el1, value)
    #define HW_CPU_SET_DBGCLAIMCLR_EL1(value) HW_CPU_SET_SYSREG(dbgclaimclr_el1, value)

    #define HW_CPU_GET_FAR_EL1(value) HW_CPU_GET_SYSREG(far_el1, value)
    #define HW_CPU_SET_FAR_EL1(value) HW_CPU_SET_SYSREG(far_el1, value)

    #define HW_CPU_GET_DBGVCR32_EL2(value) HW_CPU_GET_SYSREG(dbgvcr32_el2, value)
    #define HW_CPU_SET_DBGVCR32_EL2(value) HW_CPU_SET_SYSREG(dbgvcr32_el2, value)

    #define HW_CPU_GET_SDER32_EL3(value) HW_CPU_GET_SYSREG(sder32_el3, value)
    #define HW_CPU_SET_SDER32_EL3(value) HW_CPU_SET_SYSREG(sder32_el3, value)

    #define HW_CPU_GET_MDCR_EL2(value) HW_CPU_GET_SYSREG(mdcr_el2, value)
    #define HW_CPU_SET_MDCR_EL2(value) HW_CPU_SET_SYSREG(mdcr_el2, value)

    #define HW_CPU_GET_MDCR_EL3(value) HW_CPU_GET_SYSREG(mdcr_el3, value)
    #define HW_CPU_SET_MDCR_EL3(value) HW_CPU_SET_SYSREG(mdcr_el3, value)

    #define HW_CPU_GET_SPSR_EL3(value) HW_CPU_GET_SYSREG(spsr_el3, value)
    #define HW_CPU_SET_SPSR_EL3(value) HW_CPU_SET_SYSREG(spsr_el3, value)

    #define HW_CPU_GET_DBGBVR0_EL1(value) HW_CPU_GET_SYSREG(dbgbvr0_el1, value)
    #define HW_CPU_SET_DBGBVR0_EL1(value) HW_CPU_SET_SYSREG(dbgbvr0_el1, value)
    #define HW_CPU_GET_DBGBCR0_EL1(value) HW_CPU_GET_SYSREG(dbgbcr0_el1, value)
    #define HW_CPU_SET_DBGBCR0_EL1(value) HW_CPU_SET_SYSREG(dbgbcr0_el1, value)
    #define HW_CPU_GET_DBGBVR1_EL1(value) HW_CPU_GET_SYSREG(dbgbvr1_el1, value)
    #define HW_CPU_SET_DBGBVR1_EL1(value) HW_CPU_SET_SYSREG(dbgbvr1_el1, value)
    #define HW_CPU_GET_DBGBCR1_EL1(value) HW_CPU_GET_SYSREG(dbgbcr1_el1, value)
    #define HW_CPU_SET_DBGBCR1_EL1(value) HW_CPU_SET_SYSREG(dbgbcr1_el1, value)
    #define HW_CPU_GET_DBGBVR2_EL1(value) HW_CPU_GET_SYSREG(dbgbvr2_el1, value)
    #define HW_CPU_SET_DBGBVR2_EL1(value) HW_CPU_SET_SYSREG(dbgbvr2_el1, value)
    #define HW_CPU_GET_DBGBCR2_EL1(value) HW_CPU_GET_SYSREG(dbgbcr2_el1, value)
    #define HW_CPU_SET_DBGBCR2_EL1(value) HW_CPU_SET_SYSREG(dbgbcr2_el1, value)
    #define HW_CPU_GET_DBGBVR3_EL1(value) HW_CPU_GET_SYSREG(dbgbvr3_el1, value)
    #define HW_CPU_SET_DBGBVR3_EL1(value) HW_CPU_SET_SYSREG(dbgbvr3_el1, value)
    #define HW_CPU_GET_DBGBCR3_EL1(value) HW_CPU_GET_SYSREG(dbgbcr3_el1, value)
    #define HW_CPU_SET_DBGBCR3_EL1(value) HW_CPU_SET_SYSREG(dbgbcr3_el1, value)
    #define HW_CPU_GET_DBGBVR4_EL1(value) HW_CPU_GET_SYSREG(dbgbvr4_el1, value)
    #define HW_CPU_SET_DBGBVR4_EL1(value) HW_CPU_SET_SYSREG(dbgbvr4_el1, value)
    #define HW_CPU_GET_DBGBCR4_EL1(value) HW_CPU_GET_SYSREG(dbgbcr4_el1, value)
    #define HW_CPU_SET_DBGBCR4_EL1(value) HW_CPU_SET_SYSREG(dbgbcr4_el1, value)
    #define HW_CPU_GET_DBGBVR5_EL1(value) HW_CPU_GET_SYSREG(dbgbvr5_el1, value)
    #define HW_CPU_SET_DBGBVR5_EL1(value) HW_CPU_SET_SYSREG(dbgbvr5_el1, value)
    #define HW_CPU_GET_DBGBCR5_EL1(value) HW_CPU_GET_SYSREG(dbgbcr5_el1, value)
    #define HW_CPU_SET_DBGBCR5_EL1(value) HW_CPU_SET_SYSREG(dbgbcr5_el1, value)

    #define HW_CPU_GET_DBGWVR0_EL1(value) HW_CPU_GET_SYSREG(dbgwvr0_el1, value)
    #define HW_CPU_SET_DBGWVR0_EL1(value) HW_CPU_SET_SYSREG(dbgwvr0_el1, value)
    #define HW_CPU_GET_DBGWCR0_EL1(value) HW_CPU_GET_SYSREG(dbgwcr0_el1, value)
    #define HW_CPU_SET_DBGWCR0_EL1(value) HW_CPU_SET_SYSREG(dbgwcr0_el1, value)
    #define HW_CPU_GET_DBGWVR1_EL1(value) HW_CPU_GET_SYSREG(dbgwvr1_el1, value)
    #define HW_CPU_SET_DBGWVR1_EL1(value) HW_CPU_SET_SYSREG(dbgwvr1_el1, value)
    #define HW_CPU_GET_DBGWCR1_EL1(value) HW_CPU_GET_SYSREG(dbgwcr1_el1, value)
    #define HW_CPU_SET_DBGWCR1_EL1(value) HW_CPU_SET_SYSREG(dbgwcr1_el1, value)
    #define HW_CPU_GET_DBGWVR2_EL1(value) HW_CPU_GET_SYSREG(dbgwvr2_el1, value)
    #define HW_CPU_SET_DBGWVR2_EL1(value) HW_CPU_SET_SYSREG(dbgwvr2_el1, value)
    #define HW_CPU_GET_DBGWCR2_EL1(value) HW_CPU_GET_SYSREG(dbgwcr2_el1, value)
    #define HW_CPU_SET_DBGWCR2_EL1(value) HW_CPU_SET_SYSREG(dbgwcr2_el1, value)
    #define HW_CPU_GET_DBGWVR3_EL1(value) HW_CPU_GET_SYSREG(dbgwvr3_el1, value)
    #define HW_CPU_SET_DBGWVR3_EL1(value) HW_CPU_SET_SYSREG(dbgwvr3_el1, value)
    #define HW_CPU_GET_DBGWCR3_EL1(value) HW_CPU_GET_SYSREG(dbgwcr3_el1, value)
    #define HW_CPU_SET_DBGWCR3_EL1(value) HW_CPU_SET_SYSREG(dbgwcr3_el1, value)

    #define HW_CPU_GET_CNTFRQ_EL0(value) HW_CPU_GET_SYSREG(cntfrq_el0, value)
    #define HW_CPU_SET_CNTFRQ_EL0(value) HW_CPU_SET_SYSREG(cntfrq_el0, value)

    #define HW_CPU_GET_CNTHCTL_EL2(value) HW_CPU_GET_SYSREG(cnthctl_el2, value)
    #define HW_CPU_SET_CNTHCTL_EL2(value) HW_CPU_SET_SYSREG(cnthctl_el2, value)

    #define HW_CPU_GET_ACTLR_EL2(value) HW_CPU_GET_SYSREG(actlr_el2, value)
    #define HW_CPU_SET_ACTLR_EL2(value) HW_CPU_SET_SYSREG(actlr_el2, value)

    #define HW_CPU_GET_ACTLR_EL3(value) HW_CPU_GET_SYSREG(actlr_el3, value)
    #define HW_CPU_SET_ACTLR_EL3(value) HW_CPU_SET_SYSREG(actlr_el3, value)

    #define HW_CPU_GET_HCR_EL2(value) HW_CPU_GET_SYSREG(hcr_el2, value)
    #define HW_CPU_SET_HCR_EL2(value) HW_CPU_SET_SYSREG(hcr_el2, value)

    #define HW_CPU_GET_DACR32_EL2(value) HW_CPU_GET_SYSREG(dacr32_el2, value)
    #define HW_CPU_SET_DACR32_EL2(value) HW_CPU_SET_SYSREG(dacr32_el2, value)

    #define HW_CPU_GET_SCTLR_EL2(value) HW_CPU_GET_SYSREG(sctlr_el2, value)
    #define HW_CPU_SET_SCTLR_EL2(value) HW_CPU_SET_SYSREG(sctlr_el2, value)

    #define HW_CPU_GET_SCTLR_EL1(value) HW_CPU_GET_SYSREG(sctlr_el1, value)
    #define HW_CPU_SET_SCTLR_EL1(value) HW_CPU_SET_SYSREG(sctlr_el1, value)

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/system-control-register-el3 */
    struct SctlrEl3 {
        using M   = util::BitPack64::Field< 0, 1>;
        using A   = util::BitPack64::Field< 1, 1>;
        using C   = util::BitPack64::Field< 2, 1>;
        using Sa  = util::BitPack64::Field< 3, 1>;
        using I   = util::BitPack64::Field<12, 1>;
        using Wxn = util::BitPack64::Field<19, 1>;
        using Ee  = util::BitPack64::Field<25, 1>;

        static constexpr u64 Res1 = 0x30C50830;
    };

    /*  https://static.docs.arm.com/ddi0487/fb/DDI0487F_b_armv8_arm.pdf */
    struct SctlrEl2 {
        using M       = util::BitPack64::Field< 0, 1>;
        using A       = util::BitPack64::Field< 1, 1>;
        using C       = util::BitPack64::Field< 2, 1>;
        using Sa      = util::BitPack64::Field< 3, 1>;
        using I       = util::BitPack64::Field<12, 1>;
        using Wxn     = util::BitPack64::Field<19, 1>;
        using Ee      = util::BitPack64::Field<25, 1>;

        static constexpr u64 Res1 = 0x30C50830;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/system-control-register-el1 */
    struct SctlrEl1 {
        using M       = util::BitPack64::Field< 0, 1>;
        using A       = util::BitPack64::Field< 1, 1>;
        using C       = util::BitPack64::Field< 2, 1>;
        using Sa      = util::BitPack64::Field< 3, 1>;
        using Sa0     = util::BitPack64::Field< 4, 1>;
        using Cp15BEn = util::BitPack64::Field< 5, 1>;
        using Thee    = util::BitPack64::Field< 6, 1>;
        using Itd     = util::BitPack64::Field< 7, 1>;
        using Sed     = util::BitPack64::Field< 8, 1>;
        using Uma     = util::BitPack64::Field< 9, 1>;
        using I       = util::BitPack64::Field<12, 1>;
        using Dze     = util::BitPack64::Field<14, 1>;
        using Uct     = util::BitPack64::Field<15, 1>;
        using Ntwi    = util::BitPack64::Field<16, 1>;
        using Ntwe    = util::BitPack64::Field<18, 1>;
        using Wxn     = util::BitPack64::Field<19, 1>;
        using E0e     = util::BitPack64::Field<24, 1>;
        using Ee      = util::BitPack64::Field<25, 1>;
        using Uci     = util::BitPack64::Field<26, 1>;

        static constexpr u64 Res1 = 0x30D00800;
    };

    /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0488c/BABGIHHJ.html */
    struct ScrEl3 {
        using Ns          = util::BitPack32::Field< 0, 1>;
        using Irq         = util::BitPack32::Field< 1, 1>;
        using Fiq         = util::BitPack32::Field< 2, 1>;
        using Ea          = util::BitPack32::Field< 3, 1>;
        using Fw          = util::BitPack32::Field< 4, 1>;
        using Aw          = util::BitPack32::Field< 5, 1>;
        using Net         = util::BitPack32::Field< 6, 1>;
        using Smd         = util::BitPack32::Field< 7, 1>;
        using Hce         = util::BitPack32::Field< 8, 1>;
        using Sif         = util::BitPack32::Field< 9, 1>;
        using RwCortexA53 = util::BitPack32::Field<10, 1>;
        using StCortexA53 = util::BitPack32::Field<11, 1>;
        using Twi         = util::BitPack32::Field<12, 1>;
        using Twe         = util::BitPack32::Field<13, 1>;
    };

    /* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0488c/CIHDEBIG.html */
    struct CptrEl3 {
        using Tfp   = util::BitPack32::Field<10, 1>;
        using Tta   = util::BitPack32::Field<20, 1>;
        using Tcpac = util::BitPack32::Field<31, 1>;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/translation-control-register-el3 */
    struct TcrEl3 {
        using T0sz  = util::BitPack32::Field< 0, 6>;
        using Irgn0 = util::BitPack32::Field< 8, 2>;
        using Orgn0 = util::BitPack32::Field<10, 2>;
        using Sh0   = util::BitPack32::Field<12, 2>;
        using Tg0   = util::BitPack32::Field<14, 2>;
        using Ps    = util::BitPack32::Field<16, 3>;
        using Tbi   = util::BitPack32::Field<20, 1>;

        static constexpr u32 Res1 = 0x80800000;
    };

    struct ClidrEl1 {
        using Ctype1 = util::BitPack32::Field< 0, 3>;
        using Ctype2 = util::BitPack32::Field< 3, 3>;
        using Ctype3 = util::BitPack32::Field< 6, 3>;
        using Louis  = util::BitPack32::Field<21, 3>;
        using Loc    = util::BitPack32::Field<24, 3>;
        using Louu   = util::BitPack32::Field<27, 3>;
    };

    struct CcsidrEl1 {
        using LineSize      = util::BitPack32::Field< 0,  3>;
        using Associativity = util::BitPack32::Field< 3, 10>;
        using NumSets       = util::BitPack32::Field<13, 15>;
        using Wa            = util::BitPack32::Field<28,  1>;
        using Ra            = util::BitPack32::Field<29,  1>;
        using Wb            = util::BitPack32::Field<30,  1>;
        using Wt            = util::BitPack32::Field<31,  1>;
    };

    struct CsselrEl1 {
        using InD   = util::BitPack32::Field<0, 1>;
        using Level = util::BitPack32::Field<1, 3>;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/cpu-auxiliary-control-register-el1 */
    struct CpuactlrEl1CortexA57 {
        /* TODO: Other bits */
        using NonCacheableStreamingEnhancement                                          = util::BitPack64::Field<24, 1>;
        /* TODO: Other bits */
        using DisableLoadPassDmb                                                        = util::BitPack64::Field<59, 1>;
        using ForceProcessorRcgEnablesActive                                            = util::BitPack64::Field<63, 1>;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/cpu-extended-control-register-el1 */
    struct CpuectlrEl1CortexA57 {
        using ProcessorDynamicRetentionControl         = util::BitPack64::Field< 0, 2>;
        using Smpen                                    = util::BitPack64::Field< 6, 1>;
        using L2LoadStoreDataPrefetchDistance          = util::BitPack64::Field<32, 2>;
        using L2InstructionFetchPrefetchDistance       = util::BitPack64::Field<35, 2>;
        using DisableTableWalkDescriptorAccessPrefetch = util::BitPack64::Field<38, 1>;
    };

    /* https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers/dbgauthstatus_el1 */
    struct DbgAuthStatusEl1 {
        using Nsid  = util::BitPack32::Field<0, 2>;
        using Nsnid = util::BitPack32::Field<2, 2>;
        using Sid   = util::BitPack32::Field<4, 2>;
        using Snid  = util::BitPack32::Field<6, 2>;
    };

    /* https://developer.arm.com/docs/ddi0595/b/aarch64-system-registers/cnthctl_el2 */
    struct CnthctlEl2 {
        using El1PctEn = util::BitPack32::Field<0, 1>;
        using El1PcEn  = util::BitPack32::Field<1, 1>;
        using EvntEn   = util::BitPack32::Field<2, 1>;
        using EvntDir  = util::BitPack32::Field<3, 1>;
        using EvntI    = util::BitPack32::Field<4, 4>;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/auxiliary-control-register-el3 */
    struct ActlrCortexA57 {
        using Cpuactlr = util::BitPack32::Field<0, 1>;
        using Cpuectlr = util::BitPack32::Field<1, 1>;
        using L2ctlr   = util::BitPack32::Field<4, 1>;
        using L2ectlr  = util::BitPack32::Field<5, 1>;
        using L2actlr  = util::BitPack32::Field<6, 1>;
    };

    /* https://developer.arm.com/docs/ddi0488/h/system-control/aarch64-register-descriptions/hypervisor-configuration-register-el2 */
    struct HcrEl2 {
        using Rw = util::BitPack64::Field<31, 1>;
    };

    struct EsrEl3 {
        using Iss = util::BitPack32::Field< 0, 25>;
        using Il  = util::BitPack32::Field<25,  1>;
        using Ec  = util::BitPack32::Field<26,  6>;
    };

}
