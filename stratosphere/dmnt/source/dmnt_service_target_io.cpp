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
#include <stratosphere.hpp>
#include "dmnt_service.hpp"

namespace std {

    template<>
    struct hash<ams::dmnt::TargetIOFileHandle> {
        u64 operator()(ams::dmnt::TargetIOFileHandle const &handle) const noexcept {
            return handle.GetValue();
        }
    };

}

namespace ams::dmnt {

    namespace {

        enum TIOCreateOption : u32 {
            TIOCreateOption_CreateNew = 1,
            TIOCreateOption_CreateAlways = 2,
            TIOCreateOption_OpenExisting = 3,
            TIOCreateOption_OpenAlways = 4,
            TIOCreateOption_ResetSize = 5,
        };

        /* Nintendo uses actual pointers as file handles. We'll add a layer of indirection... */
        bool g_sd_initialized = false;
        os::Mutex g_sd_lock(false);
        FsFileSystem g_sd_fs;

        os::Mutex g_file_handle_lock(false);
        u64 g_cur_fd;
        std::unordered_map<TargetIOFileHandle, FsFile> g_file_handles;

        Result EnsureSdInitialized() {
            std::scoped_lock lk(g_sd_lock);
            R_SUCCEED_IF(g_sd_initialized);

            R_TRY(fsOpenSdCardFileSystem(&g_sd_fs));
            g_sd_initialized = true;
            return ResultSuccess();
        }

        TargetIOFileHandle GetNewFileHandle(FsFile f) {
            std::scoped_lock lk(g_file_handle_lock);

            TargetIOFileHandle fd = { .value = g_cur_fd++ };
            g_file_handles[fd] = f;
            return fd;
        }

        Result GetFileByHandle(FsFile *out, TargetIOFileHandle handle) {
            std::scoped_lock lk(g_file_handle_lock);

            if (g_file_handles.find(handle) != g_file_handles.end()) {
                *out = g_file_handles[handle];
                return ResultSuccess();
            }

            return fs::ResultInvalidArgument();
        }

        Result CloseFileByHandle(TargetIOFileHandle handle) {
            std::scoped_lock lk(g_file_handle_lock);

            if (g_file_handles.find(handle) != g_file_handles.end()) {
                fsFileClose(&g_file_handles[handle]);
                g_file_handles.erase(handle);
                return ResultSuccess();
            }

            return fs::ResultInvalidArgument();
        }

        void FixPath(char *dst, size_t dst_size, const sf::InBuffer &path) {
            dst[dst_size - 1] = 0;
            strncpy(dst, "/", dst_size - 1);

            const char *src = reinterpret_cast<const char *>(path.GetPointer());
            size_t src_idx = 0;
            size_t dst_idx = 1;
            while (src_idx < path.GetSize() && (src[src_idx] == '/' || src[src_idx] == '\\')) {
                src_idx++;
            }

            while (src_idx < path.GetSize() && dst_idx < dst_size - 1 && src[src_idx] != 0) {
                if (src[src_idx] == '\\') {
                    dst[dst_idx] = '/';
                } else {
                    dst[dst_idx] = src[src_idx];
                }

                src_idx++;
                dst_idx++;
            }

            if (dst_idx < dst_size) {
                dst[dst_idx] = 0;
            }
        }

    }

    Result DebugMonitorService::TargetIO_FileOpen(sf::Out<TargetIOFileHandle> out_hnd, const sf::InBuffer &path, int open_mode, u32 create_mode) {
        R_TRY(EnsureSdInitialized());

        char fs_path[FS_MAX_PATH];
        FixPath(fs_path, sizeof(fs_path), path);

        /* Create file as required by mode. */
        if (create_mode == TIOCreateOption_CreateAlways) {
            fsFsDeleteFile(&g_sd_fs, fs_path);
            R_TRY(fsFsCreateFile(&g_sd_fs, fs_path, 0, 0));
        } else if (create_mode == TIOCreateOption_CreateNew) {
            R_TRY(fsFsCreateFile(&g_sd_fs, fs_path, 0, 0));
        } else if (create_mode == TIOCreateOption_OpenAlways) {
            fsFsCreateFile(&g_sd_fs, fs_path, 0, 0);
        }

        /* Open the file, guard to prevent failure to close. */
        FsFile f;
        R_TRY(fsFsOpenFile(&g_sd_fs, fs_path, open_mode, &f));
        auto file_guard = SCOPE_GUARD { fsFileClose(&f); };

        /* Set size if needed. */
        if (create_mode == TIOCreateOption_ResetSize) {
            R_TRY(fsFileSetSize(&f, 0));
        }

        /* Cancel guard, output file handle. */
        file_guard.Cancel();
        out_hnd.SetValue(GetNewFileHandle(f));

        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileClose(TargetIOFileHandle hnd) {
        return CloseFileByHandle(hnd);
    }

    Result DebugMonitorService::TargetIO_FileRead(TargetIOFileHandle hnd, const sf::OutNonSecureBuffer &out_data, sf::Out<u32> out_read, s64 offset) {
        FsFile f;
        size_t read = 0;

        R_TRY(GetFileByHandle(&f, hnd));
        R_TRY(fsFileRead(&f, offset, out_data.GetPointer(), out_data.GetSize(), FsReadOption_None, &read));

        out_read.SetValue(static_cast<u32>(read));
        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileWrite(TargetIOFileHandle hnd, const sf::InNonSecureBuffer &data, sf::Out<u32> out_written, s64 offset) {
        FsFile f;

        R_TRY(GetFileByHandle(&f, hnd));
        R_TRY(fsFileWrite(&f, offset, data.GetPointer(), data.GetSize(), FsWriteOption_None));

        out_written.SetValue(data.GetSize());
        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileSetAttributes(const sf::InBuffer &path, const sf::InBuffer &attributes) {
        /* I don't really know why this command exists, Horizon doesn't allow you to set any attributes. */
        /* N just returns ResultSuccess unconditionally here. */
        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileGetInformation(const sf::InBuffer &path, const sf::OutArray<u64> &out_info, sf::Out<int> is_directory) {
        R_TRY(EnsureSdInitialized());

        char fs_path[FS_MAX_PATH];
        FixPath(fs_path, sizeof(fs_path), path);

        for (size_t i = 0; i < out_info.GetSize(); i++) {
            out_info[i] = 0;
        }
        is_directory.SetValue(0);

        FsFile f;
        if (R_SUCCEEDED(fsFsOpenFile(&g_sd_fs, fs_path, FsOpenMode_Read, &f))) {
            ON_SCOPE_EXIT { fsFileClose(&f); };

            /* N doesn't check this return code. */
            if (out_info.GetSize() > 0) {
                fsFileGetSize(&f, reinterpret_cast<s64 *>(&out_info[0]));
            }

            /* TODO: N does not call fsFsGetFileTimestampRaw here, but we possibly could. */
        } else {
            FsDir dir;
            R_TRY(fsFsOpenDirectory(&g_sd_fs, fs_path, FsDirOpenMode_ReadFiles | FsDirOpenMode_ReadDirs, &dir));
            fsDirClose(&dir);
            is_directory.SetValue(1);
        }

        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileSetTime(const sf::InBuffer &path, u64 create, u64 access, u64 modify) {
        /* This is another function that doesn't really need to exist, because Horizon doesn't let you set anything. */
        return ResultSuccess();
    }

    Result DebugMonitorService::TargetIO_FileSetSize(const sf::InBuffer &input, s64 size) {
        /* Why does this function take in a path and not a file handle? */
        R_TRY(EnsureSdInitialized());

        /* We will try to be better than N, here. N only treats input as a path. */
        FsFile f;
        if (input.GetSize() == sizeof(TargetIOFileHandle)) {
            R_UNLESS(R_FAILED(GetFileByHandle(&f, *reinterpret_cast<const TargetIOFileHandle *>(input.GetPointer()))), fsFileSetSize(&f, size));
        }

        char fs_path[FS_MAX_PATH];
        FixPath(fs_path, sizeof(fs_path), input);

        R_TRY(fsFsOpenFile(&g_sd_fs, fs_path, FsOpenMode_Write, &f));
        ON_SCOPE_EXIT { fsFileClose(&f); };

        return fsFileSetSize(&f, size);
    }

    Result DebugMonitorService::TargetIO_FileDelete(const sf::InBuffer &path) {
        R_TRY(EnsureSdInitialized());

        char fs_path[FS_MAX_PATH];
        FixPath(fs_path, sizeof(fs_path), path);

        return fsFsDeleteFile(&g_sd_fs, fs_path);
    }

    Result DebugMonitorService::TargetIO_FileMove(const sf::InBuffer &src_path, const sf::InBuffer &dst_path) {
        R_TRY(EnsureSdInitialized());

        char fs_src_path[FS_MAX_PATH];
        char fs_dst_path[FS_MAX_PATH];
        FixPath(fs_src_path, sizeof(fs_src_path), src_path);
        FixPath(fs_dst_path, sizeof(fs_dst_path), dst_path);

        return fsFsRenameFile(&g_sd_fs, fs_src_path, fs_dst_path);
    }

}
