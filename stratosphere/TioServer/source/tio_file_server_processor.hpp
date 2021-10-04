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
#include "tio_file_server_htcs_server.hpp"
#include "tio_file_server_packet.hpp"

namespace ams::tio {

    class FileServerProcessor {
        private:
            bool m_is_inserted{};
            bool m_is_mounted{};
            size_t m_request_buffer_size{};
            FileServerHtcsServer &m_htcs_server;
            size_t m_open_file_count{};
            size_t m_open_directory_count{};
            fs::FileHandle m_files[0x80]{};
            fs::DirectoryHandle m_directories[0x80]{};
            os::SdkMutex m_fs_mutex{};
            os::SdkMutex m_mutex{};
        public:
            constexpr FileServerProcessor(FileServerHtcsServer &htcs_server) : m_htcs_server(htcs_server) { /* ... */ }

            void SetInserted(bool ins) { m_is_inserted = ins; }
            void SetRequestBufferSize(size_t size) { m_request_buffer_size = size; }
        public:
            bool ProcessRequest(FileServerRequestHeader *header, u8 *body, int socket);

            void Unmount();
        private:
            bool SendResponse(const FileServerResponseHeader &header, const void *body, int socket);
    };

}