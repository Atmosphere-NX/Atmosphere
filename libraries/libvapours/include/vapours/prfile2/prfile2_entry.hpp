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
#include <vapours/prfile2/prfile2_common.hpp>

namespace ams::prfile2 {

    constexpr inline const auto LongNamePathCharacters = 260;

    struct Volume;

    struct DirectoryEntry {
        u16 long_name[LongNamePathCharacters];
        u32 attr;
        u64 file_size;
        u8 create_time_ms;
        u16 create_time;
        u16 create_date;
        u8 create_flags;
        u8 access_time_ms;
        u16 access_time;
        u16 access_date;
        u8 access_flags;
        u8 modify_time_ms;
        u16 modify_time;
        u16 modify_date;
        u8 modify_flags;
        Volume *volume;
        u32 path_len;
        u32 start_cluster;
        u32 entry_sector;
        u16 entry_offset;
        /* ... */
        u8 num_entry_lfns;
        u8 ordinal;
        u8 check_sum;
        char short_name[13];
        u8 small_letter_flag;
        /* ... */
        u16 full_path[LongNamePathCharacters];
    };

    struct DirectoryTail {
        u32 tracker_size;
        pf::TailBuf tracker_buf[1];
        u32 *tracker_bits;
    };

}
