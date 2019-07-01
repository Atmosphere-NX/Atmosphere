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
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <switch.h>
#include <strings.h>
#include <ctype.h>

#include "setsys_settings_items.hpp"
#include "../utils.hpp"
#include "../ini.h"

struct SettingsItemValue {
    size_t size;
    u8 *data;
};

std::map<std::string, SettingsItemValue> g_settings_items;

static bool g_threw_fatal = false;
static HosThread g_fatal_thread;

static void FatalThreadFunc(void *arg) {
    svcSleepThread(5000000000ULL);
    fatalSimple(static_cast<Result>(reinterpret_cast<uintptr_t>(arg)));
}

static bool IsCorrectFormat(const char *str, size_t len) {
    if (len > 0 && str[len - 1] == '.') {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        const char c = *(str++);

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

Result SettingsItemManager::ValidateName(const char *name, size_t max_size) {
    if (name == nullptr) {
        return ResultSettingsItemNameNull;
    }

    const size_t name_len = strnlen(name, std::min(max_size, MaxNameLength + 1));
    if (name_len == 0) {
        return ResultSettingsItemNameEmpty;
    } else if (name_len > MaxNameLength) {
        return ResultSettingsItemNameTooLong;
    }

    if (!IsCorrectFormat(name, name_len)) {
        return ResultSettingsItemNameInvalidFormat;
    }

    return ResultSuccess;
}

Result SettingsItemManager::ValidateName(const char *name) {
    return ValidateName(name, MaxNameLength + 1);
}

Result SettingsItemManager::ValidateKey(const char *key, size_t max_size) {
    if (key == nullptr) {
        return ResultSettingsItemKeyNull;
    }

    const size_t key_len = strnlen(key, std::min(max_size, MaxKeyLength + 1));
    if (key_len == 0) {
        return ResultSettingsItemKeyEmpty;
    } else if (key_len > MaxKeyLength) {
        return ResultSettingsItemKeyTooLong;
    }

    if (!IsCorrectFormat(key, key_len)) {
        return ResultSettingsItemKeyInvalidFormat;
    }

    return ResultSuccess;
}

Result SettingsItemManager::ValidateKey(const char *key) {
    return ValidateKey(key, MaxKeyLength + 1);
}

static bool IsHexadecimal(const char *str) {
    while (*str) {
        if (isxdigit((unsigned char)*str)) {
            str++;
        } else {
            return false;
        }
    }
    return true;
}

static char hextoi(char c) {
    if ('a' <= c && c <= 'f') return c - 'a' + 0xA;
    if ('A' <= c && c <= 'F') return c - 'A' + 0xA;
    if ('0' <= c && c <= '9') return c - '0';
    return 0;
}

static Result ParseValue(const char *name, const char *key, const char *val_tup) {
    const char *delimiter = strchr(val_tup, '!');
    const char *value_str = delimiter + 1;
    const char *type = val_tup;

    if (delimiter == NULL) {
        return ResultSettingsItemValueInvalidFormat;
    }

    while (isspace((unsigned char)*type) && type != delimiter) {
        type++;
    }

    size_t type_len = delimiter - type;
    size_t value_len = strlen(value_str);
    if (delimiter == NULL || value_len == 0 || type_len == 0) {
        return ResultSettingsItemValueInvalidFormat;
    }

    std::string kv = std::string(name).append("!").append(key);
    SettingsItemValue value;

    if (strncasecmp(type, "str", type_len) == 0 || strncasecmp(type, "string", type_len) == 0) {
        /* String */
        value.size = value_len + 1;
        value.data = reinterpret_cast<u8 *>(strdup(value_str));
        if (value.data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }
    } else if (strncasecmp(type, "hex", type_len) == 0 || strncasecmp(type, "bytes", type_len) == 0) {
        /* hex */
        if (value_len % 2 || !IsHexadecimal(value_str)) {
            return ResultSettingsItemValueInvalidFormat;
        }
        value.size = value_len / 2;
        u8 *data = reinterpret_cast<u8 *>(malloc(value.size));
        if (data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }

        memset(data, 0, value.size);
        for (size_t i = 0; i < value_len; i++) {
            data[i >> 1] |= hextoi(value_str[i]) << (4 * (i & 1));
        }

        value.data = data;
    }  else if (strncasecmp(type, "u8", type_len) == 0) {
        /* u8 */
        value.size = sizeof(u8);
        u8 *data = reinterpret_cast<u8 *>(malloc(value.size));
        if (data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }
        *data = (u8)(strtoul(value_str, nullptr, 0));
        value.data = reinterpret_cast<u8 *>(data);
    } else if (strncasecmp(type, "u16", type_len) == 0) {
        /* u16 */
        value.size = sizeof(u16);
        u16 *data = reinterpret_cast<u16 *>(malloc(value.size));
        if (data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }
        *data = (u16)(strtoul(value_str, nullptr, 0));
        value.data = reinterpret_cast<u8 *>(data);
    } else if (strncasecmp(type, "u32", type_len) == 0) {
        /* u32 */
        value.size = sizeof(u32);
        u32 *data = reinterpret_cast<u32 *>(malloc(value.size));
        if (data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }
        *data = (u32)(strtoul(value_str, nullptr, 0));
        value.data = reinterpret_cast<u8 *>(data);
    } else if (strncasecmp(type, "u64", type_len) == 0) {
        /* u64 */
        value.size = sizeof(u64);
        u64 *data = reinterpret_cast<u64 *>(malloc(value.size));
        if (data == nullptr) {
            return ResultSettingsItemValueAllocationFailed;
        }
        *data = (u64)(strtoul(value_str, nullptr, 0));
        value.data = reinterpret_cast<u8 *>(data);
    } else {
        return ResultSettingsItemValueInvalidFormat;
    }

    g_settings_items[kv] = value;
    return ResultSuccess;
}

static Result ParseSettingsItemValue(const char *name, const char *key, const char *value) {
    /* Validate name and key, then parse value. */
    R_TRY(SettingsItemManager::ValidateName(name));
    R_TRY(SettingsItemManager::ValidateKey(name));
    R_TRY(ParseValue(name, key, value));
    return ResultSuccess;
}

static int SettingsItemIniHandler(void *user, const char *name, const char *key, const char *value) {
    Result *user_res = reinterpret_cast<Result *>(user);

    /* Stop parsing after we fail to parse a value. */
    if (R_FAILED(*user_res)) {
        return 0;
    }

    *user_res = ParseSettingsItemValue(name, key, value);
    return R_SUCCEEDED(*user_res) ? 1 : 0;
}

static Result LoadConfigurationImpl() {
    /* Open file. */
    FsFile config_file;
    R_TRY(Utils::OpenSdFile("/atmosphere/system_settings.ini", FS_OPEN_READ, &config_file));
    ON_SCOPE_EXIT {
        fsFileClose(&config_file);
    };

    /* Allocate buffer. */
    std::string config_buf(0xFFFF, '\0');

    /* Read from file. */
    size_t actual_size;
    R_TRY(fsFileRead(&config_file, 0, config_buf.data(), config_buf.size(), FS_READOPTION_NONE, &actual_size));

    /* Parse. */
    Result parse_res = ResultSuccess;
    ini_parse_string(config_buf.c_str(), SettingsItemIniHandler, &parse_res);
    return parse_res;
}

void SettingsItemManager::LoadConfiguration() {
    const Result load_res = LoadConfigurationImpl();
    if (R_FAILED(load_res) && !g_threw_fatal) {
        /* Report error if we encountered one. */
        g_threw_fatal = true;
        g_fatal_thread.Initialize(&FatalThreadFunc, reinterpret_cast<void *>(load_res), 0x1000, 49);
        g_fatal_thread.Start();
    }
}

Result SettingsItemManager::GetValueSize(const char *name, const char *key, u64 *out_size) {
    const std::string kv = std::string(name).append("!").append(key);

    auto it = g_settings_items.find(kv);
    if (it == g_settings_items.end()) {
        return ResultSettingsItemNotFound;
    }

    *out_size = it->second.size;
    return ResultSuccess;
}

Result SettingsItemManager::GetValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size) {
    const std::string kv = std::string(name).append("!").append(key);

    auto it = g_settings_items.find(kv);
    if (it == g_settings_items.end()) {
        return ResultSettingsItemNotFound;
    }

    size_t copy_size = it->second.size;
    if (max_size < copy_size) {
        copy_size = max_size;
    }
    *out_size = copy_size;

    memcpy(out, it->second.data, copy_size);
    return ResultSuccess;
}
