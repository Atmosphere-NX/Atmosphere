/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::kern::smc {

    namespace {

        struct SecureMonitorArguments {
            u64 x[8];
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
                __asm__ __volatile__("smc #1"
                                    : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
                                    :
                                    : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "cc", "memory"
                                    );
                /* TODO: Restore X18 */
            }

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

    }

    /* SMC functionality needed for init. */
    namespace init {

        void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
            SecureMonitorArguments args = { FunctionId_GetConfig, static_cast<u32>(config_item) };
            CallPrivilegedSecureMonitorFunctionForInit(args);
            MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));
            for (size_t i = 0; i < num_qwords && i < 7; i++) {
                out[i] = args.x[1 + i];
            }
        }

        void GenerateRandomBytes(void *dst, size_t size) {
            /* Call SmcGenerateRandomBytes() */
            /* TODO: Lock this to ensure only one core calls at once. */
            SecureMonitorArguments args = { FunctionId_GenerateRandomBytes, size };
            MESOSPHERE_ABORT_UNLESS(size <= sizeof(args) - sizeof(args.x[0]));
            CallPrivilegedSecureMonitorFunctionForInit(args);
            MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.x[0]) == SmcResult::Success));

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

    void NORETURN Panic(u32 color) {
        SecureMonitorArguments args = { FunctionId_Panic, color };
        CallPrivilegedSecureMonitorFunction(args);
        while (true) { /* ... */ }
    }

}