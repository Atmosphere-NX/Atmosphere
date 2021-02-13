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
            Result OpenDirectory(s32 *out_handle, const char *path, fs::OpenDirectoryMode mode, bool case_sensitive) { return ConvertToFsResult(m_impl.OpenDirectory(out_handle, path, mode, case_sensitive)); }
            Result CloseDirectory(s32 handle) { return ConvertToFsResult(m_impl.CloseDirectory(handle)); }

            Result GetEntryCount(s64 *out, s32 handle) { return ConvertToFsResult(m_impl.GetEntryCount(out, handle)); }
            Result ReadDirectory(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) { return ConvertToFsResult(m_impl.ReadDirectory(out, out_entries, max_out_entries, handle)); }
            Result ReadDirectoryLarge(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) { return ConvertToFsResult( m_impl.ReadDirectoryLarge(out, out_entries, max_out_entries, handle)); }
            Result GetPriorityForDirectory(s32 *out, s32 handle) { return ConvertToFsResult(m_impl.GetPriorityForDirectory(out, handle)); }
            Result SetPriorityForDirectory(s32 priority, s32 handle) { return ConvertToFsResult(m_impl.SetPriorityForDirectory(priority, handle)); }
    };

    void InitializeClient(htclow::HtclowManager *manager);
    void FinalizeClient();

    Client &GetClient();

}
