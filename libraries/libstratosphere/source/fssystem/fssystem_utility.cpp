/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::fssystem {

    namespace {

        inline Result EnsureDirectoryExists(fs::fsa::IFileSystem *fs, const char *path) {
            R_TRY_CATCH(fs->CreateDirectory(path)) {
                R_CATCH(fs::ResultPathAlreadyExists) { /* If path already exists, there's no problem. */ }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

    }

    Result CopyFile(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const char *dst_parent_path, const char *src_path, const fs::DirectoryEntry *entry, void *work_buf, size_t work_buf_size) {
        /* Open source file. */
        std::unique_ptr<fs::fsa::IFile> src_file;
        R_TRY(src_fs->OpenFile(&src_file, src_path, fs::OpenMode_Read));

        /* Open dst file. */
        std::unique_ptr<fs::fsa::IFile> dst_file;
        {
            char dst_path[fs::EntryNameLengthMax + 1];
            const size_t original_size = static_cast<size_t>(std::snprintf(dst_path, sizeof(dst_path), "%s%s", dst_parent_path, entry->name));
            /* TODO: Error code? N aborts here. */
            AMS_ASSERT(original_size < sizeof(dst_path));

            R_TRY(dst_fs->CreateFile(dst_path, entry->file_size));
            R_TRY(dst_fs->OpenFile(&dst_file, dst_path, fs::OpenMode_Write));
        }

        /* Read/Write file in work buffer sized chunks. */
        s64 remaining = entry->file_size;
        s64 offset = 0;
        while (remaining > 0) {
            size_t read_size;
            R_TRY(src_file->Read(&read_size, offset, work_buf, work_buf_size, fs::ReadOption()));
            R_TRY(dst_file->Write(offset, work_buf, read_size, fs::WriteOption()));

            remaining -= read_size;
            offset    += read_size;
        }

        return ResultSuccess();
    }

    Result CopyDirectoryRecursively(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const char *dst_path, const char *src_path, void *work_buf, size_t work_buf_size) {
        char dst_path_buf[fs::EntryNameLengthMax + 1];
        const size_t original_size = static_cast<size_t>(std::snprintf(dst_path_buf, sizeof(dst_path_buf), "%s", dst_path));
        AMS_ASSERT(original_size < sizeof(dst_path_buf));

        return IterateDirectoryRecursively(src_fs, src_path,
            [&](const char *path, const fs::DirectoryEntry &entry) -> Result { /* On Enter Directory */
                /* Update path, create new dir. */
                std::strncat(dst_path_buf, entry.name, sizeof(dst_path_buf) - strnlen(dst_path_buf, sizeof(dst_path_buf) - 1) - 1);
                std::strncat(dst_path_buf, "/",        sizeof(dst_path_buf) - strnlen(dst_path_buf, sizeof(dst_path_buf) - 1) - 1);
                return dst_fs->CreateDirectory(dst_path_buf);
            },
            [&](const char *path, const fs::DirectoryEntry &entry) -> Result { /* On Exit Directory */
                /* Check we have a parent directory. */
                const size_t len = strnlen(dst_path_buf, sizeof(dst_path_buf));
                R_UNLESS(len >= 2, fs::ResultInvalidPathFormat());

                /* Find previous separator, add null terminator */
                char *cur = &dst_path_buf[len - 2];
                while (!PathTool::IsSeparator(*cur) && cur > dst_path_buf) {
                    cur--;
                }
                cur[1] = StringTraits::NullTerminator;

                return ResultSuccess();
            },
            [&](const char *path, const fs::DirectoryEntry &entry) -> Result { /* On File */
                return CopyFile(dst_fs, src_fs, dst_path_buf, path, &entry, work_buf, work_buf_size);
            }
        );
    }

    Result EnsureDirectoryExistsRecursively(fs::fsa::IFileSystem *fs, const char *path) {
        /* Normalize the path. */
        char normalized_path[fs::EntryNameLengthMax + 1];
        size_t normalized_path_len;
        R_TRY(PathTool::Normalize(normalized_path, &normalized_path_len, path, sizeof(normalized_path)));

        /* Repeatedly call CreateDirectory on each directory leading to the target. */
        for (size_t i = 1; i < normalized_path_len; i++) {
            /* If we detect a separator, create the directory. */
            if (PathTool::IsSeparator(normalized_path[i])) {
                normalized_path[i] = StringTraits::NullTerminator;
                R_TRY(EnsureDirectoryExists(fs, normalized_path));
                normalized_path[i] = StringTraits::DirectorySeparator;
            }
        }

        /* Call CreateDirectory on the final path. */
        R_TRY(EnsureDirectoryExists(fs, normalized_path));

        return ResultSuccess();
    }

}
