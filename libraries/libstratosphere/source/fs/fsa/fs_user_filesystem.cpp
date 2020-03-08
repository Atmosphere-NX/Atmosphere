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
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->CreateFile(sub_path, size, option);
    }

    Result DeleteFile(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->DeleteFile(sub_path);
    }

    Result CreateDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->CreateDirectory(sub_path);
    }

    Result DeleteDirectory(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->DeleteDirectory(sub_path);
    }

    Result DeleteDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->DeleteDirectoryRecursively(sub_path);
    }

    Result RenameFile(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        const char *old_sub_path;
        const char *new_sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(old_accessor), std::addressof(old_sub_path), old_path));
        R_TRY(impl::FindFileSystem(std::addressof(new_accessor), std::addressof(new_sub_path), new_path));

        R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
        return old_accessor->RenameFile(old_sub_path, new_sub_path);
    }

    Result RenameDirectory(const char *old_path, const char *new_path) {
        impl::FileSystemAccessor *old_accessor;
        impl::FileSystemAccessor *new_accessor;
        const char *old_sub_path;
        const char *new_sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(old_accessor), std::addressof(old_sub_path), old_path));
        R_TRY(impl::FindFileSystem(std::addressof(new_accessor), std::addressof(new_sub_path), new_path));

        R_UNLESS(old_accessor == new_accessor, fs::ResultRenameToOtherFileSystem());
        return old_accessor->RenameDirectory(old_sub_path, new_sub_path);
    }

    Result GetEntryType(DirectoryEntryType *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->GetEntryType(out, sub_path);
    }

    Result OpenFile(FileHandle *out_file, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        R_UNLESS(out_file != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<impl::FileAccessor> file_accessor;
        R_TRY(accessor->OpenFile(std::addressof(file_accessor), sub_path, static_cast<OpenMode>(mode)));

        out_file->handle = file_accessor.release();
        return ResultSuccess();
    }

    Result OpenDirectory(DirectoryHandle *out_dir, const char *path, int mode) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        R_UNLESS(out_dir != nullptr, fs::ResultNullptrArgument());

        std::unique_ptr<impl::DirectoryAccessor> dir_accessor;
        R_TRY(accessor->OpenDirectory(std::addressof(dir_accessor), sub_path, static_cast<OpenDirectoryMode>(mode)));

        out_dir->handle = dir_accessor.release();
        return ResultSuccess();
    }

    Result CleanDirectoryRecursively(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->CleanDirectoryRecursively(sub_path);
    }

    Result GetFreeSpaceSize(s64 *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->GetFreeSpaceSize(out, sub_path);
    }

    Result GetTotalSpaceSize(s64 *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->GetTotalSpaceSize(out, sub_path);
    }

    Result SetConcatenationFileAttribute(const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->QueryEntry(nullptr, 0, nullptr, 0, fsa::QueryId::SetConcatenationFileAttribute, sub_path);
    }

    Result GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        R_TRY(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), path));

        return accessor->GetFileTimeStampRaw(out, sub_path);
    }

    Result OpenFile(FileHandle *out, std::unique_ptr<fsa::IFile> &&file, int mode) {
        R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto file_accessor = std::make_unique<impl::FileAccessor>(std::move(file), nullptr, static_cast<OpenMode>(mode));
        R_UNLESS(file_accessor != nullptr, fs::ResultAllocationFailureInNew());
        out->handle = file_accessor.release();

        return ResultSuccess();
    }

    namespace {

        Result CommitImpl(const char *path) {
            impl::FileSystemAccessor *accessor;
            R_TRY(impl::Find(std::addressof(accessor), path));

            return accessor->Commit();
        }

    }

    Result Commit(const char *path) {
        return CommitImpl(path);
    }

    Result CommitSaveData(const char *path) {
        return CommitImpl(path);
    }


}
