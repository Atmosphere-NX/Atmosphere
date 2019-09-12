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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>

static FirmwareVersion g_firmware_version = FirmwareVersion_Min;
static bool g_HasCached = 0;
static Mutex g_Mutex;

static void _CacheValues(void)
{
    if (__atomic_load_n(&g_HasCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_Mutex);

    if (g_HasCached) {
        mutexUnlock(&g_Mutex);
        return;
    }

    u32 target_fw = 0;
    {
        SecmonArgs args = {0};
        args.X[0] = 0xC3000002; /* smcGetConfig */
        args.X[1] = 65000; /* ConfigItem_ExosphereVersion */
        R_ASSERT(svcCallSecureMonitor(&args));
        if (args.X[0] != 0) {
            std::abort();
        }

        target_fw = (args.X[1] >> 0x08) & 0xFF;
    }

    switch (static_cast<AtmosphereTargetFirmware>(target_fw)) {
        case AtmosphereTargetFirmware_100:
            g_firmware_version = FirmwareVersion_100;
            break;
        case AtmosphereTargetFirmware_200:
            g_firmware_version = FirmwareVersion_200;
            break;
        case AtmosphereTargetFirmware_300:
            g_firmware_version = FirmwareVersion_300;
            break;
        case AtmosphereTargetFirmware_400:
            g_firmware_version = FirmwareVersion_400;
            break;
        case AtmosphereTargetFirmware_500:
            g_firmware_version = FirmwareVersion_500;
            break;
        case AtmosphereTargetFirmware_600:
        case AtmosphereTargetFirmware_620:
            g_firmware_version = FirmwareVersion_600;
            break;
        case AtmosphereTargetFirmware_700:
            g_firmware_version = FirmwareVersion_700;
            break;
        case AtmosphereTargetFirmware_800:
            g_firmware_version = FirmwareVersion_800;
            break;
        case AtmosphereTargetFirmware_810:
            g_firmware_version = FirmwareVersion_810;
            break;
        case AtmosphereTargetFirmware_900:
            g_firmware_version = FirmwareVersion_900;
            break;
        default:
            std::abort();
            break;
    }

    __atomic_store_n(&g_HasCached, true, __ATOMIC_SEQ_CST);

    mutexUnlock(&g_Mutex);
}

FirmwareVersion GetRuntimeFirmwareVersion() {
    _CacheValues();
    return g_firmware_version;
}

void SetFirmwareVersionForLibnx() {
    u32 major = 0, minor = 0, micro = 0;
    switch (GetRuntimeFirmwareVersion()) {
        case FirmwareVersion_100:
            major = 1;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_200:
            major = 2;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_300:
            major = 3;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_400:
            major = 4;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_500:
            major = 5;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_600:
            major = 6;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_700:
            major = 7;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_800:
            major = 8;
            minor = 0;
            micro = 0;
            break;
        case FirmwareVersion_810:
            major = 8;
            minor = 1;
            micro = 0;
            break;
        case FirmwareVersion_900:
            major = 9;
            minor = 0;
            micro = 0;
            break;
        default:
            std::abort();
            break;
    }
    hosversionSet(MAKEHOSVERSION(major, minor, micro));
}
