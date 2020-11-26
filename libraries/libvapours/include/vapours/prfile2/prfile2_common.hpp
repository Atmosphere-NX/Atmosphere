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

    enum InternalError {
        InternalError_NoError = 0,
        InternalError_Ok      = InternalError_NoError,

        InternalError_Generic = -1,

        InternalError_InvalidFileName        =  1,
        InternalError_InvalidPathName        =  2,
        InternalError_FileNotFound           =  3,
        InternalError_TooManyVolumesAttached =  4,
        InternalError_DirectoryFull          =  5,
        InternalError_VolumeFull             =  6,
        InternalError_InvalidDiskFormat      =  7,
        InternalError_FileAlreadyExists      =  8,
        InternalError_VolumeNotMounted       =  9,
        InternalError_InvalidParameter       = 10,
        InternalError_WriteProtected         = 11,
        InternalError_UnsupportedFormat      = 12,
        InternalError_BrokenClusterChain     = 13,
        InternalError_InvalidClusterNum      = 14,
        InternalError_InvalidBpb             = 15,
        InternalError_AccessOutOfVolume      = 16,
        InternalError_DriverError            = 17,
        InternalError_InvalidVolumeLabel     = 18,
        InternalError_FileOpened             = 19,
        InternalError_NotADirectory          = 20,
        InternalError_TooManyFilesOpenedS    = 21,
        InternalError_TooManyFilesOpenedU    = 22,
        InternalError_NotAFile               = 23,
        InternalError_ReadOnly               = 24,
        InternalError_LockError              = 25,
        InternalError_InternalError          = 26,
        InternalError_EndOfFile              = 27,
        InternalError_AccessNotAllowed       = 28,
        InternalError_DirectoryNotEmpty      = 29,
        InternalError_NotEnoughCachePages    = 30,
        InternalError_DifferentDrive         = 31,
        InternalError_DifferentEntry         = 32,
        InternalError_InvalidEntry           = 33,
        InternalError_InvalidSector          = 34,
        InternalError_BrokenVolume           = 35,
        InternalError_NotEffective           = 36,
        InternalError_FileSizeOver           = 37,
        InternalError_InvalidFileDiscriptor  = 38,
        InternalError_InvalidLockFile        = 39,
        InternalError_ExtensionNotRegistered = 40,
        InternalError_ExtensionError         = 41,
    };

    constexpr inline const u32 InvalidSector = std::numeric_limits<u32>::max();

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
