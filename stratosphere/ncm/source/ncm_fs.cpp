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

#include <map>

#include "ncm_fs.hpp"
#include "ncm_path_utils.hpp"

namespace sts::ncm {

    Result HasFile(bool* out, const char* path) {
        errno = 0;
        struct stat st;

        *out = false;
        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            *out = true;
        }

        if (errno != 0) {
            return fsdevGetLastResult();
        }
    
        return ResultSuccess;
    }

    Result HasDirectory(bool* out, const char* path) {
        errno = 0;
        struct stat st;

        *out = false;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            *out = true;
        }

        if (errno != 0) {
            return fsdevGetLastResult();
        }
    
        return ResultSuccess;
    }

    Result CheckContentStorageDirectoriesExist(const char* root_path) {
        char content_root[FS_MAX_PATH] = {0};
        char placeholder_root[FS_MAX_PATH] = {0};

        errno = 0;

        bool has_root = false;
        R_TRY(HasDirectory(&has_root, root_path));
        if (!has_root) {
            return ResultNcmStorageRootNotFound;
        }
        
        path::GetContentRootPath(content_root, root_path);

        bool has_content_root = false;
        R_TRY(HasDirectory(&has_content_root, content_root));
        if (!has_content_root) {
            return ResultNcmStoragePathNotFound;
        }

        path::GetPlaceHolderRootPath(placeholder_root, root_path);

        bool has_placeholder_root = false;
        R_TRY(HasDirectory(&has_placeholder_root, placeholder_root));
        if (!has_placeholder_root) {
            return ResultNcmStoragePathNotFound;
        }

        return ResultSuccess;
    }

    Result EnsureContentAndPlaceHolderRoot(const char* root_path) {
        char content_root[FS_MAX_PATH] = {0};
        char placeholder_root[FS_MAX_PATH] = {0};

        path::GetContentRootPath(content_root, root_path);
        R_TRY(EnsureDirectoryRecursively(content_root));
        path::GetPlaceHolderRootPath(placeholder_root, root_path);
        R_TRY(EnsureDirectoryRecursively(placeholder_root));

        return ResultSuccess;
    }

    Result EnsureDirectoryRecursively(const char* dir_path) {
        return EnsureRecursively(dir_path, true);
    }

    Result EnsureRecursively(const char* path, bool is_dir) {
        if (!path) {
            return ResultFsNullptrArgument;
        }

        size_t path_len = strlen(path);
        char working_path_buf[FS_MAX_PATH] = {0};

        if (path_len + 1 < FS_MAX_PATH) {
            strncpy(working_path_buf + 1, path, FS_MAX_PATH-1);

            if (path_len != 0) {
                for (size_t i = 0; i < path_len; i++) {
                    if (i != 0 && working_path_buf[i + 1] == '/' && working_path_buf[i] != ':') {
                        /* Wipe the errno to prevent cross-contamination */
                        errno = 0;
                        /* Temporarily make the path terminate before the '/' */
                        working_path_buf[i + 1] = 0;
                        mkdir(working_path_buf + 1, S_IRWXU);

                        if (errno != 0) {
                            R_TRY_CATCH(fsdevGetLastResult()) {
                                R_CATCH(ResultFsPathAlreadyExists) {
                                    /* If the path already exists, that's okay. Anything else is an error. */
                                }
                            } R_END_TRY_CATCH;
                        }

                        /* Restore the path to its former state */
                        working_path_buf[i + 1] = '/';
                    }
                }
            }
        } else {
            return ResultNcmAllocationFailed;
        }

        return ResultSuccess;
    }

    Result EnsureParentDirectoryRecursively(const char* path) {
        return EnsureRecursively(path, false);
    }

    Result GetGameCardHandle(FsGameCardHandle* out_handle) {
        FsDeviceOperator devop;
        R_TRY(fsOpenDeviceOperator(&devop));

        /* Ensure we close even on early return. */
        ON_SCOPE_EXIT { fsDeviceOperatorClose(&devop); };

        R_TRY(fsDeviceOperatorGetGameCardHandle(&devop, out_handle));
        return ResultSuccess;
    }

    static u32 g_mount_index = 0;
    static HosMutex g_mount_index_lock;

    MountName CreateUniqueMountName() {
        std::scoped_lock<HosMutex> lk(g_mount_index_lock);
        MountName mount_name;
        g_mount_index++;
        snprintf(mount_name.name, sizeof(MountName), "@ncm%08x", g_mount_index);
        return mount_name;
    }

    Result MountSystemSaveData(const char* mount_point, FsSaveDataSpaceId space_id, u64 save_id) {
        if (!mount_point) {
            return ResultFsNullptrArgument;
        }
        
        FsSave save = {
            .saveID = save_id,
            .saveDataType = FsSaveDataType_SystemSaveData,
        };

        FsFileSystem fs;
        R_TRY(fsMountSystemSaveData(&fs, space_id, &save));

        if (fsdevMountDevice(mount_point, fs) == -1) {
            std::abort();
        }

        return ResultSuccess;
    }

    std::map<std::string, FsContentStorageId> g_mount_content_storage;

    Result MountContentStorage(const char* mount_point, FsContentStorageId id) {
        if (!mount_point) {
            return ResultFsNullptrArgument;
        }

        FsFileSystem fs;
        R_TRY(fsOpenContentStorageFileSystem(&fs, id));

        if (fsdevMountDevice(mount_point, fs) == -1) {
            std::abort();
        }

        g_mount_content_storage[mount_point] = id;
        return ResultSuccess;
    }

    Result MountGameCardPartition(const char* mount_point, const FsGameCardHandle handle, FsGameCardPartiton partition) {
        FsFileSystem fs;
        R_TRY(fsOpenGameCardFileSystem(&fs, &handle, partition));

        if (fsdevMountDevice(mount_point, fs) == -1) {
            std::abort();
        }

        return ResultSuccess;
    }

    Result Unmount(const char* mount_point) {
        if (!mount_point) {
            return ResultFsNullptrArgument;
        }

        /* Erase any content storage mappings which may potentially exist. */
        g_mount_content_storage.erase(mount_point);

        if (fsdevUnmountDevice(mount_point) == -1) {
            std::abort();
        }

        return ResultSuccess;
    }

    constexpr const char* SystemContentMountName = "@SystemContent";
    constexpr const char* UserContentMountName = "@UserContent";
    constexpr const char* SdCardContentMountName = "@SdCardContent";

    Result ConvertToFsCommonPath(char* out_common_path, size_t out_len, const char* path) {
        if (!out_common_path || !path) {
            return ResultFsNullptrArgument;
        }

        MountName mount_name = {0};
        const char* unqual_path = strchr(path, ':');

        /* We should be given a qualified path. */
        if (!unqual_path || unqual_path > path + 0xf) {
            return ResultFsUnqualifiedPath;
        }

        strncpy(mount_name.name, path, unqual_path - path);
    
        if (!fsdevGetDeviceFileSystem(mount_name.name) || g_mount_content_storage.find(mount_name.name) == g_mount_content_storage.end()) {
            return ResultFsMountNameNotFound;
        }

        FsContentStorageId content_storage_id = g_mount_content_storage[mount_name.name];
        char translated_path[FS_MAX_PATH] = {0};
        const char* common_mount_name;

        switch (content_storage_id) {
            case FS_CONTENTSTORAGEID_NandSystem:
                common_mount_name = SystemContentMountName;
                break;
            
            case FS_CONTENTSTORAGEID_NandUser:
                common_mount_name = UserContentMountName;
                break;

            case FS_CONTENTSTORAGEID_SdCard:
                common_mount_name = SdCardContentMountName;
                break;

            default:
                std::abort();
        };

        if (fsdevTranslatePath(path, NULL, translated_path) == -1) {
            std::abort();
        }

        snprintf(out_common_path, out_len, "%s:/%s", common_mount_name, translated_path);
        return ResultSuccess;
    }

    Result GetSaveDataFlags(u32* out_flags, u64 save_id) {
        FsSaveDataExtraData extra_data;
        
        R_TRY(fsReadSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), save_id));
        *out_flags = extra_data.flags;
        return ResultSuccess;
    }

    Result SetSaveDataFlags(u64 save_id, FsSaveDataSpaceId space_id, u32 flags) {
        FsSaveDataExtraData extra_data;
        
        R_TRY(fsReadSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), save_id));
        extra_data.flags = flags;
        R_TRY(fsWriteSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), space_id, save_id));
        return ResultSuccess;
    }

}