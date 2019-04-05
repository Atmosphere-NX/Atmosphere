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
#include <cstring>
#include <cstdlib>
#include <switch.h>

#include "fs_save_utils.hpp"

Result FsSaveUtils::GetSaveDataSpaceIdString(const char **out_str, u8 space_id) {
    const char *str = nullptr;

    switch (space_id) {
        case FsSaveDataSpaceId_NandSystem:
        case 100: /* ProperSystem */
        case 101: /* SafeMode */
            str = "sys";
            break;
        case FsSaveDataSpaceId_NandUser:
            str = "user";
            break;
        case FsSaveDataSpaceId_SdCard:
            str = "sd_sys";
            break;
        case FsSaveDataSpaceId_TemporaryStorage:
            str = "temp";
            break;
        case 4: /* SdUser */
            str = "sd_user";
            break;
        default: /* Unexpected */
            str = nullptr;
            break;
    }

    if (str == nullptr) {
        return ResultFsInvalidSaveDataSpaceId;
    }

    if (out_str) {
        *out_str = str;
    }

    return ResultSuccess;
}

Result FsSaveUtils::GetSaveDataTypeString(const char **out_str, u8 save_data_type) {
    const char *str = nullptr;

    switch (save_data_type) {
        case FsSaveDataType_SystemSaveData:
            str = "system";
            break;
        case FsSaveDataType_SaveData:
            str = "account";
            break;
        case FsSaveDataType_BcatDeliveryCacheStorage:
            str = "bcat";
            break;
        case FsSaveDataType_DeviceSaveData:
            str = "device";
            break;
        case FsSaveDataType_TemporaryStorage:
            str = "temp";
            break;
        case FsSaveDataType_CacheStorage:
            str = "cache";
            break;
        case 6: /* System Bcat Save */
            str = "bcat_sys";
            break;
        default: /* Unexpected */
            str = nullptr;
            break;
    }

    if (str == nullptr) {
        /* TODO: Better result? */
        return ResultFsInvalidArgument;
    }

    if (out_str) {
        *out_str = str;
    }

    return ResultSuccess;
}

Result FsSaveUtils::GetSaveDataDirectoryPath(FsPath &out_path, u8 space_id, u8 save_data_type, u64 title_id, u128 user_id, u64 save_id) {
    const char *space_id_str, *save_type_str;
    Result rc;

    /* Get space_id, save_data_type strings. */
    if (R_FAILED((rc = GetSaveDataSpaceIdString(&space_id_str, space_id))) || R_FAILED((rc = GetSaveDataTypeString(&save_type_str, save_data_type)))) {
        return rc;
    }

    /* Clear and initialize the path. */
    std::memset(&out_path, 0, sizeof(out_path));
    const bool is_system = (save_id != 0 && user_id == 0);
    size_t out_path_len;
    if (is_system) {
        out_path_len = static_cast<size_t>(snprintf(out_path.str, sizeof(out_path.str), "/atmosphere/saves/sysnand/%s/%s/%016lx",
                                                    space_id_str, save_type_str, save_id));
    } else {
        out_path_len = static_cast<size_t>(snprintf(out_path.str, sizeof(out_path.str), "/atmosphere/saves/sysnand/%s/%s/%016lx/%016lx%016lx",
                                                    space_id_str, save_type_str, title_id, static_cast<u64>(user_id >> 64ul), static_cast<u64>(user_id)));
    }
    if (out_path_len >= sizeof(out_path)) {
        /* TODO: Should we abort here? */
        return ResultFsTooLongPath;
    }

    return ResultSuccess;
}
