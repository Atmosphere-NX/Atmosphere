/*
 * Copyright (c) 2019 Adubbz
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
#include <stratosphere.hpp>
#include <sys/dirent.h>

namespace ams::ncm::fs {

    Result OpenFile(FILE** out, const char* path, u32 mode);
    Result WriteFile(FILE* f, size_t offset, const void* buffer, size_t size, u32 option);
    Result ReadFile(FILE* f, size_t offset, void* buffer, size_t size);

    Result HasFile(bool* out, const char* path);
    Result HasDirectory(bool* out, const char* path);

    Result CheckContentStorageDirectoriesExist(const char* root_path);
    Result EnsureContentAndPlaceHolderRoot(const char* root_path);

    Result EnsureDirectoryRecursively(const char* dir_path);
    Result EnsureRecursively(const char* path);
    /* Create all parent directories for a file path */
    Result EnsureParentDirectoryRecursively(const char* path);

    Result GetGameCardHandle(FsGameCardHandle* out_handle);

    MountName CreateUniqueMountName();
    Result GetMountNameFromPath(MountName* mount_name, const char* path);

    Result MountSystemSaveData(const char* mount_point, FsSaveDataSpaceId space_id, u64 save_id);
    Result MountContentStorage(const char* mount_point, FsContentStorageId id);
    Result MountGameCardPartition(const char* mount_point, const FsGameCardHandle handle, FsGameCardPartition partition);
    Result Unmount(const char* mount_point);
    Result ConvertToFsCommonPath(char* out_common_path, size_t len, const char* path);

    Result GetSaveDataFlags(u32* out_flags, u64 save_id);
    Result SetSaveDataFlags(u64 save_id, FsSaveDataSpaceId space_id, u32 flags);

    template<typename F>
    Result TraverseDirectory(bool* out_should_continue, const char* root_path, int max_level, F f) {
        DIR *dir;
        struct dirent* dir_entry = nullptr;
        R_UNLESS(max_level >= 1, ResultSuccess());
        
        bool retry_dir_read = true;
        while (retry_dir_read) {
            retry_dir_read = false;

            R_UNLESS((dir = opendir(root_path)) != nullptr, fsdevGetLastResult());
            ON_SCOPE_EXIT { closedir(dir); };

            while ((dir_entry = readdir(dir)) != nullptr) {
                if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
                    continue;
                }

                char current_path[FS_MAX_PATH];
                AMS_ABORT_UNLESS(snprintf(current_path, FS_MAX_PATH-1, "%s/%s", root_path, dir_entry->d_name) >= 0);
                
                bool should_continue = true;
                bool should_retry_dir_read = false;
                R_TRY(f(&should_continue, &should_retry_dir_read, current_path, dir_entry));

                /* If the provided function wishes to terminate immediately, we should respect it. */
                if (!should_continue) {
                    *out_should_continue = false;
                    return ResultSuccess();
                }
                if (should_retry_dir_read) {
                    retry_dir_read = true;
                    break;
                }

                if (dir_entry->d_type == DT_DIR) {
                    R_TRY(TraverseDirectory(&should_continue, current_path, max_level-1, f));

                    if (!should_continue) {
                        *out_should_continue = false;
                        return ResultSuccess();
                    }
                }
            }
        }

        return ResultSuccess();
    };

    template<typename F>
    Result TraverseDirectory(const char* root_path, int max_level, F f) {
        bool should_continue = false;
        return TraverseDirectory(&should_continue, root_path, max_level, f);
    }

}
