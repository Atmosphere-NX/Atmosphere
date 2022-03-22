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
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        constexpr inline size_t MaxPackagePathLength = 0x100;

        Result ConvertToFsCommonPath(char *dst, size_t dst_size, const char *package_root_path, const char *entry_path) {
            char package_path[MaxPackagePathLength];

            const size_t path_len = util::SNPrintf(package_path, sizeof(package_path), "%s%s", package_root_path, entry_path);
            AMS_ABORT_UNLESS(path_len < MaxPackagePathLength);

            return fs::ConvertToFsCommonPath(dst, dst_size, package_path);
        }

        Result LoadContentMeta(ncm::AutoBuffer *out, const char *package_root_path, const fs::DirectoryEntry &entry) {
            AMS_ABORT_UNLESS(impl::PathView(entry.name).HasSuffix(".cnmt.nca"));

            char path[MaxPackagePathLength];
            R_TRY(ConvertToFsCommonPath(path, sizeof(path), package_root_path, entry.name));

            return ncm::ReadContentMetaPath(out, path);
        }

        template<typename F>
        Result ForEachFileInDirectory(const char *root_path, F f) {
            /* Open the directory. */
            fs::DirectoryHandle dir;
            R_TRY(fs::OpenDirectory(std::addressof(dir), root_path, fs::OpenDirectoryMode_File));
            ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

            while (true) {
                /* Read the current entry. */
                s64 count;
                fs::DirectoryEntry entry;
                R_TRY(fs::ReadDirectory(std::addressof(count), std::addressof(entry), dir, 1));
                if (count == 0) {
                    break;
                }

                /* Invoke our handler on the entry. */
                bool done;
                R_TRY(f(std::addressof(done), entry));
                R_SUCCEED_IF(done);
            }

            return ResultSuccess();
        }

    }

    Result ContentMetaDatabaseBuilder::BuildFromPackageContentMeta(void *buf, size_t size, const ContentInfo &meta_info) {
        /* Create a reader for the content meta. */
        ncm::PackagedContentMetaReader package_meta_reader(buf, size);

        /* Allocate space to hold the converted meta. */
        const size_t meta_size = package_meta_reader.CalculateConvertContentMetaSize();
        std::unique_ptr<char[]> meta(new (std::nothrow) char[meta_size]);

        /* Convert the meta from packaged form to normal form. */
        package_meta_reader.ConvertToContentMeta(meta.get(), meta_size, meta_info);
        ncm::ContentMetaReader meta_reader(meta.get(), meta_size);

        /* Insert the new metas into the database. */
        R_TRY(m_db->Set(package_meta_reader.GetKey(), meta_reader.GetData(), meta_reader.GetSize()));

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
        return m_db->Commit();
    }

    Result ContentMetaDatabaseBuilder::BuildFromPackage(const char *package_root_path) {
        /* Build the database by writing every entry in the package. */
        R_TRY(ForEachFileInDirectory(package_root_path, [&](bool *done, const fs::DirectoryEntry &entry) -> Result {
            /* Never early terminate. */
            *done = false;

            /* We have nothing to list if we're not looking at a meta. */
            R_SUCCEED_IF(!impl::PathView(entry.name).HasSuffix(".cnmt.nca"));

            /* Read the content meta path, and build. */
            ncm::AutoBuffer package_meta;
            R_TRY(LoadContentMeta(std::addressof(package_meta), package_root_path, entry));

            /* Try to parse a content id from the name. */
            auto content_id = GetContentIdFromString(entry.name, sizeof(entry.name));
            R_UNLESS(content_id, ncm::ResultInvalidPackageFormat());

            /* Build using the meta. */
            return this->BuildFromPackageContentMeta(package_meta.Get(), package_meta.GetSize(), ContentInfo::Make(*content_id, entry.file_size, ContentType::Meta));
        }));

        /* Commit our changes. */
        return m_db->Commit();
    }

    Result ContentMetaDatabaseBuilder::Cleanup() {
        /* This cleans up the content meta by removing all entries. */
        while (true) {
            /* List as many keys as we can. */
            constexpr s32 MaxKeys = 64;
            ContentMetaKey keys[MaxKeys];
            auto list_count = m_db->ListContentMeta(keys, MaxKeys);

            /* Remove the listed keys. */
            for (auto i = 0; i < list_count.written; i++) {
                R_TRY(m_db->Remove(keys[i]));
            }

            /* If there aren't more keys to read, we're done. */
            if (list_count.written < MaxKeys) {
                break;
            }
        }

        /* Commit our deletions. */
        return m_db->Commit();
    }

    Result ListApplicationPackage(s32 *out_count, ApplicationId *out_ids, size_t max_out_ids, const char *package_root_path) {
        size_t count = 0;
        R_TRY(ForEachFileInDirectory(package_root_path, [&](bool *done, const fs::DirectoryEntry &entry) -> Result {
            /* Never early terminate. */
            *done = false;

            /* We have nothing to list if we're not looking at a meta. */
            R_SUCCEED_IF(!impl::PathView(entry.name).HasSuffix(".cnmt.nca"));

            /* Read the content meta path, and build. */
            ncm::AutoBuffer package_meta;
            R_TRY(LoadContentMeta(std::addressof(package_meta), package_root_path, entry));

            /* Create a reader for the meta. */
            ncm::PackagedContentMetaReader package_meta_reader(package_meta.Get(), package_meta.GetSize());

            /* Write the key to output if we're reading an application. */
            const auto &key = package_meta_reader.GetKey();
            if (key.type == ContentMetaType::Application) {
                R_UNLESS(count < max_out_ids, ncm::ResultBufferInsufficient());

                out_ids[count++] = { key.id };
            }

            return ResultSuccess();
        }));

        *out_count = static_cast<s32>(count);
        return ResultSuccess();
    }

}
