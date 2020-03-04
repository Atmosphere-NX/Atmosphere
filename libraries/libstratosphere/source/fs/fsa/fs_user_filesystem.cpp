/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software {

    }
 you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY {

    }
 without even the implied warranty of MERCHANTABILITY or
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
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->CreateFile(impl::GetSubPath(path), size, option);
    }

    Result DeleteFile(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->DeleteFile(impl::GetSubPath(path));
    }

    Result CreateDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->CreateDirectory(impl::GetSubPath(path));
    }

    Result DeleteDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->DeleteDirectory(impl::GetSubPath(path));
    }

    Result DeleteDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->DeleteDirectoryRecursively(impl::GetSubPath(path));
    }

    Result RenameFile(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        R_TRY(impl::FindFileSystem(std::addressof(old_accessor), old_path));
        R_TRY(impl::FindFileSystem(std::addressof(new_accessor), new_path));

        R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
        return old_accessor->RenameFile(impl::GetSubPath(old_path), impl::GetSubPath(new_path));
    }

    Result RenameDirectory(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        R_TRY(impl::FindFileSystem(std::addressof(old_accessor), old_path));
        R_TRY(impl::FindFileSystem(std::addressof(new_accessor), new_path));

        R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
        return old_accessor->RenameDirectory(impl::GetSubPath(old_path), impl::GetSubPath(new_path));
    }

    Result GetEntryType(DirectoryEntryType *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->GetEntryType(out, impl::GetSubPath(path));
    }

    Result OpenFile(FileHandle *out_file, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        R_UNLESS(out_file != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<impl::FileAccessor> file_accessor;
        R_TRY(accessor->OpenFile(std::addressof(file_accessor), impl::GetSubPath(path), static_cast<OpenMode>(mode)));

        out_file->handle = file_accessor.release();
        return ResultSuccess();
    }

    Result OpenDirectory(DirectoryHandle *out_dir, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        R_UNLESS(out_dir != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<impl::DirectoryAccessor> dir_accessor;
        R_TRY(accessor->OpenDirectory(std::addressof(dir_accessor), impl::GetSubPath(path), static_cast<OpenDirectoryMode>(mode)));

        out_dir->handle = dir_accessor.release();
        return ResultSuccess();
    }

    Result CleanDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->CleanDirectoryRecursively(impl::GetSubPath(path));
    }

    Result GetFreeSpaceSize(s64 *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->GetFreeSpaceSize(out, impl::GetSubPath(path));
    }

    Result GetTotalSpaceSize(s64 *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->GetTotalSpaceSize(out, impl::GetSubPath(path));
    }


    Result SetConcatenationFileAttribute(const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->QueryEntry(nullptr, 0, nullptr, 0, fsa::QueryId::SetConcatenationFileAttribute, impl::GetSubPath(path));
    }

    Result GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), path));

        return accessor->GetFileTimeStampRaw(out, impl::GetSubPath(path));
    }

    Result OpenFile(FileHandle *out, std::unique_ptr<fsa::IFile> &&file, int mode) {
        R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<impl::FileAccessor> file_accessor(new impl::FileAccessor(std::move(file), nullptr, static_cast<OpenMode>(mode)));
        R_UNLESS(file_accessor != nullptr, fs::ResultAllocationFailureInUserFileSystem());
        out->handle = file_accessor.release();

        return ResultSuccess();
    }


}
