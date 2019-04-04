/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <cstring>
#include <cstdlib>
#include <switch.h>

#include "fs_dir_utils.hpp"
#include "fs_ifile.hpp"

Result FsDirUtils::CopyFile(IFileSystem *dst_fs, IFileSystem *src_fs, const FsPath &dst_parent_path, const FsPath &src_path, const FsDirectoryEntry *dir_ent, void *work_buf, size_t work_buf_size) {
    Result rc;
    std::unique_ptr<IFile> src_file;
    std::unique_ptr<IFile> dst_file;
    const u64 file_size = dir_ent->fileSize;

    /* Open source file for reading. */
    if (R_FAILED((rc = src_fs->OpenFile(src_file, src_path, OpenMode_Read)))) {
        return rc;
    }

    /* Create and open destination file. */
    {
        FsPath dst_path;
        if (static_cast<size_t>(snprintf(dst_path.str, sizeof(dst_path.str), "%s%s", dst_parent_path.str, dir_ent->name)) >= sizeof(dst_path)) {
            /* TODO: Error code? N aborts here. */
            std::abort();
        }

        if (R_FAILED((rc = dst_fs->CreateFile(dst_path, file_size)))) {
            return rc;
        }
        if (R_FAILED((rc = dst_fs->OpenFile(dst_file, dst_path, OpenMode_Write)))) {
            return rc;
        }
    }

    /* Read/Write work_buf_size chunks. */
    u64 offset = 0;
    while (offset < file_size) {
        u64 read_size;
        if (R_FAILED((rc = src_file->Read(&read_size, offset, work_buf, work_buf_size)))) {
            return rc;
        }
        if (R_FAILED((rc = dst_file->Write(offset, work_buf, read_size)))) {
            return rc;
        }

        offset += read_size;
    }
    
    return ResultSuccess;
}

Result FsDirUtils::CopyDirectoryRecursively(IFileSystem *dst_fs, IFileSystem *src_fs, const FsPath &dst_path, const FsPath &src_path, void *work_buf, size_t work_buf_size) {
    FsPath work_path = dst_path;

    return IterateDirectoryRecursively(src_fs, src_path, 
        [&](const FsPath &path, const FsDirectoryEntry *dir_ent) -> Result { /* On Enter Directory */
            /* Update path, create new dir. */
            strncat(work_path.str, dir_ent->name, sizeof(work_path) - strnlen(work_path.str, sizeof(work_path) - 1) - 1);
            strncat(work_path.str, "/", sizeof(work_path) - strnlen(work_path.str, sizeof(work_path) - 1) - 1);
            return dst_fs->CreateDirectory(work_path);
        }, 
        [&](const FsPath &path, const FsDirectoryEntry *dir_ent) -> Result { /* On Exit Directory */
            /* Check we have a parent directory. */
            const size_t work_path_len = strnlen(work_path.str, sizeof(work_path));
            if (work_path_len < 2) {
                return ResultFsInvalidPathFormat;
            }

            /* Find previous separator, add NULL terminator */
            char *p = &work_path.str[work_path_len - 2];
            while (*p != '/' && p > work_path.str) {
                p--;
            }
            p[1] = 0;

            return ResultSuccess;
        }, 
        [&](const FsPath &path, const FsDirectoryEntry *dir_ent) -> Result { /* On File */
            /* Just copy the file to the new fs. */
            return CopyFile(dst_fs, src_fs, work_path, path, dir_ent, work_buf, work_buf_size);
        });
}