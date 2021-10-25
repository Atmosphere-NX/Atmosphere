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