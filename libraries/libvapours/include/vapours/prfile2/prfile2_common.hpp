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
#include <vapours/prfile2/prfile2_build_config.hpp>
#include <vapours/prfile2/pf/prfile2_pf_config.hpp>
#include <vapours/prfile2/pf/prfile2_pf_api_common.hpp>

namespace ams::prfile2 {

    constexpr inline const auto MaximumOpenFileCountSystem      = pf::MaximumFileCount;
    constexpr inline const auto MaximumOpenFileCountUser        = pf::MaximumFileCount;
    constexpr inline const auto MaximumOpenDirectoryCountSystem = pf::MaximumDirectoryCount;
    constexpr inline const auto MaximumOpenDirectoryCountUser   = pf::MaximumDirectoryCount;

    enum Error {
        Error_NoError        =   0,
        Error_Ok             =   Error_NoError,

        Error_EPERM          =   1,
        Error_ENOENT         =   2,
        Error_ESRCH          =   3,
        Error_EIO            =   5,
        Error_ENOEXEC        =   8,
        Error_EBADF          =   9,
        Error_ENOMEM         =  12,
        Error_EACCES         =  13,
        Error_EBUSY          =  16,
        Error_EEXIST         =  17,
        Error_ENODEV         =  19,
        Error_EISDIR         =  21,
        Error_EINVAL         =  22,
        Error_ENFILE         =  23,
        Error_EMFILE         =  24,
        Error_EFBIG          =  27,
        Error_ENOSPC         =  28,
        Error_ENOLCK         =  46,
        Error_ENOSYS         =  88,
        Error_ENOTEMPTY      =  90,

        Error_EMOD_NOTREG    = 100,
        Error_EMOD_NOTSPRT   = 101,
        Error_EMOD_FCS       = 102,
        Error_EMOD_SAFE      = 103,

        Error_ENOMEDIUM      = 123,

        Error_EEXFAT_NOTSPRT = 200,

        Error_DFNC           = 0x1000,

        Error_SYSTEM         = -1,
    };

    constexpr inline const u32 InvalidSector  = std::numeric_limits<u32>::max();
    constexpr inline const u32 InvalidCluster = std::numeric_limits<u32>::max();

    enum OpenMode {
        OpenMode_None        = 0,
        OpenMode_Write       = (1u << 0),
        OpenMode_Read        = (1u << 1),
        OpenMode_Append      = (1u << 2),
        OpenMode_Plus        = (1u << 3),
        OpenMode_NoOverwrite = (1u << 4),
        OpenMode_V           = (1u << 5),
        OpenMode_ContCluster = (1u << 6),
    };

    enum FatFormat {
        FatFormat_Fat12 = 0,
        FatFormat_Fat16 = 1,
        FatFormat_Fat32 = 2,
        FatFormat_ExFat = 3,

        FatFormat_Invalid = -1,
    };

}
