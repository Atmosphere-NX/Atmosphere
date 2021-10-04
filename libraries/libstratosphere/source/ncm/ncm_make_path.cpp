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

namespace ams::ncm {

    namespace {

        void MakeContentName(PathString *out, ContentId id) {
            out->SetFormat("%s.nca", GetContentIdString(id).data);
        }

        void MakePlaceHolderName(PathString *out, PlaceHolderId id) {
            auto &bytes = id.uuid.data;
            char tmp[3];

            /* Create a hex string from bytes. */
            for (size_t i = 0; i < sizeof(bytes); i++) {
                util::SNPrintf(tmp, util::size(tmp), "%02x", bytes[i]);
                out->Append(tmp);
            }

            /* Append file extension. */
            out->Append(".nca");
        }

        u16 Get16BitSha256HashPrefix(ContentId id) {
            u8 hash[crypto::Sha256Generator::HashSize];
            crypto::GenerateSha256Hash(hash, sizeof(hash), std::addressof(id), sizeof(id));
            return static_cast<u16>(hash[0]) | (static_cast<u16>(hash[1]) << 8);
        }

        u8 Get8BitSha256HashPrefix(ContentId id) {
            u8 hash[crypto::Sha256Generator::HashSize];
            crypto::GenerateSha256Hash(hash, sizeof(hash), std::addressof(id), sizeof(id));
            return hash[0];
        }

        u8 Get8BitSha256HashPrefix(PlaceHolderId id) {
            u8 hash[crypto::Sha256Generator::HashSize];
            crypto::GenerateSha256Hash(hash, sizeof(hash), std::addressof(id), sizeof(id));
            return hash[0];
        }

    }

    void MakeFlatContentFilePath(PathString *out, ContentId content_id, const char *root_path) {
        /* Create the content name from the content id. */
        PathString content_name;
        MakeContentName(std::addressof(content_name), content_id);

        /* Format the output path. */
        out->SetFormat("%s/%s", root_path, content_name.Get());
    }

    void MakeSha256HierarchicalContentFilePath_ForFat4KCluster(PathString *out, ContentId content_id, const char *root_path) {
        /* Hash the content id. */
        const u16 hash = Get16BitSha256HashPrefix(content_id);
        const u32 hash_upper = (hash >> 10) & 0x3f;
        const u32 hash_lower = (hash >>  4) & 0x3f;

        /* Create the content name from the content id. */
        PathString content_name;
        MakeContentName(std::addressof(content_name), content_id);

        /* Format the output path. */
        out->SetFormat("%s/%08X/%08X/%s", root_path, hash_upper, hash_lower, content_name.Get());
    }

    void MakeSha256HierarchicalContentFilePath_ForFat32KCluster(PathString *out, ContentId content_id, const char *root_path) {
        /* Hash the content id. */
        const u32 hash = (Get16BitSha256HashPrefix(content_id) >> 6) & 0x3FF;

        /* Create the content name from the content id. */
        PathString content_name;
        MakeContentName(std::addressof(content_name), content_id);

        /* Format the output path. */
        out->SetFormat("%s/%08X/%s", root_path, hash, content_name.Get());
    }

    void MakeSha256HierarchicalContentFilePath_ForFat16KCluster(PathString *out, ContentId content_id, const char *root_path) {
        /* Hash the content id. */
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(content_id));

        /* Create the content name from the content id. */
        PathString content_name;
        MakeContentName(std::addressof(content_name), content_id);

        /* Format the output path. */
        out->SetFormat("%s/%08X/%s", root_path, hash_byte, content_name.Get());
    }

    size_t GetHierarchicalContentDirectoryDepth(MakeContentPathFunction func) {
        if (func == MakeFlatContentFilePath) {
            return 1;
        } else if (func == MakeSha256HierarchicalContentFilePath_ForFat4KCluster) {
            return 3;
        } else if (func == MakeSha256HierarchicalContentFilePath_ForFat16KCluster ||
                   func == MakeSha256HierarchicalContentFilePath_ForFat32KCluster) {
            return 2;
        } else {
            AMS_ABORT();
        }
    }

    void MakeFlatPlaceHolderFilePath(PathString *out, PlaceHolderId placeholder_id, const char *root_path) {
        /* Create the placeholder name from the placeholder id. */
        PathString placeholder_name;
        MakePlaceHolderName(std::addressof(placeholder_name), placeholder_id);

        /* Format the output path. */
        out->SetFormat("%s/%s", root_path, placeholder_name.Get());
    }

    void MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster(PathString *out, PlaceHolderId placeholder_id, const char *root_path) {
        /* Hash the placeholder id. */
        const u32 hash_byte = static_cast<u32>(Get8BitSha256HashPrefix(placeholder_id));

        /* Create the placeholder name from the placeholder id. */
        PathString placeholder_name;
        MakePlaceHolderName(std::addressof(placeholder_name), placeholder_id);

        /* Format the output path. */
        out->SetFormat("%s/%08X/%s", root_path, hash_byte, placeholder_name.Get());
    }

    size_t GetHierarchicalPlaceHolderDirectoryDepth(MakePlaceHolderPathFunction func) {
        if (func == MakeFlatPlaceHolderFilePath) {
            return 1;
        } else if (func == MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster) {
            return 2;
        } else {
            AMS_ABORT();
        }
    }

}
