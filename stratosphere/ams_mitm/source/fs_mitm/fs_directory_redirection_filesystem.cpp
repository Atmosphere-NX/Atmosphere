/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fs_directory_redirection_filesystem.hpp"
#include "fs_path_utils.hpp"

static char *GetNormalizedDirectory(const char *dir_prefix) {
    /* Normalize the path. */
    char normal_path[FS_MAX_PATH + 1];
    size_t normal_path_len;
    Result rc = FsPathUtils::Normalize(normal_path, sizeof(normal_path), dir_prefix, &normal_path_len);
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

    char *output = static_cast<char *>(malloc(normal_path_len + 1));
    if (output == nullptr) {
        std::abort();
        /* TODO: Result error code? */
    }

    std::strncpy(output, normal_path, normal_path_len + 1);
    output[normal_path_len] = 0;
    return output;
}

Result DirectoryRedirectionFileSystem::Initialize(const char *before, const char *after) {
    if (strnlen(before, FS_MAX_PATH) >= FS_MAX_PATH || strnlen(after, FS_MAX_PATH) >= FS_MAX_PATH) {
        return ResultFsTooLongPath;
    }

    this->before_dir = GetNormalizedDirectory(before);
    this->after_dir = GetNormalizedDirectory(after);
    this->before_dir_len = strlen(this->before_dir);
    this->after_dir_len = strlen(this->after_dir);

    return ResultSuccess;
}

Result DirectoryRedirectionFileSystem::GetFullPath(char *out, size_t out_size, const char *relative_path) {
    FsPath tmp_rel_path;
    Result rc = FsPathUtils::Normalize(tmp_rel_path.str, sizeof(tmp_rel_path), relative_path, nullptr);
    if (R_FAILED(rc)) {
        return rc;
    }

    if (std::strncmp(tmp_rel_path.str, this->before_dir, this->before_dir_len) == 0) {
        if (this->after_dir_len + strnlen(tmp_rel_path.str, FS_MAX_PATH) - this->before_dir_len > out_size) {
            return ResultFsTooLongPath;
        }

        /* Copy after path. */
        std::strncpy(out, this->after_dir, out_size);
        out[out_size - 1] = 0;

        /* Normalize it. */
        return FsPathUtils::Normalize(out + this->after_dir_len - 1, out_size - (this->after_dir_len - 1), relative_path + this->before_dir_len - 1, nullptr);
    } else if (std::memcmp(tmp_rel_path.str, this->before_dir, this->before_dir_len - 1) == 0) {
        /* Handling raw directory. */
        if (this->after_dir_len + 1 > out_size) {
            return ResultFsTooLongPath;
        }
        std::strncpy(out, this->after_dir, out_size);
        out[out_size - 1] = 0;
        return ResultSuccess;
    } else {
        return FsPathUtils::Normalize(out, out_size, relative_path, nullptr);
    }
}

Result DirectoryRedirectionFileSystem::CreateFileImpl(const FsPath &path, uint64_t size, int flags) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->CreateFile(full_path, size, flags);
}

Result DirectoryRedirectionFileSystem::DeleteFileImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->DeleteFile(full_path);
}

Result DirectoryRedirectionFileSystem::CreateDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->CreateDirectory(full_path);
}

Result DirectoryRedirectionFileSystem::DeleteDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->DeleteDirectory(full_path);
}

Result DirectoryRedirectionFileSystem::DeleteDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->DeleteDirectoryRecursively(full_path);
}

Result DirectoryRedirectionFileSystem::RenameFileImpl(const FsPath &old_path, const FsPath &new_path) {
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

Result DirectoryRedirectionFileSystem::RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) {
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

Result DirectoryRedirectionFileSystem::GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->GetEntryType(out, full_path);
}

Result DirectoryRedirectionFileSystem::OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->OpenFile(out_file, full_path, mode);
}

Result DirectoryRedirectionFileSystem::OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->OpenDirectory(out_dir, full_path, mode);
}

Result DirectoryRedirectionFileSystem::CommitImpl() {
    return this->base_fs->Commit();
}

Result DirectoryRedirectionFileSystem::GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->GetFreeSpaceSize(out, full_path);
}

Result DirectoryRedirectionFileSystem::GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->GetTotalSpaceSize(out, full_path);
}

Result DirectoryRedirectionFileSystem::CleanDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->CleanDirectoryRecursively(full_path);
}

Result DirectoryRedirectionFileSystem::GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->GetFileTimeStampRaw(out, full_path);
}

Result DirectoryRedirectionFileSystem::QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
    Result rc;
    FsPath full_path;

    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    return this->base_fs->QueryEntry(out, out_size, in, in_size, query, full_path);
}
