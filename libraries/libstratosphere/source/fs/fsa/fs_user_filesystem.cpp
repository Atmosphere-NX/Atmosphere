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
#include <stratosphere.hpp>
#include "fs_filesystem_accessor.hpp"
#include "fs_file_accessor.hpp"
#include "fs_directory_accessor.hpp"
#include "fs_mount_utils.hpp"
#include "fs_user_mount_table.hpp"

namespace ams::fs {

    Result CreateFile(const char *path, s64 size) {
        R_RETURN(CreateFile(path, size, 0));
    }

    Result CreateFile(const char* path, s64 size, int option) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CreateFile(sub_path, size, option), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_SIZE, path, size));
        R_SUCCEED();
    }

    Result DeleteFile(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteFile(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        R_SUCCEED();
    }

    Result CreateDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CreateDirectory(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        R_SUCCEED();
    }

    Result DeleteDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteDirectory(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        R_SUCCEED();
    }

    Result DeleteDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteDirectoryRecursively(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        R_SUCCEED();
    }

    Result RenameFile(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        const char *old_sub_path;
        const char *new_sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(old_accessor), std::addressof(old_sub_path), old_path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(new_accessor), std::addressof(new_sub_path), new_path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));

        auto rename_impl = [=]() -> Result {
            R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
            R_TRY(old_accessor->RenameFile(old_sub_path, new_sub_path));
            R_SUCCEED();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(rename_impl(), nullptr, old_accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        R_SUCCEED();
    }

    Result RenameDirectory(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        const char *old_sub_path;
        const char *new_sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(old_accessor), std::addressof(old_sub_path), old_path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(new_accessor), std::addressof(new_sub_path), new_path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));

        auto rename_impl = [=]() -> Result {
            R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
            R_TRY(old_accessor->RenameDirectory(old_sub_path, new_sub_path));
            R_SUCCEED();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(rename_impl(), nullptr, old_accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        R_SUCCEED();
    }

    Result GetEntryType(DirectoryEntryType *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->GetEntryType(out, sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_ENTRY_TYPE(out, path)));
        R_SUCCEED();
    }

    Result OpenFile(FileHandle *out_file, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        std::unique_ptr<impl::FileAccessor> file_accessor;

        auto open_impl = [&]() -> Result {
            R_UNLESS(out_file != nullptr, fs::ResultNullptrArgument());
            R_TRY(accessor->OpenFile(std::addressof(file_accessor), sub_path, static_cast<OpenMode>(mode)));
            R_SUCCEED();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(open_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        out_file->handle = file_accessor.release();
        R_SUCCEED();
    }

    Result OpenDirectory(DirectoryHandle *out_dir, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        std::unique_ptr<impl::DirectoryAccessor> dir_accessor;

        auto open_impl = [&]() -> Result {
            R_UNLESS(out_dir != nullptr, fs::ResultNullptrArgument());
            R_TRY(accessor->OpenDirectory(std::addressof(dir_accessor), sub_path, static_cast<OpenDirectoryMode>(mode)));
            R_SUCCEED();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(open_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        out_dir->handle = dir_accessor.release();
        R_SUCCEED();
    }

    Result CleanDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CleanDirectoryRecursively(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        R_SUCCEED();
    }

    Result GetFreeSpaceSize(s64 *out, const char *path) {
        /* Find the filesystem without access logging. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Get the total space size. */
        AMS_FS_R_TRY(accessor->GetFreeSpaceSize(out, sub_path));

        R_SUCCEED();
    }

    Result GetTotalSpaceSize(s64 *out, const char *path) {
        /* Find the filesystem without access logging. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        /* Get the total space size. */
        AMS_FS_R_TRY(accessor->GetTotalSpaceSize(out, sub_path));

        R_SUCCEED();
    }

    Result SetConcatenationFileAttribute(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        AMS_FS_R_TRY(accessor->QueryEntry(nullptr, 0, nullptr, 0, fsa::QueryId::SetConcatenationFileAttribute, sub_path));

        R_SUCCEED();
    }

    Result OpenFile(FileHandle *out, std::unique_ptr<fsa::IFile> &&file, int mode) {
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto file_accessor = std::make_unique<impl::FileAccessor>(std::move(file), nullptr, static_cast<OpenMode>(mode));
        AMS_FS_R_UNLESS(file_accessor != nullptr, fs::ResultAllocationMemoryFailedNew());
        out->handle = file_accessor.release();

        R_SUCCEED();
    }

    namespace {

        Result CommitImpl(const char *mount_name, const char *func_name) {
            impl::FileSystemAccessor *accessor{};
            AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::Find(std::addressof(accessor), mount_name), AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, mount_name));

            AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM_WITH_NAME(accessor->Commit(), nullptr, accessor, func_name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, mount_name));
            R_SUCCEED();
        }

    }

    Result Commit(const char *mount_name) {
        R_RETURN(CommitImpl(mount_name, AMS_CURRENT_FUNCTION_NAME));
    }

    Result CommitSaveData(const char *mount_name) {
        R_RETURN(CommitImpl(mount_name, AMS_CURRENT_FUNCTION_NAME));
    }

}
