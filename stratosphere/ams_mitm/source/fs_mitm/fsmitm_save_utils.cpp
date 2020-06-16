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
#include "fsmitm_save_utils.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        Result GetSaveDataSpaceIdString(const char **out_str, u8 space_id) {
            switch (static_cast<SaveDataSpaceId>(space_id)) {
                case SaveDataSpaceId::System:
                case SaveDataSpaceId::ProperSystem:
                    *out_str = "sys";
                    break;
                case SaveDataSpaceId::User:
                    *out_str = "user";
                    break;
                case SaveDataSpaceId::SdSystem:
                    *out_str = "sd_sys";
                    break;
                case SaveDataSpaceId::Temporary:
                    *out_str = "temp";
                    break;
                case SaveDataSpaceId::SdUser:
                    *out_str = "sd_user";
                    break;
                case SaveDataSpaceId::SafeMode:
                    *out_str = "safe";
                    break;
                default:
                    return fs::ResultInvalidSaveDataSpaceId();
            }

            return ResultSuccess();
        }

        Result GetSaveDataTypeString(const char **out_str, SaveDataType save_data_type) {
            switch (save_data_type) {
                case SaveDataType::System:
                    *out_str = "system";
                    break;
                case SaveDataType::Account:
                    *out_str = "account";
                    break;
                case SaveDataType::Bcat:
                    *out_str = "bcat";
                    break;
                case SaveDataType::Device:
                    *out_str = "device";
                    break;
                case SaveDataType::Temporary:
                    *out_str = "temp";
                    break;
                case SaveDataType::Cache:
                    *out_str = "cache";
                    break;
                case SaveDataType::SystemBcat:
                    *out_str = "system_bcat";
                    break;
                default:
                    /* TODO: Better result? */
                    return fs::ResultInvalidArgument();
            }

            return ResultSuccess();
        }

        constexpr inline bool IsEmptyAccountId(const UserId &uid) {
            return uid == InvalidUserId;
        }


    }

    Result SaveUtil::GetDirectorySaveDataPath(char *dst, size_t dst_size, ncm::ProgramId program_id, u8 space_id, const fs::SaveDataAttribute &attribute) {
        /* Saves should be separate for emunand vs sysnand. */
        const char *emummc_str = emummc::IsActive() ? "emummc" : "sysmmc";

        /* Get space_id, save_data_type strings. */
        const char *space_id_str, *save_type_str;
        R_TRY(GetSaveDataSpaceIdString(&space_id_str, space_id));
        R_TRY(GetSaveDataTypeString(&save_type_str, attribute.type));

        /* Initialize the path. */
        const bool is_system = attribute.system_save_data_id != InvalidSystemSaveDataId && IsEmptyAccountId(attribute.user_id);
        size_t out_path_len;
        if (is_system) {
            out_path_len = static_cast<size_t>(std::snprintf(dst, dst_size, "/atmosphere/saves/%s/%s/%s/%016lx", emummc_str, space_id_str, save_type_str, attribute.system_save_data_id));
        } else {
            out_path_len = static_cast<size_t>(std::snprintf(dst, dst_size, "/atmosphere/saves/%s/%s/%s/%016lx/%016lx%016lx", emummc_str, space_id_str, save_type_str, static_cast<u64>(program_id), attribute.user_id.data[1], attribute.user_id.data[0]));
        }

        R_UNLESS(out_path_len < dst_size, fs::ResultTooLongPath());

        return ResultSuccess();
    }

}
