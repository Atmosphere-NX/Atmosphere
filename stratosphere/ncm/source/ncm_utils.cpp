/*
 * Copyright (c) 2019 Adubbz
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

#include "ncm_utils.hpp"

namespace sts::ncm {

    void GetStringFromContentId(char* out, ContentId content_id) {
        for (size_t i = 0; i < sizeof(ContentId); i++) {
            snprintf(out+i, 3, "%02x", content_id.uuid[i]);
        }
    }

    void GetStringFromPlaceHolderId(char* out, PlaceHolderId placeholder_id) {
        for (size_t i = 0; i < sizeof(PlaceHolderId); i++) {
            snprintf(out+i, 3, "%02x", placeholder_id.uuid[i]);
        }
    }

    Result GetPlaceHolderIdFromDirEntry(PlaceHolderId* out, struct dirent* dir_entry) {
        if (strnlen(dir_entry->d_name, 0x30) != 0x24 || strncmp(dir_entry->d_name + 0x20, ".nca", 4) != 0) {
            return ResultNcmInvalidPlaceHolderDirectoryEntry;
        }

        PlaceHolderId placeholder_id = {0};
        char byte_string[2];
        char* end_ptr;
        u64 converted_val;

        for (size_t i = 0; i < sizeof(PlaceHolderId); i++) {
            char* name_char_pair = dir_entry->d_name + i * 2;         
        
            byte_string[0] = name_char_pair[0];
            byte_string[1] = name_char_pair[1];

            converted_val = strtoull(byte_string, &end_ptr, 0x10);
            placeholder_id.uuid[i] = (u8)converted_val;
        }

        *out = placeholder_id;
        return ResultSuccess;
    }

    std::optional<ContentId> GetContentIdFromString(const char* str, size_t len) {
        ContentId content_id = {0};

        if (len < 0x20) {
            return std::nullopt;
        }

        char byte_string[2];
        char* end_ptr;
        u64 converted_val;

        for (size_t i = 0; i < sizeof(ContentId); i++) {
            const char* char_par = str + i * 2;         
        
            byte_string[0] = char_par[0];
            byte_string[1] = char_par[1];

            converted_val = strtoull(byte_string, &end_ptr, 0x10);
            content_id.uuid[i] = (u8)converted_val;
        }

        return std::optional<ContentId>(content_id);
    }

}