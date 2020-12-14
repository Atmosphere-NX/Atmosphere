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
#include <stratosphere.hpp>
#include "setsys_mitm_service.hpp"
#include "settings_sd_kvs.hpp"

namespace ams::mitm::settings {

    using namespace ams::settings;

    namespace {

        os::Mutex g_firmware_version_lock(false);
        bool g_cached_firmware_version;
        settings::FirmwareVersion g_firmware_version;
        settings::FirmwareVersion g_ams_firmware_version;

        void CacheFirmwareVersion() {
            std::scoped_lock lk(g_firmware_version_lock);

            if (AMS_LIKELY(g_cached_firmware_version)) {
                return;
            }

            /* Mount firmware version data archive. */
            {
                R_ABORT_UNLESS(ams::fs::MountSystemData("sysver", ncm::SystemDataId::SystemVersion));
                ON_SCOPE_EXIT { ams::fs::Unmount("sysver"); };

                /* Firmware version file must exist. */
                ams::fs::FileHandle file;
                R_ABORT_UNLESS(ams::fs::OpenFile(std::addressof(file), "sysver:/file", fs::OpenMode_Read));
                ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

                /* Must be possible to read firmware version from file. */
                R_ABORT_UNLESS(ams::fs::ReadFile(file, 0, std::addressof(g_firmware_version), sizeof(g_firmware_version)));

                g_ams_firmware_version = g_firmware_version;
            }

            /* Modify the atmosphere firmware version to display a custom version string. */
            {
                const auto api_info = exosphere::GetApiInfo();
                const char emummc_char = emummc::IsActive() ? 'E' : 'S';

                /* NOTE: While Mesosphere is in experimental/opt-in, we will display it as part of the firmware. */
                const char mesosphere_char = svc::IsKernelMesosphere() ? 'M' : '0';

                /* TODO: Remove separate display for mesosphere vs not mesosphere in Atmosphere 1.0.0. */
                AMS_ABORT_UNLESS(api_info.GetMajorVersion() == 0);

                /* NOTE: We have carefully accounted for the size of the string we print. */
                /* No truncation occurs assuming two-digits for all version number components. */
                char display_version[sizeof(g_ams_firmware_version.display_version)];

                std::snprintf(display_version, sizeof(display_version), "%s|AMS %c.%u.%u|%c", g_ams_firmware_version.display_version, mesosphere_char, api_info.GetMinorVersion(), api_info.GetMicroVersion(), emummc_char);

                std::memcpy(g_ams_firmware_version.display_version, display_version, sizeof(display_version));
            }

            g_cached_firmware_version = true;
        }

        Result GetFirmwareVersionImpl(settings::FirmwareVersion *out, const sm::MitmProcessInfo &client_info) {
            /* Ensure that we have the firmware version cached. */
            CacheFirmwareVersion();

            /* We want to give a special firmware version to the home menu title, and nothing else. */
            /* This means Qlaunch + Maintenance Menu, and nothing else. */
            if (client_info.program_id == ncm::SystemAppletId::Qlaunch || client_info.program_id == ncm::SystemAppletId::MaintenanceMenu) {
                *out = g_ams_firmware_version;
            } else {
                *out = g_firmware_version;
            }

            return ResultSuccess();
        }

    }

    Result SetSysMitmService::GetFirmwareVersion(sf::Out<settings::FirmwareVersion> out) {
        R_TRY(GetFirmwareVersionImpl(out.GetPointer(), this->client_info));

        /* GetFirmwareVersion sanitizes the revision fields. */
        out.GetPointer()->revision_major = 0;
        out.GetPointer()->revision_minor = 0;
        return ResultSuccess();
    }

    Result SetSysMitmService::GetFirmwareVersion2(sf::Out<settings::FirmwareVersion> out) {
        return GetFirmwareVersionImpl(out.GetPointer(), this->client_info);
    }

    Result SetSysMitmService::GetSettingsItemValueSize(sf::Out<u64> out_size, const settings::fwdbg::SettingsName &name, const settings::fwdbg::SettingsItemKey &key) {
        R_TRY_CATCH(settings::fwdbg::GetSdCardKeyValueStoreSettingsItemValueSize(out_size.GetPointer(), name.value, key.value)) {
            R_CATCH_RETHROW(sf::impl::ResultRequestContextChanged)
            R_CONVERT_ALL(sm::mitm::ResultShouldForwardToSession());
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result SetSysMitmService::GetSettingsItemValue(sf::Out<u64> out_size, const sf::OutBuffer &out, const settings::fwdbg::SettingsName &name, const settings::fwdbg::SettingsItemKey &key) {
        R_TRY_CATCH(settings::fwdbg::GetSdCardKeyValueStoreSettingsItemValue(out_size.GetPointer(), out.GetPointer(), out.GetSize(), name.value, key.value)) {
            R_CATCH_RETHROW(sf::impl::ResultRequestContextChanged)
            R_CONVERT_ALL(sm::mitm::ResultShouldForwardToSession());
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result SetSysMitmService::GetDebugModeFlag(sf::Out<bool> out) {
        /* If we're not processing for am, just return the real flag value. */
        R_UNLESS(this->client_info.program_id == ncm::SystemProgramId::Am, sm::mitm::ResultShouldForwardToSession());

        /* Retrieve the user configuration. */
        u8 en = 0;
        settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "enable_am_debug_mode");

        out.SetValue(en != 0);
        return ResultSuccess();
    }

}
