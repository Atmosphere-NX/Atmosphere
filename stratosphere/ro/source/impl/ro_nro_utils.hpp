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

namespace ams::ro::impl {

    /* Utilities for working with NROs. */
    Result MapNro(u64 *out_base_address, os::NativeHandle process_handle, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size);
    Result SetNroPerms(os::NativeHandle process_handle, u64 base_address, u64 rx_size, u64 ro_size, u64 rw_size);
    Result UnmapNro(os::NativeHandle process_handle, u64 base_address, u64 nro_heap_address, u64 bss_heap_address, u64 bss_heap_size, u64 code_size, u64 rw_size);

}