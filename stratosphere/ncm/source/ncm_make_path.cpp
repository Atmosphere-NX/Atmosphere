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

    void MakeContentPathFlat(PathString *out, ContentId content_id, const PathString &root) {
        Path content_name;
        GetContentFileName(content_name.str, content_id);
        out->SetFormat("%s/%s", root.Get(), content_name.str);
    }

    void MakeContentPathDualLayered(PathString *out, ContentId content_id, const PathString &root) {
        const u16 hash = Get16BitSha256HashPrefix(content_id.uuid);
        const u32 hash_lower = (hash >> 4) & 0x3f;
        const u32 hash_upper = (hash >> 10) & 0x3f;

        Path content_name;
        GetContentFileName(content_name.str, content_id);
        out->SetFormat("%s/%08X/%08X/%s", root.Get(), hash_upper, hash_lower, content_name.str);
    }

    void MakeContentPath10BitLayered(PathString *out, ContentId content_id, const PathString &root) {
        const u32 hash = (Get16BitSha256HashPrefix(content_id.uuid) >> 6) & 0x3FF;

        Path content_name;
        GetContentFileName(content_name.str, content_id);
        out->SetFormat("%s/%08X/%s", root.Get(), hash, content_name.str);
    }

    void MakeContentPathHashByteLayered(PathString *out, ContentId content_id, const PathString &root) {
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(content_id.uuid));

        Path content_name;
        GetContentFileName(content_name.str, content_id);
        out->SetFormat("%s/%08X/%s", root.Get(), hash_byte, content_name.str);
    }

    void MakePlaceHolderPathFlat(PathString *out, PlaceHolderId placeholder_id, const PathString &root) {
        Path placeholder_name;
        GetPlaceHolderFileName(placeholder_name.str, placeholder_id);
        out->SetFormat("%s/%s", root.Get(), placeholder_name.str);
    }

    void MakePlaceHolderPathHashByteLayered(PathString *out, PlaceHolderId placeholder_id, const PathString &root) {
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(placeholder_id.uuid));

        Path placeholder_name;
        GetPlaceHolderFileName(placeholder_name.str, placeholder_id);
        out->SetFormat("%s/%08X/%s", root.Get(), hash_byte, placeholder_name.str);
    }

}
