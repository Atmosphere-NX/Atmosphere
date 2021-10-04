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
#include "tio_file_server_processor.hpp"

namespace ams::tio {

    namespace {

        constexpr inline int ProtocolVersion = 1;

    }

    void FileServerProcessor::Unmount() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Close all our directories. */
        if (m_open_directory_count > 0) {
            for (size_t i = 0; i < util::size(m_directories); ++i) {
                if (m_directories[i].handle != nullptr) {
                    fs::CloseDirectory(m_directories[i]);
                    m_directories[i] = {};
                    --m_open_directory_count;
                }
            }
        }
        AMS_ABORT_UNLESS(m_open_directory_count == 0);

        /* Close all our files. */
        if (m_open_file_count > 0) {
            for (size_t i = 0; i < util::size(m_files); ++i) {
                if (m_files[i].handle != nullptr) {
                    fs::CloseFile(m_files[i]);
                    m_files[i] = {};
                    --m_open_file_count;
                }
            }
        }
        AMS_ABORT_UNLESS(m_open_file_count == 0);

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

        /* The SD card must be inserted and mounted for us to process requests. */
        if (m_is_inserted && m_is_mounted) {
            switch (header->packet_type) {
                case PacketType::CreateDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const CreateDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Create the directory. */
                        response_header.result = fs::CreateDirectory(param->path);
                    }
                    break;
                case PacketType::DeleteDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const DeleteDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Delete the directory. */
                        response_header.result = fs::DeleteDirectory(param->path);
                    }
                    break;
                case PacketType::DeleteDirectoryRecursively:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const DeleteDirectoryRecursivelyParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Delete the directory. */
                        response_header.result = fs::DeleteDirectoryRecursively(param->path);
                    }
                    break;
                case PacketType::OpenDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const OpenDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Open the directory. */
                        fs::DirectoryHandle handle;
                        response_header.result = fs::OpenDirectory(std::addressof(handle), param->path, param->open_mode);
                        if (R_SUCCEEDED(response_header.result)) {
                            std::scoped_lock lk(m_mutex);

                            if (m_open_directory_count < util::size(m_directories)) {
                                /* Insert the directory into our table. */
                                u64 index = std::numeric_limits<u64>::max();
                                for (size_t i = 0; i < util::size(m_directories); ++i) {
                                    if (m_directories[i].handle == nullptr) {
                                        m_directories[i] = handle;
                                        index = i;
                                        ++m_open_directory_count;
                                        break;
                                    }
                                }
                                AMS_ABORT_UNLESS(index < util::size(m_directories));

                                /* Return the index. */
                                response_header.body_size = sizeof(index);
                                std::memcpy(body, std::addressof(index), sizeof(index));
                            } else {
                                /* We can't actually open the directory. */
                                fs::CloseDirectory(handle);

                                response_header.result = fs::ResultOpenCountLimit();
                            }
                        }
                    }
                    break;
                case PacketType::CloseDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const CloseDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param)) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the directory handle is valid. */
                        if (param->handle >= util::size(m_directories) || m_directories[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Lock the filesystem. */
                        std::scoped_lock lk2(m_fs_mutex);

                        /* Close the directory. */
                        fs::CloseDirectory(m_directories[param->handle]);
                        m_directories[param->handle].handle = {};
                        --m_open_directory_count;
                    }
                    break;
                case PacketType::RenameDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const RenameDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param) + param->old_len + param->new_len) {
                            return false;
                        }

                        /* Delete the directory. */
                        const char *old_path = param->data + 0;
                        const char *new_path = param->data + param->old_len;
                        response_header.result = fs::RenameDirectory(old_path, new_path);
                    }
                    break;
                case PacketType::CreateFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const CreateFileParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Create the file. */
                        response_header.result = fs::CreateFile(param->path, param->size, static_cast<int>(param->option));
                    }
                    break;
                case PacketType::DeleteFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const DeleteFileParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Delete the file. */
                        response_header.result = fs::DeleteFile(param->path);
                    }
                    break;
                case PacketType::OpenFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const OpenFileParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Open the file. */
                        fs::FileHandle handle;
                        response_header.result = fs::OpenFile(std::addressof(handle), param->path, param->mode);
                        if (R_SUCCEEDED(response_header.result)) {
                            std::scoped_lock lk(m_mutex);

                            if (m_open_file_count < util::size(m_files)) {
                                /* Insert the file into our table. */
                                u64 index = std::numeric_limits<u64>::max();
                                for (size_t i = 0; i < util::size(m_files); ++i) {
                                    if (m_files[i].handle == nullptr) {
                                        m_files[i] = handle;
                                        index = i;
                                        ++m_open_file_count;
                                        break;
                                    }
                                }
                                AMS_ABORT_UNLESS(index < util::size(m_files));

                                /* Return the index. */
                                response_header.body_size = sizeof(index);
                                std::memcpy(body, std::addressof(index), sizeof(index));
                            } else {
                                /* We can't actually open the file. */
                                fs::CloseFile(handle);

                                response_header.result = fs::ResultOpenCountLimit();
                            }
                        }
                    }
                    break;
                case PacketType::FlushFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const FlushFileParam *>(body);
                        if (header->body_size != sizeof(*param)) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param->handle >= util::size(m_files) || m_files[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Lock the filesystem. */
                        std::scoped_lock lk2(m_fs_mutex);

                        /* Flush the file. */
                        response_header.result = fs::FlushFile(m_files[param->handle]);
                    }
                    break;
                case PacketType::CloseFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const CloseFileParam *>(body);
                        if (header->body_size != sizeof(*param)) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param->handle >= util::size(m_files) || m_files[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Lock the filesystem. */
                        std::scoped_lock lk2(m_fs_mutex);

                        /* Close the directory. */
                        fs::CloseFile(m_files[param->handle]);
                        m_files[param->handle].handle = {};
                        --m_open_file_count;
                    }
                    break;
                case PacketType::RenameFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const RenameFileParam *>(body);
                        if (header->body_size != sizeof(*param) + param->old_len + param->new_len) {
                            return false;
                        }

                        /* Delete the directory. */
                        const char *old_path = param->data + 0;
                        const char *new_path = param->data + param->old_len;
                        response_header.result = fs::RenameFile(old_path, new_path);
                    }
                    break;
                case PacketType::ReadFile:
                    {
                        /* Get the parameters. */
                        const auto param = *reinterpret_cast<const ReadFileParam *>(body);
                        if (header->body_size != sizeof(param)) {
                            return false;
                        }

                        /* Check that the read is valid. */
                        if (param.size + sizeof(u64) > m_request_buffer_size) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Prepare response variables. */
                        u64 *out_size = reinterpret_cast<u64 *>(body);
                        void *dst     = out_size + 1;

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param.handle >= util::size(m_files) || m_files[param.handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Read the file. */
                        size_t read_size;
                        response_header.result = fs::ReadFile(std::addressof(read_size), m_files[param.handle], param.offset, dst, param.size);

                        if (R_SUCCEEDED(response_header.result)) {
                            *out_size = read_size;
                            response_header.body_size = sizeof(u64) + read_size;
                        }
                    }
                    break;
                case PacketType::WriteFile:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const WriteFileParam *>(body);
                        if (header->body_size != sizeof(*param) + param->size) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param->handle >= util::size(m_files) || m_files[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Lock the filesystem. */
                        std::scoped_lock lk2(m_fs_mutex);

                        /* Write the file. */
                        response_header.result = fs::WriteFile(m_files[param->handle], param->offset, body + sizeof(*param), param->size, param->option);
                    }
                    break;
                case PacketType::GetEntryType:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const GetEntryTypeParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Get the entry type. */
                        fs::DirectoryEntryType type;
                        response_header.result = fs::GetEntryType(std::addressof(type), param->path);

                        if (R_SUCCEEDED(response_header.result)) {
                            /* Return the type. */
                            response_header.body_size = sizeof(type);
                            std::memcpy(body, std::addressof(type), sizeof(type));

                            static_assert(sizeof(type) == sizeof(u32));
                        }
                    }
                    break;
                case PacketType::ReadDirectory:
                    {
                        /* Get the parameters. */
                        const auto param = *reinterpret_cast<const ReadDirectoryParam *>(body);
                        if (header->body_size != sizeof(param)) {
                            return false;
                        }

                        /* Check that the read is valid. */
                        if (sizeof(s64) + param.count * sizeof(fs::DirectoryEntry) > m_request_buffer_size) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Prepare response variables. */
                        s64 *out_count = reinterpret_cast<s64 *>(body);
                        fs::DirectoryEntry *dst = reinterpret_cast<fs::DirectoryEntry *>(out_count + 1);

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the directory handle is valid. */
                        if (param.handle >= util::size(m_directories) || m_directories[param.handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Read the directory. */
                        response_header.result = fs::ReadDirectory(out_count, dst, m_directories[param.handle], param.count);

                        if (R_SUCCEEDED(response_header.result)) {
                            response_header.body_size = sizeof(s64) + *out_count * sizeof(fs::DirectoryEntry);
                        }
                    }
                    break;
                case PacketType::GetFileSize:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const GetFileSizeParam *>(body);
                        if (header->body_size != sizeof(*param)) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param->handle >= util::size(m_files) || m_files[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Get the file size. */
                        response_header.result = fs::GetFileSize(reinterpret_cast<s64 *>(body), m_files[param->handle]);

                        if (R_SUCCEEDED(response_header.result)) {
                            response_header.body_size = sizeof(s64);
                        }
                    }
                    break;
                case PacketType::SetFileSize:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const SetFileSizeParam *>(body);
                        if (header->body_size != sizeof(*param)) {
                            return false;
                        }

                        /* Lock ourselves. */
                        std::scoped_lock lk(m_mutex);

                        /* Check that the file handle is valid. */
                        if (param->handle >= util::size(m_files) || m_files[param->handle].handle == nullptr) {
                            response_header.result = fs::ResultDataCorrupted();
                            break;
                        }

                        /* Lock the filesystem. */
                        std::scoped_lock lk2(m_fs_mutex);

                        /* Get the file size. */
                        response_header.result = fs::SetFileSize(m_files[param->handle], param->size);
                    }
                    break;
                case PacketType::GetTotalSpaceSize:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const GetTotalSpaceSizeParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Get the total space size. */
                        s64 size;
                        response_header.result = fs::GetTotalSpaceSize(std::addressof(size), param->path);

                        if (R_SUCCEEDED(response_header.result)) {
                            /* Return the size. */
                            response_header.body_size = sizeof(size);
                            std::memcpy(body, std::addressof(size), sizeof(size));
                        }
                    }
                    break;
                case PacketType::GetFreeSpaceSize:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const GetFreeSpaceSizeParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Get the free space size. */
                        s64 size;
                        response_header.result = fs::GetFreeSpaceSize(std::addressof(size), param->path);

                        if (R_SUCCEEDED(response_header.result)) {
                            /* Return the size. */
                            response_header.body_size = sizeof(size);
                            std::memcpy(body, std::addressof(size), sizeof(size));
                        }
                    }
                    break;
                case PacketType::Stat:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const StatParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Prepare a response stat structure. */
                        struct {
                            fs::DirectoryEntryType type;
                            s64 file_size;
                            fs::FileTimeStampRaw file_timestamp;
                        } out = {};
                        static_assert(sizeof(out) == 0x30);

                        /* Get the entry type. */
                        response_header.result = fs::GetEntryType(std::addressof(out.type), param->path);
                        if (R_FAILED(response_header.result)) {
                            break;
                        }

                        /* If the path is a file, get further information. */
                        if (out.type == fs::DirectoryEntryType_File) {
                            /* Try to get the file size. */
                            {
                                fs::FileHandle handle;
                                const auto open_result = fs::OpenFile(std::addressof(handle), param->path, fs::OpenMode_Read);
                                if (R_SUCCEEDED(open_result)) {
                                    ON_SCOPE_EXIT { fs::CloseFile(handle); };

                                    response_header.result = fs::GetFileSize(std::addressof(out.file_size), handle);
                                    if (R_FAILED(response_header.result)) {
                                        break;
                                    }
                                } else {
                                    if (fs::ResultTargetLocked::Includes(open_result)) {
                                        out.file_size = 0;
                                    } else {
                                        response_header.result = open_result;
                                        break;
                                    }
                                }
                            }

                            /* Get the file timestamp. */
                            response_header.result = fs::GetFileTimeStampRawForDebug(std::addressof(out.file_timestamp), param->path);
                            if (R_FAILED(response_header.result)) {
                                break;
                            }
                        }

                        /* If we successfully got the stat information, send it as response. */
                        if (R_SUCCEEDED(response_header.result)) {
                            response_header.body_size = sizeof(out);
                            std::memcpy(body, std::addressof(out), sizeof(out));
                        }
                    }
                    break;
                case PacketType::ListDirectory:
                    {
                        /* Get the parameters. */
                        const auto *param = reinterpret_cast<const ListDirectoryParam *>(body);
                        if (header->body_size != sizeof(*param) + param->path_len) {
                            return false;
                        }

                        /* Open the directory. */
                        fs::DirectoryHandle handle;
                        response_header.result = fs::OpenDirectory(std::addressof(handle), param->path, fs::OpenDirectoryMode_All);
                        if (R_FAILED(response_header.result)) {
                            break;
                        }

                        /* When we're done, close the handle. */
                        ON_SCOPE_EXIT { fs::CloseDirectory(handle); };

                        /* Get the directory entry count. */
                        s64 count;
                        response_header.result = fs::GetDirectoryEntryCount(std::addressof(count), handle);
                        if (R_FAILED(response_header.result)) {
                            break;
                        }
                        /* Determine whether we can send the response in one go. */
                        const size_t needed_size = sizeof(s64) + sizeof(u64) + sizeof(fs::DirectoryEntry) * count;
                        if (needed_size <= m_request_buffer_size) {
                            /* We can perform the entire read in one send. */
                            struct {
                                s64 count;
                                u64 size;
                                fs::DirectoryEntry entries[];
                            } *out = reinterpret_cast<decltype(out)>(body);

                            s64 read_count;
                            response_header.result = fs::ReadDirectory(std::addressof(read_count), out->entries, handle, count);
                            if (R_FAILED(response_header.result)) {
                                break;
                            }

                            /* Set the output. */
                            out->count = read_count;
                            out->size  = read_count * sizeof(fs::DirectoryEntry);

                            /* Set the response body size. */
                            response_header.body_size = sizeof(*out) + out->size;
                        } else {
                            /* We have to use multiple sends. */
                            /* Lock our server. */
                            std::scoped_lock lk(m_htcs_server.GetMutex());

                            /* Send the response header. */
                            response_header.body_size = needed_size;
                            if (m_htcs_server.Send(socket, std::addressof(header), sizeof(header), 0) != sizeof(header)) {
                                return false;
                            }

                            /* Send the body header. */
                            struct {
                                s64 count;
                                u64 size;
                            } out = { count, count * sizeof(fs::DirectoryEntry) };
                            if (m_htcs_server.Send(socket, std::addressof(out), sizeof(out), 0) != sizeof(out)) {
                                return false;
                            }

                            /* Loop sending entries. */
                            s64 remaining = count;
                            do {
                                /* Determine how many entries we can read. */
                                const s64 cur = std::min<s64>(remaining, static_cast<s64>(m_request_buffer_size / sizeof(fs::DirectoryEntry)));

                                /* NOTE: Nintendo does not check the output of this call. */
                                s64 read_count = 0;
                                fs::ReadDirectory(std::addressof(read_count), reinterpret_cast<fs::DirectoryEntry *>(body), handle, cur);

                                /* Send the current entries. */
                                const ssize_t cur_size = read_count * sizeof(fs::DirectoryEntry);
                                if (m_htcs_server.Send(socket, body, cur_size, 0) != cur_size) {
                                    return false;
                                }

                                /* Advance. */
                                remaining -= read_count;
                            } while (remaining > 0);

                            /* We've sent the entirety of our response, so early return. */
                            return true;
                        }
                    }
                    break;
                default:
                    /* Unsupported packet. */
                    return false;
            }

            /* Send the response. */
            return this->SendResponse(response_header, body, socket);
        } else if (m_is_mounted) {
            /* The SD card is mounted but not inserted, so we should unmount it. */
            this->Unmount();
        }

        /* We failed to process the request due to SD card not being inserted or mounted. */
        response_header.result = fs::ResultSdCardAccessFailed();

        return this->SendResponse(response_header, body, socket);
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
