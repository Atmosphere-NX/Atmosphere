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
#include <exosphere.hpp>
#include "../fatfs/ff.h"
#include "fatal_fs_api.hpp"

namespace ams::fs {

    namespace {

        constexpr size_t MaxFiles = 8;

        constinit bool g_is_sd_mounted = false;

        FATFS g_sd_fs;

        constinit FIL g_files[MaxFiles] = {};
        constinit bool g_files_opened[MaxFiles] = {};
        constinit int g_open_modes[MaxFiles] = {};

        Result TranslateFatFsError(FRESULT res) {
            switch (res) {
                case FR_OK:
                    R_SUCCEED();
                case FR_DISK_ERR:
                    R_THROW(fs::ResultMmcAccessFailed());
                case FR_INT_ERR:
                    R_THROW(fs::ResultPreconditionViolation());
                case FR_NOT_READY:
                    R_THROW(fs::ResultMmcAccessFailed());
                case FR_NO_FILE:
                    R_THROW(fs::ResultPathNotFound());
                case FR_NO_PATH:
                    R_THROW(fs::ResultPathNotFound());
                case FR_INVALID_NAME:
                    R_THROW(fs::ResultInvalidPath());
                case FR_DENIED:
                    R_THROW(fs::ResultPermissionDenied());
                case FR_EXIST:
                    R_THROW(fs::ResultPathAlreadyExists());
                case FR_INVALID_OBJECT:
                    R_THROW(fs::ResultInvalidArgument());
                case FR_WRITE_PROTECTED:
                    R_THROW(fs::ResultWriteNotPermitted());
                case FR_INVALID_DRIVE:
                    R_THROW(fs::ResultInvalidMountName());
                case FR_NOT_ENABLED:
                    R_THROW(fs::ResultInvalidMountName()); /* BAD/TODO */
                case FR_NO_FILESYSTEM:
                    R_THROW(fs::ResultInvalidMountName()); /* BAD/TODO */
                case FR_TIMEOUT:
                    R_THROW(fs::ResultTargetLocked()); /* BAD/TODO */
                case FR_LOCKED:
                    R_THROW(fs::ResultTargetLocked());
                case FR_NOT_ENOUGH_CORE:
                    R_THROW(fs::ResultPreconditionViolation()); /* BAD/TODO */
                case FR_TOO_MANY_OPEN_FILES:
                    R_THROW(fs::ResultPreconditionViolation()); /* BAD/TODO */
                case FR_INVALID_PARAMETER:
                    R_THROW(fs::ResultInvalidArgument());
                default:
                    R_THROW(fs::ResultInternal());
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

        ALWAYS_INLINE size_t GetFileIndex(FIL *fp) {
            const size_t file_index = (fp - g_files);
            AMS_ASSERT(file_index < MaxFiles);
            return file_index;
        }

    }

    bool MountSdCard() {
        AMS_ASSERT(!g_is_sd_mounted);
        g_is_sd_mounted = f_mount(std::addressof(g_sd_fs), "sdmc", 1) == FR_OK;
        return g_is_sd_mounted;
    }

    void UnmountSdCard() {
        AMS_ASSERT(g_is_sd_mounted);
        f_unmount("sdmc");
        g_is_sd_mounted = false;
    }

    Result CreateFile(const char *path, s64 size) {
        /* Create the file. */
        FIL fp;
        R_TRY(TranslateFatFsError(f_open(std::addressof(fp), path, FA_CREATE_NEW | FA_READ | FA_WRITE)));

        /* Ensure that we close the file when we're done with it. */
        ON_SCOPE_EXIT { f_close(std::addressof(fp)); };

        /* Expand the file. */
        R_TRY(TranslateFatFsError(f_expand(std::addressof(fp), size, 1)));

        R_SUCCEED();
    }

    Result CreateDirectory(const char *path) {
        R_RETURN(TranslateFatFsError(f_mkdir(path)));
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
                R_SUCCEED();
            }
        }
        R_THROW(fs::ResultOpenCountLimit());
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        /* Option is unused. */
        AMS_UNUSED(option);

        /* Seek to the offset we're reading at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Read the data. */
        u32 br;
        R_TRY(TranslateFatFsError(f_read(GetInternalFile(handle), buffer, size, std::addressof(br))));

        /* Check that we read the correct amount. */
        R_UNLESS(br == size, fs::ResultOutOfRange());

        R_SUCCEED();
    }

    Result ReadFile(FileHandle handle, s64 offset, void *buffer, size_t size) {
        R_RETURN(ReadFile(handle, offset, buffer, size, fs::ReadOption::None));
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) {
        /* Option is unused. */
        AMS_UNUSED(option);

        /* Seek to the offset we're reading at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Read the data. */
        u32 br;
        R_TRY(TranslateFatFsError(f_read(GetInternalFile(handle), buffer, size, std::addressof(br))));

        /* Set the output size. */
        *out = br;

        R_SUCCEED();
    }

    Result ReadFile(size_t *out, FileHandle handle, s64 offset, void *buffer, size_t size) {
        R_RETURN(ReadFile(out, handle, offset, buffer, size, fs::ReadOption::None));
    }

    Result GetFileSize(s64 *out, FileHandle handle) {
        FIL *fp = GetInternalFile(handle);
        *out = f_size(fp);
        R_SUCCEED();
    }

    Result FlushFile(FileHandle handle) {
        R_RETURN(TranslateFatFsError(f_sync(GetInternalFile(handle))));
    }

    Result WriteFile(FileHandle handle, s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) {
        /* Seek to the offset we're writing at. */
        R_TRY(TranslateFatFsError(f_lseek(GetInternalFile(handle), offset)));

        /* Write the data. */
        u32 bw;
        R_TRY(TranslateFatFsError(f_write(GetInternalFile(handle), buffer, size, std::addressof(bw))));

        /* Check that we wrote the correct amount. */
        R_UNLESS(bw == size, fs::ResultOutOfRange());

        /* If we should, flush the file. */
        if (option.HasFlushFlag()) {
            R_TRY(FlushFile(handle));
        }

        R_SUCCEED();
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

        R_SUCCEED();
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
