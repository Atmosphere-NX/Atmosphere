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
#include "fs_directory_savedata_filesystem.hpp"
#include "fs_dir_utils.hpp"

class DirectorySaveDataFile : public IFile {
    private:
        std::unique_ptr<IFile> base_file;
        DirectorySaveDataFileSystem *parent_fs; /* TODO: shared_ptr + enabled_shared_from_this? */
        int open_mode;
    public:
        DirectorySaveDataFile(std::unique_ptr<IFile> f, DirectorySaveDataFileSystem *p, int m) : base_file(std::move(f)), parent_fs(p), open_mode(m) {
            /* ... */
        }

        virtual ~DirectorySaveDataFile() {
            /* Observe closing of writable file. */
            if (this->open_mode & OpenMode_Write) {
                this->parent_fs->OnWritableFileClose();
            }
        }
    public:
        virtual Result ReadImpl(u64 *out, u64 offset, void *buffer, u64 size) override {
            return this->base_file->Read(out, offset, buffer, size);
        }
        virtual Result GetSizeImpl(u64 *out) override {
            return this->base_file->GetSize(out);
        }
        virtual Result FlushImpl() override {
            return this->base_file->Flush();
        }
        virtual Result WriteImpl(u64 offset, void *buffer, u64 size, bool flush) override {
            return this->base_file->Write(offset, buffer, size, flush);
        }
        virtual Result SetSizeImpl(u64 size) override {
            return this->base_file->SetSize(size);
        }
        virtual Result OperateRangeImpl(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return this->base_file->OperateRange(operation_type, offset, size, out_range_info);
        }
};

/* ================================================================================================ */

Result DirectorySaveDataFileSystem::Initialize() {
    Result rc;
    DirectoryEntryType ent_type;

    /* Check that the working directory exists. */
    if (R_FAILED((rc = this->fs->GetEntryType(&ent_type, WorkingDirectoryPath)))) {
        /* If path isn't found, create working directory and committed directory. */
        if (rc == ResultFsPathNotFound) {
            if (R_FAILED((rc = this->fs->CreateDirectory(WorkingDirectoryPath)))) {
                return rc;
            }
            if (R_FAILED((rc = this->fs->CreateDirectory(CommittedDirectoryPath)))) {
                return rc;
            }
        } else {
            return rc;
        }
    }

    /* Now check for the committed directory. */
    rc = this->fs->GetEntryType(&ent_type, CommittedDirectoryPath);
    if (R_SUCCEEDED(rc)) {
        /* If committed exists, synchronize it to the working directory. */
        return this->SynchronizeDirectory(WorkingDirectoryPath, CommittedDirectoryPath);
    } else if (rc == ResultFsPathNotFound) {
        if (R_FAILED((rc = this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath)))) {
            return rc;
        }
        return this->fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath);
    } else {
        return rc;
    }
}

Result DirectorySaveDataFileSystem::SynchronizeDirectory(const FsPath &dst_dir, const FsPath &src_dir) {
    Result rc;

    /* Delete destination dir and recreate it. */
    if (R_FAILED((rc = this->fs->DeleteDirectoryRecursively(dst_dir)))) {
        /* Nintendo returns error unconditionally, but I think that's a bug in their code. */
        if (rc != ResultFsPathNotFound) {
            return rc;
        }
    }
    if (R_FAILED((rc = this->fs->CreateDirectory(dst_dir)))) {
        return rc;
    }

    /* Get a buffer to work with. */
    void *work_buf = nullptr;
    size_t work_buf_size = 0;
    if (R_FAILED((rc = this->AllocateWorkBuffer(&work_buf, &work_buf_size, IdealWorkBuffersize)))) {
        return rc;
    }
    ON_SCOPE_EXIT { free(work_buf); };

    return FsDirUtils::CopyDirectoryRecursively(this->fs, dst_dir, src_dir, work_buf, work_buf_size);
}

Result DirectorySaveDataFileSystem::AllocateWorkBuffer(void **out_buf, size_t *out_size, const size_t ideal_size) {
    size_t try_size = ideal_size;

    /* Repeatedly try to allocate until success. */
    while (try_size > 0x200) {
        void *buf = malloc(try_size);
        if (buf != nullptr) {
            *out_buf = buf;
            *out_size = try_size;
            return ResultSuccess;
        }

        /* Divide size by two. */
        try_size >>= 1;
    }

    /* TODO: Return a result here? Nintendo does not, but they have other allocation failed results. */
    /* Consider returning ResultFsAllocationFailureInDirectorySaveDataFileSystem? */
    std::abort();
}

Result DirectorySaveDataFileSystem::GetFullPath(char *out, size_t out_size, const char *relative_path) {
    /* Validate path. */
    if (1 + strnlen(relative_path, FS_MAX_PATH) >= out_size) {
        return ResultFsTooLongPath;
    }
    if (relative_path[0] != '/') {
        return ResultFsInvalidPath;
    }
    
    /* Copy working directory path. */
    std::strncpy(out, WorkingDirectoryPath.str, out_size);
    out[out_size-1] = 0;
    
    /* Normalize it. */
    constexpr size_t working_len = WorkingDirectoryPathLen - 1;
    return FsPathUtils::Normalize(out + working_len, out_size - working_len, relative_path, nullptr);
}

void DirectorySaveDataFileSystem::OnWritableFileClose() {
    std::scoped_lock<HosMutex> lk(this->lock);
    this->open_writable_files--;

    /* TODO: Abort if < 0? N does not. */
}

/* ================================================================================================ */

Result DirectorySaveDataFileSystem::CreateFileImpl(const FsPath &path, uint64_t size, int flags) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->CreateFile(full_path, size, flags);
}

Result DirectorySaveDataFileSystem::DeleteFileImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->DeleteFile(full_path);
}

Result DirectorySaveDataFileSystem::CreateDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->CreateDirectory(full_path);
}

Result DirectorySaveDataFileSystem::DeleteDirectoryImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->DeleteDirectory(full_path);
}

Result DirectorySaveDataFileSystem::DeleteDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->DeleteDirectoryRecursively(full_path);
}

Result DirectorySaveDataFileSystem::RenameFileImpl(const FsPath &old_path, const FsPath &new_path) {
    Result rc;
    FsPath full_old_path, full_new_path;
    
    if (R_FAILED((rc = GetFullPath(full_old_path, old_path)))) {
        return rc;
    }
    
    if (R_FAILED((rc = GetFullPath(full_new_path, new_path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->RenameFile(full_old_path, full_new_path);
}

Result DirectorySaveDataFileSystem::RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) {
    Result rc;
    FsPath full_old_path, full_new_path;
    
    if (R_FAILED((rc = GetFullPath(full_old_path, old_path)))) {
        return rc;
    }
    
    if (R_FAILED((rc = GetFullPath(full_new_path, new_path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->RenameDirectory(full_old_path, full_new_path);
}

Result DirectorySaveDataFileSystem::GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->GetEntryType(out, full_path);
}

Result DirectorySaveDataFileSystem::OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }

    std::scoped_lock<HosMutex> lk(this->lock);

    {
        /* Open the raw file. */
        std::unique_ptr<IFile> file;
        if (R_FAILED((rc = this->fs->OpenFile(file, full_path, mode)))) {
            return rc;
        }

        /* Create DirectorySaveDataFile wrapper. */
        out_file = std::make_unique<DirectorySaveDataFile>(std::move(file), this, mode);
    }

    /* Check for allocation failure. */
    if (out_file == nullptr) {
        return ResultFsAllocationFailureInDirectorySaveDataFileSystem;
    }

    /* Increment open writable files, if needed. */
    if (mode & OpenMode_Write) {
        this->open_writable_files++;
    }

    return ResultSuccess;
}

Result DirectorySaveDataFileSystem::OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->OpenDirectory(out_dir, full_path, mode);
}

Result DirectorySaveDataFileSystem::CommitImpl() {
    /* Here, Nintendo does the following (with retries): */
    /* - Rename Committed -> Synchronizing. */
    /* - Synchronize Working -> Synchronizing (deleting Synchronizing). */
    /* - Rename Synchronizing -> Committed. */
    /* I think this is not the optimal order to do things, as the previous committed directory */
    /* will be deleted if there is an error during synchronization. */
    /* Instead, we will synchronize first, then delete committed, then rename. */

    std::scoped_lock<HosMutex> lk(this->lock);
    Result rc;

    /* Ensure we don't have any open writable files. */
    if (this->open_writable_files != 0) {
        return ResultFsPreconditionViolation;
    }

    const auto SynchronizeWorkingDir = [&]() { return this->SynchronizeDirectory(SynchronizingDirectoryPath, WorkingDirectoryPath); };
    const auto DeleteCommittedDir = [&]() { return this->fs->DeleteDirectoryRecursively(CommittedDirectoryPath); };
    const auto RenameSynchDir = [&]() { return this->fs->RenameDirectory(SynchronizingDirectoryPath, CommittedDirectoryPath); };

    /* Synchronize working directory. */
    if (R_FAILED((rc = FsDirUtils::RetryUntilTargetNotLocked(std::move(SynchronizeWorkingDir))))) {
        return rc;
    }

    /* Delete committed directory. */
    if (R_FAILED((rc = FsDirUtils::RetryUntilTargetNotLocked(std::move(DeleteCommittedDir))))) {
        /* It is okay for us to not have a committed directory here. */
        if (rc != ResultFsPathNotFound) {
            return rc;
        }
    }

    /* Rename synchronizing directory to committed directory. */
    if (R_FAILED((rc = FsDirUtils::RetryUntilTargetNotLocked(std::move(RenameSynchDir))))) {
        return rc;
    }

    /* TODO: Should I call this->fs->Commit()? Nintendo does not. */
    return rc;
}

Result DirectorySaveDataFileSystem::GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    /* TODO: How should this work? N returns ResultFsNotImplemented. */
    return ResultFsNotImplemented;
}

Result DirectorySaveDataFileSystem::GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    /* TODO: How should this work? N returns ResultFsNotImplemented. */
    return ResultFsNotImplemented;
}

Result DirectorySaveDataFileSystem::CleanDirectoryRecursivelyImpl(const FsPath &path) {
    Result rc;
    FsPath full_path;
    
    if (R_FAILED((rc = GetFullPath(full_path, path)))) {
        return rc;
    }
    
    std::scoped_lock<HosMutex> lk(this->lock);
    return this->fs->CleanDirectoryRecursively(full_path);
}

Result DirectorySaveDataFileSystem::GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
    /* TODO: How should this work? N returns ResultFsNotImplemented. */
    return ResultFsNotImplemented;
}

Result DirectorySaveDataFileSystem::QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
    /* TODO: How should this work? N returns ResultFsNotImplemented. */
    return ResultFsNotImplemented;
}