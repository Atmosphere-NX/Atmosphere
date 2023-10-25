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
#include <vapours.hpp>

namespace ams::fat {

    constexpr inline size_t FatErrorNameMaxLength = 0x10;

    struct FatError {
        int error;
        int extra_error;
        int drive_id;
        char name[FatErrorNameMaxLength];
        u8 reserved[4];
    };
    static_assert(sizeof(FatError) == 0x20);
    static_assert(util::is_pod<FatError>::value);

    struct FatReportInfo1 {
        u16 open_file_peak_count;
        u16 open_directory_peak_count;
    };
    static_assert(sizeof(FatReportInfo1) == 4);
    static_assert(util::is_pod<FatReportInfo1>::value);

    struct FatReportInfo2 {
        u16 open_unique_file_entry_peak_count;
        u16 open_unique_directory_entry_peak_count;
    };
    static_assert(sizeof(FatReportInfo2) == 4);
    static_assert(util::is_pod<FatReportInfo2>::value);

    struct FatSafeInfo {
        u32 result;
        u32 error_number;
        u32 safe_error_number;
    };
    static_assert(sizeof(FatSafeInfo) == 12);
    static_assert(util::is_pod<FatSafeInfo>::value);

}
