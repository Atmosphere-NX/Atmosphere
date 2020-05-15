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
#include <exosphere/secmon/secmon_emummc_context.hpp>

namespace ams::secmon {

    enum SecureMonitorConfigurationFlag : u32 {
        SecureMonitorConfigurationFlag_None                                   = (0u << 0),
        SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForKernel  = (1u << 1),
        SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForUser    = (1u << 2),
        SecureMonitorConfigurationFlag_DisableUserModeExceptionHandlers       = (1u << 3),
        SecureMonitorConfigurationFlag_EnableUserModePerformanceCounterAccess = (1u << 4),
        SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary        = (1u << 5),
        SecureMonitorConfigurationFlag_AllowWritingToCalibrationBinarySysmmc  = (1u << 6),

        SecureMonitorConfigurationFlag_Default = SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForKernel,
    };

    struct SecureMonitorStorageConfiguration {
        static constexpr u32 Magic = util::FourCC<'E','X','O','0'>::Code;

        u32 magic;
        ams::TargetFirmware target_firmware;
        u32 flags;
        u32 reserved[5];
        EmummcConfiguration emummc_cfg;

        constexpr bool IsValid() const { return this->magic == Magic; }
    };
    static_assert(util::is_pod<SecureMonitorStorageConfiguration>::value);
    static_assert(sizeof(SecureMonitorStorageConfiguration) == 0x130);

    struct SecureMonitorConfiguration {
        ams::TargetFirmware target_firmware;
        s32 key_generation;
        u32 flags;
        u32 reserved[(0x80 - 0x0C) / sizeof(u32)];

        constexpr void CopyFrom(const SecureMonitorStorageConfiguration &storage) {
            this->target_firmware = storage.target_firmware;
            this->flags           = storage.flags;
        }

        constexpr ams::TargetFirmware GetTargetFirmware() const { return this->target_firmware; }
        constexpr int GetKeyGeneration() const { return this->key_generation; }

        constexpr bool IsDevelopmentFunctionEnabledForKernel()  const { return (this->flags & SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForKernel)  != 0; }
        constexpr bool IsDevelopmentFunctionEnabledForUser()    const { return (this->flags & SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForUser)    != 0; }
        constexpr bool DisableUserModeExceptionHandlers()       const { return (this->flags & SecureMonitorConfigurationFlag_DisableUserModeExceptionHandlers)       != 0; }
        constexpr bool EnableUserModePerformanceCounterAccess() const { return (this->flags & SecureMonitorConfigurationFlag_EnableUserModePerformanceCounterAccess) != 0; }
        constexpr bool ShouldUseBlankCalibrationBinary()        const { return (this->flags & SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary)        != 0; }
        constexpr bool AllowWritingToCalibrationBinarySysmmc()  const { return (this->flags & SecureMonitorConfigurationFlag_AllowWritingToCalibrationBinarySysmmc)  != 0; }

        constexpr bool IsDevelopmentFunctionEnabled(bool for_kern) const { return for_kern ? this->IsDevelopmentFunctionEnabledForKernel() : this->IsDevelopmentFunctionEnabledForUser(); }
    };
    static_assert(util::is_pod<SecureMonitorConfiguration>::value);
    static_assert(sizeof(SecureMonitorConfiguration) == 0x80);

    constexpr inline const SecureMonitorConfiguration DefaultSecureMonitorConfiguration = {
        .target_firmware = ams::TargetFirmware_Current,
        .flags           = SecureMonitorConfigurationFlag_Default,
    };

}
