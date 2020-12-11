/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/fs/fs_path_utils.hpp>

namespace ams::fssystem {

    namespace impl {

        /* Iteration. */
        template<typename OnEnterDir, typename OnExitDir, typename OnFile>
        Result IterateDirectoryRecursivelyImpl(fs::fsa::IFileSystem *fs, char *work_path, size_t work_path_size, fs::DirectoryEntry *dir_ent, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
            /* Open the directory. */
            std::unique_ptr<fs::fsa::IDirectory> dir;
            R_TRY(fs->OpenDirectory(&dir, work_path, fs::OpenDirectoryMode_All));

            const size_t parent_len = strnlen(work_path, work_path_size - 1);

            /* Read and handle entries. */
            while (true) {
                /* Read a single entry. */
                s64 read_count = 0;
                R_TRY(dir->Read(&read_count, dir_ent, 1));

                /* If we're out of entries, we're done. */
                if (read_count == 0) {
                    break;
                }

                /* Validate child path size. */
                const size_t child_name_len = strnlen(dir_ent->name, sizeof(dir_ent->name) - 1);
                const bool is_dir = dir_ent->type == fs::DirectoryEntryType_Directory;
                const size_t separator_size = is_dir ? 1 : 0;
                R_UNLESS(parent_len + child_name_len + separator_size < work_path_size, fs::ResultTooLongPath());

                /* Set child path. */
                std::strncat(work_path, dir_ent->name, work_path_size - parent_len - 1);
                {
                    if (is_dir) {
                        /* Enter directory. */
                        R_TRY(on_enter_dir(work_path, *dir_ent));

                        /* Append separator, recurse. */
                        std::strncat(work_path, "/", work_path_size - (parent_len + child_name_len) - 1);
                        R_TRY(IterateDirectoryRecursivelyImpl(fs, work_path, work_path_size, dir_ent, on_enter_dir, on_exit_dir, on_file));

                        /* Exit directory. */
                        R_TRY(on_exit_dir(work_path, *dir_ent));
                    } else {
                        /* Call file handler. */
                        R_TRY(on_file(work_path, *dir_ent));
                    }
                }

                /* Restore parent path. */
                work_path[parent_len] = fs::StringTraits::NullTerminator;
            }

            return ResultSuccess();
        }

        /* TODO: Cleanup. */

    }

    /* Iteration API */
    template<typename OnEnterDir, typename OnExitDir, typename OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, const char *root_path, char *work_path, size_t work_path_size, fs::DirectoryEntry *dir_ent_buf, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        AMS_ABORT_UNLESS(work_path_size >= fs::EntryNameLengthMax + 1);

        /* Get size of the root path. */
        size_t root_path_len = strnlen(root_path, fs::EntryNameLengthMax + 1);
        R_UNLESS(root_path_len <= fs::EntryNameLengthMax, fs::ResultTooLongPath());

        /* Copy root path in, add a / if necessary. */
        std::memcpy(work_path, root_path, root_path_len);
        if (!fs::PathNormalizer::IsSeparator(work_path[root_path_len - 1])) {
            work_path[root_path_len++] = fs::StringTraits::DirectorySeparator;
        }

        /* Make sure the result path is still valid. */
        R_UNLESS(root_path_len <= fs::EntryNameLengthMax, fs::ResultTooLongPath());
        work_path[root_path_len] = fs::StringTraits::NullTerminator;

        return impl::IterateDirectoryRecursivelyImpl(fs, work_path, work_path_size, dir_ent_buf, on_enter_dir, on_exit_dir, on_file);
    }

    template<typename OnEnterDir, typename OnExitDir, typename OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, const char *root_path, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        fs::DirectoryEntry dir_entry = {};
        char work_path[fs::EntryNameLengthMax + 1] = {};
        return IterateDirectoryRecursively(fs, root_path, work_path, sizeof(work_path), &dir_entry, on_enter_dir, on_exit_dir, on_file);
    }

    template<typename OnEnterDir, typename OnExitDir, typename OnFile>
    Result IterateDirectoryRecursively(fs::fsa::IFileSystem *fs, OnEnterDir on_enter_dir, OnExitDir on_exit_dir, OnFile on_file) {
        return IterateDirectoryRecursively(fs, fs::PathNormalizer::RootPath, on_enter_dir, on_exit_dir, on_file);
    }

    /* TODO: Cleanup API */

    /* Copy API. */
    Result CopyFile(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const char *dst_parent_path, const char *src_path, const fs::DirectoryEntry *dir_ent, void *work_buf, size_t work_buf_size);
    NX_INLINE Result CopyFile(fs::fsa::IFileSystem *fs, const char *dst_parent_path, const char *src_path, const fs::DirectoryEntry *dir_ent, void *work_buf, size_t work_buf_size) {
        return CopyFile(fs, fs, dst_parent_path, src_path, dir_ent, work_buf, work_buf_size);
    }

    Result CopyDirectoryRecursively(fs::fsa::IFileSystem *dst_fs, fs::fsa::IFileSystem *src_fs, const char *dst_path, const char *src_path, void *work_buf, size_t work_buf_size);
    NX_INLINE Result CopyDirectoryRecursively(fs::fsa::IFileSystem *fs, const char *dst_path, const char *src_path, void *work_buf, size_t work_buf_size) {
        return CopyDirectoryRecursively(fs, fs, dst_path, src_path, work_buf, work_buf_size);
    }

    /* Semaphore adapter class. */
    class SemaphoreAdapter : public os::Semaphore {
        public:
            SemaphoreAdapter(int c, int mc) : os::Semaphore(c, mc) { /* ... */ }

            bool try_lock() {
                return this->TryAcquire();
            }

            void unlock() {
                this->Release();
            }
    };

    /* Other utility. */
    Result HasFile(bool *out, fs::fsa::IFileSystem *fs, const char *path);
    Result HasDirectory(bool *out, fs::fsa::IFileSystem *fs, const char *path);

    Result EnsureDirectoryRecursively(fs::fsa::IFileSystem *fs, const char *path);
    Result EnsureParentDirectoryRecursively(fs::fsa::IFileSystem *fs, const char *path);

    template<typename F>
    NX_INLINE Result RetryFinitelyForTargetLocked(F f) {
        /* Retry up to 10 times, 100ms between retries. */
        constexpr s32 MaxRetryCount = 10;
        constexpr u64 RetryWaitTime = 100'000'000ul;

        s32 remaining_retries = MaxRetryCount;
        while (true) {
            R_TRY_CATCH(f()) {
                R_CATCH(fs::ResultTargetLocked) {
                    R_UNLESS(remaining_retries > 0, fs::ResultTargetLocked());

                    remaining_retries--;
                    svcSleepThread(RetryWaitTime);
                    continue;
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }
    }

    void AddCounter(void *counter, size_t counter_size, u64 value);

}
