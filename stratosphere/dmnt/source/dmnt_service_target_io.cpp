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
        return 0;
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
        return 0;
    }
    return 0x2EE202;
}

static Result CloseFileByHandle(u64 handle) {
    std::scoped_lock<HosMutex> lk(g_file_handle_lock);
    if (g_file_handles.find(handle) != g_file_handles.end()) {
        fsFileClose(&g_file_handles[handle]);
        g_file_handles.erase(handle);
        return 0;
    }
    return 0x2EE202;
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
        return 0xF601;
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
        return 0xF601;
    }
    
    return CloseFileByHandle(hnd[0]);
}

Result DebugMonitorService::TargetIO_FileRead(InBuffer<u64> hnd, OutBuffer<u8> out_data, Out<u32> out_read, u64 offset) {
    if (hnd.num_elements != 1) {
        return 0xF601;
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

Result DebugMonitorService::TargetIO_FileWrite(InBuffer<u64> hnd, InBuffer<u8> data, Out<u32> out_written, u64 offset) {
    if (hnd.num_elements != 1) {
        return 0xF601;
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
