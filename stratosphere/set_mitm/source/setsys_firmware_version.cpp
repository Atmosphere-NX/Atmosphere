/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "setsys_firmware_version.hpp"

static HosMutex g_version_mutex;
static bool g_got_version = false;
static SetSysFirmwareVersion g_fw_version = {0};

Result VersionManager::GetFirmwareVersion(u64 title_id, SetSysFirmwareVersion *out) {
    std::scoped_lock<HosMutex> lock(g_version_mutex);
    if (!g_got_version) {
        Result rc = setsysGetFirmwareVersion(&g_fw_version);
        if (R_FAILED(rc)) {
            return rc;
        }
        
        /* Modify the output firmware version. */
        {
            u32 major, minor, micro;
            char display_version[sizeof(g_fw_version.display_version)] = {0};
            
            GetAtmosphereApiVersion(&major, &minor, &micro, nullptr, nullptr);
            snprintf(display_version, sizeof(display_version), "%s (AMS %u.%u.%u)", g_fw_version.display_version, major, minor, micro);
            
            memcpy(g_fw_version.display_version, display_version, sizeof(g_fw_version.display_version));
        }
        
        g_got_version = true;
    }
    
    /* Report atmosphere string to qlaunch, maintenance and nothing else. */
    if (title_id == 0x0100000000001000ULL || title_id == 0x0100000000001015ULL) {
        *out = g_fw_version;
        return 0;
    } else {
        return setsysGetFirmwareVersion(out);
    }
}