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
#include <mesosphere.hpp>
#include "kern_secure_monitor.hpp"

namespace ams::kern::board::qemu::virt::smc {

    namespace {

        struct SecureMonitorArguments {
            u64 x[8];
        };

        enum UserFunctionId : u32 {
            UserFunctionId_SetConfig                       = 0xC3000401,
            UserFunctionId_GetConfig                       = 0xC3000002,
            UserFunctionId_GetResult                       = 0xC3000003,
            UserFunctionId_GetResultData                   = 0xC3000404,
            UserFunctionId_ModularExponentiate             = 0xC3000E05,
            UserFunctionId_GenerateRandomBytes             = 0xC3000006,
            UserFunctionId_GenerateAesKek                  = 0xC3000007,
            UserFunctionId_LoadAesKey                      = 0xC3000008,
            UserFunctionId_ComputeAes                      = 0xC3000009,
            UserFunctionId_GenerateSpecificAesKey          = 0xC300000A,
            UserFunctionId_ComputeCmac                     = 0xC300040B,
            UserFunctionId_ReencryptDeviceUniqueData       = 0xC300D60C,
            UserFunctionId_DecryptDeviceUniqueData         = 0xC300100D,
            UserFunctionId_ModularExponentiateByStorageKey = 0xC300060F,
            UserFunctionId_PrepareEsDeviceUniqueKey        = 0xC3000610,
            UserFunctionId_LoadPreparedAesKey              = 0xC3000011,
            UserFunctionId_PrepareEsCommonTitleKey         = 0xC3000012,
        };

        enum FunctionId : u32 {
            FunctionId_CpuSuspend          = 0xC4000001,
            FunctionId_CpuOff              = 0x84000002,
            FunctionId_CpuOn               = 0xC4000003,
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

                    __asm__ __volatile__("smc #0"
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
            __asm__ __volatile__("smc #0"
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

    }

    void CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        SecureMonitorArguments args = { FunctionId_CpuOn, core_id, static_cast<u64>(entrypoint), static_cast<u64>(arg) };
        CallPrivilegedSecureMonitorFunction(args);
        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));
    }

    void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        MESOSPHERE_LOG("Received SMC [%p %p %p %p %p %p %p %p] from %s\n", reinterpret_cast<void *>(args->r[0]), reinterpret_cast<void *>(args->r[1]), reinterpret_cast<void *>(args->r[2]), reinterpret_cast<void *>(args->r[3]), reinterpret_cast<void *>(args->r[4]), reinterpret_cast<void *>(args->r[5]), reinterpret_cast<void *>(args->r[6]), reinterpret_cast<void *>(args->r[7]), GetCurrentProcess().GetName());

        switch (args->r[0]) {
            case UserFunctionId_GetConfig:
                {
                    switch (static_cast<ConfigItem>(args->r[1])) {
                        case ConfigItem::ExosphereApiVersion:
                            args->r[1] = (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MAJOR & 0xFF) << 56) |
                                         (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MINOR & 0xFF) << 48) |
                                         (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MICRO & 0xFF) << 40) |
                                         (static_cast<u64>(13)                                      << 32) |
                                         (static_cast<u64>(GetTargetFirmware())                     <<  0);
                            break;
                        default:
                            MESOSPHERE_PANIC("Unhandled GetConfig\n");
                    }

                    args->r[0] = static_cast<u64>(SmcResult::Success);
                }
                break;
            default:
                MESOSPHERE_PANIC("Unhandled SMC [%p %p %p %p %p %p %p %p]", reinterpret_cast<void *>(args->r[0]), reinterpret_cast<void *>(args->r[1]), reinterpret_cast<void *>(args->r[2]), reinterpret_cast<void *>(args->r[3]), reinterpret_cast<void *>(args->r[4]), reinterpret_cast<void *>(args->r[5]), reinterpret_cast<void *>(args->r[6]), reinterpret_cast<void *>(args->r[7]));
        }
    }

}