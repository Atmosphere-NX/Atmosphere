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
#include <vapours/prfile2/prfile2_handle.hpp>

namespace ams::prfile2::pf {

    using DriveCharacter = char;

    enum Error {
        Error_NoError = 0,
        Error_Ok      = Error_NoError,

        Error_Generic = -1,

        Error_InvalidFileName        =  1,
        Error_InvalidPathName        =  2,
        Error_FileNotFound           =  3,
        Error_TooManyVolumesAttached =  4,
        Error_DirectoryFull          =  5,
        Error_VolumeFull             =  6,
        Error_InvalidDiskFormat      =  7,
        Error_FileAlreadyExists      =  8,
        Error_VolumeNotMounted       =  9,
        Error_InvalidParameter       = 10,
        Error_WriteProtected         = 11,
        Error_UnsupportedFormat      = 12,
        Error_BrokenClusterChain     = 13,
        Error_InvalidClusterNum      = 14,
        Error_InvalidBpb             = 15,
        Error_AccessOutOfVolume      = 16,
        Error_DriverError            = 17,
        Error_InvalidVolumeLabel     = 18,
        Error_FileOpened             = 19,
        Error_NotADirectory          = 20,
        Error_TooManyFilesOpenedS    = 21,
        Error_TooManyFilesOpenedU    = 22,
        Error_NotAFile               = 23,
        Error_ReadOnly               = 24,
        Error_LockError              = 25,
        Error_InternalError          = 26,
        Error_EndOfFile              = 27,
        Error_AccessNotAllowed       = 28,
        Error_DirectoryNotEmpty      = 29,
        Error_NotEnoughCachePages    = 30,
        Error_DifferentDrive         = 31,
        Error_DifferentEntry         = 32,
        Error_InvalidEntry           = 33,
        Error_InvalidSector          = 34,
        Error_BrokenVolume           = 35,
        Error_NotEffective           = 36,
        Error_FileSizeOver           = 37,
        Error_InvalidFileDiscriptor  = 38,
        Error_InvalidLockFile        = 39,
        Error_ExtensionNotRegistered = 40,
        Error_ExtensionError         = 41,
    };

    constexpr inline const int ReturnValueNoError = 0;
    constexpr inline const int ReturnValueError   = -1;

    constexpr inline int ConvertReturnValue(Error err) {
        if (AMS_LIKELY(err == Error_NoError)) {
            return ReturnValueNoError;
        } else {
            return ReturnValueError;
        }
    }

    struct CachePage {
        u16 status;
        u16 option;
        u8 *buffer_head;
        u8 *buffer_cur;
        u8 *buffer_dirty_start;
        u8 *buffer_dirty_end;
        u32 size;
        u32 sector;
        void *signature;
        CachePage *next;
        CachePage *prev;
    };
    static_assert(util::is_pod<CachePage>::value);

    constexpr inline auto SectorBufferSize     = 0x200;
    constexpr inline auto Log2SectorBufferSize = 9;
    static_assert((1u << Log2SectorBufferSize) == SectorBufferSize);

    using SectorBuffer = u8[SectorBufferSize];
    static_assert(sizeof(SectorBuffer) == SectorBufferSize);

    struct CacheSetting {
        CachePage *pages;
        SectorBuffer *buffers;
        u16 num_fat_pages;
        u16 num_data_pages;
        u32 fat_buf_size;
        u32 data_buf_size;
    };

    struct DriveTable {
        CacheSetting *cache;
        HandleType partition_handle;
        DriveCharacter drive_char;
        u8 status;

        template<size_t Ix>
        constexpr ALWAYS_INLINE bool GetStatusBit() const {
            constexpr u32 Mask = (1u << Ix);
            return (this->status & Mask) != 0;
        }

        template<size_t Ix>
        constexpr ALWAYS_INLINE void SetStatusBit(bool en) {
            constexpr u32 Mask = (1u << Ix);
            if (en) {
                this->status |= Mask;
            } else {
                this->status &= ~Mask;
            }
        }

        constexpr bool IsAttached() const { return this->GetStatusBit<0>(); }
        constexpr void SetAttached(bool en) { this->SetStatusBit<0>(en); }

        constexpr bool IsMounted() const { return this->GetStatusBit<1>(); }
        constexpr void SetMounted(bool en) { this->SetStatusBit<1>(en); }

        constexpr bool IsDiskInserted() const { return this->GetStatusBit<2>(); }
        constexpr void SetDiskInserted(bool en) { this->SetStatusBit<2>(en); }

        constexpr bool IsFlag12() const { return this->GetStatusBit<12>(); }
        constexpr void SetFlag12(bool en) { this->SetStatusBit<12>(en); }
    };

    using TailBuf = u32;

}
