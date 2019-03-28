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

#include "../utils.hpp"

#include "fs_filesystem_types.hpp"
#include "fs_path_utils.hpp"

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

        Result CreateFile(const FsPath &path, uint64_t size, int flags) {
            return CreateFileImpl(path, size, flags);
        }

        Result CreateFile(const FsPath &path, uint64_t size) {
            return CreateFileImpl(path, size, 0);
        }

        Result DeleteFile(const FsPath &path) {
            return DeleteFileImpl(path);
        }

        Result CreateDirectory(const FsPath &path) {
            return CreateDirectoryImpl(path);
        }

        Result DeleteDirectory(const FsPath &path) {
            return DeleteDirectoryImpl(path);
        }

        Result DeleteDirectoryRecursively(const FsPath &path) {
            return DeleteDirectoryRecursivelyImpl(path);
        }

        Result RenameFile(const FsPath &old_path, const FsPath &new_path) {
            return RenameFileImpl(old_path, new_path);
        }

        Result RenameDirectory(const FsPath &old_path, const FsPath &new_path) {
            return RenameDirectoryImpl(old_path, new_path);
        }

        Result GetEntryType(DirectoryEntryType *out, const FsPath &path) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetEntryTypeImpl(out, path);
        }

        Result OpenFile(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
            if (!(mode & OpenMode_ReadWrite)) {
                return ResultFsInvalidArgument;
            }
            if (mode & ~OpenMode_All) {
                return ResultFsInvalidArgument;
            }
            return OpenFileImpl(out_file, path, mode);
        }

        Result OpenDirectory(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
            if (!(mode & DirectoryOpenMode_All)) {
                return ResultFsInvalidArgument;
            }
            if (mode & ~DirectoryOpenMode_All) {
                return ResultFsInvalidArgument;
            }
            return OpenDirectoryImpl(out_dir, path, mode);
        }

        Result Commit() {
            return CommitImpl();
        }

        Result GetFreeSpaceSize(uint64_t *out, const FsPath &path) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetFreeSpaceSizeImpl(out, path);
        }

        Result GetTotalSpaceSize(uint64_t *out, const FsPath &path) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetTotalSpaceSizeImpl(out, path);
        }

        Result CleanDirectoryRecursively(const FsPath &path) {
            return CleanDirectoryRecursivelyImpl(path);
        }

        Result GetFileTimeStampRaw(FsTimeStampRaw *out, const FsPath &path) {
            if (out == nullptr) {
                return ResultFsNullptrArgument;
            }
            return GetFileTimeStampRawImpl(out, path);
        }

        Result QueryEntry(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
            return QueryEntryImpl(out, out_size, in, in_size, query, path);
        }


    protected:
        /* ...? */
    private:
        virtual Result CreateFileImpl(const FsPath &path, uint64_t size, int flags) = 0;
        virtual Result DeleteFileImpl(const FsPath &path) = 0;
        virtual Result CreateDirectoryImpl(const FsPath &path) = 0;
        virtual Result DeleteDirectoryImpl(const FsPath &path) = 0;
        virtual Result DeleteDirectoryRecursivelyImpl(const FsPath &path) = 0;
        virtual Result RenameFileImpl(const FsPath &old_path, const FsPath &new_path) = 0;
        virtual Result RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) = 0;
        virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) = 0;
        virtual Result OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) = 0;
        virtual Result OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) = 0;
        virtual Result CommitImpl() = 0;

        virtual Result GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }

        virtual Result GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }

        virtual Result CleanDirectoryRecursivelyImpl(const FsPath &path) = 0;

        virtual Result GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
            (void)(out);
            (void)(path);
            return ResultFsNotImplemented;
        }

        virtual Result QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
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
        virtual Result CreateFile(InPointer<char> in_path, uint64_t size, int flags) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->CreateFile(path, size, flags);
        }

        virtual Result DeleteFile(InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->DeleteFile(path);
        }

        virtual Result CreateDirectory(InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->CreateDirectory(path);
        }

        virtual Result DeleteDirectory(InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->DeleteDirectory(path);
        }

        virtual Result DeleteDirectoryRecursively(InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->DeleteDirectoryRecursively(path);
        }

        virtual Result RenameFile(InPointer<char> in_old_path, InPointer<char> in_new_path) final {
            FsPath old_path;
            FsPath new_path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&old_path, in_old_path.pointer)))) {
                return rc;
            }
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&new_path, in_new_path.pointer)))) {
                return rc;
            }

            return this->base_fs->RenameFile(old_path, new_path);
        }

        virtual Result RenameDirectory(InPointer<char> in_old_path, InPointer<char> in_new_path) final {
            FsPath old_path;
            FsPath new_path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&old_path, in_old_path.pointer)))) {
                return rc;
            }
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&new_path, in_new_path.pointer)))) {
                return rc;
            }

            return this->base_fs->RenameDirectory(old_path, new_path);
        }


        virtual Result GetEntryType(Out<u32> out_type, InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            DirectoryEntryType type;
            rc = this->base_fs->GetEntryType(&type, path);
            if (R_SUCCEEDED(rc)) {
                out_type.SetValue(type);
            }
            return rc;
        }

        virtual Result OpenFile(Out<std::shared_ptr<IFileInterface>> out_intf, InPointer<char> in_path, uint32_t mode) final {
            FsPath path;
            std::unique_ptr<IFile> out_file;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            rc = this->base_fs->OpenFile(out_file, path, static_cast<OpenMode>(mode));
            if (R_SUCCEEDED(rc)) {
                out_intf.SetValue(std::make_shared<IFileInterface>(std::move(out_file)));
                /* TODO: Nintendo checks allocation success here, should we?. */
            }
            return rc;
        }

        virtual Result OpenDirectory(Out<std::shared_ptr<IDirectoryInterface>> out_intf, InPointer<char> in_path, uint32_t mode) final {
            FsPath path;
            std::unique_ptr<IDirectory> out_dir;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            rc = this->base_fs->OpenDirectory(out_dir, path, static_cast<DirectoryOpenMode>(mode));

            if (R_SUCCEEDED(rc)) {
                out_intf.SetValue(std::make_shared<IDirectoryInterface>(std::move(out_dir)));
                /* TODO: Nintendo checks allocation success here, should we?. */
            }

            return rc;
        }

        virtual Result Commit() final {
            return this->base_fs->Commit();
        }

        virtual Result GetFreeSpaceSize(Out<uint64_t> out_size, InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->GetFreeSpaceSize(out_size.GetPointer(), path);
        }

        virtual Result GetTotalSpaceSize(Out<uint64_t> out_size, InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->GetTotalSpaceSize(out_size.GetPointer(), path);
        }

        virtual Result CleanDirectoryRecursively(InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->CleanDirectoryRecursively(path);
        }

        virtual Result GetFileTimeStampRaw(Out<FsTimeStampRaw> out_timestamp, InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->GetFileTimeStampRaw(out_timestamp.GetPointer(), path);
        }

        virtual Result QueryEntry(OutBuffer<char, BufferType_Type1> out_buffer, InBuffer<char, BufferType_Type1> in_buffer, int query, InPointer<char> in_path) final {
            FsPath path;

            Result rc;
            if (R_FAILED((rc = FsPathUtils::ConvertPathForServiceObject(&path, in_path.pointer)))) {
                return rc;
            }

            return this->base_fs->QueryEntry(out_buffer.buffer, out_buffer.num_elements, in_buffer.buffer, in_buffer.num_elements, query, path);
        }
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
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

            /* 3.0.0- */
            MakeServiceCommandMeta<FsIFileSystemCmd_CleanDirectoryRecursively, &IFileSystemInterface::CleanDirectoryRecursively, FirmwareVersion_300>(),
            MakeServiceCommandMeta<FsIFileSystemCmd_GetFileTimeStampRaw, &IFileSystemInterface::GetFileTimeStampRaw, FirmwareVersion_300>(),

            /* 4.0.0- */
            MakeServiceCommandMeta<FsIFileSystemCmd_QueryEntry, &IFileSystemInterface::QueryEntry, FirmwareVersion_400>(),
        };
};

class ProxyFileSystem : public IFileSystem {
    private:
        std::unique_ptr<FsFileSystem> base_fs;
    public:
        ProxyFileSystem(FsFileSystem *fs) : base_fs(fs) {
            /* ... */
        }

        ProxyFileSystem(std::unique_ptr<FsFileSystem> fs) : base_fs(std::move(fs)) {
            /* ... */
        }

        ProxyFileSystem(FsFileSystem fs) {
            this->base_fs = std::make_unique<FsFileSystem>(fs);
        }

        virtual ~ProxyFileSystem() {
            fsFsClose(this->base_fs.get());
        }

    public:
        virtual Result CreateFileImpl(const FsPath &path, uint64_t size, int flags) {
            return fsFsCreateFile(this->base_fs.get(), path.str, size, flags);
        }

        virtual Result DeleteFileImpl(const FsPath &path) {
            return fsFsDeleteFile(this->base_fs.get(), path.str);
        }

        virtual Result CreateDirectoryImpl(const FsPath &path) {
            return fsFsCreateDirectory(this->base_fs.get(), path.str);
        }

        virtual Result DeleteDirectoryImpl(const FsPath &path) {
            return fsFsDeleteDirectory(this->base_fs.get(), path.str);
        }

        virtual Result DeleteDirectoryRecursivelyImpl(const FsPath &path) {
            return fsFsDeleteDirectoryRecursively(this->base_fs.get(), path.str);
        }

        virtual Result RenameFileImpl(const FsPath &old_path, const FsPath &new_path) {
            return fsFsRenameFile(this->base_fs.get(), old_path.str, new_path.str);
        }

        virtual Result RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) {
            return fsFsRenameDirectory(this->base_fs.get(), old_path.str, new_path.str);
        }

        virtual Result GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) {
            FsEntryType type;

            Result rc = fsFsGetEntryType(this->base_fs.get(), path.str, &type);
            if (R_SUCCEEDED(rc)) {
                *out = static_cast<DirectoryEntryType>(static_cast<u32>(type));
            }

            return rc;
        }
        virtual Result OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
            FsFile f;

            Result rc = fsFsOpenFile(this->base_fs.get(), path.str, static_cast<int>(mode), &f);
            if (R_SUCCEEDED(rc)) {
                out_file = std::make_unique<ProxyFile>(f);
            }

            return rc;
        }

        virtual Result OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
            FsDir d;

            Result rc = fsFsOpenDirectory(this->base_fs.get(), path.str, static_cast<int>(mode), &d);
            if (R_SUCCEEDED(rc)) {
                out_dir = std::make_unique<ProxyDirectory>(d);
            }

            return rc;
        }

        virtual Result CommitImpl() {
            return fsFsCommit(this->base_fs.get());
        }

        virtual Result GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
            return fsFsGetFreeSpace(this->base_fs.get(), path.str, out);
        }

        virtual Result GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
            return fsFsGetTotalSpace(this->base_fs.get(), path.str, out);
        }

        virtual Result CleanDirectoryRecursivelyImpl(const FsPath &path) {
            return fsFsCleanDirectoryRecursively(this->base_fs.get(), path.str);
        }

        virtual Result GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
            return fsFsGetFileTimeStampRaw(this->base_fs.get(), path.str, out);
        }

        virtual Result QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
            return fsFsQueryEntry(this->base_fs.get(), out, out_size, in, in_size, path.str,static_cast<FsFileSystemQueryType>(query));
        }
};