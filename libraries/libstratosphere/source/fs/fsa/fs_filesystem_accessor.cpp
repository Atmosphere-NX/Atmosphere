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
            for (auto it = list.cbegin(); it != list.cend(); ++it) {
                if (it.operator->() == desired) {
                    list.erase(it);
                    return;
                }
            }

            /* This should never happen. */
            AMS_ABORT();
        }

        Result SetMountName(char *dst, const char *name) {
            R_UNLESS(name[0] != 0, fs::ResultInvalidMountName());

            const size_t n_len = util::Strlcpy<char>(dst, name, MountNameLengthMax + 1);
            R_UNLESS(n_len <= MountNameLengthMax, fs::ResultInvalidMountName());

            R_SUCCEED();
        }

        template<typename List>
        Result ValidateNoOpenWriteModeFiles(List &list) {
            for (auto it = list.cbegin(); it != list.cend(); it++) {
                R_UNLESS((it->GetOpenMode() & OpenMode_Write) == 0, fs::ResultWriteModeFileNotClosed());
            }
            R_SUCCEED();
        }

    }

    FileSystemAccessor::FileSystemAccessor(const char *n, std::unique_ptr<fsa::IFileSystem> &&fs, std::unique_ptr<fsa::ICommonMountNameGenerator> &&generator)
        : m_impl(std::move(fs)), m_open_list_lock(), m_mount_name_generator(std::move(generator)),
          m_access_log_enabled(false), m_data_cache_attachable(false), m_path_cache_attachable(false), m_path_cache_attached(false), m_multi_commit_supported(false),
          m_path_flags()
    {
        R_ABORT_UNLESS(SetMountName(m_name.str, n));

        if (std::strcmp(m_name.str, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME) == 0) {
            m_path_flags.AllowWindowsPath();
        }
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
        R_RETURN(m_mount_name_generator->GenerateCommonMountName(dst, dst_size));
    }

    std::shared_ptr<fssrv::impl::FileSystemInterfaceAdapter> FileSystemAccessor::GetMultiCommitTarget() {
        if (m_multi_commit_supported) {
            AMS_ABORT("TODO: Support multi commit");
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

    Result FileSystemAccessor::SetUpPath(fs::Path *out, const char *p) {
        /* Initialize the path appropriately. */
        bool normalized;
        size_t len;
        if (R_SUCCEEDED(PathFormatter::IsNormalized(std::addressof(normalized), std::addressof(len), p, m_path_flags)) && normalized) {
            /* We can use the input buffer directly. */
            R_TRY(out->SetShallowBuffer(p));
        } else {
            /* Initialize with appropriate slash replacement. */
            if (m_path_flags.IsWindowsPathAllowed()) {
                R_TRY(out->InitializeWithReplaceForwardSlashes(p));
            } else {
                R_TRY(out->InitializeWithReplaceBackslash(p));
            }

            /* Ensure we're normalized. */
            R_TRY(out->Normalize(m_path_flags));
        }

        /* Check the path isn't too long. */
        R_UNLESS(out->GetLength() <= fs::EntryNameLengthMax, fs::ResultTooLongPath());
        R_SUCCEED();
    }

    Result FileSystemAccessor::CreateFile(const char *path, s64 size, int option) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->CreateFile(normalized_path, size, option));
        } else {
            R_TRY(m_impl->CreateFile(normalized_path, size, option));
        }

        R_SUCCEED();
    }

    Result FileSystemAccessor::DeleteFile(const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->DeleteFile(normalized_path));
    }

    Result FileSystemAccessor::CreateDirectory(const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->CreateDirectory(normalized_path));
    }

    Result FileSystemAccessor::DeleteDirectory(const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->DeleteDirectory(normalized_path));
    }

    Result FileSystemAccessor::DeleteDirectoryRecursively(const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->DeleteDirectoryRecursively(normalized_path));
    }

    Result FileSystemAccessor::RenameFile(const char *old_path, const char *new_path) {
        /* Create path. */
        fs::Path normalized_old_path;
        fs::Path normalized_new_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_old_path), old_path));
        R_TRY(this->SetUpPath(std::addressof(normalized_new_path), new_path));

        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->RenameFile(normalized_old_path, normalized_new_path));
        } else {
            R_TRY(m_impl->RenameFile(normalized_old_path, normalized_new_path));
        }

        R_SUCCEED();
    }

    Result FileSystemAccessor::RenameDirectory(const char *old_path, const char *new_path) {
        /* Create path. */
        fs::Path normalized_old_path;
        fs::Path normalized_new_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_old_path), old_path));
        R_TRY(this->SetUpPath(std::addressof(normalized_new_path), new_path));

        if (m_path_cache_attached) {
            /* TODO: Path cache */
            R_TRY(m_impl->RenameDirectory(normalized_old_path, normalized_new_path));
        } else {
            R_TRY(m_impl->RenameDirectory(normalized_old_path, normalized_new_path));
        }

        R_SUCCEED();
    }

    Result FileSystemAccessor::GetEntryType(DirectoryEntryType *out, const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->GetEntryType(out, normalized_path));
    }

    Result FileSystemAccessor::OpenFile(std::unique_ptr<FileAccessor> *out_file, const char *path, OpenMode mode) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        std::unique_ptr<fsa::IFile> file;
        R_TRY(m_impl->OpenFile(std::addressof(file), normalized_path, mode));

        auto accessor = new FileAccessor(std::move(file), this, mode);
        R_UNLESS(accessor != nullptr, fs::ResultAllocationMemoryFailedInFileSystemAccessorA());

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
        R_SUCCEED();
    }

    Result FileSystemAccessor::OpenDirectory(std::unique_ptr<DirectoryAccessor> *out_dir, const char *path, OpenDirectoryMode mode) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        std::unique_ptr<fsa::IDirectory> dir;
        R_TRY(m_impl->OpenDirectory(std::addressof(dir), normalized_path, mode));

        auto accessor = new DirectoryAccessor(std::move(dir), *this);
        R_UNLESS(accessor != nullptr, fs::ResultAllocationMemoryFailedInFileSystemAccessorB());

        {
            std::scoped_lock lk(m_open_list_lock);
            m_open_dir_list.push_back(*accessor);
        }

        out_dir->reset(accessor);
        R_SUCCEED();
    }

    Result FileSystemAccessor::Commit() {
        {
            std::scoped_lock lk(m_open_list_lock);
            R_ABORT_UNLESS(ValidateNoOpenWriteModeFiles(m_open_file_list));
        }
        R_RETURN(m_impl->Commit());
    }

    Result FileSystemAccessor::GetFreeSpaceSize(s64 *out, const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->GetFreeSpaceSize(out, normalized_path));
    }

    Result FileSystemAccessor::GetTotalSpaceSize(s64 *out, const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->GetTotalSpaceSize(out, normalized_path));
    }

    Result FileSystemAccessor::CleanDirectoryRecursively(const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->CleanDirectoryRecursively(normalized_path));
    }

    Result FileSystemAccessor::GetFileTimeStampRaw(FileTimeStampRaw *out, const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->GetFileTimeStampRaw(out, normalized_path));
    }

    Result FileSystemAccessor::QueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fsa::QueryId query, const char *path) {
        /* Create path. */
        fs::Path normalized_path;
        R_TRY(this->SetUpPath(std::addressof(normalized_path), path));

        R_RETURN(m_impl->QueryEntry(dst, dst_size, src, src_size, query, normalized_path));
    }


}
