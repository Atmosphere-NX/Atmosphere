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
#include <vapours.hpp>

namespace ams::kern::init {

    struct KernelLayout {
        u32 rx_offset;
        u32 rx_end_offset;
        u32 ro_offset;
        u32 ro_end_offset;
        u32 rw_offset;
        u32 rw_end_offset;
        u32 bss_offset;
        u32 bss_end_offset;
        u32 ini_load_offset;
        u32 dynamic_offset;
        u32 init_array_offset;
        u32 init_array_end_offset;
    };
    static_assert(util::is_pod<KernelLayout>::value);
    static_assert(sizeof(KernelLayout) == 0x30);

}