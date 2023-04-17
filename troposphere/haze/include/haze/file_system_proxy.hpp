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
#pragma once

#include <haze/common.hpp>
#include <haze/event_reactor.hpp>

namespace haze {

    class FileSystemProxy final {
        private:
            EventReactor *m_reactor;
            FsFileSystem *m_filesystem;
        public:
            constexpr explicit FileSystemProxy() : m_reactor(), m_filesystem() { /* ... */ }

            void Initialize(EventReactor *reactor, FsFileSystem *fs) {
                HAZE_ASSERT(fs != nullptr);

                m_reactor = reactor;
                m_filesystem = fs;
            }

            void Finalize() {
                m_reactor = nullptr;
                m_filesystem = nullptr;
            }
        private:
            template <typename F, typename... Args>
            Result ForwardResult(F func, Args &&... args) {
                /* Perform the method call, collecting its result. */
                const Result rc = func(std::forward<Args>(args)...);

                /* If the event loop was stopped, return that here. */
                R_TRY(m_reactor->GetResult());

                /* Otherwise, return the call result. */
                R_RETURN(rc);
            }
        public:
            Result GetTotalSpace(const char *path, s64 *out) {
                R_RETURN(this->ForwardResult(fsFsGetTotalSpace, m_filesystem, path, out));
            }

            Result GetFreeSpace(const char *path, s64 *out) {
                R_RETURN(this->ForwardResult(fsFsGetFreeSpace, m_filesystem, path, out));
            }

            Result GetEntryType(const char *path, FsDirEntryType *out_entry_type) {
                R_RETURN(this->ForwardResult(fsFsGetEntryType, m_filesystem, path, out_entry_type));
            }

            Result CreateFile(const char* path, s64 size, u32 option) {
                R_RETURN(this->ForwardResult(fsFsCreateFile, m_filesystem, path, size, option));
            }

            Result DeleteFile(const char* path) {
                R_RETURN(this->ForwardResult(fsFsDeleteFile, m_filesystem, path));
            }

            Result RenameFile(const char *old_path, const char *new_path) {
                R_RETURN(this->ForwardResult(fsFsRenameFile, m_filesystem, old_path, new_path));
            }

            Result OpenFile(const char *path, u32 mode, FsFile *out_file) {
                R_RETURN(this->ForwardResult(fsFsOpenFile, m_filesystem, path, mode, out_file));
            }

            Result GetFileSize(FsFile *file, s64 *out_size) {
                R_RETURN(this->ForwardResult(fsFileGetSize, file, out_size));
            }

            Result SetFileSize(FsFile *file, s64 size) {
                R_RETURN(this->ForwardResult(fsFileSetSize, file, size));
            }

            Result ReadFile(FsFile *file, s64 off, void *buf, u64 read_size, u32 option, u64 *out_bytes_read) {
                R_RETURN(this->ForwardResult(fsFileRead, file, off, buf, read_size, option, out_bytes_read));
            }

            Result WriteFile(FsFile *file, s64 off, const void *buf, u64 write_size, u32 option) {
                R_RETURN(this->ForwardResult(fsFileWrite, file, off, buf, write_size, option));
            }

            void CloseFile(FsFile *file) {
                fsFileClose(file);
            }

            Result CreateDirectory(const char* path) {
                R_RETURN(this->ForwardResult(fsFsCreateDirectory, m_filesystem, path));
            }

            Result DeleteDirectoryRecursively(const char* path) {
                R_RETURN(this->ForwardResult(fsFsDeleteDirectoryRecursively, m_filesystem, path));
            }

            Result RenameDirectory(const char *old_path, const char *new_path) {
                R_RETURN(this->ForwardResult(fsFsRenameDirectory, m_filesystem, old_path, new_path));
            }

            Result OpenDirectory(const char *path, u32 mode, FsDir *out_dir) {
                R_RETURN(this->ForwardResult(fsFsOpenDirectory, m_filesystem, path, mode, out_dir));
            }

            Result ReadDirectory(FsDir *d, s64 *out_total_entries, size_t max_entries, FsDirectoryEntry *buf) {
                R_RETURN(this->ForwardResult(fsDirRead, d, out_total_entries, max_entries, buf));
            }

            Result GetDirectoryEntryCount(FsDir *d, s64 *out_count) {
                R_RETURN(this->ForwardResult(fsDirGetEntryCount, d, out_count));
            }

            void CloseDirectory(FsDir *d) {
                fsDirClose(d);
            }
    };

}
