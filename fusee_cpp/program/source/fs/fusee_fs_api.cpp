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
#include <exosphere.hpp>
#include "../fatfs/ff.h"
#include "fusee_fs_api.hpp"

namespace ams::fs {

    static_assert(sizeof(DirectoryEntry) == sizeof(FILINFO));

    namespace {

        constexpr size_t MaxFiles = 8;
        constexpr size_t MaxDirectories = 2;

        constinit bool g_is_sd_mounted = false;
        constinit bool g_is_sys_mounted = false;

        alignas(0x10) constinit FATFS g_sd_fs = {};
        alignas(0x10) constinit FATFS g_sys_fs = {};

        alignas(0x10) constinit FIL g_files[MaxFiles] = {};
        alignas(0x10) constinit DIR g_dirs[MaxDirectories] = {};
        constinit bool g_files_opened[MaxFiles] = {};
        constinit bool g_dirs_opened[MaxFiles] = {};
        constinit int g_open_modes[MaxFiles] = {};

        Result TranslateFatFsError(FRESULT res) {
            switch (res) {
                case FR_OK:
                    return ResultSuccess();
                case FR_DISK_ERR:
                    return fs::ResultMmcAccessFailed();
                case FR_INT_ERR:
                    return fs::ResultPreconditionViolation();
                case FR_NOT_READY:
                    return fs::ResultMmcAccessFailed();
                case FR_NO_FILE:
                    return fs::ResultPathNotFound();
                case FR_NO_PATH:
                    return fs::ResultPathNotFound();
                case FR_INVALID_NAME:
                    return fs::ResultInvalidPath();
                case FR_DENIED:
                    return fs::ResultPermissionDenied();
                case FR_EXIST:
                    return fs::ResultPathAlreadyExists();
                case FR_INVALID_OBJECT:
                    return fs::ResultInvalidArgument();
                case FR_WRITE_PROTECTED:
                    return fs::ResultWriteNotPermitted();
                case FR_INVALID_DRIVE:
                    return fs::ResultInvalidMountName();
                case FR_NOT_ENABLED:
                    return fs::ResultInvalidMountName(); /* BAD/TODO */
                case FR_NO_FILESYSTEM:
                    return fs::ResultInvalidMountName(); /* BAD/TODO */
                case FR_TIMEOUT:
                    return fs::ResultTargetLocked(); /* BAD/TODO */
                case FR_LOCKED:
                    return fs::ResultTargetLocked();
                case FR_NOT_ENOUGH_CORE:
                    return fs::ResultPreconditionViolation(); /* BAD/TODO */
                case FR_TOO_MANY_OPEN_FILES:
                    return fs::ResultPreconditionViolation(); /* BAD/TODO */
                case FR_INVALID_PARAMETER:
                    return fs::ResultInvalidArgument();
                default:
                    return fs::ResultInternal();
            }
        }

        int TranslateToFatFsMode(int mode) {
            int fmode = FA_OPEN_EXISTING;
            if ((mode & OpenMode_Read) != 0) {
                fmode |= FA_READ;
            }
            if ((mode & OpenMode_Write) != 0) {
                fmode |= FA_WRITE;
            }
            if ((mode & OpenMode_AllowAppend) != 0) {
                fmode |= FA_OPEN_APPEND;
            }
            return fmode;
        }

        FIL *GetInternalFile(FileHandle handle) {
            return static_cast<FIL *>(handle._handle);
        }

        DIR *GetInternalDirectory(DirectoryHandle handle) {
            return static_cast<DIR *>(handle._handle);
        }

        ALWAYS_INLINE size_t GetFileIndex(FIL *fp) {
            const size_t file_index = (fp - g_files);
            AMS_ASSERT(file_index < MaxFiles);
            return file_index;
        }

        ALWAYS_INLINE size_t GetDirectoryIndex(DIR *dp) {
            const size_t dir_index = (dp - g_dirs);
            AMS_ASSERT(dir_index < MaxDirectories);
            return dir_index;
        }

    }

    bool MountSdCard() {
        AMS_ASSERT(!g_is_sd_mounted);
        g_is_sd_mounted = f_mount(std::addressof(g_sd_fs), "sdmc:", 1) == FR_OK;
        return g_is_sd_mounted;
    }

    void UnmountSdCard() {
        AMS_ASSERT(g_is_sd_mounted);
        f_unmount("sdmc:");
        g_is_sd_mounted = false;
    }

    bool MountSystem() {
        AMS_ASSERT(!g_is_sys_mounted);
        g_is_sys_mounted = f_mount(std::addressof(g_sys_fs), "sys:", 1) == FR_OK;
        return g_is_sys_mounted;
    }

    void UnmountSystem() {
        AMS_ASSERT(g_is_sys_mounted);
        f_unmount("sys:");
        g_is_sys_mounted = false;
    }

    Result GetEntryType(DirectoryEntryType *out_entry_type, bool *out_archive, const char *path) {
        /* Get the file info. */
        FILINFO info;
        R_TRY(TranslateFatFsError(f_stat(path, std::addressof(info))));

        /* Handle the file. */
        *out_entry_type = (info.fattrib & AM_DIR) ? DirectoryEntryType_Directory : DirectoryEntryType_File;
        *out_archive    = (info.fattrib & AM_ARC);

        return ResultSuccess();
    }

    Result CreateFile(const char *path, s64 size) {
        /* Create the file. */
        FIL fp;
        R_TRY(TranslateFatFsError(f_open(std::addressof(fp), path, FA_CREATE_NEW | FA_READ | FA_WRITE)));

        /* Ensure that we close the file when we're done with it. */
        ON_SCOPE_EXIT { f_close(std::addressof(fp)); };

        /* Expand the file. */
        R_TRY(TranslateFatFsError(f_expand(std::addressof(fp), size, 1)));

        return ResultSuccess();
    }

    Result CreateDirectory(const char *path) {
        return TranslateFatFsError(f_mkdir(path));
    }

    Result OpenFile(FileHandle *out_file, const char *path, int mode) {
        /* Find a free file. */
        for (size_t i = 0; i < MaxFiles; ++i) {
            if (!g_files_opened[i]) {
                /* Open the file. */
                FIL *fp = std::addressof(g_files[i]);
                R_TRY(TranslateFatFsError(f_open(fp, path, TranslateToFatFsMode(mode))));

                /* Set the output. */
                out_file->_handle = fp;
                g_files_opened[i] = true;
                g_open_modes[i] = mode;
                return ResultSuccess();
            }
        }
        return fs::ResultOpenCountLimit();
    }

    Result OpenDirectory(DirectoryHandle *out_dir, const char *path) {
        /* Find a free directory. */
        for (size_t i = 0; i < MaxDirectories; ++i) {
            if (!g_dirs_opened[i]) {
                /* Open the file. */
                DIR *dp = std::addressof(g_dirs[i]);
                R_TRY(TranslateFatFsError(f_opendir(dp, path)));

                /* Set the output. */
                out_dir->_handle = dp;
                g_dirs_opened[i] = true;
                return ResultSuccess();
            }
        }
        return fs::ResultOpenCountLimit();
    }

    Result ReadDirectory(s64 *out_count, DirectoryEntry *out_entries, DirectoryHandle handle, s64 max_entries) {
        DIR * const dp = GetInternalDirectory(handle);

        s64 count = 0;
        while (count < max_entries) {
            R_TRY(TranslateFatFsError(f_readdir(dp, reinterpret_cast<FILINFO *>(out_entries + count))));

            if (out_entries[count].file_name[0] == '\x00') {
                break;
            }

            ++count;
        }

        *out_count = count;
        return ResultSuccess();
    }

    void CloseDirectory(DirectoryHandle handle) {
        const size_t index = GetDirectoryIndex(GetInternalDirectory(handle));
        f_closedir(std::addressof(g_dirs[index]));
        g_dirs_opened[index] = false;
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        /* Option is unused. */
        AMS_UNUSED(option);

        /* Seek to the offset we're reading at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Read the data. */
        UINT br;
        R_TRY(TranslateFatFsError(f_read(GetInternalFile(handle), buffer, size, std::addressof(br))));

        /* Check that we read the correct amount. */
        R_UNLESS(br == size, fs::ResultOutOfRange());

        return ResultSuccess();
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(handle, offset, buffer, size, fs::ReadOption::None);
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        /* Option is unused. */
        AMS_UNUSED(option);

        /* Seek to the offset we're reading at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Read the data. */
        UINT br;
        R_TRY(TranslateFatFsError(f_read(GetInternalFile(handle), buffer, size, std::addressof(br))));

        /* Set the output size. */
        *out = br;

        return ResultSuccess();
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size) {
        return ReadFile(out, handle, offset, buffer, size, fs::ReadOption::None);
    }

    Result GetFileSize(s64 *out, FileHandle handle) {
        FIL *fp = GetInternalFile(handle);
        *out = f_size(fp);
        return ResultSuccess();
    }

    Result FlushFile(FileHandle handle) {
        return TranslateFatFsError(f_sync(GetInternalFile(handle)));
    }

    Result WriteFile(FileHandle handle, s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
        /* Seek to the offset we're writing at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Write the data. */
        UINT bw;
        R_TRY(TranslateFatFsError(f_write(GetInternalFile(handle), buffer, size, std::addressof(bw))));

        /* Check that we wrote the correct amount. */
        R_UNLESS(bw == size, fs::ResultOutOfRange());

        /* If we should, flush the file. */
        if (option.HasFlushFlag()) {
            R_TRY(FlushFile(handle));
        }

        return ResultSuccess();
    }

    Result SetFileSize(FileHandle handle, s64 size) {
        FIL *fp = GetInternalFile(handle);

        /* Check if we have nothing to do. */
        const size_t fsize = f_size(fp);
        R_SUCCEED_IF(static_cast<FSIZE_t>(size) == fsize);

        /* NOTE/TODO: This may not preserve file data. Do this in a way that does? */

        /* Truncate the file. */
        R_TRY(TranslateFatFsError(f_truncate(fp)));

        /* Expand the file. */
        R_TRY(TranslateFatFsError(f_expand(fp, size, 1)));

        /* Ensure the file is synchronized. */
        R_TRY(FlushFile(handle));

        /* Check that our expansion succeeded. */
        AMS_ASSERT(f_size(fp) == static_cast<FSIZE_t>(size));

        return ResultSuccess();
    }

    int GetFileOpenMode(FileHandle handle) {
        return g_open_modes[GetFileIndex(GetInternalFile(handle))];
    }

    void CloseFile(FileHandle handle) {
        const size_t index = GetFileIndex(GetInternalFile(handle));
        f_close(std::addressof(g_files[index]));
        g_open_modes[index] = 0;
        g_files_opened[index] = false;
    }

}
