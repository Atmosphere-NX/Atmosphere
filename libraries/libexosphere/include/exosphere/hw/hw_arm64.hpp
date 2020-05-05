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
#include <exosphere/hw/hw_arm64_system_registers.hpp>
#include <exosphere/hw/hw_arm64_cache.hpp>

namespace ams::hw::arch::arm64 {

    ALWAYS_INLINE void DataSynchronizationBarrier() {
        __asm__ __volatile__("dsb sy" ::: "memory");
    }

    ALWAYS_INLINE void DataSynchronizationBarrierInnerShareable() {
        __asm__ __volatile__("dsb ish" ::: "memory");
    }

    ALWAYS_INLINE void DataMemoryBarrier() {
        __asm__ __volatile__("dmb sy" ::: "memory");
    }

    ALWAYS_INLINE void InstructionSynchronizationBarrier() {
        __asm__ __volatile__("isb" ::: "memory");
    }

    ALWAYS_INLINE void WaitForInterrupt() {
        __asm__ __volatile__("wfi" ::: "memory");
    }

    ALWAYS_INLINE void WaitForEvent() {
        __asm__ __volatile__("wfe" ::: "memory");
    }

    ALWAYS_INLINE void SendEvent() {
        __asm__ __volatile__("sev" ::: "memory");
    }

    ALWAYS_INLINE int CountLeadingZeros(u64 v) {
        u64 z;
        __asm__ __volatile__("clz %[z], %[v]" : [z]"=r"(z) : [v]"r"(v));
        return z;
    }

    ALWAYS_INLINE int CountLeadingZeros(u32 v) {
        u32 z;
        __asm__ __volatile__("clz %w[z], %w[v]" : [z]"=r"(z) : [v]"r"(v));
        return z;
    }

    ALWAYS_INLINE int GetCurrentCoreId() {
        u64 mpidr;
        HW_CPU_GET_MPIDR_EL1(mpidr);
        return mpidr & 0xFF;
    }

}
