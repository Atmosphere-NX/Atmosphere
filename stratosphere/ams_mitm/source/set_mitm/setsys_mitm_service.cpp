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

        int g_kfr_firmware_version = 0;

        void CacheFirmwareVersion() {
            std::scoped_lock lk(g_firmware_version_lock);

            if (AMS_LIKELY(g_cached_firmware_version)) {
                return;
            }

            {
                /* Mount the SD card. */
                if (R_SUCCEEDED(fs::MountSdCard("sdmc"))) {
                    ams::fs::FileHandle file;
                    if (R_SUCCEEDED(fs::OpenFile(std::addressof(file), "sdmc:/switch/kefirupdater/version", fs::OpenMode_Read))) {
                        ON_SCOPE_EXIT { ams::fs::CloseFile(file); };

                        /* Get file size. */
                        s64 file_size;
                        R_ABORT_UNLESS(fs::GetFileSize(std::addressof(file_size), file));

                        /* Allocate cheat txt buffer. */
                        char *kef_txt = static_cast<char *>(std::malloc(file_size + 1));
                        ON_SCOPE_EXIT { std::free(kef_txt); };

                        /* Read cheats into buffer. */
                        R_ABORT_UNLESS(fs::ReadFile(file, 0, kef_txt, file_size));
                        kef_txt[file_size] = '\x00';

                        g_kfr_firmware_version = strtol(kef_txt, NULL, 10);
                    }
                }
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

                /* GCC complains about the following snprintf possibly truncating, but this is not a problem and has been carefully accounted for. */
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wformat-truncation"
                {
                    char display_version[sizeof(g_ams_firmware_version.display_version)];
                    if ( g_kfr_firmware_version != 0 )
                        std::snprintf(display_version, sizeof(display_version), "%s|KEF%d-%u.%u.%u|%c", g_ams_firmware_version.display_version, g_kfr_firmware_version, api_info.GetMajorVersion(), api_info.GetMinorVersion(), api_info.GetMicroVersion(), emummc_char);
                    else
                        std::snprintf(display_version, sizeof(display_version), "%s|KEF-%u.%u.%u|%c", g_ams_firmware_version.display_version, api_info.GetMajorVersion(), api_info.GetMinorVersion(), api_info.GetMicroVersion(), emummc_char);
                    
                    std::memcpy(g_ams_firmware_version.display_version, display_version, sizeof(display_version));
                }
                #pragma GCC diagnostic pop
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

}
