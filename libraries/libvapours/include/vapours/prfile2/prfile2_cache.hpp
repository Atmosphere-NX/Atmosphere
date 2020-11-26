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

    constexpr inline const auto MinimumFatBufferSize  = 1;
    constexpr inline const auto MaximumFatBufferSize  = (1 << 16) - 1;
    constexpr inline const auto MinimumDataBufferSize = 1;
    constexpr inline const auto MaximumDataBufferSize = (1 << 15) - 1;

    constexpr inline const auto MinimumFatPages  = 1;
    constexpr inline const auto MinimumDataPages = 4;

    struct SectorCache {
        u32 mode;
        u16 num_fat_pages;
        u16 num_data_pages;
        pf::CachePage *current_fat;
        pf::CachePage *current_data;
        pf::SectorBuffer *buffers;
        u32 fat_buf_size;
        u32 data_buf_size;
        void *signature;
    };

}

namespace ams::prfile2::cache {

    /* ... */

}
