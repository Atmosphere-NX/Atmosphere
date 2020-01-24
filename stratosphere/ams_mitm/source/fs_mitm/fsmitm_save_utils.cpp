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
#include "fsmitm_save_utils.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        Result GetSaveDataSpaceIdString(const char **out_str, u8 space_id) {
            switch (space_id) {
                case FsSaveDataSpaceId_System:
                case FsSaveDataSpaceId_ProperSystem:
                    *out_str = "sys";
                    break;
                case FsSaveDataSpaceId_User:
                    *out_str = "user";
                    break;
                case FsSaveDataSpaceId_SdSystem:
                    *out_str = "sd_sys";
                    break;
                case FsSaveDataSpaceId_Temporary:
                    *out_str = "temp";
                    break;
                case FsSaveDataSpaceId_SdUser:
                    *out_str = "sd_user";
                    break;
                case FsSaveDataSpaceId_SafeMode:
                    *out_str = "safe";
                    break;
                default:
                    return fs::ResultInvalidSaveDataSpaceId();
            }

            return ResultSuccess();
        }

        Result GetSaveDataTypeString(const char **out_str, u8 save_data_type) {
            switch (save_data_type) {
                case FsSaveDataType_System:
                    *out_str = "system";
                    break;
                case FsSaveDataType_Account:
                    *out_str = "account";
                    break;
                case FsSaveDataType_Bcat:
                    *out_str = "bcat";
                    break;
                case FsSaveDataType_Device:
                    *out_str = "device";
                    break;
                case FsSaveDataType_Temporary:
                    *out_str = "temp";
                    break;
                case FsSaveDataType_Cache:
                    *out_str = "cache";
                    break;
                case FsSaveDataType_SystemBcat:
                    *out_str = "system_bcat";
                    break;
                default:
                    /* TODO: Better result? */
                    return fs::ResultInvalidArgument();
            }

            return ResultSuccess();
        }

        constexpr inline bool IsEmptyAccountId(const AccountUid &uid) {
            constexpr AccountUid empty_uid = {};
            return std::memcmp(&uid, &empty_uid, sizeof(uid)) == 0;
        }


    }

    Result SaveUtil::GetDirectorySaveDataPath(char *dst, size_t dst_size, ncm::ProgramId program_id, u8 space_id, const FsSaveDataAttribute &attribute) {
        /* Saves should be separate for emunand vs sysnand. */
        const char *emummc_str = emummc::IsActive() ? "emummc" : "sysmmc";

        /* Get space_id, save_data_type strings. */
        const char *space_id_str, *save_type_str;
        R_TRY(GetSaveDataSpaceIdString(&space_id_str, space_id));
        R_TRY(GetSaveDataTypeString(&save_type_str, attribute.save_data_type));

        /* Initialize the path. */
        const bool is_system = attribute.system_save_data_id != 0 && IsEmptyAccountId(attribute.uid);
        size_t out_path_len;
        if (is_system) {
            out_path_len = static_cast<size_t>(std::snprintf(dst, dst_size, "/atmosphere/saves/%s/%s/%s/%016lx", emummc_str, space_id_str, save_type_str, attribute.system_save_data_id));
        } else {
            out_path_len = static_cast<size_t>(std::snprintf(dst, dst_size, "/atmosphere/saves/%s/%s/%s/%016lx/%016lx%016lx", emummc_str, space_id_str, save_type_str, static_cast<u64>(program_id), attribute.uid.uid[1], attribute.uid.uid[0]));
        }

        R_UNLESS(out_path_len < dst_size, fs::ResultTooLongPath());

        return ResultSuccess();
    }

}
