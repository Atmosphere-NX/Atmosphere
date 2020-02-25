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
#include <fcntl.h>
#include <sys/iosupport.h>

#include "ncm_fs.hpp"
#include "ncm_path_utils.hpp"

namespace ams::ncm::fs {

    Result OpenFile(FILE** out, const char* path, u32 mode) {
        bool has = false;

        /* Manually check if the file already exists, so it doesn't get created automatically. */
        R_TRY(HasFile(&has, path));
        R_UNLESS(has, ams::fs::ResultPathNotFound());

        const char* fopen_mode = "";

        if (mode & FsOpenMode_Write) {
            fopen_mode = "r+b";
        } else if (mode & FsOpenMode_Read) {
            fopen_mode = "rb";
        } 
        FILE* f = fopen(path, fopen_mode);
        R_UNLESS(f != nullptr, fsdevGetLastResult());

        *out = f;
        return ResultSuccess();
    }

    Result WriteFile(FILE* f, size_t offset, const void* buffer, size_t size, u32 option) {
        R_UNLESS(fseek(f, 0, SEEK_END) == 0, fsdevGetLastResult());
        size_t existing_size = ftell(f);

        R_UNLESS(offset + size <= existing_size, ams::fs::ResultFileExtensionWithoutOpenModeAllowAppend());
        R_UNLESS(fseek(f, offset, SEEK_SET) == 0, fsdevGetLastResult());
        R_UNLESS(fwrite(buffer, 1, size, f) == size, fsdevGetLastResult());

        if (option & FsWriteOption_Flush) {
            fflush(f);
        }

        return ResultSuccess();
    }

    Result ReadFile(FILE* f, size_t offset, void* buffer, size_t size) {
        R_UNLESS(fseek(f, offset, SEEK_SET) == 0, fsdevGetLastResult());
        R_UNLESS(fread(buffer, 1, size, f) == size, fsdevGetLastResult());
        R_UNLESS(!ferror(f), fsdevGetLastResult());
        return ResultSuccess();
    }

    Result HasFile(bool* out, const char* path) {
        struct stat st;

        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            *out = true;
        } else {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ams::fs::ResultPathNotFound) {
                    *out = false;
                }
            } R_END_TRY_CATCH;
        }
    
        return ResultSuccess();
    }

    Result HasDirectory(bool* out, const char* path) {
        struct stat st;

        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            *out = true;
        } else {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ams::fs::ResultPathNotFound) {
                    *out = false;
                }
            } R_END_TRY_CATCH;
        }
    
        return ResultSuccess();
    }

    Result CheckContentStorageDirectoriesExist(const char* root_path) {
        char content_root[FS_MAX_PATH] = {0};
        char placeholder_root[FS_MAX_PATH] = {0};

        bool has_root = false;
        R_TRY(HasDirectory(&has_root, root_path));
        R_UNLESS(has_root, ncm::ResultContentStorageBaseNotFound());
        
        path::GetContentRootPath(content_root, root_path);
        bool has_content_root = false;
        R_TRY(HasDirectory(&has_content_root, content_root));
        R_UNLESS(has_content_root, ncm::ResultInvalidContentStorageBase());

        path::GetPlaceHolderRootPath(placeholder_root, root_path);
        bool has_placeholder_root = false;
        R_TRY(HasDirectory(&has_placeholder_root, placeholder_root));
        R_UNLESS(has_placeholder_root, ncm::ResultInvalidContentStorageBase());

        return ResultSuccess();
    }

    Result EnsureContentAndPlaceHolderRoot(const char* root_path) {
        char content_root[FS_MAX_PATH] = {0};
        char placeholder_root[FS_MAX_PATH] = {0};

        path::GetContentRootPath(content_root, root_path);
        R_TRY(EnsureDirectoryRecursively(content_root));
        path::GetPlaceHolderRootPath(placeholder_root, root_path);
        R_TRY(EnsureDirectoryRecursively(placeholder_root));

        return ResultSuccess();
    }

    Result EnsureDirectoryRecursively(const char* dir_path) {
        R_TRY(EnsureRecursively(dir_path));
        if (mkdir(dir_path, S_IRWXU) == -1) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CATCH(ams::fs::ResultPathAlreadyExists) {
                    /* If the path already exists, that's okay. Anything else is an error. */
                }
            } R_END_TRY_CATCH;
        }
        return ResultSuccess();
    }

    Result EnsureRecursively(const char* path) {
        R_UNLESS(path, ams::fs::ResultNullptrArgument());

        size_t path_len = strlen(path);
        char working_path_buf[FS_MAX_PATH] = {0};

        R_UNLESS(path_len + 1 < FS_MAX_PATH, ncm::ResultAllocationFailed());
        strncpy(working_path_buf + 1, path, FS_MAX_PATH-1);

        if (path_len != 0) {
            for (size_t i = 0; i < path_len; i++) {
                if (i != 0 && working_path_buf[i + 1] == '/' && working_path_buf[i] != ':') {
                    /* Temporarily make the path terminate before the '/' */
                    working_path_buf[i + 1] = 0;
                    if (mkdir(working_path_buf + 1, S_IRWXU) == -1) {
                        R_TRY_CATCH(fsdevGetLastResult()) {
                            R_CATCH(ams::fs::ResultPathAlreadyExists) {
                                /* If the path already exists, that's okay. Anything else is an error. */
                            }
                        } R_END_TRY_CATCH;
                    }

                    /* Restore the path to its former state */
                    working_path_buf[i + 1] = '/';
                }
            }
        }

        return ResultSuccess();
    }

    Result EnsureParentDirectoryRecursively(const char* path) {
        return EnsureRecursively(path);
    }

    Result GetGameCardHandle(FsGameCardHandle* out_handle) {
        FsDeviceOperator devop;
        R_TRY(fsOpenDeviceOperator(&devop));

        /* Ensure we close even on early return. */
        ON_SCOPE_EXIT { fsDeviceOperatorClose(&devop); };

        R_TRY(fsDeviceOperatorGetGameCardHandle(&devop, out_handle));
        return ResultSuccess();
    }

    static u32 g_mount_index = 0;
    static os::Mutex g_mount_index_lock;

    MountName CreateUniqueMountName() {
        std::scoped_lock lk(g_mount_index_lock);
        MountName mount_name;
        g_mount_index++;
        snprintf(mount_name.name, sizeof(MountName), "@ncm%08x", g_mount_index);
        return mount_name;
    }

    Result GetMountNameFromPath(MountName* mount_name, const char* path) {
        const char* unqual_path = strchr(path, ':');

        /* We should be given a qualified path. */
        R_UNLESS(unqual_path, ams::fs::ResultInvalidMountName());
        R_UNLESS(unqual_path <= path + 0xf, ams::fs::ResultInvalidMountName());

        strncpy(mount_name->name, path, unqual_path - path);
        return ResultSuccess();
    }

    Result MountSystemSaveData(const char* mount_point, FsSaveDataSpaceId space_id, u64 save_id) {
        R_UNLESS(mount_point, ams::fs::ResultNullptrArgument());
        
        FsSaveDataAttribute save = {
            .system_save_data_id = save_id,
            .save_data_type = FsSaveDataType_System,
        };

        FsFileSystem fs;
        R_TRY(fsOpenSaveDataFileSystemBySystemSaveDataId(&fs, space_id, &save));
        AMS_ABORT_UNLESS(fsdevMountDevice(mount_point, fs) != -1);

        return ResultSuccess();
    }

    constexpr const char* SystemContentMountName = "@SystemContent";
    constexpr const char* UserContentMountName = "@UserContent";
    constexpr const char* SdCardContentMountName = "@SdCardContent";
    constexpr const char* GameCardMountNameBase = "@Gc";

    constexpr const char* GameCardPartitionLetters[3] = { "U", "N", "S" };

    /* Maps mount names to their common mount names. */
    std::map<std::string, std::string> g_mount_content_storage;

    Result MountContentStorage(const char* mount_point, FsContentStorageId id) {
        R_UNLESS(mount_point, ams::fs::ResultNullptrArgument());

        FsFileSystem fs;
        R_TRY(fsOpenContentStorageFileSystem(&fs, id));
        AMS_ABORT_UNLESS(fsdevMountDevice(mount_point, fs) != -1);

        switch (id) {
            case FsContentStorageId_System:
                g_mount_content_storage[mount_point] = SystemContentMountName;
                break;
            
            case FsContentStorageId_User:
                g_mount_content_storage[mount_point] = UserContentMountName;
                break;

            case FsContentStorageId_SdCard:
                g_mount_content_storage[mount_point] = SdCardContentMountName;
                break;

            default:
                std::abort();
        };
        return ResultSuccess();
    }

    Result MountGameCardPartition(const char* mount_point, const FsGameCardHandle handle, FsGameCardPartition partition) {
        AMS_ABORT_UNLESS(partition <= 2);
        
        FsFileSystem fs;
        R_TRY(fsOpenGameCardFileSystem(&fs, &handle, partition));
        AMS_ABORT_UNLESS(fsdevMountDevice(mount_point, fs) != -1);

        MountName mount = {0};
        snprintf(mount.name, sizeof(MountName), "%s%s%08x", GameCardMountNameBase, GameCardPartitionLetters[partition], handle.value);
        g_mount_content_storage[mount_point] = mount.name;
        return ResultSuccess();
    }

    Result Unmount(const char* mount_point) {
        R_UNLESS(mount_point, ams::fs::ResultNullptrArgument());
        /* Erase any content storage mappings which may potentially exist. */
        g_mount_content_storage.erase(mount_point);
        AMS_ABORT_UNLESS(fsdevUnmountDevice(mount_point) != -1);
        return ResultSuccess();
    }

    Result ConvertToFsCommonPath(char* out_common_path, size_t out_len, const char* path) {
        R_UNLESS(out_common_path, ams::fs::ResultNullptrArgument());
        R_UNLESS(path, ams::fs::ResultNullptrArgument());

        MountName mount_name = {0};
        R_TRY(GetMountNameFromPath(&mount_name, path));
        R_UNLESS(fsdevGetDeviceFileSystem(mount_name.name), ams::fs::ResultNotMounted());
        R_UNLESS(g_mount_content_storage.find(mount_name.name) != g_mount_content_storage.end(), ams::fs::ResultNotMounted());

        char translated_path[FS_MAX_PATH] = {0};
        std::string common_mount_name = g_mount_content_storage[mount_name.name];

        AMS_ABORT_UNLESS(fsdevTranslatePath(path, NULL, translated_path) != -1);
        snprintf(out_common_path, out_len, "%s:%s", common_mount_name.c_str(), translated_path);
        return ResultSuccess();
    }

    Result GetSaveDataFlags(u32* out_flags, u64 save_id) {
        FsSaveDataExtraData extra_data;
        
        R_TRY(fsReadSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), save_id));
        *out_flags = extra_data.flags;
        return ResultSuccess();
    }

    Result SetSaveDataFlags(u64 save_id, FsSaveDataSpaceId space_id, u32 flags) {
        FsSaveDataExtraData extra_data;
        
        R_TRY(fsReadSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), save_id));
        extra_data.flags = flags;
        R_TRY(fsWriteSaveDataFileSystemExtraData(&extra_data, sizeof(FsSaveDataExtraData), space_id, save_id));
        return ResultSuccess();
    }

}
