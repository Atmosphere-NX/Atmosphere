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
            FunctionId_ShowError           = 0xC3000006,
            FunctionId_ConfigureCarveout   = 0xC3000007,
            FunctionId_ReadWriteRegister   = 0xC3000008,

            /* NOTE: Atmosphere extension for mesosphere. This ID is subject to change at any time. */
            FunctionId_SetConfig           = 0xC3000409,
        };

        constexpr size_t GenerateRandomBytesSizeMax = sizeof(::ams::svc::lp64::SecureMonitorArguments) - sizeof(::ams::svc::lp64::SecureMonitorArguments{}.r[0]);

        /* Global lock for generate random bytes. */
        constinit KSpinLock g_generate_random_lock;

        bool TryGetConfigImpl(u64 *out, size_t num_qwords, ConfigItem config_item) {
            /* Create the arguments .*/
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GetConfig, static_cast<u32>(config_item) } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* If successful, copy the output. */
            const bool success = static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
            if (AMS_LIKELY(success)) {
                for (size_t i = 0; i < num_qwords && i < 7; i++) {
                    out[i] = args.r[1 + i];
                }
            }

            return success;
        }

        bool SetConfigImpl(ConfigItem config_item, u64 value) {
            /* Create the arguments .*/
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_SetConfig, static_cast<u32>(config_item), 0, value } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* Return whether the call was successful. */
            return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
        }

        bool ReadWriteRegisterImpl(u32 *out, u64 address, u32 mask, u32 value) {
            /* Create the arguments .*/
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ReadWriteRegister, address, mask, value } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* Unconditionally write the output. */
            *out = static_cast<u32>(args.r[1]);

            /* Return whether the call was successful. */
            return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
        }

        bool GenerateRandomBytesImpl(void *dst, size_t size) {
            /* Create the arguments. */
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_GenerateRandomBytes, size } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* If successful, copy the output. */
            const bool success = static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
            if (AMS_LIKELY(success)) {
                std::memcpy(dst, std::addressof(args.r[1]), size);
            }

            return success;
        }

        bool ConfigureCarveoutImpl(size_t which, uintptr_t address, size_t size) {
            /* Create the arguments .*/
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ConfigureCarveout, static_cast<u64>(which), static_cast<u64>(address), static_cast<u64>(size) } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* Return whether the call was successful. */
            return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
        }

        bool ShowErrorImpl(u32 color) {
            /* Create the arguments .*/
            ams::svc::lp64::SecureMonitorArguments args = { { FunctionId_ShowError, color } };

            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_Supervisor>(args.r);

            /* Return whether the call was successful. */
            return static_cast<SmcResult>(args.r[0]) == SmcResult::Success;
        }

        void CallSecureMonitorFromUserImpl(ams::svc::lp64::SecureMonitorArguments *args) {
            /* Call into the secure monitor. */
            ::ams::kern::arch::arm64::smc::SecureMonitorCall<SmcId_User>(args->r);
        }

    }

    /* SMC functionality needed for init. */
    namespace init {

        void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
            /* Ensure we successfully get the config. */
            MESOSPHERE_INIT_ABORT_UNLESS(TryGetConfigImpl(out, num_qwords, config_item));
        }

        void GenerateRandomBytes(void *dst, size_t size) {
            /* Check that the size is valid. */
            MESOSPHERE_INIT_ABORT_UNLESS(0 < size && size <= GenerateRandomBytesSizeMax);

            /* Ensure we successfully generate the random bytes. */
            MESOSPHERE_INIT_ABORT_UNLESS(GenerateRandomBytesImpl(dst, size));
        }

        void ReadWriteRegister(u32 *out, u64 address, u32 mask, u32 value) {
            /* Ensure we successfully access the register. */
            MESOSPHERE_INIT_ABORT_UNLESS(ReadWriteRegisterImpl(out, address, mask, value));
        }

    }

    bool TryGetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Get the config. */
        return TryGetConfigImpl(out, num_qwords, config_item);
    }

    void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item) {
        /* Ensure we successfully get the config. */
        MESOSPHERE_ABORT_UNLESS(TryGetConfig(out, num_qwords, config_item));
    }

    bool SetConfig(ConfigItem config_item, u64 value) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Set the config. */
        return SetConfigImpl(config_item, value);
    }

    bool ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Access the register. */
        return ReadWriteRegisterImpl(out, address, mask, value);
    }

    void ConfigureCarveout(size_t which, uintptr_t address, size_t size) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Ensure that we successfully configure the carveout. */
        MESOSPHERE_ABORT_UNLESS(ConfigureCarveoutImpl(which, address, size));
    }

    void GenerateRandomBytes(void *dst, size_t size) {
        /* Check that the size is valid. */
        MESOSPHERE_ABORT_UNLESS(0 < size && size <= GenerateRandomBytesSizeMax);

        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Acquire the exclusive right to generate random bytes. */
        KScopedSpinLock lk(g_generate_random_lock);

        /* Ensure we successfully generate the random bytes. */
        MESOSPHERE_ABORT_UNLESS(GenerateRandomBytesImpl(dst, size));
    }

    void ShowError(u32 color) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Ensure we successfully show the error. */
        MESOSPHERE_ABORT_UNLESS(ShowErrorImpl(color));
    }

    void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        /* Disable interrupts. */
        KScopedInterruptDisable di;

        /* Perform the call. */
        CallSecureMonitorFromUserImpl(args);
    }

}