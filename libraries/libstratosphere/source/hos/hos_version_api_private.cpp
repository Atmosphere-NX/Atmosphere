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
#include <stratosphere.hpp>

namespace ams::hos {

    namespace {

        settings::FirmwareVersion GetSettingsFirmwareVersion() {
            /* Mount the system version title. */
            R_ABORT_UNLESS(ams::fs::MountSystemData("sysver", ncm::SystemDataId::SystemVersion));
            ON_SCOPE_EXIT { ams::fs::Unmount("sysver"); };

            /* Read the firmware version file. */
            ams::fs::FileHandle file;
            R_ABORT_UNLESS(ams::fs::OpenFile(std::addressof(file), "sysver:/file", fs::OpenMode_Read));
            ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

            /* Must be possible to read firmware version from file. */
            settings::FirmwareVersion firmware_version;
            R_ABORT_UNLESS(ams::fs::ReadFile(file, 0, std::addressof(firmware_version), sizeof(firmware_version)));

            return firmware_version;
        }

    }

    void InitializeVersionInternal(bool allow_approximate);

    void SetNonApproximateVersionInternal() {
        /* Get the settings . */
        const auto firmware_version = GetSettingsFirmwareVersion();

        /* Set the exosphere api version. */
        R_ABORT_UNLESS(spl::SetConfig(spl::ConfigItem::ExosphereApiVersion, (static_cast<u32>(firmware_version.major) << 24) | (static_cast<u32>(firmware_version.minor) << 16) | (static_cast<u32>(firmware_version.micro) <<  8)));

        /* Update our own version value. */
        InitializeVersionInternal(false);
    }

}
