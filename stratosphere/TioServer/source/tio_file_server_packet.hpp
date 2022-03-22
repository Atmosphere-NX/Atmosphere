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

namespace ams::tio {

    enum class PacketType : u32 {
        /* Control commands. */
        Connect    = 0,
        Disconnect = 1,

        /* Direct filesystem access. */
        CreateDirectory            = 500,
        DeleteDirectory            = 501,
        DeleteDirectoryRecursively = 502,
        OpenDirectory              = 503,
        CloseDirectory             = 504,
        RenameDirectory            = 505,
        CreateFile                 = 506,
        DeleteFile                 = 507,
        OpenFile                   = 508,
        FlushFile                  = 509,
        CloseFile                  = 510,
        RenameFile                 = 511,
        ReadFile                   = 512,
        WriteFile                  = 513,
        GetEntryType               = 514,
        ReadDirectory              = 515,
        GetFileSize                = 516,
        SetFileSize                = 517,
        GetTotalSpaceSize          = 518,
        GetFreeSpaceSize           = 519,

        /* Utilities. */
        Stat          = 1000,
        ListDirectory = 1001,
    };

    struct FileServerRequestHeader {
        u64 request_id;
        PacketType packet_type;
        u32 body_size;
    };

    struct FileServerResponseHeader {
        u64 request_id;
        Result result;
        u32 body_size;
    };
    static_assert(sizeof(FileServerRequestHeader) == sizeof(FileServerResponseHeader));

    struct CreateDirectoryParam {
        u32 path_len;
        char path[];
    };

    struct DeleteDirectoryParam {
        u32 path_len;
        char path[];
    };

    struct DeleteDirectoryRecursivelyParam {
        u32 path_len;
        char path[];
    };

    struct OpenDirectoryParam {
        u32 path_len;
        fs::OpenDirectoryMode open_mode;
        char path[];
    };
    static_assert(sizeof(OpenDirectoryParam) == 0x8);

    struct CloseDirectoryParam {
        u64 handle;
    };

    struct RenameDirectoryParam {
        u32 old_len;
        u32 new_len;
        char data[];
    };

    struct CreateFileParam {
        s64 size;
        u32 path_len;
        fs::CreateOption option;
        char path[];
    };
    static_assert(sizeof(CreateFileParam) == 0x10);

    struct DeleteFileParam {
        u32 path_len;
        char path[];
    };

    struct OpenFileParam {
        u32 path_len;
        fs::OpenMode mode;
        char path[];
    };
    static_assert(sizeof(OpenFileParam) == 0x8);

    struct FlushFileParam {
        u64 handle;
    };

    struct CloseFileParam {
        u64 handle;
    };

    struct RenameFileParam {
        u32 old_len;
        u32 new_len;
        char data[];
    };

    struct ReadFileParam {
        u64 handle;
        s64 offset;
        u64 size;
        fs::ReadOption option;
    };
    static_assert(sizeof(ReadFileParam) == 0x20);

    struct WriteFileParam {
        u64 handle;
        s64 offset;
        u64 size;
        fs::WriteOption option;
    };
    static_assert(sizeof(WriteFileParam) == 0x20);

    struct GetEntryTypeParam {
        u32 path_len;
        char path[];
    };

    struct ReadDirectoryParam {
        u64 handle;
        s64 count;
    };

    struct GetFileSizeParam {
        u64 handle;
    };

    struct SetFileSizeParam {
        u64 handle;
        s64 size;
    };

    struct GetTotalSpaceSizeParam {
        u32 path_len;
        char path[];
    };

    struct GetFreeSpaceSizeParam {
        u32 path_len;
        char path[];
    };

    struct StatParam {
        u32 path_len;
        char path[];
    };

    struct ListDirectoryParam {
        u32 path_len;
        char path[];
    };

}
