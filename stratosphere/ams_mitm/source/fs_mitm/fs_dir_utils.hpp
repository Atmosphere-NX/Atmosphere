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

#pragma once
#include <switch.h>

#include "fs_path_utils.hpp"
#include "fs_ifilesystem.hpp"

class FsDirUtils {
    private:
        template<typename OnEnterDir, typename OnExitDir, typename OnFile>
        static Result IterateDirectoryRecursivelyInternal(IFileSystem *fs, FsPath &work_path, FsDirectoryEntry *ent_buf, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            Result rc;
            std::unique_ptr<IDirectory> dir;

            /* Open the directory. */
            if (R_FAILED((rc = fs->OpenDirectory(dir, work_path, DirectoryOpenMode_All)))) {
                return rc;
            }

            const size_t parent_len = strnlen(work_path.str, sizeof(work_path.str - 1));

            /* Read and handle entries. */
            while (true) {
                /* Read a single entry. */
                u64 read_count;
                if (R_FAILED((rc = dir->Read(&read_count, ent_buf, 1)))) {
                    return rc;
                }

                /* If we're out of entries, we're done. */
                if (read_count == 0) {
                    break;
                }

                const size_t child_name_len = strnlen(ent_buf->name, sizeof(ent_buf->name) - 1);
                const bool is_dir = ent_buf->type == ENTRYTYPE_DIR;
                const size_t child_path_len = parent_len + child_name_len + (is_dir ? 1 : 0);

                /* Validate child path size. */
                if (child_path_len >= sizeof(work_path.str)) {
                    return ResultFsTooLongPath;
                }

                strncat(work_path.str, ent_buf->name, sizeof(work_path.str) - 1 - parent_len);
                if (is_dir) {
                    /* Enter directory. */
                    if (R_FAILED((rc = on_enter_dir(work_path, ent_buf)))) {
                        return rc;
                    }

                    /* Append separator, recurse. */
                    strncat(work_path.str, "/", sizeof(work_path.str) - 1 - parent_len - child_name_len);
                    if (R_FAILED((rc = IterateDirectoryRecursivelyInternal(fs, work_path, ent_buf, on_enter_dir, on_exit_dir, on_file)))) {
                        return rc;
                    }

                    /* Exit directory. */
                    if (R_FAILED((rc = on_exit_dir(work_path, ent_buf)))) {
                        return rc;
                    }
                } else {
                    /* Call file handler. */
                    if (R_FAILED((rc = on_file(work_path, ent_buf)))) {
                        return rc;
                    }
                }

                /* Restore parent path. */
                work_path.str[parent_len] = 0;
            }

            return ResultSuccess;
        }

    public:
        template<typename OnEnterDir, typename OnExitDir, typename OnFile>
        static Result IterateDirectoryRecursively(IFileSystem *fs, const FsPath &root_path, FsPath &work_path, FsDirectoryEntry *ent_buf, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            /* Ensure valid root path. */
            size_t root_path_len = strnlen(root_path.str, sizeof(root_path.str));
            if (root_path_len > FS_MAX_PATH - 1 || ((root_path_len == FS_MAX_PATH - 1) && root_path.str[root_path_len-1] != '/')) {
                return ResultFsTooLongPath;
            }

            /* Copy path, ensure terminating separator. */
            memcpy(work_path.str, root_path.str, root_path_len);
            if (work_path.str[root_path_len-1] != '/') {
                root_path_len++;
                work_path.str[root_path_len-1] = '/';
            }
            work_path.str[root_path_len] = 0;

            /* Actually iterate. */
            return IterateDirectoryRecursivelyInternal(fs, work_path, ent_buf, on_enter_dir, on_exit_dir, on_file);
        }

        /* Helper for not specifying work path/entry buffer. */
        template<typename OnEnterDir, typename OnExitDir, typename OnFile>
        static Result IterateDirectoryRecursively(IFileSystem *fs, const FsPath &root_path, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            FsDirectoryEntry dir_ent = {0};
            FsPath work_path = {0};
            return IterateDirectoryRecursively(fs, root_path, work_path, &dir_ent, on_enter_dir, on_exit_dir, on_file);
        }

        /* Helper for iterating over the filesystem root. */
        template<typename OnEnterDir, typename OnExitDir, typename OnFile>
        static Result IterateDirectoryRecursively(IFileSystem *fs, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            return IterateDirectoryRecursively(fs, FsPathUtils::RootPath, on_enter_dir, on_exit_dir, on_file);
        }

        /* Copy API. */
        static Result CopyFile(IFileSystem *dst_fs, IFileSystem *src_fs, const FsPath &dst_parent_path, const FsPath &src_path, const FsDirectoryEntry *dir_ent, void *work_buf, size_t work_buf_size);
        static Result CopyFile(IFileSystem *fs, const FsPath &dst_parent_path, const FsPath &src_path, const FsDirectoryEntry *dir_ent, void *work_buf, size_t work_buf_size) {
            return CopyFile(fs, fs, dst_parent_path, src_path, dir_ent, work_buf, work_buf_size);
        }

        static Result CopyDirectoryRecursively(IFileSystem *dst_fs, IFileSystem *src_fs, const FsPath &dst_path, const FsPath &src_path, void *work_buf, size_t work_buf_size);
        static Result CopyDirectoryRecursively(IFileSystem *fs, const FsPath &dst_path, const FsPath &src_path, void *work_buf, size_t work_buf_size) {
            return CopyDirectoryRecursively(fs, fs, dst_path, src_path, work_buf, work_buf_size);
        }

        /* Other Utility. */
        template<typename F>
        static Result RetryUntilTargetNotLocked(F f) {
            const size_t MaxRetries = 10;
            Result rc = ResultSuccess;

            for (size_t i = 0; i < MaxRetries; i++) {
                rc = f();

                if (rc != ResultFsTargetLocked) {
                    break;
                }

                /* If target is locked, wait 100ms and try again. */
                svcSleepThread(100'000'000ul);
            }

            return rc;
        }
};
