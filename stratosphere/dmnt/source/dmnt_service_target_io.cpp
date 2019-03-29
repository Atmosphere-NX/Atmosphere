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
 
#include <unordered_map>
#include <switch.h>
#include "dmnt_service.hpp"

enum TIOCreateOption : u32 {
    TIOCreateOption_CreateNew = 1,
    TIOCreateOption_CreateAlways = 2,
    TIOCreateOption_OpenExisting = 3,
    TIOCreateOption_OpenAlways = 4,
    TIOCreateOption_ResetSize = 5,
};

/* Nintendo uses actual pointers as file handles. We'll add a layer of indirection... */
static bool g_sd_initialized = false;
static HosMutex g_sd_lock;
static FsFileSystem g_sd_fs;

static HosMutex g_file_handle_lock;
static u64 g_cur_fd = 0;
static std::unordered_map<u64, FsFile> g_file_handles;

static Result EnsureSdInitialized() {
    std::scoped_lock<HosMutex> lk(g_sd_lock);
    if (g_sd_initialized) {
        return ResultSuccess;
    }
    
    Result rc = fsMountSdcard(&g_sd_fs);
    if (R_SUCCEEDED(rc)) {
        g_sd_initialized = true;
    }
    return rc;
}

static u64 GetFileHandle(FsFile f) {
    std::scoped_lock<HosMutex> lk(g_file_handle_lock);
    
    u64 fd = g_cur_fd++;
    g_file_handles[fd] = f;
    return fd;
}

static Result GetFileByHandle(FsFile *out, u64 handle) {
    std::scoped_lock<HosMutex> lk(g_file_handle_lock);
    if (g_file_handles.find(handle) != g_file_handles.end()) {
        *out = g_file_handles[handle];
        return ResultSuccess;
    }
    return ResultFsInvalidArgument;
}

static Result CloseFileByHandle(u64 handle) {
    std::scoped_lock<HosMutex> lk(g_file_handle_lock);
    if (g_file_handles.find(handle) != g_file_handles.end()) {
        fsFileClose(&g_file_handles[handle]);
        g_file_handles.erase(handle);
        return ResultSuccess;
    }
    return ResultFsInvalidArgument;
}

static void FixPath(char *dst, size_t dst_size, InBuffer<char> &path) {
    dst[dst_size - 1] = 0;
    strncpy(dst, "/", dst_size - 1);
    
    const char *src = path.buffer;
    size_t src_idx = 0;
    size_t dst_idx = 1;
    while (src_idx < path.num_elements && (src[src_idx] == '/' || src[src_idx] == '\\')) {
        src_idx++;
    }
    
    while (src_idx < path.num_elements && dst_idx < dst_size - 1 && src[src_idx] != 0) {
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

Result DebugMonitorService::TargetIO_FileOpen(OutBuffer<u64> out_hnd, InBuffer<char> path, int open_mode, u32 create_mode) {
    if (out_hnd.num_elements != 1) {
        /* Serialization error. */
        return ResultKernelConnectionClosed;
    }
    
    Result rc = EnsureSdInitialized();
    if (R_FAILED(rc)) {
        return rc;
    }
    
    char fs_path[FS_MAX_PATH];
    FixPath(fs_path, sizeof(fs_path), path);
    
    if (create_mode == TIOCreateOption_CreateAlways) {
        fsFsDeleteFile(&g_sd_fs, fs_path);
        rc = fsFsCreateFile(&g_sd_fs, fs_path, 0, 0);
    } else if (create_mode == TIOCreateOption_CreateNew) {
        rc = fsFsCreateFile(&g_sd_fs, fs_path, 0, 0);
    }
    
    if (R_FAILED(rc)) {
        return rc;
    }
    
    FsFile f;
    rc = fsFsOpenFile(&g_sd_fs, fs_path, open_mode, &f);
    if (R_FAILED(rc)) {
        if (create_mode == TIOCreateOption_OpenAlways) {
            fsFsCreateFile(&g_sd_fs, fs_path, 0, 0);
            rc = fsFsOpenFile(&g_sd_fs, fs_path, open_mode, &f);
        }
    }
    
    if (R_SUCCEEDED(rc)) {
        if (create_mode == TIOCreateOption_ResetSize) {
            rc = fsFileSetSize(&f, 0);
        }
        if (R_SUCCEEDED(rc)) {
            out_hnd[0] = GetFileHandle(f);
        } else {
            fsFileClose(&f);
        }
    }
    
    return rc;
}

Result DebugMonitorService::TargetIO_FileClose(InBuffer<u64> hnd) {
    if (hnd.num_elements != 1) {
        /* Serialization error. */
        return ResultKernelConnectionClosed;
    }
    
    return CloseFileByHandle(hnd[0]);
}

Result DebugMonitorService::TargetIO_FileRead(InBuffer<u64> hnd, OutBuffer<u8, BufferType_Type1> out_data, Out<u32> out_read, u64 offset) {
    if (hnd.num_elements != 1) {
        /* Serialization error. */
        return ResultKernelConnectionClosed;
    }
    
    FsFile f;
    Result rc = GetFileByHandle(&f, hnd[0]);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    size_t read = 0;
    rc = fsFileRead(&f, offset, out_data.buffer, out_data.num_elements, &read);
    out_read.SetValue(static_cast<u32>(read));
    return rc;
}

Result DebugMonitorService::TargetIO_FileWrite(InBuffer<u64> hnd, InBuffer<u8, BufferType_Type1> data, Out<u32> out_written, u64 offset) {
    if (hnd.num_elements != 1) {
        /* Serialization error. */
        return ResultKernelConnectionClosed;
    }
    
    FsFile f;
    Result rc = GetFileByHandle(&f, hnd[0]);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    rc = fsFileWrite(&f, offset, data.buffer, data.num_elements);
    if (R_SUCCEEDED(rc)) {
        out_written.SetValue(data.num_elements);
    }
    
    return rc;
}

Result DebugMonitorService::TargetIO_FileSetAttributes(InBuffer<char> path, InBuffer<u8> attributes) {
    /* I don't really know why this command exists, Horizon doesn't allow you to set any attributes. */
    /* N just returns ResultSuccess unconditionally here. */
    return ResultSuccess;
}

Result DebugMonitorService::TargetIO_FileGetInformation(InBuffer<char> path, OutBuffer<u64> out_info, Out<int> is_directory) {
    if (out_info.num_elements != 4) {
        /* Serialization error. */
        return ResultKernelConnectionClosed;
    }
    
    Result rc = EnsureSdInitialized();
    if (R_FAILED(rc)) {
        return rc;
    }
    
    char fs_path[FS_MAX_PATH];
    FixPath(fs_path, sizeof(fs_path), path);
    
    for (size_t i = 0; i < out_info.num_elements; i++) {
        out_info[i] = 0;
    }
    is_directory.SetValue(0);
    
    FsFile f;
    rc = fsFsOpenFile(&g_sd_fs, fs_path, FS_OPEN_READ, &f);
    if (R_SUCCEEDED(rc)) {
        ON_SCOPE_EXIT { fsFileClose(&f); };
        
        /* N doesn't check this return code. */
        fsFileGetSize(&f, &out_info[0]);
        
        /* TODO: N does not call fsFsGetFileTimestampRaw here, but we possibly could. */
    } else {
        FsDir dir;
        rc = fsFsOpenDirectory(&g_sd_fs, fs_path, FS_DIROPEN_FILE | FS_DIROPEN_DIRECTORY, &dir);
        if (R_SUCCEEDED(rc)) {
            fsDirClose(&dir);
            is_directory.SetValue(1);
        }
    }
    
    return rc;
}

Result DebugMonitorService::TargetIO_FileSetTime(InBuffer<char> path, u64 create, u64 access, u64 modify) {
    /* This is another function that doesn't really need to exist, because Horizon doesn't let you set anything. */
    return ResultSuccess;
}

Result DebugMonitorService::TargetIO_FileSetSize(InBuffer<char> input, u64 size) {
    /* Why does this function take in a path and not a file handle? */
    
    /* We will try to be better than N, here. N only treats input as a path. */
    if (input.num_elements == sizeof(u64)) {
        FsFile f;
        if (R_SUCCEEDED(GetFileByHandle(&f, reinterpret_cast<u64 *>(input.buffer)[0]))) {
            return fsFileSetSize(&f, size);
        }
    }
    
    Result rc = EnsureSdInitialized();
    if (R_FAILED(rc)) {
        return rc;
    }
    
    char fs_path[FS_MAX_PATH];
    FixPath(fs_path, sizeof(fs_path), input);
    
    FsFile f;
    rc = fsFsOpenFile(&g_sd_fs, fs_path, FS_OPEN_WRITE, &f);
    if (R_SUCCEEDED(rc)) {
        rc = fsFileSetSize(&f, size);
        fsFileClose(&f);
    }
    
    return rc;
}

Result DebugMonitorService::TargetIO_FileDelete(InBuffer<char> path) {
    Result rc = EnsureSdInitialized();
    if (R_FAILED(rc)) {
        return rc;
    }
    
    char fs_path[FS_MAX_PATH];
    FixPath(fs_path, sizeof(fs_path), path);
    
    return fsFsDeleteFile(&g_sd_fs, fs_path);
}

Result DebugMonitorService::TargetIO_FileMove(InBuffer<char> path0, InBuffer<char> path1) {
    Result rc = EnsureSdInitialized();
    if (R_FAILED(rc)) {
        return rc;
    }
    
    char fs_path0[FS_MAX_PATH];
    char fs_path1[FS_MAX_PATH];
    FixPath(fs_path0, sizeof(fs_path0), path0);
    FixPath(fs_path1, sizeof(fs_path1), path1);
    
    return fsFsRenameFile(&g_sd_fs, fs_path0, fs_path1);
}
