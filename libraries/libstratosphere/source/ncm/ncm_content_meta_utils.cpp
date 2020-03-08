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
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        using FilePathString = kvdb::BoundedString<64>;

        bool IsContentMetaFileName(const char *name) {
            return impl::PathView(name).HasSuffix(".cnmt");
        }

    }

    Result ReadContentMetaPath(AutoBuffer *out, const char *path) {
        /* Mount the content. */
        auto mount_name = impl::CreateUniqueMountName();
        R_TRY(fs::MountContent(mount_name.str, path, fs::ContentType_Meta));
        ON_SCOPE_EXIT { fs::Unmount(mount_name.str); };

        /* Open the root directory. */
        auto root_path = impl::GetRootDirectoryPath(mount_name);
        fs::DirectoryHandle dir;
        R_TRY(fs::OpenDirectory(std::addressof(dir), root_path.str, fs::OpenDirectoryMode_File));
        ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

        /* Loop directory reading until we find the entry we're looking for. */
        while (true) {
            /* Read one entry, and finish when we fail to read. */
            fs::DirectoryEntry entry;
            s64 num_read;
            R_TRY(fs::ReadDirectory(std::addressof(num_read), std::addressof(entry), dir, 1));
            if (num_read == 0) {
                break;
            }

            /* If this is the content meta file, parse it. */
            if (IsContentMetaFileName(entry.name)) {
                /* Create the file path. */
                FilePathString file_path(root_path.str);
                file_path.Append(entry.name);

                /* Open the content meta file. */
                fs::FileHandle file;
                R_TRY(fs::OpenFile(std::addressof(file), file_path, fs::OpenMode_Read));
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                /* Get the meta size. */
                s64 file_size;
                R_TRY(fs::GetFileSize(std::addressof(file_size), file));
                const size_t meta_size = static_cast<size_t>(file_size);

                /* Create a buffer for the meta. */
                R_TRY(out->Initialize(meta_size));

                /* Read the meta into the buffer. */
                return fs::ReadFile(file, 0, out->Get(), meta_size);
            }
        }

        return ncm::ResultContentMetaNotFound();
    }

}
