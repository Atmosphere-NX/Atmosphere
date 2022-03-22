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

namespace ams::kern::board::nintendo::nx::smc {

    namespace {

        enum UserFunctionId : u32 {
            UserFunctionId_SetConfig                       = 0xC3000401,
            UserFunctionId_GetConfigUser                   = 0xC3000002,
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
            FunctionId_GetConfig           = 0xC3000004,
            FunctionId_GenerateRandomBytes = 0xC3000005,
            FunctionId_Panic               = 0xC3000006,
            FunctionId_ConfigureCarveout   = 0xC3000007,
            FunctionId_ReadWriteRegister   = 0xC3000008,

            /* NOTE: Atmosphere extension for mesosphere. This ID is subject to change at any time. */
            FunctionId_SetConfig           = 0xC3000409,
        };

        /* Global lock for generate random bytes. */
        constinit KSpinLock g_generate_random_lock;

    }

    /* SMC functionality needed for init. */
    namespace init {

        void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GetConfig, static_cast<u32>(config_item) } };

            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, false>(args.r);
            MESOSPHERE_INIT_ABORT_UNLESS((static_cast<SmcResult>(args.r[0]) == SmcResult::Success));

            for (size_t i = 0; i < num_qwords && i < 7; i++) {
                out[i] = args.r[1 + i];
            }
        }

        void GenerateRandomBytes(void *dst, size_t size) {
            /* Call SmcGenerateRandomBytes() */
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GenerateRandomBytes, size } };
            MESOSPHERE_INIT_ABORT_UNLESS(size <= sizeof(args) - sizeof(args.r[0]));

            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, false>(args.r);
            MESOSPHERE_INIT_ABORT_UNLESS((static_cast<SmcResult>(args.r[0]) == SmcResult::Success));

            /* Copy output. */
            std::memcpy(dst, std::addressof(args.r[1]), size);
        }

        bool ReadWriteRegister(u32 *out, u64 address, u32 mask, u32 value) {
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ReadWriteRegister, address, mask, value } };

            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, false>(args.r);
            MESOSPHERE_INIT_ABORT_UNLESS((static_cast<SmcResult>(args.r[0]) == SmcResult::Success));

            *out = args.r[1];

            return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
        }

    }

    bool TryGetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GetConfig, static_cast<u32>(config_item) } };

        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);
        if (AMS_UNLIKELY(static_cast<SmcResult>(args.r[0]) != SmcResult::Success)) {
            return false;
        }

        for (size_t i = 0; i < num_qwords && i < 7; i++) {
            out[i] = args.r[1 + i];
        }

        return true;
    }

    void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        MESOSPHERE_ABORT_UNLESS(TryGetConfig(out, num_qwords, config_item));
    }

    bool SetConfig(ConfigItem config_item, u64 value) {
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_SetConfig, static_cast<u32>(config_item), 0, value } };

        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);

        return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
    }

    bool ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ReadWriteRegister, address, mask, value } };

        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);

        *out = static_cast<u32>(args.r[1]);
        return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
    }

    void ConfigureCarveout(size_t which, uintptr_t address, size_t size) {
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ConfigureCarveout, static_cast<u64>(which), static_cast<u64>(address), static_cast<u64>(size) } };

        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);

        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.r[0]) == SmcResult::Success));
    }

    void GenerateRandomBytes(void *dst, size_t size) {
        /* Setup for call. */
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GenerateRandomBytes, size } };
        MESOSPHERE_ABORT_UNLESS(size <= sizeof(args) - sizeof(args.r[0]));

        /* Make call. */
        {
            KScopedInterruptDisable intr_disable;
            KScopedSpinLock lk(g_generate_random_lock);

            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);
        }
        MESOSPHERE_ABORT_UNLESS((static_cast<SmcResult>(args.r[0]) == SmcResult::Success));

        /* Copy output. */
        std::memcpy(dst, std::addressof(args.r[1]), size);
    }

    void NORETURN Panic(u32 color) {
        ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_Panic, color } };

        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor, true>(args.r);

        AMS_INFINITE_LOOP();
    }

    void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_User, true>(args->r);
    }

}