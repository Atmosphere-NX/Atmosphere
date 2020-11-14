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
#include "fs_filesystem_accessor.hpp"
#include "fs_file_accessor.hpp"
#include "fs_directory_accessor.hpp"
#include "fs_mount_utils.hpp"
#include "fs_user_mount_table.hpp"

namespace ams::fs {

    Result CreateFile(const char *path, s64 size) {
        return CreateFile(path, size, 0);
    }

    Result CreateFile(const char* path, s64 size, int option) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CreateFile(sub_path, size, option), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_SIZE, path, size));
        return ResultSuccess();
    }

    Result DeleteFile(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteFile(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        return ResultSuccess();
    }

    Result CreateDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CreateDirectory(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        return ResultSuccess();
    }

    Result DeleteDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteDirectory(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        return ResultSuccess();
    }

    Result DeleteDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->DeleteDirectoryRecursively(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        return ResultSuccess();
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
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(rename_impl(), nullptr, old_accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        return ResultSuccess();
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
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(rename_impl(), nullptr, old_accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME, old_path, new_path));
        return ResultSuccess();
    }

    Result GetEntryType(DirectoryEntryType *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->GetEntryType(out, sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_ENTRY_TYPE(out, path)));
        return ResultSuccess();
    }

    Result OpenFile(FileHandle *out_file, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        std::unique_ptr<impl::FileAccessor> file_accessor;

        auto open_impl = [&]() -> Result {
            R_UNLESS(out_file != nullptr, fs::ResultNullptrArgument());
            R_TRY(accessor->OpenFile(std::addressof(file_accessor), sub_path, static_cast<OpenMode>(mode)));
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(open_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        out_file->handle = file_accessor.release();
        return ResultSuccess();
    }

    Result OpenDirectory(DirectoryHandle *out_dir, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        std::unique_ptr<impl::DirectoryAccessor> dir_accessor;

        auto open_impl = [&]() -> Result {
            R_UNLESS(out_dir != nullptr, fs::ResultNullptrArgument());
            R_TRY(accessor->OpenDirectory(std::addressof(dir_accessor), sub_path, static_cast<OpenDirectoryMode>(mode)));
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(open_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE, path, static_cast<u32>(mode)));

        out_dir->handle = dir_accessor.release();
        return ResultSuccess();
    }

    Result CleanDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(accessor->CleanDirectoryRecursively(sub_path), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));
        return ResultSuccess();
    }

    Result GetFreeSpaceSize(s64 *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path = nullptr;

        /* Get the accessor. */
        auto find_impl = [&]() -> Result {
            R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
            R_UNLESS(path != nullptr, fs::ResultNullptrArgument());
            if (impl::IsValidMountName(path)) {
                R_TRY(impl::Find(std::addressof(accessor), path));
            } else {
                R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));
            }
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(find_impl(), AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_SPACE_SIZE(out, path)));

        /* Get the space size. */
        auto get_size_impl = [&]() -> Result {
            R_UNLESS(sub_path == nullptr || std::strcmp(sub_path, "/") == 0, fs::ResultInvalidMountName());
            R_TRY(accessor->GetFreeSpaceSize(out, "/"));
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(get_size_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_SPACE_SIZE(out, path)));
        return ResultSuccess();
    }

    Result GetTotalSpaceSize(s64 *out, const char *path) {
        /* NOTE: Nintendo does not do access logging here, and does not support mount-name instead of path. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path = nullptr;

        /* Get the accessor. */
        auto find_impl = [&]() -> Result {
            R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
            R_UNLESS(path != nullptr, fs::ResultNullptrArgument());
            if (impl::IsValidMountName(path)) {
                R_TRY(impl::Find(std::addressof(accessor), path));
            } else {
                R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));
            }
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(find_impl(), AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_SPACE_SIZE(out, path)));

        /* Get the space size. */
        auto get_size_impl = [&]() -> Result {
            R_UNLESS(sub_path == nullptr || std::strcmp(sub_path, "/") == 0, fs::ResultInvalidMountName());
            R_TRY(accessor->GetTotalSpaceSize(out, "/"));
            return ResultSuccess();
        };

        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(get_size_impl(), nullptr, accessor, AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_SPACE_SIZE(out, path)));
        return ResultSuccess();
    }

    Result SetConcatenationFileAttribute(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        AMS_FS_R_TRY(accessor->QueryEntry(nullptr, 0, nullptr, 0, fsa::QueryId::SetConcatenationFileAttribute, sub_path));

        return ResultSuccess();
    }

    Result OpenFile(FileHandle *out, std::unique_ptr<fsa::IFile> &&file, int mode) {
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto file_accessor = std::make_unique<impl::FileAccessor>(std::move(file), nullptr, static_cast<OpenMode>(mode));
        AMS_FS_R_UNLESS(file_accessor != nullptr, fs::ResultAllocationFailureInNew());
        out->handle = file_accessor.release();

        return ResultSuccess();
    }

    namespace {

        Result CommitImpl(const char *path, const char *func_name) {
            impl::FileSystemAccessor *accessor;
            AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(impl::Find(std::addressof(accessor), path), AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, path));

            AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM_WITH_NAME(accessor->Commit(), nullptr, accessor, func_name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, path));
            return ResultSuccess();
        }

    }

    Result Commit(const char *path) {
        return CommitImpl(path, AMS_CURRENT_FUNCTION_NAME);
    }

    Result CommitSaveData(const char *path) {
        return CommitImpl(path, AMS_CURRENT_FUNCTION_NAME);
    }

}
