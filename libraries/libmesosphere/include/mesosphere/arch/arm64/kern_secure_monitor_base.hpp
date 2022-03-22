/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_select_interrupt_manager.hpp>

namespace ams::kern::arch::arm64::smc {

    template<int SmcId, bool DisableInterrupt>
    void SecureMonitorCall(u64 *buf) {
        /* Load arguments into registers. */
        register u64 x0 asm("x0") = buf[0];
        register u64 x1 asm("x1") = buf[1];
        register u64 x2 asm("x2") = buf[2];
        register u64 x3 asm("x3") = buf[3];
        register u64 x4 asm("x4") = buf[4];
        register u64 x5 asm("x5") = buf[5];
        register u64 x6 asm("x6") = buf[6];
        register u64 x7 asm("x7") = buf[7];

        /* Perform the call. */
        if constexpr (DisableInterrupt) {
            KScopedInterruptDisable di;

            /* Backup the current thread pointer. */
            const uintptr_t current_thread_pointer_value = cpu::GetCurrentThreadPointerValue();

            __asm__ __volatile__("smc %c[smc_id]"
                                : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                : [smc_id]"i"(SmcId)
                                : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                );

            /* Restore the current thread pointer into X18. */
            cpu::SetCurrentThreadPointerValue(current_thread_pointer_value);
        } else {
            /* Backup the current thread pointer. */
            const uintptr_t current_thread_pointer_value = cpu::GetCurrentThreadPointerValue();

            __asm__ __volatile__("smc %c[smc_id]"
                                : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                : [smc_id]"i"(SmcId)
                                : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                );

            /* Restore the current thread pointer into X18. */
            cpu::SetCurrentThreadPointerValue(current_thread_pointer_value);
        }

        /* Store arguments to output. */
        buf[0] = x0;
        buf[1] = x1;
        buf[2] = x2;
        buf[3] = x3;
        buf[4] = x4;
        buf[5] = x5;
        buf[6] = x6;
        buf[7] = x7;
    }

    enum PsciFunction {
        PsciFunction_CpuSuspend = 0xC4000001,
        PsciFunction_CpuOff     = 0x84000002,
        PsciFunction_CpuOn      = 0xC4000003,
    };

    template<int SmcId, bool DisableInterrupt>
    u64 PsciCall(PsciFunction function, u64 x1 = 0, u64 x2 = 0, u64 x3 = 0, u64 x4 = 0, u64 x5 = 0, u64 x6 = 0, u64 x7 = 0) {
        ams::svc::lp64::SecureMonitorArguments args = { { function, x1, x2, x3, x4, x5, x6, x7 } };

        SecureMonitorCall<SmcId, DisableInterrupt>(args.r);

        return args.r[0];
    }

    template<int SmcId, bool DisableInterrupt>
    u64 CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        return PsciCall<SmcId, DisableInterrupt>(PsciFunction_CpuOn, core_id, entrypoint, arg);
    }

}
