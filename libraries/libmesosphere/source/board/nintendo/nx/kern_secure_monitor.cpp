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
#include <mesosphere.hpp>
#include "kern_secure_monitor.hpp"

namespace ams::kern::board::nintendo::nx::smc {

    namespace {

        struct SecureMonitorArguments {
            u64 x[8];
        };

        enum UserFunctionId : u32 {
            UserFunctionId_SetConfig = 0xC3000401,
        };

        enum FunctionId : u32 {
            FunctionId_CpuSuspend          = 0xC4000001,
            FunctionId_CpuOff              = 0x84000002,
            FunctionId_CpuOn               = 0xC4000003,
            FunctionId_GetConfig           = 0xC3000004,
            FunctionId_GenerateRandomBytes = 0xC3000005,
            FunctionId_Panic               = 0xC3000006,
            FunctionId_ConfigureCarveout   = 0xC3000007,
            FunctionId_ReadWriteRegister   = 0xC3000008,

            /* NOTE: Atmosphere extension for mesosphere. This ID is subject to change at any time. */
            FunctionId_SetConfig           = 0xC3000409,
        };

        void CallPrivilegedSecureMonitorFunction(SecureMonitorArguments &args) {
            /* Load arguments into registers. */
            register u64 x0 asm("x0") = args.x[0];
            register u64 x1 asm("x1") = args.x[1];
            register u64 x2 asm("x2") = args.x[2];
            register u64 x3 asm("x3") = args.x[3];
            register u64 x4 asm("x4") = args.x[4];
            register u64 x5 asm("x5") = args.x[5];
            register u64 x6 asm("x6") = args.x[6];
            register u64 x7 asm("x7") = args.x[7];

            /* Actually make the call. */
            {
                /* Disable interrupts while making the call. */
                KScopedInterruptDisable intr_disable;

                {
                    /* Backup the current thread pointer. */
                    const uintptr_t current_thread_pointer_value = cpu::GetCurrentThreadPointerValue();

                    __asm__ __volatile__("smc #1"
                                        : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                        :
                                        : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                        );

                    /* Restore the current thread pointer into X18. */
                    cpu::SetCurrentThreadPointerValue(current_thread_pointer_value);

                    /* Store arguments to output. */
                    args.x[0] = x0;
                    args.x[1] = x1;
                    args.x[2] = x2;
                    args.x[3] = x3;
                    args.x[4] = x4;
                    args.x[5] = x5;
                    args.x[6] = x6;
                    args.x[7] = x7;
                }
            }
        }

        void CallUserSecureMonitorFunction(ams::svc::lp64::SecureMonitorArguments *args) {
            /* Load arguments into registers. */
            register u64 x0 asm("x0") = args->r[0];
            register u64 x1 asm("x1") = args->r[1];
            register u64 x2 asm("x2") = args->r[2];
            register u64 x3 asm("x3") = args->r[3];
            register u64 x4 asm("x4") = args->r[4];
            register u64 x5 asm("x5") = args->r[5];
            register u64 x6 asm("x6") = args->r[6];
            register u64 x7 asm("x7") = args->r[7];

            /* Actually make the call. */
            {
                /* Disable interrupts while making the call. */
                KScopedInterruptDisable intr_disable;

                {
                    /* Backup the current thread pointer. */
                    const uintptr_t current_thread_pointer_value = cpu::GetCurrentThreadPointerValue();

                    __asm__ __volatile__("smc #0"
                                        : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                        :
                                        : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                        );

                    /* Restore the current thread pointer into X18. */
                    cpu::SetCurrentThreadPointerValue(current_thread_pointer_value);

                    /* Store arguments to output. */
                    args->r[0] = x0;
                    args->r[1] = x1;
                    args->r[2] = x2;
                    args->r[3] = x3;
                    args->r[4] = x4;
                    args->r[5] = x5;
                    args->r[6] = x6;
                    args->r[7] = x7;
                }
            }
        }

        void CallPrivilegedSecureMonitorFunctionForInit(SecureMonitorArguments &args) {
            /* Load arguments into registers. */
            register u64 x0 asm("x0") = args.x[0];
            register u64 x1 asm("x1") = args.x[1];
            register u64 x2 asm("x2") = args.x[2];
            register u64 x3 asm("x3") = args.x[3];
            register u64 x4 asm("x4") = args.x[4];
            register u64 x5 asm("x5") = args.x[5];
            register u64 x6 asm("x6") = args.x[6];
            register u64 x7 asm("x7") = args.x[7];

            /* Actually make the call. */
            __asm__ __volatile__("smc #1"
                                : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                :
                                : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                );

            /* Store arguments to output. */
            args.x[0] = x0;
            args.x[1] = x1;
            args.x[2] = x2;
            args.x[3] = x3;
            args.x[4] = x4;
            args.x[5] = x5;
            args.x[6] = x6;
            args.x[7] = x7;
        }

        /* Global lock for generate random bytes. */
        KSpinLock g_generate_random_lock;

    }

    /* SMC functionality needed for init. */
    namespace init {

        void CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
            SecureMonitorArguments args = { FunctionId_CpuOn, core_id, entrypoint, arg };
            CallPrivilegedSecureMonitorFunctionForInit(args);
        }

        void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
            SecureMonitorArguments args = { FunctionId_GetConfig, static_cast<u32>(config_item) };
            CallPrivilegedSecureMonitorFunctionForInit(args);
            MESOSPHERE_INIT_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));
            for (size_t i = 0; i < num_qwords && i < 7; i++) {
                out[i] = args.x[1 + i];
            }
        }

        void GenerateRandomBytes(void *dst, size_t size) {
            /* Call SmcGenerateRandomBytes() */
            SecureMonitorArguments args = { FunctionId_GenerateRandomBytes, size };
            MESOSPHERE_INIT_ABORT_UNLESS(size <= sizeof(args) - sizeof(args.x[0]));

            CallPrivilegedSecureMonitorFunctionForInit(args);
            MESOSPHERE_INIT_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));

            /* Copy output. */
            std::memcpy(dst, &args.x[1], size);
        }

        bool ReadWriteRegister(u32 *out, u64 address, u32 mask, u32 value) {
            SecureMonitorArguments args = { FunctionId_ReadWriteRegister, address, mask, value };
            CallPrivilegedSecureMonitorFunctionForInit(args);
            *out = args.x[1];
            return static_cast<SmcResult>(args.x[0]) == SmcResult::Success;
        }

    }

    bool TryGetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        SecureMonitorArguments args = { FunctionId_GetConfig, static_cast<u32>(config_item) };
        CallPrivilegedSecureMonitorFunction(args);
        if (static_cast<SmcResult>(args.x[0]) != SmcResult::Success) {
            return false;
        }

        for (size_t i = 0; i < num_qwords && i < 7; i++) {
            out[i] = args.x[1 + i];
        }

        return true;
    }

    void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        MESOSPHERE_ABORT_UNLESS(TryGetConfig(out, num_qwords, config_item));
    }

    bool SetConfig(ConfigItem config_item, u64 value) {
        SecureMonitorArguments args = { FunctionId_SetConfig, static_cast<u32>(config_item), 0, value };
        CallPrivilegedSecureMonitorFunction(args);
        return static_cast<SmcResult>(args.x[0]) == SmcResult::Success;
    }

    bool ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        SecureMonitorArguments args = { FunctionId_ReadWriteRegister, address, mask, value };
        CallPrivilegedSecureMonitorFunction(args);
        *out = static_cast<u32>(args.x[1]);
        return static_cast<SmcResult>(args.x[0]) == SmcResult::Success;
    }

    void ConfigureCarveout(size_t which, uintptr_t address, size_t size) {
        SecureMonitorArguments args = { FunctionId_ConfigureCarveout, static_cast<u64>(which), static_cast<u64>(address), static_cast<u64>(size) };
        CallPrivilegedSecureMonitorFunction(args);
        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));
    }

    void CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        SecureMonitorArguments args = { FunctionId_CpuOn, core_id, static_cast<u64>(entrypoint), static_cast<u64>(arg) };
        CallPrivilegedSecureMonitorFunction(args);
        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));
    }

    void GenerateRandomBytes(void *dst, size_t size) {
        /* Setup for call. */
        SecureMonitorArguments args = { FunctionId_GenerateRandomBytes, size };
        MESOSPHERE_ABORT_UNLESS(size <= sizeof(args) - sizeof(args.x[0]));

        /* Make call. */
        {
            KScopedInterruptDisable intr_disable;
            KScopedSpinLock lk(g_generate_random_lock);
            CallPrivilegedSecureMonitorFunction(args);
        }
        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));

        /* Copy output. */
        std::memcpy(dst, &args.x[1], size);
    }

    void NORETURN Panic(u32 color) {
        SecureMonitorArguments args = { FunctionId_Panic, color };
        CallPrivilegedSecureMonitorFunction(args);
        AMS_INFINITE_LOOP();
    }

    void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        CallUserSecureMonitorFunction(args);
    }

}