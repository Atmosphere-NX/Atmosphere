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

#include <cstring>
#include <switch.h>
#include <stratosphere.hpp>

#include "../utils.hpp"
#include "fs_subdirectory_filesystem.hpp"
#include "fs_path_utils.hpp"

Result SubDirectoryFileSystem::Initialize(const char *bp) {
    if (strnlen(bp, FS_MAX_PATH) >= FS_MAX_PATH) {
        return ResultFsTooLongPath;
    }
    
    /* Normalize the path. */
    char normal_path[FS_MAX_PATH + 1];
    size_t normal_path_len;
    Result rc = FsPathUtils::Normalize(normal_path, sizeof(normal_path), bp, &normal_path_len);
    if (R_FAILED(rc)) {
        /* N calls svcBreak here. */
        std::abort();
    }
    
    /* Ensure terminating '/' */
    if (normal_path[normal_path_len-1] != '/') {
        if (normal_path_len + 2 > sizeof(normal_path)) {
            std::abort();
        }
        
        strncat(normal_path, "/", 2);
        normal_path[sizeof(normal_path)-1] = 0;
        normal_path_len++;
    }
    
    this->base_path_len = normal_path_len + 1;
    this->base_path = reinterpret_cast<char *>(malloc(this->base_path_len));
    if (this->base_path == nullptr) {
        return ResultFsAllocationFailureInSubDirectoryFileSystem;
    }
    
    std::strncpy(this->base_path, normal_path, this->base_path_len);
    this->base_path[this->base_path_len-1] = 0;
    return ResultSuccess;
}

Result SubDirectoryFileSystem::GetFullPath(char *out, size_t out_size, const char *relative_path) {
    if (this->base_path_len + strnlen(relative_path, FS_MAX_PATH) > out_size) {
        return ResultFsTooLongPath;
    }
    
    /* Copy base path. */
    std::strncpy(out, this->base_path, out_size);
    out[out_size-1] = 0;
    
    /* Normalize it. */
    return FsPathUtils::Normalize(out + this->base_path_len - 2, out_size - (this->base_path_len - 2), relative_path, nullptr);
}

Result SubDirectoryFileSystem::CreateFileImpl(const FsPath &path, uint64_t size, int flags) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->CreateFile(full_path, size, flags);
}

Result SubDirectoryFileSystem::DeleteFileImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->DeleteFile(full_path);
}

Result SubDirectoryFileSystem::CreateDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->CreateDirectory(full_path);
}

Result SubDirectoryFileSystem::DeleteDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->DeleteDirectory(full_path);
}

Result SubDirectoryFileSystem::DeleteDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->DeleteDirectoryRecursively(full_path);
}

Result SubDirectoryFileSystem::RenameFileImpl(const FsPath &old_path, const FsPath &new_path) {
    Result rc;
    FsPath full_old_path, full_new_path;
    
    if (R_FAILED((rc = GetFullPath(full_old_path, old_path)))) {
        return rc;
    }
    
    if (R_FAILED((rc = GetFullPath(full_new_path, new_path)))) {
        return rc;
    }
    
    return this->base_fs->RenameFile(full_old_path, full_new_path);
}

Result SubDirectoryFileSystem::RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) {
    Result rc;
    FsPath full_old_path, full_new_path;
    
    if (R_FAILED((rc = GetFullPath(full_old_path, old_path)))) {
        return rc;
    }
    
    if (R_FAILED((rc = GetFullPath(full_new_path, new_path)))) {
        return rc;
    }
    
    return this->base_fs->RenameDirectory(full_old_path, full_new_path);
}

Result SubDirectoryFileSystem::GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->GetEntryType(out, full_path);
}

Result SubDirectoryFileSystem::OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->OpenFile(out_file, full_path, mode);
}

Result SubDirectoryFileSystem::OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->OpenDirectory(out_dir, full_path, mode);
}

Result SubDirectoryFileSystem::CommitImpl() {
    return this->base_fs->Commit();
}

Result SubDirectoryFileSystem::GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->GetFreeSpaceSize(out, full_path);
}

Result SubDirectoryFileSystem::GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->GetTotalSpaceSize(out, full_path);
}   

Result SubDirectoryFileSystem::CleanDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->CleanDirectoryRecursively(full_path);
}

Result SubDirectoryFileSystem::GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->GetFileTimeStampRaw(out, full_path);
}

Result SubDirectoryFileSystem::QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    return this->base_fs->QueryEntry(out, out_size, in, in_size, query, full_path);
}
