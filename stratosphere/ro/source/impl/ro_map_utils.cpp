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
#include <stratosphere.hpp>
#include "ro_map_utils.hpp"

namespace ams::ro::impl {

    namespace {


        ALWAYS_INLINE u64 GetCurrentProcessInfo(svc::InfoType info_type) {
            u64 value;
            R_ABORT_UNLESS(svc::GetInfo(std::addressof(value), info_type, svc::PseudoHandle::CurrentProcess, 0));
            return value;
        }

    }

    Result SearchFreeRegion(uintptr_t *out, size_t mapping_size) {
        /* Get address space extents. */
        const uintptr_t heap_start  = GetCurrentProcessInfo(svc::InfoType_HeapRegionAddress);
        const size_t    heap_size   = GetCurrentProcessInfo(svc::InfoType_HeapRegionSize);
        const uintptr_t alias_start = GetCurrentProcessInfo(svc::InfoType_AliasRegionAddress);
        const size_t    alias_size  = GetCurrentProcessInfo(svc::InfoType_AliasRegionSize);
        const uintptr_t aslr_start  = GetCurrentProcessInfo(svc::InfoType_AslrRegionAddress);
        const size_t    aslr_size   = GetCurrentProcessInfo(svc::InfoType_AslrRegionSize);

        /* Iterate upwards to find a free region. */
        uintptr_t address = aslr_start;
        while (true) {
            /* Declare variables for memory querying. */
            svc::MemoryInfo mem_info;
            svc::PageInfo page_info;

            /* Check that we're still within bounds. */
            R_UNLESS(address < address + mapping_size, ro::ResultOutOfAddressSpace());

            /* If we're within the heap region, skip to the end of the heap region. */
            if (heap_size != 0 && !(address + mapping_size - 1 < heap_start || heap_start + heap_size - 1 < address)) {
                R_UNLESS(address < heap_start + heap_size, ro::ResultOutOfAddressSpace());
                address = heap_start + heap_size;
                continue;
            }

            /* If we're within the alias region, skip to the end of the alias region. */
            if (alias_size != 0 && !(address + mapping_size - 1 < alias_start || alias_start + alias_size - 1 < address)) {
                R_UNLESS(address < alias_start + alias_size, ro::ResultOutOfAddressSpace());
                address = alias_start + alias_size;
                continue;
            }

            /* Get the current memory range. */
            R_ABORT_UNLESS(svc::QueryMemory(std::addressof(mem_info), std::addressof(page_info), address));

            /* If the memory range is free and big enough, use it. */
            if (mem_info.state == svc::MemoryState_Free && mapping_size <= ((mem_info.base_address + mem_info.size) - address)) {
                *out = address;
                return ResultSuccess();
            }

            /* Check that we can advance. */
            R_UNLESS(address < mem_info.base_address + mem_info.size,                        ro::ResultOutOfAddressSpace());
            R_UNLESS(mem_info.base_address + mem_info.size - 1 < aslr_start + aslr_size - 1, ro::ResultOutOfAddressSpace());

            /* Advance. */
            address = mem_info.base_address + mem_info.size;
        }
    }

}
