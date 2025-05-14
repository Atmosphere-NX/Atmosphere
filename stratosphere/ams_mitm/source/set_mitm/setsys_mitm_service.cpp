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
#include "setsys_mitm_service.hpp"
#include "settings_sd_kvs.hpp"
#include "setsys_shim.h"

namespace ams::mitm::settings {

    using namespace ams::settings;

    namespace {

        constexpr const char ExternalBluetoothDatabasePath[] = "@Sdcard:/atmosphere/bluetooth_devices.db";

        constinit os::SdkMutex g_firmware_version_lock;
        constinit bool g_cached_firmware_version;
        constinit settings::FirmwareVersion g_firmware_version;
        constinit settings::FirmwareVersion g_ams_firmware_version;

        void CacheFirmwareVersion() {
            if (AMS_LIKELY(g_cached_firmware_version)) {
                return;
            }

            std::scoped_lock lk(g_firmware_version_lock);

            if (AMS_UNLIKELY(g_cached_firmware_version)) {
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

                /* NOTE: We have carefully accounted for the size of the string we print. */
                /* No truncation occurs assuming two-digits for all version number components. */
                char display_version[sizeof(g_ams_firmware_version.display_version)];

                util::SNPrintf(display_version, sizeof(display_version), "%s|AMS %u.%u.%u|%c", g_ams_firmware_version.display_version, api_info.GetMajorVersion(), api_info.GetMinorVersion(), api_info.GetMicroVersion(), emummc_char);

                std::memcpy(g_ams_firmware_version.display_version, display_version, sizeof(display_version));
            }

            g_cached_firmware_version = true;
        }

        Result GetFirmwareVersionImpl(settings::FirmwareVersion *out, const sm::MitmProcessInfo &client_info) {
            /* Ensure that we have the firmware version cached. */
            CacheFirmwareVersion();

            /* We want to give a special firmware version to the home menu title, and nothing else. */
            /* This means qlaunch + maintenance menu, and nothing else. */
            if (client_info.program_id == ncm::SystemAppletId::Qlaunch || client_info.program_id == ncm::SystemAppletId::MaintenanceMenu) {
                *out = g_ams_firmware_version;
            } else {
                *out = g_firmware_version;
            }

            R_SUCCEED();
        }

        bool IsExternalBluetoothDatabaseEnabled() {
            u8 en = 0;
            settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "enable_external_bluetooth_db");
            return en;
        }

        bool HasExternalBluetoothDatabase() {
            bool file_exists;
            R_ABORT_UNLESS(fs::HasFile(std::addressof(file_exists), ExternalBluetoothDatabasePath));
            return file_exists;
        }

        Result ReadExternalBluetoothDatabase(s32 *entries_read, settings::BluetoothDevicesSettings *db, size_t db_max_size) {
            /* Open the external database file. */
            fs::FileHandle file;
            R_TRY(fs::OpenFile(std::addressof(file), ExternalBluetoothDatabasePath, fs::OpenMode_Read));
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Read the number of database entries stored in external database. */
            u64 total_entries;
            R_TRY(fs::ReadFile(file, 0, std::addressof(total_entries), sizeof(total_entries)));

            u64 db_offset = sizeof(total_entries);
            if (total_entries > db_max_size) {
                /* Pairings are stored from least to most recent. Add offset to skip the older entries that won't fit. */
                db_offset += (total_entries - db_max_size) * sizeof(settings::BluetoothDevicesSettings);

                /* Cap number of database entries read to size of database on this firmware. */
                total_entries = db_max_size;
            }

            /* Read database entries. */
            R_TRY(fs::ReadFile(file, db_offset, db, total_entries * sizeof(settings::BluetoothDevicesSettings)));

            /* Convert entries to the old format if running on a firmware below 13.0.0. */
            if (hos::GetVersion() < hos::Version_13_0_0) {
                for (size_t i = 0; i < total_entries; ++i) {
                    /* Copy the newer name field to the older one. */
                    util::TSNPrintf(db[i].name, sizeof(db[i].name), "%s", db[i].name2);

                    /* Clear the newer name field. */
                    std::memset(db[i].name2, 0, sizeof(db[i].name2));
                }
            }

            /* Set the output. */
            *entries_read = static_cast<s32>(total_entries);
            R_SUCCEED();
        }

        Result StoreExternalBluetoothDatabase(const settings::BluetoothDevicesSettings *db, u64 total_entries) {
            /* Open the external database file. */
            fs::FileHandle file;
            R_TRY(fs::OpenFile(std::addressof(file), ExternalBluetoothDatabasePath, fs::OpenMode_Write));
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Ensure the file is the appropriate size for the number of entries. */
            R_TRY(fs::SetFileSize(file, sizeof(total_entries) + total_entries * sizeof(settings::BluetoothDevicesSettings)));

            /* Write the number of database entries. */
            R_TRY(fs::WriteFile(file, 0, std::addressof(total_entries), sizeof(total_entries), fs::WriteOption::None));

            /* Write the database entries. */
            u64 db_offset = sizeof(total_entries);
            if (hos::GetVersion() < hos::Version_13_0_0) {
                /* Convert entries to the new format if running on a firmware below 13.0.0. */
                for (size_t i = 0; i < total_entries; ++i) {
                    /* Make a copy of the current database entry. */
                    settings::BluetoothDevicesSettings tmp = db[i];

                    /* Copy the older name field to the newer one. */
                    util::TSNPrintf(tmp.name2, sizeof(tmp.name2), "%s", tmp.name);

                    /* Clear the original name field. */
                    std::memset(tmp.name, 0, sizeof(tmp.name));

                    /* Write the converted database entry. */
                    R_TRY(fs::WriteFile(file, db_offset, std::addressof(tmp), sizeof(settings::BluetoothDevicesSettings), fs::WriteOption::None));

                    /* Advance to the next database entry. */
                    db_offset += sizeof(settings::BluetoothDevicesSettings);
                }

                /* Flush the data we've written. */
                R_TRY(fs::FlushFile(file));
            } else {
                /* We can just write the database to the file. */
                R_TRY(fs::WriteFile(file, db_offset, db, total_entries * sizeof(settings::BluetoothDevicesSettings), fs::WriteOption::Flush));
            }

            R_SUCCEED();
        }

    }

    Result SetSysMitmService::GetFirmwareVersion(sf::Out<settings::FirmwareVersion> out) {
        R_TRY(GetFirmwareVersionImpl(out.GetPointer(), m_client_info));

        /* GetFirmwareVersion sanitizes the revision fields. */
        out.GetPointer()->revision_major = 0;
        out.GetPointer()->revision_minor = 0;
        R_SUCCEED();
    }

    Result SetSysMitmService::GetFirmwareVersion2(sf::Out<settings::FirmwareVersion> out) {
        R_RETURN(GetFirmwareVersionImpl(out.GetPointer(), m_client_info));
    }

    Result SetSysMitmService::SetBluetoothDevicesSettings(const sf::InMapAliasArray<settings::BluetoothDevicesSettings> &settings) {
        /* We only want to perform additional logic when the external database setting is enabled. */
        R_UNLESS(IsExternalBluetoothDatabaseEnabled(), sm::mitm::ResultShouldForwardToSession());

        /* Create the external database if it doesn't exist. */
        if (!HasExternalBluetoothDatabase()) {
            R_TRY(fs::CreateFile(ExternalBluetoothDatabasePath, 0));
        }

        /* Backup the local database to the sd card. */
        R_TRY(StoreExternalBluetoothDatabase(settings.GetPointer(), settings.GetSize()));

        /* Ensure that the updated database is stored to the system save as usual. */
        static_assert(sizeof(settings::BluetoothDevicesSettings) == sizeof(::SetSysBluetoothDevicesSettings));
        R_TRY(setsysSetBluetoothDevicesSettingsFwd(m_forward_service.get(), reinterpret_cast<const ::SetSysBluetoothDevicesSettings *>(settings.GetPointer()), settings.GetSize()));

        R_SUCCEED();
    }

    Result SetSysMitmService::GetBluetoothDevicesSettings(sf::Out<s32> out_count, const sf::OutMapAliasArray<settings::BluetoothDevicesSettings> &out) {
        /* We only want to perform additional logic when the external database setting is enabled. */
        R_UNLESS(IsExternalBluetoothDatabaseEnabled(), sm::mitm::ResultShouldForwardToSession());

        if (!HasExternalBluetoothDatabase()) {
            /* Fetch the local database from the system save. */
            static_assert(sizeof(settings::BluetoothDevicesSettings) == sizeof(::SetSysBluetoothDevicesSettings));
            R_TRY(setsysGetBluetoothDevicesSettingsFwd(m_forward_service.get(), out_count.GetPointer(), reinterpret_cast<::SetSysBluetoothDevicesSettings *>(out.GetPointer()), out.GetSize()));

            /* Create the external database file. */
            R_TRY(fs::CreateFile(ExternalBluetoothDatabasePath, 0));

            /* Backup the local database to the sd card. */
            R_TRY(StoreExternalBluetoothDatabase(out.GetPointer(), out_count.GetValue()));
        } else {
            /* Read the external database from the sd card. */
            R_TRY(ReadExternalBluetoothDatabase(out_count.GetPointer(), out.GetPointer(), out.GetSize()));
        }

        R_SUCCEED();
    }

    Result SetSysMitmService::GetSettingsItemValueSize(sf::Out<u64> out_size, const settings::SettingsName &name, const settings::SettingsItemKey &key) {
        R_TRY_CATCH(settings::fwdbg::GetSdCardKeyValueStoreSettingsItemValueSize(out_size.GetPointer(), name.value, key.value)) {
            R_CATCH_RETHROW(sf::impl::ResultRequestContextChanged)
            R_CONVERT_ALL(sm::mitm::ResultShouldForwardToSession());
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result SetSysMitmService::GetSettingsItemValue(sf::Out<u64> out_size, const sf::OutBuffer &out, const settings::SettingsName &name, const settings::SettingsItemKey &key) {
        R_TRY_CATCH(settings::fwdbg::GetSdCardKeyValueStoreSettingsItemValue(out_size.GetPointer(), out.GetPointer(), out.GetSize(), name.value, key.value)) {
            R_CATCH_RETHROW(sf::impl::ResultRequestContextChanged)
            R_CONVERT_ALL(sm::mitm::ResultShouldForwardToSession());
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result SetSysMitmService::GetDebugModeFlag(sf::Out<bool> out) {
        /* If we're not processing for am, just return the real flag value. */
        R_UNLESS(m_client_info.program_id == ncm::SystemProgramId::Am, sm::mitm::ResultShouldForwardToSession());

        /* Retrieve the user configuration. */
        u8 en = 0;
        settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "enable_am_debug_mode");

        out.SetValue(en != 0);
        R_SUCCEED();
    }

}
