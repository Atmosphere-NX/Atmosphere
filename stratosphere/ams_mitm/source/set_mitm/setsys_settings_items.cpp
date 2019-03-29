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
    Result rc = (Result)((uintptr_t)arg);
    
    svcSleepThread(5000000000ULL);
    
    fatalSimple(rc);
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
        if (isxdigit(*str)) {
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
    
    while (isspace(*type) && type != delimiter) {
        type++;
    }
    
    size_t type_len = delimiter - type;
    size_t value_len = strlen(value_str);
    if (delimiter == NULL || value_len == 0 || type_len == 0) {
        return ResultSettingsItemValueInvalidFormat;
    }
    
    std::string kv = std::string(name) + "!" + std::string(key);
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

static int SettingsItemIniHandler(void *user, const char *name, const char *key, const char *value) {
    Result rc = *(reinterpret_cast<Result *>(user));
    ON_SCOPE_EXIT { *(reinterpret_cast<Result *>(user)) = rc; };
    
    if (R_SUCCEEDED(rc)) {
        rc = SettingsItemManager::ValidateName(name);
    }
    if (R_SUCCEEDED(rc)) {
        rc = SettingsItemManager::ValidateKey(name);
    }
    if (R_SUCCEEDED(rc)) {
        rc = ParseValue(name, key, value);
    }
    
    return R_SUCCEEDED(rc) ? 1 : 0;
}

void SettingsItemManager::LoadConfiguration() {    
    /* Open file. */
    FsFile config_file;
    Result rc = Utils::OpenSdFile("/atmosphere/system_settings.ini", FS_OPEN_READ, &config_file);
    if (R_FAILED(rc)) {
        return;
    }
    ON_SCOPE_EXIT {
        fsFileClose(&config_file);
    };
    
    /* Allocate buffer. */
    char *config_buf = new char[0x10000];
    std::memset(config_buf, 0, 0x10000);
    ON_SCOPE_EXIT {
        delete[] config_buf;
    };
    
    /* Read from file. */
    if (R_SUCCEEDED(rc)) {
        size_t actual_size;
        rc = fsFileRead(&config_file, 0, config_buf, 0xFFFF, &actual_size);
    }
    
    if (R_SUCCEEDED(rc)) {
        ini_parse_string(config_buf, SettingsItemIniHandler, &rc);
    }
    
    /* Report error if we encountered one. */
    if (R_FAILED(rc) && !g_threw_fatal) {
        g_threw_fatal = true;
        g_fatal_thread.Initialize(&FatalThreadFunc, reinterpret_cast<void *>(rc), 0x1000, 49);
        g_fatal_thread.Start();
    }
}

Result SettingsItemManager::GetValueSize(const char *name, const char *key, u64 *out_size) {
    std::string kv = std::string(name) + "!" + std::string(key);
    
    auto it = g_settings_items.find(kv);
    if (it == g_settings_items.end()) {
        return ResultSettingsItemNotFound;
    }
    
    *out_size = it->second.size;
    return ResultSuccess;
}

Result SettingsItemManager::GetValue(const char *name, const char *key, void *out, size_t max_size, u64 *out_size) {
    std::string kv = std::string(name) + "!" + std::string(key);
    
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
