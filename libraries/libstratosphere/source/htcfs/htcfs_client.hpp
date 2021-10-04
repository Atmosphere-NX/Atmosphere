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
#include <stratosphere.hpp>
#include "htcfs_client_impl.hpp"
#include "htcfs_result_utils.hpp"

namespace ams::htcfs {

    class Client {
        private:
            ClientImpl m_impl;
        public:
            Client(htclow::HtclowManager *manager) : m_impl(manager) { /* ... */ }
        public:
            Result OpenFile(s32 *out_handle, const char *path, fs::OpenMode mode, bool case_sensitive) { return ConvertToFsResult(m_impl.OpenFile(out_handle, path, mode, case_sensitive)); }
            Result FileExists(bool *out, const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.FileExists(out, path, case_sensitive)); }
            Result DeleteFile(const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.DeleteFile(path, case_sensitive)); }
            Result RenameFile(const char *old_path, const char *new_path, bool case_sensitive) { return ConvertToFsResult(m_impl.RenameFile(old_path, new_path, case_sensitive)); }
            Result GetEntryType(fs::DirectoryEntryType *out, const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.GetEntryType(out, path, case_sensitive)); }
            Result OpenDirectory(s32 *out_handle, const char *path, fs::OpenDirectoryMode mode, bool case_sensitive) { return ConvertToFsResult(m_impl.OpenDirectory(out_handle, path, mode, case_sensitive)); }
            Result DirectoryExists(bool *out, const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.DirectoryExists(out, path, case_sensitive)); }
            Result CreateDirectory(const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.CreateDirectory(path, case_sensitive)); }
            Result DeleteDirectory(const char *path, bool recursively, bool case_sensitive) { return ConvertToFsResult(m_impl.DeleteDirectory(path, recursively, case_sensitive)); }
            Result RenameDirectory(const char *old_path, const char *new_path, bool case_sensitive) { return ConvertToFsResult(m_impl.RenameDirectory(old_path, new_path, case_sensitive)); }
            Result CreateFile(const char *path, s64 size, bool case_sensitive) { return ConvertToFsResult(m_impl.CreateFile(path, size, case_sensitive)); }
            Result GetFileTimeStamp(u64 *out_create, u64 *out_access, u64 *out_modify, const char *path, bool case_sensitive) { return ConvertToFsResult(m_impl.GetFileTimeStamp(out_create, out_access, out_modify, path, case_sensitive)); }
            Result GetCaseSensitivePath(char *dst, size_t dst_size, const char *path) { return ConvertToFsResult(m_impl.GetCaseSensitivePath(dst, dst_size, path)); }
            Result GetDiskFreeSpace(s64 *out_free, s64 *out_total, s64 *out_total_free, const char *path) { return ConvertToFsResult(m_impl.GetDiskFreeSpace(out_free, out_total, out_total_free, path)); }

            Result CloseDirectory(s32 handle) { return ConvertToFsResult(m_impl.CloseDirectory(handle)); }

            Result GetEntryCount(s64 *out, s32 handle) { return ConvertToFsResult(m_impl.GetEntryCount(out, handle)); }
            Result ReadDirectory(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) { return ConvertToFsResult(m_impl.ReadDirectory(out, out_entries, max_out_entries, handle)); }
            Result ReadDirectoryLarge(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) { return ConvertToFsResult(m_impl.ReadDirectoryLarge(out, out_entries, max_out_entries, handle)); }
            Result GetPriorityForDirectory(s32 *out, s32 handle) { return ConvertToFsResult(m_impl.GetPriorityForDirectory(out, handle)); }
            Result SetPriorityForDirectory(s32 priority, s32 handle) { return ConvertToFsResult(m_impl.SetPriorityForDirectory(priority, handle)); }

            Result CloseFile(s32 handle) { return ConvertToFsResult(m_impl.CloseFile(handle)); }

            Result ReadFile(s64 *out, void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::ReadOption option) { return ConvertToFsResult(m_impl.ReadFile(out, buffer, handle, offset, buffer_size, option)); }
            Result ReadFileLarge(s64 *out, void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::ReadOption option) { return ConvertToFsResult(m_impl.ReadFileLarge(out, buffer, handle, offset, buffer_size, option)); }
            Result WriteFile(const void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::WriteOption option) { return ConvertToFsResult(m_impl.WriteFile(buffer, handle, offset, buffer_size, option)); }
            Result WriteFileLarge(const void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::WriteOption option) { return ConvertToFsResult(m_impl.WriteFileLarge(buffer, handle, offset, buffer_size, option)); }
            Result GetFileSize(s64 *out, s32 handle) { return ConvertToFsResult(m_impl.GetFileSize(out, handle)); }
            Result SetFileSize(s64 size, s32 handle) { return ConvertToFsResult(m_impl.SetFileSize(size, handle)); }
            Result FlushFile(s32 handle) { return ConvertToFsResult(m_impl.FlushFile(handle)); }
            Result GetPriorityForFile(s32 *out, s32 handle) { return ConvertToFsResult(m_impl.GetPriorityForFile(out, handle)); }
            Result SetPriorityForFile(s32 priority, s32 handle) { return ConvertToFsResult(m_impl.SetPriorityForFile(priority, handle)); }

            Result GetWorkingDirectory(char *dst, size_t dst_size) { return ConvertToFsResult(m_impl.GetWorkingDirectory(dst, dst_size)); }
            Result GetWorkingDirectorySize(s32 *out) { return ConvertToFsResult(m_impl.GetWorkingDirectorySize(out)); }
    };

    void InitializeClient(htclow::HtclowManager *manager);
    void FinalizeClient();

    Client &GetClient();

}
