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

namespace ams::htcfs {

    constexpr inline s16 HtcfsProtocol      = 1;
    constexpr inline s16 MaxProtocolVersion = 1;

    enum class PacketCategory : u16 {
        Request  = 0,
        Response = 1,
    };

    enum class PacketType : u16 {
        GetMaxProtocolVersion   = 0,
        SetProtocolVersion      = 1,
        GetEntryType            = 16,
        OpenFile                = 32,
        CloseFile               = 33,
        GetPriorityForFile      = 34,
        SetPriorityForFile      = 35,
        CreateFile              = 36,
        DeleteFile              = 37,
        RenameFile              = 38,
        FileExists              = 39,
        ReadFile                = 40,
        WriteFile               = 41,
        FlushFile               = 42,
        GetFileTimeStamp        = 43,
        GetFileSize             = 44,
        SetFileSize             = 45,
        ReadFileLarge           = 46,
        WriteFileLarge          = 47,
        OpenDirectory           = 48,
        CloseDirectory          = 49,
        GetPriorityForDirectory = 50,
        SetPriorityForDirectory = 51,
        CreateDirectory         = 52,
        DeleteDirectory         = 53,
        RenameDirectory         = 54,
        DirectoryExists         = 55,
        ReadDirectory           = 56,
        GetEntryCount           = 57,
        GetWorkingDirectory     = 58,
        GetWorkingDirectorySize = 59,
        GetCaseSensitivePath    = 60,
        GetDiskFreeSpace        = 61,
        ReadDirectoryLarge      = 62,
    };

    struct Header {
        s16 protocol;
        s16 version;
        PacketCategory packet_category;
        PacketType packet_type;
        s64 body_size;
        s64 params[5];
        s64 reserved;
    };
    static_assert(util::is_pod<Header>::value);
    static_assert(sizeof(Header) == 0x40);

    class HeaderFactory {
        private:
            s16 m_version;
        public:
            HeaderFactory() : m_version() { /* ... */ }
        public:
            s16 GetVersion() const { return m_version; }
            void SetVersion(s16 version) { m_version = version; }
        public:
            ALWAYS_INLINE void MakeRequestHeader(Header *out, PacketType packet_type, s64 body_size = 0, s64 param0 = 0, s64 param1 = 0, s64 param2 = 0, s64 param3 = 0, s64 param4 = 0) {
                /* Set protocol and version. */
                out->protocol = HtcfsProtocol;
                out->version  = m_version;

                /* Set type and category. */
                out->packet_category = PacketCategory::Request;
                out->packet_type     = packet_type;

                /* Set body size. */
                out->body_size = body_size;

                /* Set params. */
                out->params[0] = param0;
                out->params[1] = param1;
                out->params[2] = param2;
                out->params[3] = param3;
                out->params[4] = param4;

                /* Clear reserved. */
                out->reserved = 0;
            }

            void MakeGetMaxProtocolVersionHeader(Header *out) {
                return this->MakeRequestHeader(out, PacketType::GetMaxProtocolVersion, 0);
            }

            void MakeSetProtocolVersionHeader(Header *out, s16 version) {
                return this->MakeRequestHeader(out, PacketType::SetProtocolVersion, 0, version);
            }

            void MakeOpenFileHeader(Header *out, int path_len, fs::OpenMode mode, bool case_sensitive, s64 cache_size) {
                return this->MakeRequestHeader(out, PacketType::OpenFile, path_len, static_cast<s64>(mode), case_sensitive ? 1 : 0, cache_size);
            }

            void MakeFileExistsHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::FileExists, path_len, case_sensitive ? 1 : 0);
            }

            void MakeDeleteFileHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::DeleteFile, path_len, case_sensitive ? 1 : 0);
            }

            void MakeRenameFileHeader(Header *out, int old_path_len, int new_path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::RenameFile, old_path_len + new_path_len, old_path_len, new_path_len, case_sensitive ? 1 : 0);
            }

            void MakeGetEntryTypeHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::GetEntryType, path_len, case_sensitive ? 1 : 0);
            }

            void MakeOpenDirectoryHeader(Header *out, int path_len, fs::OpenDirectoryMode mode, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::OpenDirectory, path_len, static_cast<s64>(mode), case_sensitive ? 1 : 0);
            }

            void MakeDirectoryExistsHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::DirectoryExists, path_len, case_sensitive ? 1 : 0);
            }

            void MakeCreateDirectoryHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::CreateDirectory, path_len, case_sensitive ? 1 : 0);
            }

            void MakeDeleteDirectoryHeader(Header *out, int path_len, bool recursively, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::DeleteDirectory, path_len, recursively ? 1 : 0, case_sensitive ? 1 : 0);
            }

            void MakeRenameDirectoryHeader(Header *out, int old_path_len, int new_path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::RenameDirectory, old_path_len + new_path_len, old_path_len, new_path_len, case_sensitive ? 1 : 0);
            }

            void MakeCreateFileHeader(Header *out, int path_len, s64 size, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::CreateFile, path_len, size, case_sensitive ? 1 : 0);
            }

            void MakeGetFileTimeStampHeader(Header *out, int path_len, bool case_sensitive) {
                return this->MakeRequestHeader(out, PacketType::GetFileTimeStamp, path_len, case_sensitive ? 1 : 0);
            }

            void MakeGetCaseSensitivePathHeader(Header *out, int path_len) {
                return this->MakeRequestHeader(out, PacketType::GetCaseSensitivePath, path_len);
            }

            void MakeGetDiskFreeSpaceHeader(Header *out, int path_len) {
                return this->MakeRequestHeader(out, PacketType::GetDiskFreeSpace, path_len);
            }

            void MakeCloseDirectoryHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::CloseDirectory, 0, handle);
            }

            void MakeGetEntryCountHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::GetEntryCount, 0, handle);
            }

            void MakeGetWorkingDirectoryHeader(Header *out) {
                return this->MakeRequestHeader(out, PacketType::GetWorkingDirectory, 0);
            }

            void MakeGetWorkingDirectorySizeHeader(Header *out) {
                return this->MakeRequestHeader(out, PacketType::GetWorkingDirectorySize, 0);
            }

            void MakeReadDirectoryHeader(Header *out, s32 handle, size_t max_out_entries) {
                return this->MakeRequestHeader(out, PacketType::ReadDirectory, 0, handle, max_out_entries);
            }

            void MakeReadDirectoryLargeHeader(Header *out, s32 handle, size_t max_out_entries, u16 data_channel_id) {
                return this->MakeRequestHeader(out, PacketType::ReadDirectoryLarge, 0, handle, max_out_entries, data_channel_id);
            }

            void MakeGetPriorityForDirectoryHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::GetPriorityForDirectory, 0, handle);
            }

            void MakeSetPriorityForDirectoryHeader(Header *out, s32 handle, s32 priority) {
                return this->MakeRequestHeader(out, PacketType::SetPriorityForDirectory, 0, handle, priority);
            }

            void MakeCloseFileHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::CloseFile, 0, handle);
            }

            void MakeReadFileHeader(Header *out, s32 handle, s64 offset, s64 buffer_size) {
                return this->MakeRequestHeader(out, PacketType::ReadFile, 0, handle, offset, buffer_size);
            }

            void MakeReadFileLargeHeader(Header *out, s32 handle, s64 offset, s64 buffer_size, u16 data_channel_id) {
                return this->MakeRequestHeader(out, PacketType::ReadFileLarge, 0, handle, offset, buffer_size, data_channel_id);
            }

            void MakeWriteFileHeader(Header *out, s64 buffer_size, s32 handle, u32 option, s64 offset) {
                return this->MakeRequestHeader(out, PacketType::WriteFile, buffer_size, handle, option, offset);
            }

            void MakeWriteFileLargeHeader(Header *out, s32 handle, u32 option, s64 offset, s64 buffer_size, u16 data_channel_id) {
                return this->MakeRequestHeader(out, PacketType::WriteFileLarge, 0, handle, option, offset, buffer_size, data_channel_id);
            }

            void MakeGetFileSizeHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::GetFileSize, 0, handle);
            }

            void MakeSetFileSizeHeader(Header *out, s32 handle, s64 size) {
                return this->MakeRequestHeader(out, PacketType::SetFileSize, 0, handle, size);
            }

            void MakeFlushFileHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::FlushFile, 0, handle);
            }

            void MakeGetPriorityForFileHeader(Header *out, s32 handle) {
                return this->MakeRequestHeader(out, PacketType::GetPriorityForFile, 0, handle);
            }

            void MakeSetPriorityForFileHeader(Header *out, s32 handle, s32 priority) {
                return this->MakeRequestHeader(out, PacketType::SetPriorityForFile, 0, handle, priority);
            }
    };

}
