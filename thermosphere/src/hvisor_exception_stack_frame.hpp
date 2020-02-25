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

#include "cpu/hvisor_cpu_exception_sysregs.hpp"

namespace ams::hvisor {

    struct alignas(16) ExceptionStackFrame {
        u64 x[31]; // x0 .. x30
        union {
            u64 sp_el1;
            u64 sp_el2;
        };
        u64 sp_el0;
        u64 elr_el2;
        u64 spsr_el2;
        cpu::ExceptionSyndromeRegister esr_el2;
        u64 far_el2;
        u64 cntpct_el0;
        u64 cntp_ctl_el0;
        u64 cntv_ctl_el0;

        constexpr bool IsA32() const    { return (spsr_el2 & cpu::PSR_MODE32) != 0; }
        constexpr bool IsThumb() const  { return IsA32() && (spsr_el2 & cpu::PSR_AA32_THUMB) != 0; }

        constexpr u32 GetT32ItFlags() const
        {
            u64 it10 = (spsr_el2 >> cpu::PSR_AA32_IT10_MASK) & cpu::PSR_AA32_IT10_MASK;
            u64 it72 = (spsr_el2 >> cpu::PSR_AA32_IT72_MASK) & cpu::PSR_AA32_IT72_MASK;
            return it72 << 2 | it10;
        }
        constexpr void SetT32ItFlags(u32 flags)
        {
            spsr_el2 &= ~(cpu::PSR_AA32_IT72_MASK << cpu::PSR_AA32_IT72_SHIFT);
            spsr_el2 &= ~(cpu::PSR_AA32_IT10_MASK << cpu::PSR_AA32_IT10_SHIFT);

            u64 it10 = flags & cpu::PSR_AA32_IT10_MASK;
            u64 it72 = (flags >> 2) & cpu::PSR_AA32_IT72_MASK;

            spsr_el2 |= it72 << cpu::PSR_AA32_IT72_SHIFT;
            spsr_el2 |= it10 << cpu::PSR_AA32_IT10_SHIFT;
        }

        constexpr bool EvaluateConditionCode(u32 conditionCode) const
        {
            u64 spsr = spsr_el2;
            if (conditionCode == 14) {
                // AL
                return true;
            } else if (conditionCode == 15) {
                // Invalid encoding
                return false;
            }

            // NZCV
            bool n = (spsr & BIT(31)) != 0;
            bool z = (spsr & BIT(30)) != 0;
            bool c = (spsr & BIT(29)) != 0;
            bool v = (spsr & BIT(28)) != 0;

            bool tableHalf[] = {
                // EQ, CS, MI, VS, HI, GE, GT
                z, c, n, v, c && !z, n == v, !z && n == v,
            };

            return (conditionCode & 1) == 0 ? tableHalf[conditionCode / 2] : !tableHalf[conditionCode / 2];
        }

        constexpr void AdvanceItState()
        {
            u32 it = GetT32ItFlags();

            // Just in case EL0 is executing A32 (& not sure if fully supported)
            if (!IsThumb() || it == 0) {
                return;
            }

            // Last instruction of the block => wipe, otherwise advance
            SetT32ItFlags((it & 7) == 0 ? 0 : (it & 0xE0) | ((it << 1) & 0x1F));
        }

        constexpr void SkipInstruction(size_t size)
        {
            AdvanceItState();
            elr_el2 += size;
        }

        template<typename T = u64>
        constexpr T ReadRegister(u32 id) const
        {
            static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
            return id == 31 ? static_cast<T>(0u) /* xzr */ : static_cast<T>(x[id]);
        }
        constexpr void WriteRegister(u32 id, u64 val)
        {
            if (id != 31) {
                // If not xzr
                x[id] = val;
            }
        }

        constexpr u64 &GetSpRef()
        {
            // Note: the return value is more or less meaningless if we took an exception from A32...
            // We try our best to reflect which privilege level the exception was took from, nonetheless

            bool spEl0 = false;
            u64 m = spsr_el2 & 0xF;
            if (IsA32()) {
                spEl0 = m == 0;
            } else {
                u64 el = m >> 2;
                spEl0 = el == 0 || (m & 1) == 0; // note: frame->sp_el2 is aliased to frame->sp_el1
            }

            return spEl0 ? sp_el0 : sp_el1;
        }

    };

    static_assert(offsetof(ExceptionStackFrame, far_el2) == 0x120, "Wrong definition for ExceptionStackFrame");
    static_assert(sizeof(ExceptionStackFrame) == 0x140, "Wrong size for ExceptionStackFrame");

    static_assert(std::is_standard_layout_v<ExceptionStackFrame>);
    static_assert(std::is_trivial_v<ExceptionStackFrame>);
}

/*void dumpStackFrame(const ExceptionStackFrame *frame, bool sameEl);
void exceptionEnterInterruptibleHypervisorCode(void);*/
