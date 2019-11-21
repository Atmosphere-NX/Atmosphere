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

#include "setsys_firmware_version.hpp"

static ams::os::Mutex g_version_mutex;
static bool g_got_version = false;
static SetSysFirmwareVersion g_ams_fw_version = {0};
static SetSysFirmwareVersion g_fw_version = {0};

void VersionManager::Initialize() {
    std::scoped_lock lock(g_version_mutex);

    if (g_got_version) {
        return;
    }

    /* Mount firmware version data archive. */
    R_ASSERT(romfsMountFromDataArchive(static_cast<u64>(ams::ncm::ProgramId::ArchiveSystemVersion), FsStorageId_NandSystem, "sysver"));
    {
        ON_SCOPE_EXIT { romfsUnmount("sysver"); };

        SetSysFirmwareVersion fw_ver;

        /* Firmware version file must exist. */
        FILE *f = fopen("sysver:/file", "rb");
        AMS_ASSERT(f != NULL);
        ON_SCOPE_EXIT { fclose(f); };

        /* Must be possible to read firmware version from file. */
        AMS_ASSERT(fread(&fw_ver, sizeof(fw_ver), 1, f) == 1);

        g_fw_version = fw_ver;
        g_ams_fw_version = fw_ver;
    }

    /* Modify the output firmware version. */
    {
        u32 major, minor, micro;
        GetAtmosphereApiVersion(&major, &minor, &micro, nullptr, nullptr);
        const char emummc_char = IsEmummc() ? 'E' : 'S';
        {
            char display_version[sizeof(g_ams_fw_version.display_version)] = {0};
            std::snprintf(display_version, sizeof(display_version), "%s|AMS %u.%u.%u|%c", g_ams_fw_version.display_version, major, minor, micro, emummc_char);
            std::memcpy(g_ams_fw_version.display_version, display_version, sizeof(g_ams_fw_version.display_version));
        }
    }

    g_got_version = true;
}

Result VersionManager::GetFirmwareVersion(ams::ncm::ProgramId program_id, SetSysFirmwareVersion *out) {
    VersionManager::Initialize();

    /* Report atmosphere string to qlaunch, maintenance and nothing else. */
    if (program_id == ams::ncm::ProgramId::AppletQlaunch || program_id == ams::ncm::ProgramId::AppletMaintenanceMenu) {
        *out = g_ams_fw_version;
    } else {
        *out = g_fw_version;
    }

    return ResultSuccess();
}