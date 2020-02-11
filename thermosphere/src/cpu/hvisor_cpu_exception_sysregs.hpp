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

#include "../defines.hpp"
#include "hvisor_cpu_sysreg_general.hpp"

namespace ams::hvisor::cpu {

    // FIXME GCC 10

    struct ExceptionSyndromeRegister {
        enum ExceptionClass : u32 {
            Uncategorized = 0x0,
            WFxTrap = 0x1,
            CP15RTTrap = 0x3,
            CP15RRTTrap = 0x4,
            CP14RTTrap = 0x5,
            CP14DTTrap = 0x6,
            AdvSIMDFPAccessTrap = 0x7,
            FPIDTrap = 0x8,
            PACTrap = 0x9,
            CP14RRTTrap = 0xC,
            BranchTargetException = 0xD, // No official enum field name from Arm yet
            IllegalState = 0xE,
            SupervisorCallA32 = 0x11,
            HypervisorCallA32 = 0x12,
            MonitorCallA32 = 0x13,
            SupervisorCallA64 = 0x15,
            HypervisorCallA64 = 0x16,
            MonitorCallA64 = 0x17,
            SystemRegisterTrap = 0x18,
            SVEAccessTrap = 0x19,
            ERetTrap = 0x1A,
            El3_ImplementationDefined = 0x1F,
            InstructionAbortLowerEl = 0x20,
            InstructionAbortSameEl = 0x21,
            PCAlignment = 0x22,
            DataAbortLowerEl = 0x24,
            DataAbortSameEl = 0x25,
            SPAlignment = 0x26,
            FPTrappedExceptionA32 = 0x28,
            FPTrappedExceptionA64 = 0x2C,
            SError = 0x2F,
            BreakpointLowerEl = 0x30,
            BreakpointSameEl = 0x31,
            SoftwareStepLowerEl = 0x32,
            SoftwareStepSameEl = 0x33,
            WatchpointLowerEl = 0x34,
            WatchpointSameEl = 0x35,
            SoftwareBreakpointA32 = 0x38,
            VectorCatchA32 = 0x3A,
            SoftwareBreakpointA64 = 0x3C,
        };

        u32 iss             : 25;   // Instruction Specific Syndrome
        u32 il              :  1;   // Instruction Length (16 or 32-bit)
        ExceptionClass ec   :  6;   // Exception Class
        u32 res0            : 32;

        constexpr size_t GetInstructionLength()
        {
            return il == 0 ? 2 : 4;
        }
    };


    struct DataAbortIss {
        u32 dfsc        : 6; // Fault status code

        u32 wnr         : 1; // Write, not Read
        u32 s1ptw       : 1; // Stage1 page table walk fault
        u32 cm          : 1; // Cache maintenance
        u32 ea          : 1; // External abort
        u32 fnv         : 1; // FAR not Valid
        u32 set         : 2; // Synchronous error type
        u32 vncr        : 1; // vncr_el2 trap

        u32 ar          : 1; // Acquire/release. Bit 14
        u32 sf          : 1; // 64-bit register used
        u32 srt         : 5; // Syndrome register transfer (register used)
        u32 sse         : 1; // Syndrome sign extend
        u32 sas         : 2; // Syndrome access size. Bit 23

        u32 isv         : 1; // Instruction syndrome valid (ISS[23:14] valid)

        constexpr bool HasValidFar()
        {
            return isv && fnv;
        }
        constexpr size_t GetAccessSize()
        {
            return BITL(sas);
        }
    };


    static_assert(std::is_standard_layout_v<ExceptionSyndromeRegister>);
    static_assert(std::is_standard_layout_v<DataAbortIss>);
    static_assert(std::is_trivial_v<ExceptionSyndromeRegister>);
    static_assert(std::is_trivial_v<DataAbortIss>);

}
