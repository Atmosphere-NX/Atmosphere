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
#include <mesosphere.hpp>

namespace ams::kern::board::qemu::virt::smc {

    enum class ConfigItem : u32 {
        /* Standard config items. */
        DisableProgramVerification  = 1,
        DramId                      = 2,
        SecurityEngineIrqNumber     = 3,
        Version                     = 4,
        HardwareType                = 5,
        IsRetail                    = 6,
        IsRecoveryBoot              = 7,
        DeviceId                    = 8,
        BootReason                  = 9,
        MemoryMode                  = 10,
        IsDebugMode                 = 11,
        KernelConfiguration         = 12,
        IsChargerHiZModeEnabled     = 13,
        IsQuest                     = 14,
        RegulatorType               = 15,
        DeviceUniqueKeyGeneration   = 16,
        Package2Hash                = 17,

        /* Extension config items for exosphere. */
        ExosphereApiVersion          = 65000,
        ExosphereNeedsReboot         = 65001,
        ExosphereNeedsShutdown       = 65002,
        ExosphereGitCommitHash       = 65003,
        ExosphereHasRcmBugPatch      = 65004,
        ExosphereBlankProdInfo       = 65005,
        ExosphereAllowCalWrites      = 65006,
        ExosphereEmummcType          = 65007,
        ExospherePayloadAddress      = 65008,
        ExosphereLogConfiguration    = 65009,
        ExosphereForceEnableUsb30    = 65010,
        ExosphereSupportedHosVersion = 65011,
    };

    enum class SmcResult {
        Success               = 0,
        NotImplemented        = 1,
        InvalidArgument       = 2,
        InProgress            = 3,
        NoAsyncOperation      = 4,
        InvalidAsyncOperation = 5,
        NotPermitted          = 6,
    };

    void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args);

}