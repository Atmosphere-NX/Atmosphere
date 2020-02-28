/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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

#include "ncm_make_path.hpp"
#include "ncm_path_utils.hpp"

namespace ams::ncm::path {

    namespace {

        u16 Get16BitSha256HashPrefix(util::Uuid uuid) {
            u8 hash[SHA256_HASH_SIZE];
            sha256CalculateHash(hash, uuid.data, sizeof(util::Uuid));
            return static_cast<u16>(hash[0]) | (static_cast<u16>(hash[1]) << 8);
        }

        u8 Get8BitSha256HashPrefix(util::Uuid uuid) {
            u8 hash[SHA256_HASH_SIZE];
            sha256CalculateHash(hash, uuid.data, sizeof(util::Uuid));
            return hash[0];
        }

    }

    void MakeContentPathFlat(char *path_out, ContentId content_id, const char *root) {
        char content_name[FS_MAX_PATH] = {0};
        GetContentFileName(content_name, content_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%s", root, content_name) >= 0);
    }

    void MakeContentPathDualLayered(char *path_out, ContentId content_id, const char *root) {
        char content_name[FS_MAX_PATH] = {0};
        const u16 hash = Get16BitSha256HashPrefix(content_id.uuid);
        const u32 hash_lower = (hash >> 4) & 0x3f;
        const u32 hash_upper = (hash >> 10) & 0x3f;

        GetContentFileName(content_name, content_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%08X/%08X/%s", root, hash_upper, hash_lower, content_name) >= 0);
    }

    void MakeContentPath10BitLayered(char *path_out, ContentId content_id, const char *root) {
        char content_name[FS_MAX_PATH] = {0};
        const u32 hash = (Get16BitSha256HashPrefix(content_id.uuid) >> 6) & 0x3FF;
        GetContentFileName(content_name, content_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%08X/%s", root, hash, content_name) >= 0);
    }

    void MakeContentPathHashByteLayered(char *path_out, ContentId content_id, const char *root) {
        char content_name[FS_MAX_PATH] = {0};
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(content_id.uuid));
        GetContentFileName(content_name, content_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%08X/%s", root, hash_byte, content_name) >= 0);
    }

    void MakePlaceHolderPathFlat(char *path_out, PlaceHolderId placeholder_id, const char *root) {
        char placeholder_name[FS_MAX_PATH] = {0};
        GetPlaceHolderFileName(placeholder_name, placeholder_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%s", root, placeholder_name) >= 0);
    }

    void MakePlaceHolderPathHashByteLayered(char *path_out, PlaceHolderId placeholder_id, const char *root) {
        char placeholder_name[FS_MAX_PATH] = {0};
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(placeholder_id.uuid));
        GetPlaceHolderFileName(placeholder_name, placeholder_id);
        AMS_ABORT_UNLESS(snprintf(path_out, FS_MAX_PATH-1, "%s/%08X/%s", root, hash_byte, placeholder_name) >= 0);
    }

}
