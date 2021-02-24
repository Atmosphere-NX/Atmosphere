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
#include "tio_file_server_processor.hpp"

namespace ams::tio {

    namespace {

        constexpr inline int ProtocolVersion = 1;

    }

    void FileServerProcessor::Unmount() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Close all our directories. */
        for (size_t i = 0; i < m_open_directory_count; ++i) {
            fs::CloseDirectory(m_directories[i]);
            m_directories[i] = {};
        }

        /* Close all our files. */
        for (size_t i = 0; i < m_open_file_count; ++i) {
            fs::CloseFile(m_files[i]);
            m_files[i] = {};
        }

        /* If we're mounted, unmount the sd card. */
        if (m_is_mounted) {
            m_is_mounted = false;
            fs::Unmount("sd");
        }
    }

    bool FileServerProcessor::ProcessRequest(FileServerRequestHeader *header, u8 *body, int socket) {
        /* Declare a response header for us to use. */
        FileServerResponseHeader response_header = {
            .request_id = header->request_id,
            .result     = ResultSuccess(),
            .body_size  = 0,
        };

        /* Handle the special control commands. */
        if (header->packet_type == PacketType::Connect) {
            /* If the SD card isn't already mounted, try to mount it. */
            if (!m_is_mounted)  {
                /* Mount the sd card. */
                m_is_mounted = !fs::ResultSdCardAccessFailed::Includes(fs::MountSdCard("sd"));

                /* Prepare the response. */
                char *response_body = reinterpret_cast<char *>(body);
                util::SNPrintf(response_body, 0x100, "{\"bufferSize\":%zu, \"sdcardMounted\":%s, \"sdcardInserted\":%s, \"version\":%d}",
                                                     m_request_buffer_size,
                                                     m_is_mounted ? "true" : "false",
                                                     m_is_inserted ? "true" : "false",
                                                     ProtocolVersion);

                /* Get the response length. */
                response_header.body_size = std::strlen(response_body);
            }

            return this->SendResponse(response_header, body, socket);
        } else if (header->packet_type == PacketType::Disconnect) {
            /* If we need to, unmount the sd card. */
            if (m_is_mounted) {
                this->Unmount();
            }

            /* Send the response. */
            return this->SendResponse(response_header, body, socket);
        }

        /* TODO: Handle remaining packet types. */
        return false;
        //
        //switch (header->packet_type) {
        //    case PacketType::CreateDirectory:
        //    case PacketType::DeleteDirectory:
        //    case PacketType::DeleteDirectoryRecursively:
        //    case PacketType::OpenDirectory:
        //    case PacketType::CloseDirectory:
        //    case PacketType::RenameDirectory:
        //    case PacketType::CreateFile:
        //    case PacketType::DeleteFile:
        //    case PacketType::OpenFile:
        //    case PacketType::FlushFile:
        //    case PacketType::CloseFile:
        //    case PacketType::RenameFile:
        //    case PacketType::ReadFile:
        //    case PacketType::WriteFile:
        //    case PacketType::GetEntryType:
        //    case PacketType::ReadDirectory:
        //    case PacketType::GetFileSize:
        //    case PacketType::SetFileSize:
        //    case PacketType::GetTotalSpaceSize:
        //    case PacketType::GetFreeSpaceSize:
        //    case PacketType::Stat:
        //    case PacketType::ListDirectory:
        //        /* TODO */
        //        return false;
        //}
    }

    bool FileServerProcessor::SendResponse(const FileServerResponseHeader &header, const void *body, int socket) {
        /* Lock our server. */
        std::scoped_lock lk(m_htcs_server.GetMutex());

        /* Send the response header. */
        if (m_htcs_server.Send(socket, std::addressof(header), sizeof(header), 0) != sizeof(header)) {
            return false;
        }

        /* If we don't have a body, we're done. */
        if (header.body_size == 0) {
            return true;
        }

        /* Send the body. */
        return m_htcs_server.Send(socket, body, header.body_size, 0) == header.body_size;
    }

}