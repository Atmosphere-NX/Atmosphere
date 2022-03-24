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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_filesystem.hpp>
#include <stratosphere/time/time_posix_time.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: Unknown */
    struct FileTimeStamp {
        time::PosixTime create;
        time::PosixTime modify;
        time::PosixTime access;
        bool is_local_time;
        char pad[7];
    };
    static_assert(util::is_pod<FileTimeStamp>::value && sizeof(FileTimeStamp) == 0x20);

    struct FileTimeStampRaw {
        s64 create;
        s64 modify;
        s64 access;
        bool is_local_time;
        char pad[7];
    };
    static_assert(util::is_pod<FileTimeStampRaw>::value && sizeof(FileTimeStampRaw) == 0x20);

    static_assert(__builtin_offsetof(FileTimeStampRaw, create) == __builtin_offsetof(FileTimeStampRaw, create));
    static_assert(__builtin_offsetof(FileTimeStampRaw, modify) == __builtin_offsetof(FileTimeStampRaw, modify));
    static_assert(__builtin_offsetof(FileTimeStampRaw, access) == __builtin_offsetof(FileTimeStampRaw, access));
    static_assert(__builtin_offsetof(FileTimeStampRaw, is_local_time) == __builtin_offsetof(FileTimeStampRaw, is_local_time));
    static_assert(__builtin_offsetof(FileTimeStampRaw, pad) == __builtin_offsetof(FileTimeStampRaw, pad));

    #if defined(ATMOSPHERE_OS_HORIZON)
    static_assert(sizeof(FileTimeStampRaw) == sizeof(::FsTimeStampRaw));
    #endif

    namespace impl {

        Result GetFileTimeStampRawForDebug(FileTimeStampRaw *out, const char *path);

    }

    Result GetFileTimeStamp(FileTimeStamp *out, const char *path);

}
