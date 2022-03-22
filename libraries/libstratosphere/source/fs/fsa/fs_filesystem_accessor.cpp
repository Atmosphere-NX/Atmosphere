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
#include "fs_file_accessor.hpp"
#include "fs_directory_accessor.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs::impl {

    namespace {

        template<typename List, typename Iter>
        void Remove(List &list, Iter *desired) {
            for (auto it = list.cbegin(); it != list.cend(); it++) {
                if (it.operator->() == desired) {
                    list.erase(it);
                    return;
                }
            }

            /* This should never happen. */
            AMS_ABORT();
        }

        Result ValidatePath(const char *mount_name, const char *path) {
            const size_t mount_name_len = strnlen(mount_name, MountNameLengthMax);
            const size_t path_len       = strnlen(path, EntryNameLengthMax);
            R_UNLESS(mount_name_len + 1 + path_len <= EntryNameLengthMax, fs::ResultTooLongPath());

            return ResultSuccess();
        }

        Result ValidateMountName(const char *name) {
            R_UNLESS(name[0] != 0, fs::ResultInvalidMountName());
            R_UNLESS(strnlen(name, sizeof(MountName)) < sizeof(MountName), fs::ResultInvalidMountName());
            return ResultSuccess();
        }

        template<typename List>
        Result ValidateNoOpenWriteModeFiles(List &list) {
            for (auto it = list.cbegin(); it != list.cend(); it++) {
                R_UNLESS((it->GetOpenMode() & OpenMode_Write) == 0, fs::ResultWriteModeFileNotClosed());
            }
            return ResultSuccess();
        }

    }

    FileSystemAccessor::FileSystemAccessor(const char *n, std::unique_ptr<fsa::IFileSystem> &&fs, std::unique_ptr<fsa::ICommonMountNameGenerator> &&generator)
        : m_impl(std::move(fs)), m_open_list_lock(), m_mount_name_generator(std::move(generator)),
          m_access_log_enabled(false), m_data_cache_attachable(false), m_path_cache_attachable(false), m_path_cache_attached(false), m_multi_commit_supported(false)
    {
        R_ABORT_UNLESS(ValidateMountName(n));
        std::strncpy(m_name.str, n, MountNameLengthMax);
        m_name.str[MountNameLengthMax] = 0;
    }

    FileSystemAccessor::~FileSystemAccessor() {
        std::scoped_lock lk(m_open_list_lock);

        /* TODO: Iterate over list entries. */

        if (!m_open_file_list.empty()) { R_ABORT_UNLESS(fs::ResultFileNotClosed()); }
        if (!m_open_dir_list.empty()) { R_ABORT_UNLESS(fs::ResultDirectoryNotClosed()); }

        if (m_path_cache_attached) {
            /* TODO: Invalidate path cache */
        }
    }

    Result FileSystemAccessor::GetCommonMountName(char *dst, size_t dst_size) const {
        R_UNLESS(m_mount_name_generator != nullptr, fs::ResultPreconditionViolation());
        return m_mount_name_generator->GenerateCommonMountName(dst, dst_size);
    }

    std::shared_ptr<fssrv::impl::FileSystemInterfaceAdapter> FileSystemAccessor::GetMultiCommitTarget() {
        if (m_multi_commit_supported) {
            /* TODO: Support multi commit. */
            AMS_ABORT();
        }
        return nullptr;
    }

    void FileSystemAccessor::NotifyCloseFile(FileAccessor *f) {
        std::scoped_lock lk(m_open_list_lock);
        Remove(m_open_file_list, f);
    }

    void FileSystemAccessor::NotifyCloseDirectory(DirectoryAccessor *d) {
        std::scoped_lock lk(m_open_list_lock);
        Remove(m_open_dir_list, d);
    }

    Result FileSystemAccessor::CreateFile(const char *path, s64 size, int option) {
        R_TRY(ValidatePath(m_name.str, path));
        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->CreateFile(path, size, option));
        } else {
            R_TRY(m_impl->CreateFile(path, size, option));
        }
        return ResultSuccess();
    }

    Result FileSystemAccessor::DeleteFile(const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->DeleteFile(path);
    }

    Result FileSystemAccessor::CreateDirectory(const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->CreateDirectory(path);
    }

    Result FileSystemAccessor::DeleteDirectory(const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->DeleteDirectory(path);
    }

    Result FileSystemAccessor::DeleteDirectoryRecursively(const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->DeleteDirectoryRecursively(path);
    }

    Result FileSystemAccessor::RenameFile(const char *old_path, const char *new_path) {
        R_TRY(ValidatePath(m_name.str, old_path));
        R_TRY(ValidatePath(m_name.str, new_path));
        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->RenameFile(old_path, new_path));
        } else {
            R_TRY(m_impl->RenameFile(old_path, new_path));
        }
        return ResultSuccess();
    }

    Result FileSystemAccessor::RenameDirectory(const char *old_path, const char *new_path) {
        R_TRY(ValidatePath(m_name.str, old_path));
        R_TRY(ValidatePath(m_name.str, new_path));
        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->RenameDirectory(old_path, new_path));
        } else {
            R_TRY(m_impl->RenameDirectory(old_path, new_path));
        }
        return ResultSuccess();
    }

    Result FileSystemAccessor::GetEntryType(DirectoryEntryType *out, const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->GetEntryType(out, path);
    }

    Result FileSystemAccessor::OpenFile(std::unique_ptr<FileAccessor> *out_file, const char *path, OpenMode mode) {
        R_TRY(ValidatePath(m_name.str, path));

        std::unique_ptr<fsa::IFile> file;
        R_TRY(m_impl->OpenFile(std::addressof(file), path, mode));

        auto accessor = new FileAccessor(std::move(file), this, mode);
        R_UNLESS(accessor != nullptr, fs::ResultAllocationFailureInFileSystemAccessorA());

        {
            std::scoped_lock lk(m_open_list_lock);
            m_open_file_list.push_back(*accessor);
        }

        if (m_path_cache_attached) {
            if (mode & OpenMode_AllowAppend) {
                /* TODO: Append Path cache */
            } else {
                /* TODO: Non-append path cache */
            }
        }

        out_file->reset(accessor);
        return ResultSuccess();
    }

    Result FileSystemAccessor::OpenDirectory(std::unique_ptr<DirectoryAccessor> *out_dir, const char *path, OpenDirectoryMode mode) {
        R_TRY(ValidatePath(m_name.str, path));

        std::unique_ptr<fsa::IDirectory> dir;
        R_TRY(m_impl->OpenDirectory(std::addressof(dir), path, mode));

        auto accessor = new DirectoryAccessor(std::move(dir), *this);
        R_UNLESS(accessor != nullptr, fs::ResultAllocationFailureInFileSystemAccessorB());

        {
            std::scoped_lock lk(m_open_list_lock);
            m_open_dir_list.push_back(*accessor);
        }

        out_dir->reset(accessor);
        return ResultSuccess();
    }

    Result FileSystemAccessor::Commit() {
        {
            std::scoped_lock lk(m_open_list_lock);
            R_ABORT_UNLESS(ValidateNoOpenWriteModeFiles(m_open_file_list));
        }
        return m_impl->Commit();
    }

    Result FileSystemAccessor::GetFreeSpaceSize(s64 *out, const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->GetFreeSpaceSize(out, path);
    }

    Result FileSystemAccessor::GetTotalSpaceSize(s64 *out, const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->GetTotalSpaceSize(out, path);
    }

    Result FileSystemAccessor::CleanDirectoryRecursively(const char *path) {
        R_TRY(ValidatePath(m_name.str, path));
        return m_impl->CleanDirectoryRecursively(path);
    }

    Result FileSystemAccessor::GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
        return m_impl->GetFileTimeStampRaw(out, path);
    }

    Result FileSystemAccessor::QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const char *path) {
        return m_impl->QueryEntry(dst, dst_size, src, src_size, query, path);
    }


}
