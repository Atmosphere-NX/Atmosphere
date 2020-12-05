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
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    using RomPathChar    = char;
    using RomFileId      = s32;
    using RomDirectoryId = s32;

    struct RomFileSystemInformation {
        s64 size;
        s64 directory_bucket_offset;
        s64 directory_bucket_size;
        s64 directory_entry_offset;
        s64 directory_entry_size;
        s64 file_bucket_offset;
        s64 file_bucket_size;
        s64 file_entry_offset;
        s64 file_entry_size;
        s64 body_offset;
    };
    static_assert(util::is_pod<RomFileSystemInformation>::value);
    static_assert(sizeof(RomFileSystemInformation) == 0x50);

    struct RomDirectoryInfo {
        /* ... */
    };
    static_assert(util::is_pod<RomDirectoryInfo>::value);

    struct RomFileInfo {
        Int64 offset;
        Int64 size;
    };
    static_assert(util::is_pod<RomFileInfo>::value);

    namespace RomStringTraits {

        constexpr inline char DirectorySeparator = '/';
        constexpr inline char NullTerminator     = '\x00';
        constexpr inline char Dot                = '.';

    }

}
