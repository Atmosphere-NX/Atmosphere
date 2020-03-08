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
#include <stratosphere.hpp>

namespace ams::ncm {


    Result ContentMetaDatabaseBuilder::BuildFromPackageContentMeta(void *buf, size_t size, const ContentInfo &meta_info) {
        /* Create a reader for the content meta. */
        ncm::PackagedContentMetaReader package_meta_reader(buf, size);

        /* Allocate space to hold the converted meta. */
        const size_t meta_size = package_meta_reader.CalculateConvertContentMetaSize();
        void *meta = std::malloc(meta_size);
        ON_SCOPE_EXIT { std::free(meta); };

        /* Convert the meta from packaged form to normal form. */
        package_meta_reader.ConvertToContentMeta(meta, meta_size, meta_info);
        ncm::ContentMetaReader meta_reader(meta, meta_size);

        /* Insert the new metas into the database. */
        R_TRY(this->db->Set(package_meta_reader.GetKey(), meta_reader.GetData(), meta_reader.GetSize()));

        /* We're done. */
        return ResultSuccess();
    }

    Result ContentMetaDatabaseBuilder::BuildFromStorage(ContentStorage *storage) {
        /* Get the total count of contents. */
        s32 total_count;
        R_TRY(storage->GetContentCount(std::addressof(total_count)));

        /* Loop over all contents, looking for a package we can build from. */
        const size_t MaxContentIds = 64;
        ContentId content_ids[MaxContentIds];
        for (s32 offset = 0; offset < total_count; /* ... */) {
            /* List contents at the current offset. */
            s32 count;
            R_TRY(storage->ListContentId(std::addressof(count), content_ids, MaxContentIds, offset));

            /* Loop the contents we listed, looking for a correct one. */
            for (s32 i = 0; i < count; i++) {
                /* Get the path for this content id. */
                auto &content_id = content_ids[i];
                ncm::Path path;
                storage->GetPath(std::addressof(path), content_id);

                /* Read the content meta path, and build. */
                ncm::AutoBuffer package_meta;
                if (R_SUCCEEDED(ncm::ReadContentMetaPath(std::addressof(package_meta), path.str))) {
                    /* Get the size of the content. */
                    s64 size;
                    R_TRY(storage->GetSize(std::addressof(size), content_id));

                    /* Build. */
                    R_TRY(this->BuildFromPackageContentMeta(package_meta.Get(), package_meta.GetSize(), ContentInfo::Make(content_id, size, ContentType::Meta)));
                }
            }

            /* Advance. */
            offset += count;
        }

        /* Commit our changes. */
        return this->db->Commit();
    }

    Result ContentMetaDatabaseBuilder::Cleanup() {
        /* This cleans up the content meta by removing all entries. */
        while (true) {
            /* List as many keys as we can. */
            constexpr s32 MaxKeys = 64;
            ContentMetaKey keys[MaxKeys];
            auto list_count = this->db->ListContentMeta(keys, MaxKeys);

            /* Remove the listed keys. */
            for (auto i = 0; i < list_count.written; i++) {
                R_TRY(this->db->Remove(keys[i]));
            }

            /* If there aren't more keys to read, we're done. */
            if (list_count.written < MaxKeys) {
                break;
            }
        }

        /* Commit our deletions. */
        return this->db->Commit();
    }

}
