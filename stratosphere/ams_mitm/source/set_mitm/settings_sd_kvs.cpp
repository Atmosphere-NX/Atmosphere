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
#include "../amsmitm_fs_utils.hpp"
#include "settings_sd_kvs.hpp"

namespace ams::settings::fwdbg {

    namespace {

        struct SdKeyValueStoreEntry {
            const char *name;
            const char *key;
            void *value;
            size_t value_size;

            constexpr inline bool HasValue() const { return this->value != nullptr; }

            constexpr inline void GetNameAndKey(char *dst) const {
                size_t offset = 0;
                for (size_t i = 0; i < std::strlen(this->name); i++) {
                    dst[offset++] = this->name[i];
                }
                dst[offset++] = '!';
                for (size_t i = 0; i < std::strlen(this->key); i++) {
                    dst[offset++] = this->key[i];
                }
                dst[offset] = 0;
            }
        };

        static_assert(util::is_pod<SdKeyValueStoreEntry>::value);

        constexpr inline bool operator==(const SdKeyValueStoreEntry &lhs, const SdKeyValueStoreEntry &rhs) {
            if (lhs.HasValue() != rhs.HasValue()) {
                return false;
            }
            return lhs.HasValue() && std::strcmp(lhs.name, rhs.name) == 0 && std::strcmp(lhs.key, rhs.key) == 0;
        }

        inline bool operator<(const SdKeyValueStoreEntry &lhs, const SdKeyValueStoreEntry &rhs) {
            AMS_ABORT_UNLESS(lhs.HasValue());
            AMS_ABORT_UNLESS(rhs.HasValue());

            char lhs_name_key[SettingsNameLengthMax + 1 + SettingsItemKeyLengthMax + 1];
            char rhs_name_key[SettingsNameLengthMax + 1 + SettingsItemKeyLengthMax + 1];

            lhs.GetNameAndKey(lhs_name_key);
            rhs.GetNameAndKey(rhs_name_key);

            return std::strcmp(lhs_name_key, rhs_name_key) < 0;
        }

        constexpr size_t MaxEntries = 0x200;
        constexpr size_t SettingsItemValueStorageSize = 0x10000;

        SettingsName    g_names[MaxEntries];
        SettingsItemKey g_item_keys[MaxEntries];
        u8 g_value_storage[SettingsItemValueStorageSize];
        size_t g_allocated_value_storage_size;

        SdKeyValueStoreEntry g_entries[MaxEntries];
        size_t g_num_entries;

        constexpr bool IsValidSettingsFormat(const char *str, size_t len) {
            AMS_ABORT_UNLESS(str != nullptr);

            if (len > 0 && str[len - 1] == '.') {
                return false;
            }

            for (size_t i = 0; i < len; i++) {
                const char c = str[i];

                if ('a' <= c && c <= 'z') {
                    continue;
                }

                if ('0' <= c && c <= '9') {
                    continue;
                }

                if (c == '-' || c == '.' || c == '_') {
                    continue;
                }

                return false;
            }

            return true;
        }

        constexpr bool IsHexadecimal(const char *str) {
            while (*str) {
                if (std::isxdigit(static_cast<unsigned char>(*str))) {
                    str++;
                } else {
                    return false;
                }
            }
            return true;
        }

        constexpr inline char hextoi(char c) {
            if ('a' <= c && c <= 'f') return c - 'a' + 0xA;
            if ('A' <= c && c <= 'F') return c - 'A' + 0xA;
            if ('0' <= c && c <= '9') return c - '0';
            return 0;
        }

        Result ValidateSettingsName(const char *name) {
            R_UNLESS(name != nullptr, settings::ResultNullSettingsName());
            const size_t len = strnlen(name, SettingsNameLengthMax + 1);
            R_UNLESS(len > 0,                          settings::ResultEmptySettingsName());
            R_UNLESS(len <= SettingsNameLengthMax,     settings::ResultTooLongSettingsName());
            R_UNLESS(IsValidSettingsFormat(name, len), settings::ResultInvalidFormatSettingsName());
            R_SUCCEED();
        }

        Result ValidateSettingsItemKey(const char *key) {
            R_UNLESS(key != nullptr, settings::ResultNullSettingsName());
            const size_t len = strnlen(key, SettingsItemKeyLengthMax + 1);
            R_UNLESS(len > 0,                         settings::ResultEmptySettingsItemKey());
            R_UNLESS(len <= SettingsNameLengthMax,    settings::ResultTooLongSettingsItemKey());
            R_UNLESS(IsValidSettingsFormat(key, len), settings::ResultInvalidFormatSettingsItemKey());
            R_SUCCEED();
        }

        Result AllocateValue(void **out, size_t size) {
            R_UNLESS(g_allocated_value_storage_size + size <= sizeof(g_value_storage), settings::ResultSettingsItemValueAllocationFailed());

            *out = g_value_storage + g_allocated_value_storage_size;
            g_allocated_value_storage_size += size;
            R_SUCCEED();
        }

        Result FindSettingsName(const char **out, const char *name) {
            for (auto &stored : g_names) {
                if (std::strcmp(stored.value, name) == 0) {
                    *out = stored.value;
                    R_SUCCEED();
                } else if (std::strcmp(stored.value, "") == 0) {
                    *out = stored.value;
                    std::strcpy(stored.value, name);
                    R_SUCCEED();
                }
            }
            R_THROW(settings::ResultSettingsItemKeyAllocationFailed());
        }

        Result FindSettingsItemKey(const char **out, const char *key) {
            for (auto &stored : g_item_keys) {
                if (std::strcmp(stored.value, key) == 0) {
                    *out = stored.value;
                    R_SUCCEED();
                } else if (std::strcmp(stored.value, "") == 0) {
                    std::strcpy(stored.value, key);
                    *out = stored.value;
                    R_SUCCEED();
                }
            }
            R_THROW(settings::ResultSettingsItemKeyAllocationFailed());
        }

        template<typename T>
        Result ParseSettingsItemIntegralValue(SdKeyValueStoreEntry &out, const char *value_str) {
            R_TRY(AllocateValue(std::addressof(out.value), sizeof(T)));
            out.value_size = sizeof(T);

            T value = static_cast<T>(strtoul(value_str, nullptr, 0));
            std::memcpy(out.value, std::addressof(value), sizeof(T));
            R_SUCCEED();
        }

        Result GetEntry(SdKeyValueStoreEntry **out, const char *name, const char *key) {
            /* Validate name/key. */
            R_TRY(ValidateSettingsName(name));
            R_TRY(ValidateSettingsItemKey(key));

            u8 dummy_value = 0;
            SdKeyValueStoreEntry test_entry { .name = name, .key = key, .value = std::addressof(dummy_value), .value_size = sizeof(dummy_value) };

            auto *begin = g_entries;
            auto *end   = begin + g_num_entries;
            auto it = std::lower_bound(begin, end, test_entry);
            R_UNLESS(it != end,         settings::ResultSettingsItemNotFound());
            R_UNLESS(*it == test_entry, settings::ResultSettingsItemNotFound());

            *out = std::addressof(*it);
            R_SUCCEED();
        }

        Result ParseSettingsItemValueImpl(const char *name, const char *key, const char *val_tup) {
            const char *delimiter = strchr(val_tup, '!');
            const char *value_str = delimiter + 1;
            const char *type = val_tup;

            R_UNLESS(delimiter != nullptr, settings::ResultInvalidFormatSettingsItemValue());

            while (std::isspace(static_cast<unsigned char>(*type)) && type != delimiter) {
                type++;
            }

            const size_t type_len = delimiter - type;
            const size_t value_len = strlen(value_str);
            R_UNLESS(type_len > 0,  settings::ResultInvalidFormatSettingsItemValue());
            R_UNLESS(value_len > 0, settings::ResultInvalidFormatSettingsItemValue());

            /* Create new value. */
            SdKeyValueStoreEntry new_value = {};

            /* Find name and key. */
            R_TRY(FindSettingsName(std::addressof(new_value.name), name));
            R_TRY(FindSettingsItemKey(std::addressof(new_value.key), key));

            if (strncasecmp(type, "str", type_len) == 0 || strncasecmp(type, "string", type_len) == 0) {
                const size_t size = value_len + 1;
                R_TRY(AllocateValue(std::addressof(new_value.value), size));
                std::memcpy(new_value.value, value_str, size);
                new_value.value_size = size;
            } else if (strncasecmp(type, "hex", type_len) == 0 || strncasecmp(type, "bytes", type_len) == 0) {
                R_UNLESS(value_len > 0,            settings::ResultInvalidFormatSettingsItemValue());
                R_UNLESS(value_len % 2 == 0,       settings::ResultInvalidFormatSettingsItemValue());
                R_UNLESS(IsHexadecimal(value_str), settings::ResultInvalidFormatSettingsItemValue());

                const size_t size = value_len / 2;
                R_TRY(AllocateValue(std::addressof(new_value.value), size));
                new_value.value_size = size;

                u8 *data = reinterpret_cast<u8 *>(new_value.value);
                for (size_t i = 0; i < size; i++) {
                    data[i >> 1] = hextoi(value_str[i]) << (4 * (i & 1));
                }
            } else if (strncasecmp(type, "u8", type_len) == 0) {
                R_TRY((ParseSettingsItemIntegralValue<u8>(new_value, value_str)));
            } else if (strncasecmp(type, "u16", type_len) == 0) {
                R_TRY((ParseSettingsItemIntegralValue<u16>(new_value, value_str)));
            } else if (strncasecmp(type, "u32", type_len) == 0) {
                R_TRY((ParseSettingsItemIntegralValue<u32>(new_value, value_str)));
            } else if (strncasecmp(type, "u64", type_len) == 0) {
                R_TRY((ParseSettingsItemIntegralValue<u64>(new_value, value_str)));
            } else {
                R_THROW(settings::ResultInvalidFormatSettingsItemValue());
            }

            /* Insert the entry. */
            bool inserted = false;
            for (auto &entry : g_entries) {
                if (!entry.HasValue() || entry == new_value) {
                    entry = new_value;
                    inserted = true;
                    break;
                }
            }

            R_UNLESS(inserted, settings::ResultSettingsItemValueAllocationFailed());
            R_SUCCEED();
        }

        Result ParseSettingsItemValue(const char *name, const char *key, const char *value) {
            R_TRY(ValidateSettingsName(name));
            R_TRY(ValidateSettingsItemKey(key));
            R_RETURN(ParseSettingsItemValueImpl(name, key, value));
        }

        static int SystemSettingsIniHandler(void *user, const char *name, const char *key, const char *value) {
            Result *parse_res = reinterpret_cast<Result *>(user);

            /* Once we fail to parse a value, don't parse further. */
            if (R_FAILED(*parse_res)) {
                return 0;
            }

            *parse_res = ParseSettingsItemValue(name, key, value);
            return R_SUCCEEDED(*parse_res) ? 1 : 0;
        }

        Result LoadSdCardKeyValueStore() {
            /* Open file. */
            /* It's okay if the file isn't readable/present, because we already loaded defaults. */
            std::unique_ptr<ams::fs::fsa::IFile> file;
            {
                FsFile f;
                R_SUCCEED_IF(R_FAILED(ams::mitm::fs::OpenAtmosphereSdFile(std::addressof(f), "/config/system_settings.ini", fs::OpenMode_Read)));
                file = std::make_unique<ams::fs::RemoteFile>(f);
            }
            AMS_ABORT_UNLESS(file != nullptr);

            Result parse_result = ResultSuccess();
            util::ini::ParseFile(file.get(), std::addressof(parse_result), SystemSettingsIniHandler);
            R_TRY(parse_result);

            R_SUCCEED();
        }

        void LoadDefaultCustomSettings() {
            /* Disable uploading error reports to Nintendo. */
            R_ABORT_UNLESS(ParseSettingsItemValue("eupld", "upload_enabled", "u8!0x0"));

            /* Enable USB 3.0 superspeed for homebrew */
            R_ABORT_UNLESS(ParseSettingsItemValue("usb", "usb30_force_enabled", spl::IsUsb30ForceEnabled() ? "u8!0x1" : "u8!0x0"));

            /* Control whether RO should ease its validation of NROs. */
            /* (note: this is normally not necessary, and ips patches can be used.) */
            R_ABORT_UNLESS(ParseSettingsItemValue("ro", "ease_nro_restriction", "u8!0x1"));

            /* Control whether lm should log to the SD card. */
            /* Note that this setting does nothing when log manager is not enabled. */
            R_ABORT_UNLESS(ParseSettingsItemValue("lm", "enable_sd_card_logging", "u8!0x1"));

            /* Control the output directory for SD card logs. */
            /* Note that this setting does nothing when log manager is not enabled/sd card logging is not enabled. */
            R_ABORT_UNLESS(ParseSettingsItemValue("lm", "sd_card_log_output_directory", "str!atmosphere/binlogs"));

            /* Control whether erpt reports should always be preserved, instead of automatically cleaning periodically. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("erpt", "disable_automatic_report_cleanup", "u8!0x0"));

            /* Atmosphere custom settings. */

            /* Reboot from fatal automatically after some number of milliseconds. */
            /* If field is not present or 0, fatal will wait indefinitely for user input. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "fatal_auto_reboot_interval", "u64!0x0"));

            /* Make the power menu's "reboot" button reboot to payload. */
            /* Set to "normal" for normal reboot, "rcm" for rcm reboot. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "power_menu_reboot_function", "str!payload"));

            /* Enable writing to BIS partitions for HBL. */
            /* This is probably undesirable for normal usage. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_hbl_bis_write", "u8!0x0"));

            /* Controls whether dmnt cheats should be toggled on or off by */
            /* default. 1 = toggled on by default, 0 = toggled off by default. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "dmnt_cheats_enabled_by_default", "u8!0x1"));

            /* Controls whether dmnt should always save cheat toggle state */
            /* for restoration on new game launch. 1 = always save toggles, */
            /* 0 = only save toggles if toggle file exists. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "dmnt_always_save_cheat_toggles", "u8!0x0"));

            /* Controls whether fs.mitm should redirect save files */
            /* to directories on the sd card. */
            /* 0 = Do not redirect, 1 = Redirect. */
            /* NOTE: EXPERIMENTAL */
            /* If you do not know what you are doing, do not touch this yet. */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "fsmitm_redirect_saves_to_sd", "u8!0x0"));

            /* Controls whether am sees system settings "DebugModeFlag" as */
            /* enabled or disabled. */
            /* 0 = Disabled (not debug mode), 1 = Enabled (debug mode) */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_am_debug_mode", "u8!0x0"));

            /* Controls whether dns.mitm is enabled. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_dns_mitm", "u8!0x1"));

            /* Controls whether dns.mitm uses the default redirections in addition to */
            /* whatever is specified in the user's hosts file. */
            /* 0 = Disabled (use hosts file contents), 1 = Enabled (use defaults and hosts file contents) */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "add_defaults_to_dns_hosts", "u8!0x1"));

            /* Controls whether dns.mitm logs to the sd card for debugging. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_dns_mitm_debug_log", "u8!0x0"));

            /* Controls whether htc is enabled. */
            /* TODO: Change this to default 1 when tma2 is ready for inclusion in atmosphere releases. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_htc", "u8!0x0"));

            /* Controls whether atmosphere's dmnt.gen2 gdbstub should run as a standalone via sockets. */
            /* Note that this setting is ignored (and treated as 0) when htc is enabled. */
            /* Note that this setting may disappear in the future. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_standalone_gdbstub", "u8!0x0"));

            /* Controls whether atmosphere's log manager is enabled. */
            /* Note that this setting is ignored (and treated as 1) when htc is enabled. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_log_manager", "u8!0x0"));

            /* Controls whether the bluetooth pairing database is redirected to the SD card (shared across sysmmc/all emummcs) */
            /* NOTE: On <13.0.0, the database size was 10 instead of 20; booting pre-13.0.0 will truncate the database. */
            /* 0 = Disabled, 1 = Enabled */
            R_ABORT_UNLESS(ParseSettingsItemValue("atmosphere", "enable_external_bluetooth_db", "u8!0x0"));

            /* Hbloader custom settings. */

            /* Controls the size of the homebrew heap when running as applet. */
            /* If set to zero, all available applet memory is used as heap. */
            /* The default is zero. */
            R_ABORT_UNLESS(ParseSettingsItemValue("hbloader", "applet_heap_size", "u64!0x0"));

            /* Controls the amount of memory to reserve when running as applet */
            /* for usage by other applets. This setting has no effect if */
            /* applet_heap_size is non-zero. The default is 0x8600000. */
            R_ABORT_UNLESS(ParseSettingsItemValue("hbloader", "applet_heap_reservation_size", "u64!0x8600000"));
        }

    }

    void InitializeSdCardKeyValueStore() {
        /* Load in hardcoded defaults. */
        /* These will be overwritten if present on the SD card. */
        LoadDefaultCustomSettings();

        /* Parse custom settings off the SD card. */
        R_ABORT_UNLESS(LoadSdCardKeyValueStore());

        /* Determine how many custom settings are present. */
        for (size_t i = 0; i < util::size(g_entries); i++) {
            if (!g_entries[i].HasValue()) {
                g_num_entries = i;
                break;
            }
        }

        /* Ensure that the custom settings entries are sorted. */
        if (g_num_entries) {
            std::sort(g_entries, g_entries + g_num_entries);
        }
    }

    Result GetSdCardKeyValueStoreSettingsItemValueSize(size_t *out_size, const char *name, const char *key) {
        SdKeyValueStoreEntry *entry = nullptr;
        R_TRY(GetEntry(std::addressof(entry), name, key));

        *out_size = entry->value_size;
        R_SUCCEED();
    }

    Result GetSdCardKeyValueStoreSettingsItemValue(size_t *out_size, void *dst, size_t dst_size, const char *name, const char *key) {
        R_UNLESS(dst != nullptr, settings::ResultNullSettingsItemValueBuffer());

        SdKeyValueStoreEntry *entry = nullptr;
        R_TRY(GetEntry(std::addressof(entry), name, key));

        const size_t size = std::min(entry->value_size, dst_size);
        if (size > 0) {
            std::memcpy(dst, entry->value, size);
        }
        *out_size = size;
        R_SUCCEED();
    }

}
