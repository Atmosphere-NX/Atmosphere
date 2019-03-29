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
static SetSysFirmwareVersion g_ams_fw_version = {0};
static SetSysFirmwareVersion g_fw_version = {0};

void VersionManager::Initialize() {
    std::scoped_lock<HosMutex> lock(g_version_mutex);

    if (g_got_version) {
        return;
    }

    /* Mount firmware version data archive. */
    if (R_SUCCEEDED(romfsMountFromDataArchive(TitleId_ArchiveSystemVersion, FsStorageId_NandSystem, "sysver"))) {
        ON_SCOPE_EXIT { romfsUnmount("sysver"); };

        SetSysFirmwareVersion fw_ver;

        /* Firmware version file must exist. */
        FILE *f = fopen("sysver:/file", "rb");
        if (f == NULL) {
            std::abort();
        }

        /* Must be possible to read firmware version from file. */
        if (fread(&fw_ver, sizeof(fw_ver), 1, f) != 1) {
            std::abort();
        }

        fclose(f);

        g_fw_version = fw_ver;
        g_ams_fw_version = fw_ver;
    } else {
        /* Failure to open data archive is an abort. */
        std::abort();
    }

    /* Modify the output firmware version. */
    {
        u32 major, minor, micro;
        char display_version[sizeof(g_ams_fw_version.display_version)] = {0};
        
        GetAtmosphereApiVersion(&major, &minor, &micro, nullptr, nullptr);
        snprintf(display_version, sizeof(display_version), "%s (AMS %u.%u.%u)", g_ams_fw_version.display_version, major, minor, micro);

        memcpy(g_ams_fw_version.display_version, display_version, sizeof(g_ams_fw_version.display_version));
    }

    g_got_version = true;
}

Result VersionManager::GetFirmwareVersion(u64 title_id, SetSysFirmwareVersion *out) {
    VersionManager::Initialize();
    
    /* Report atmosphere string to qlaunch, maintenance and nothing else. */
    if (title_id == TitleId_AppletQlaunch || title_id == TitleId_AppletMaintenanceMenu) {
        *out = g_ams_fw_version;
    } else {
        *out = g_fw_version;
    }

    return ResultSuccess;
}