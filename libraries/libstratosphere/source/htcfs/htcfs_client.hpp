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

namespace ams::htcfs {

    class Client {
        private:
            ClientImpl m_impl;
        public:
            Client(htclow::HtclowManager *manager) : m_impl(manager) { /* ... */ }
        public:
            Result OpenDirectory(s32 *out_handle, const char *path, fs::OpenDirectoryMode mode, bool case_sensitive) { return m_impl.OpenDirectory(out_handle, path, mode, case_sensitive); }
            Result CloseDirectory(s32 handle) { return m_impl.CloseDirectory(handle); }
    };

    void InitializeClient(htclow::HtclowManager *manager);
    void FinalizeClient();

    Client &GetClient();

}
