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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "fs_results.hpp"
#include "fs_filesystem_types.hpp"
#include "fs_ifile.hpp"
#include "fs_idirectory.hpp"

enum FsIFileSystemCmd : u32 {
    /* 1.0.0+ */
    FsIFileSystemCmd_CreateFile = 0,
    FsIFileSystemCmd_DeleteFile = 1,
    FsIFileSystemCmd_CreateDirectory = 2,
    FsIFileSystemCmd_DeleteDirectory = 3,
    FsIFileSystemCmd_DeleteDirectoryRecursively = 4,
    FsIFileSystemCmd_RenameFile = 5,
    FsIFileSystemCmd_RenameDirectory = 6,
    FsIFileSystemCmd_GetEntryType = 7,
    FsIFileSystemCmd_OpenFile = 8,
    FsIFileSystemCmd_OpenDirectory = 9,
    FsIFileSystemCmd_Commit = 10,
    FsIFileSystemCmd_GetFreeSpaceSize = 11,
    FsIFileSystemCmd_GetTotalSpaceSize = 12,
    
    /* 3.0.0+ */
    FsIFileSystemCmd_CleanDirectoryRecursively = 13,
    FsIFileSystemCmd_GetFileTimeStampRaw = 14,
    
    /* 4.0.0+ */
    FsIFileSystemCmd_QueryEntry = 15,
};
    
class IFile;
class IDirectory;

class IFileSystem {
    public:
        virtual ~IFileSystem() {}
        
        Result CreateFile(const char *path, uint64_t size, int flags) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return CreateFileImpl(path, size, flags);
        }
        
        Result DeleteFile(const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return DeleteFileImpl(path);                
        }
        
        Result CreateDirectory(const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return CreateDirectoryImpl(path);                
        }
        
        Result DeleteDirectory(const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return DeleteDirectoryImpl(path);                
        }
        
        Result DeleteDirectoryRecursively(const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return DeleteDirectoryRecursivelyImpl(path);                
        }
        
        Result RenameFile(const char *old_path, const char *new_path) {
            if (old_path == nullptr || new_path == nullptr) {
                return ResultFsInvalidPath;
            }
            return RenameFileImpl(old_path, new_path);
        }
        
        Result RenameDirectory(const char *old_path, const char *new_path) {
            if (old_path == nullptr || new_path == nullptr) {
                return ResultFsInvalidPath;
            }
            return RenameDirectoryImpl(old_path, new_path);
        }
        
        Result GetEntryType(DirectoryEntryType *out, const char *path) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return GetEntryTypeImpl(out, path);
        }
        
        Result OpenFile(std::unique_ptr<IFile> *out_file, const char *path, OpenMode mode) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            if (out_file == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (!(mode & OpenMode_ReadWrite)) {
                return ResultFsInvalidArgument;
            }
            if (mode & ~OpenMode_All) {
                return ResultFsInvalidArgument;
            }
            return OpenFileImpl(out_file, path, mode);
        }
        
        Result OpenDirectory(std::unique_ptr<IDirectory> *out_dir, const char *path, DirectoryOpenMode mode) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            if (out_dir == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (!(mode & DirectoryOpenMode_All)) {
                return ResultFsInvalidArgument;
            }
            if (mode & ~DirectoryOpenMode_All) {
                return ResultFsInvalidArgument;
            }
            return OpenDirectory(out_dir, path, mode);
        }
        
        Result Commit() {
            return CommitImpl();
        }
        
        Result GetFreeSpaceSize(uint64_t *out, const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetFreeSpaceSizeImpl(out, path);
        }
        
        Result GetTotalSpaceSize(uint64_t *out, const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetTotalSpaceSizeImpl(out, path);
        }
        
        Result CleanDirectoryRecursively(const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return CleanDirectoryRecursivelyImpl(path);                
        }
        
        Result GetFileTimeStampRaw(FsTimeStampRaw *out, const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetFileTimeStampRawImpl(out, path);
        }
        
        Result QueryEntry(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const char *path) {
            if (path == nullptr) {
                return ResultFsInvalidPath;
            }
            return QueryEntryImpl(out, out_size, in, in_size, query, path);
        }
        
        
    protected:
        /* ...? */
    private:
        virtual Result CreateFileImpl(const char *path, uint64_t size, int flags) = 0;
        virtual Result DeleteFileImpl(const char *path) = 0;
        virtual Result CreateDirectoryImpl(const char *path) = 0;
        virtual Result DeleteDirectoryImpl(const char *path) = 0;
        virtual Result DeleteDirectoryRecursivelyImpl(const char *path) = 0;
        virtual Result RenameFileImpl(const char *old_path, const char *new_path) = 0;
        virtual Result RenameDirectoryImpl(const char *old_path, const char *new_path) = 0;
        virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const char *path) = 0;
        virtual Result OpenFileImpl(std::unique_ptr<IFile> *out_file, const char *path, OpenMode mode) = 0;
        virtual Result OpenDirectoryImpl(std::unique_ptr<IDirectory> *out_dir, const char *path, DirectoryOpenMode mode) = 0;
        virtual Result CommitImpl() = 0;
        
        virtual Result GetFreeSpaceSizeImpl(uint64_t *out, const char *path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }
        
        virtual Result GetTotalSpaceSizeImpl(uint64_t *out, const char *path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }
        
        virtual Result CleanDirectoryRecursivelyImpl(const char *path) = 0;
        
        virtual Result GetFileTimeStampRawImpl(FsTimeStampRaw *out, const char *path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }
        
        virtual Result QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const char *path) {
            (void)(out);
            (void)(out_size);
            (void)(in);
            (void)(in_size);
            (void)(query);
            (void)(path);
            return ResultFsNotImplemented;
        }
};

class IFileSystemInterface : public IServiceObject {
    private:
        std::unique_ptr<IFileSystem> base_fs;
    public:
        IFileSystemInterface(IFileSystem *fs) : base_fs(fs) {
            /* ... */
        };
        IFileSystemInterface(std::unique_ptr<IFileSystem> fs) : base_fs(std::move(fs)) {
            /* ... */
        };
        
    private:
        /* Actual command API. */
        /* TODO.... */
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
            /*
                MakeServiceCommandMeta<FsIFileSystemCmd_CreateFile, &IFileSystemInterface::CreateFile>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_DeleteFile, &IFileSystemInterface::DeleteFile>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_CreateDirectory, &IFileSystemInterface::CreateDirectory>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_DeleteDirectory, &IFileSystemInterface::DeleteDirectory>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_DeleteDirectoryRecursively, &IFileSystemInterface::DeleteDirectoryRecursively>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_RenameFile, &IFileSystemInterface::RenameFile>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_RenameDirectory, &IFileSystemInterface::RenameDirectory>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_GetEntryType, &IFileSystemInterface::GetEntryType>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_OpenFile, &IFileSystemInterface::OpenFile>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_OpenDirectory, &IFileSystemInterface::OpenDirectory>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_Commit, &IFileSystemInterface::Commit>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_GetFreeSpaceSize, &IFileSystemInterface::GetFreeSpaceSize>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_GetTotalSpaceSize, &IFileSystemInterface::GetTotalSpaceSize>(),
            */

            /* 3.0.0- */
            /*
                MakeServiceCommandMeta<FsIFileSystemCmd_CleanDirectoryRecursively, &IFileSystemInterface::CleanDirectoryRecursively, FirmwareVersion_300>(),
                MakeServiceCommandMeta<FsIFileSystemCmd_GetFileTimeStampRaw, &IFileSystemInterface::GetFileTimeStampRaw, FirmwareVersion_300>(),
            */

            /* 4.0.0- */
            /* <FsIFileSystemCmd_QueryEntry, &IFileSystemInterface::QueryEntry, FirmwareVersion_400>(), */
        };
};
