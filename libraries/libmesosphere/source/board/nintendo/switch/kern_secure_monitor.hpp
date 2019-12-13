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
#pragma once
#include <mesosphere.hpp>

namespace ams::kern::smc {

    /* Types. */
    enum MemoryMode {
        MemoryMode_4GB = 0,
        MemoryMode_6GB = 1,
        MemoryMode_8GB = 2,
    };

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
        MemoryArrange               = 10,
        IsDebugMode                 = 11,
        KernelConfiguration         = 12,
        IsChargerHiZModeEnabled     = 13,
        IsKiosk                     = 14,
        NewHardwareType             = 15,
        NewKeyGeneration            = 16,
        Package2Hash                = 17,

        /* Extension config items for exosphere. */
        ExosphereApiVersion     = 65000,
        ExosphereNeedsReboot    = 65001,
        ExosphereNeedsShutdown  = 65002,
        ExosphereGitCommitHash  = 65003,
        ExosphereHasRcmBugPatch = 65004,
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

    /* TODO: Rest of Secure Monitor API. */
    void GetConfig(u64 *out, size_t num_qwords, ConfigItem config_item);
    void NORETURN Panic(u32 color);
    bool ReadWriteRegister(u32 *out, u64 address, u32 mask, u32 value);

}