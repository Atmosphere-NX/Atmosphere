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
#include "../amsmitm_debug.hpp"
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

        static_assert(std::is_pod<SdKeyValueStoreEntry>::value);

        constexpr inline bool operator==(const SdKeyValueStoreEntry &lhs, const SdKeyValueStoreEntry &rhs) {
            if (lhs.HasValue() != rhs.HasValue()) {
                return false;
            }
            return lhs.HasValue() && std::strcmp(lhs.name, rhs.name) == 0 && std::strcmp(lhs.key, rhs.key) == 0;
        }

        inline bool operator<(const SdKeyValueStoreEntry &lhs, const SdKeyValueStoreEntry &rhs) {
            AMS_ASSERT(lhs.HasValue());
            AMS_ASSERT(rhs.HasValue());

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
            AMS_ASSERT(str != nullptr);

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
            R_UNLESS(name != nullptr, ResultSettingsNameNull());
            const size_t len = strnlen(name, SettingsNameLengthMax + 1);
            R_UNLESS(len > 0,                          ResultSettingsNameEmpty());
            R_UNLESS(len <= SettingsNameLengthMax,     ResultSettingsNameTooLong());
            R_UNLESS(IsValidSettingsFormat(name, len), ResultSettingsNameInvalidFormat());
            return ResultSuccess();
        }

        Result ValidateSettingsItemKey(const char *key) {
            R_UNLESS(key != nullptr, ResultSettingsNameNull());
            const size_t len = strnlen(key, SettingsItemKeyLengthMax + 1);
            R_UNLESS(len > 0,                         ResultSettingsItemKeyEmpty());
            R_UNLESS(len <= SettingsNameLengthMax,    ResultSettingsItemKeyTooLong());
            R_UNLESS(IsValidSettingsFormat(key, len), ResultSettingsItemKeyInvalidFormat());
            return ResultSuccess();
        }

        Result AllocateValue(void **out, size_t size) {
            R_UNLESS(g_allocated_value_storage_size + size <= sizeof(g_value_storage), ResultSettingsItemValueAllocationFailed());

            *out = g_value_storage + g_allocated_value_storage_size;
            g_allocated_value_storage_size += size;
            return ResultSuccess();
        }

        Result FindSettingsName(const char **out, const char *name) {
            for (auto &stored : g_names) {
                if (std::strcmp(stored.value, name) == 0) {
                    *out = stored.value;
                    return ResultSuccess();
                } else if (std::strcmp(stored.value, "") == 0) {
                    *out = stored.value;
                    std::strcpy(stored.value, name);
                    return ResultSuccess();
                }
            }
            return ResultSettingsItemKeyAllocationFailed();
        }

        Result FindSettingsItemKey(const char **out, const char *key) {
            for (auto &stored : g_item_keys) {
                if (std::strcmp(stored.value, key) == 0) {
                    *out = stored.value;
                    return ResultSuccess();
                } else if (std::strcmp(stored.value, "") == 0) {
                    std::strcpy(stored.value, key);
                    *out = stored.value;
                    return ResultSuccess();
                }
            }
            return ResultSettingsItemKeyAllocationFailed();
        }

        template<typename T>
        Result ParseSettingsItemIntegralValue(SdKeyValueStoreEntry &out, const char *value_str) {
            R_TRY(AllocateValue(&out.value, sizeof(T)));
            out.value_size = sizeof(T);

            T value = static_cast<T>(strtoul(value_str, nullptr, 0));
            std::memcpy(out.value, &value, sizeof(T));
            return ResultSuccess();
        }

        Result GetEntry(SdKeyValueStoreEntry **out, const char *name, const char *key) {
            /* Validate name/key. */
            R_TRY(ValidateSettingsName(name));
            R_TRY(ValidateSettingsItemKey(key));

            u8 dummy_value = 0;
            SdKeyValueStoreEntry test_entry { .name = name, .key = key, .value = &dummy_value, .value_size = sizeof(dummy_value) };

            auto *begin = g_entries;
            auto *end   = begin + g_num_entries;
            auto it = std::lower_bound(begin, end, test_entry);
            R_UNLESS(it != end,         ResultSettingsItemNotFound());
            R_UNLESS(*it == test_entry, ResultSettingsItemNotFound());

            *out = &*it;
            return ResultSuccess();
        }

        Result ParseSettingsItemValueImpl(const char *name, const char *key, const char *val_tup) {
            const char *delimiter = strchr(val_tup, '!');
            const char *value_str = delimiter + 1;
            const char *type = val_tup;

            R_UNLESS(delimiter != nullptr, ResultSettingsItemValueInvalidFormat());

            while (std::isspace(static_cast<unsigned char>(*type)) && type != delimiter) {
                type++;
            }

            const size_t type_len = delimiter - type;
            const size_t value_len = strlen(value_str);
            R_UNLESS(type_len > 0,  ResultSettingsItemValueInvalidFormat());
            R_UNLESS(value_len > 0, ResultSettingsItemValueInvalidFormat());

            /* Create new value. */
            SdKeyValueStoreEntry new_value = {};

            /* Find name and key. */
            R_TRY(FindSettingsName(&new_value.name, name));
            R_TRY(FindSettingsItemKey(&new_value.key, key));

            if (strncasecmp(type, "str", type_len) == 0 || strncasecmp(type, "string", type_len) == 0) {
                const size_t size = value_len + 1;
                R_TRY(AllocateValue(&new_value.value, size));
                std::memcpy(new_value.value, value_str, size);
                new_value.value_size = size;
            } else if (strncasecmp(type, "hex", type_len) == 0 || strncasecmp(type, "bytes", type_len) == 0) {
                R_UNLESS(value_len > 0,            ResultSettingsItemValueInvalidFormat());
                R_UNLESS(value_len % 2 == 0,       ResultSettingsItemValueInvalidFormat());
                R_UNLESS(IsHexadecimal(value_str), ResultSettingsItemValueInvalidFormat());

                const size_t size = value_len / 2;
                R_TRY(AllocateValue(&new_value.value, size));
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
                return ResultSettingsItemValueInvalidFormat();
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

            R_UNLESS(inserted, ResultSettingsItemValueAllocationFailed());
            return ResultSuccess();
        }

        Result ParseSettingsItemValue(const char *name, const char *key, const char *value) {
            R_TRY(ValidateSettingsName(name));
            R_TRY(ValidateSettingsItemKey(key));
            return ParseSettingsItemValueImpl(name, key, value);
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
            FsFile config_file;
            R_TRY(ams::mitm::fs::OpenAtmosphereSdFile(&config_file, "/system_settings.ini", FsOpenMode_Read));
            ON_SCOPE_EXIT { fsFileClose(&config_file); };

            Result parse_result = ResultSuccess();
            util::ini::ParseFile(&config_file, &parse_result, SystemSettingsIniHandler);
            R_TRY(parse_result);

            for (size_t i = 0; i < util::size(g_entries); i++) {
                if (!g_entries[i].HasValue()) {
                    g_num_entries = i;
                    break;
                }
            }

            if (g_num_entries) {
                std::sort(g_entries, g_entries + g_num_entries);
            }

            return ResultSuccess();
        }

    }

    void InitializeSdCardKeyValueStore() {
        const Result parse_result = LoadSdCardKeyValueStore();
        if (R_FAILED(parse_result)) {
            ams::mitm::ThrowResultForDebug(parse_result);
        }
    }

    Result GetSdCardKeyValueStoreSettingsItemValueSize(size_t *out_size, const char *name, const char *key) {
        SdKeyValueStoreEntry *entry = nullptr;
        R_TRY(GetEntry(&entry, name, key));

        *out_size = entry->value_size;
        return ResultSuccess();
    }

    Result GetSdCardKeyValueStoreSettingsItemValue(size_t *out_size, void *dst, size_t dst_size, const char *name, const char *key) {
        R_UNLESS(dst != nullptr, ResultSettingsItemValueBufferNull());

        SdKeyValueStoreEntry *entry = nullptr;
        R_TRY(GetEntry(&entry, name, key));

        const size_t size = std::min(entry->value_size, dst_size);
        if (size > 0) {
            std::memcpy(dst, entry->value, size);
        }
        *out_size = size;
        return ResultSuccess();
    }

}
