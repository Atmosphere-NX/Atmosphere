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
#include "ro_nro_utils.hpp"
#include "ro_random.hpp"

namespace ams::ro::impl {

    namespace {

        ALWAYS_INLINE size_t SetupNroProcessMemoryRegions(os::ProcessMemoryRegion *regions, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size) {
            /* Reset region count. */
            size_t num_regions = 0;

            /* We always want a region for the nro. */
            regions[num_regions++] = { nro_heap_address, nro_heap_size };

            /* If we have bss, create a region for bss. */
            if (bss_heap_size > 0) {
                regions[num_regions++] = { bss_heap_address, bss_heap_size };
            }

            return num_regions;
        }

    }

    Result MapNro(u64 *out_base_address, os::NativeHandle process_handle, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size) {
        /* Set up the process memory regions. */
        os::ProcessMemoryRegion regions[2];
        const size_t num_regions = SetupNroProcessMemoryRegions(regions, nro_heap_address, nro_heap_size, bss_heap_address, bss_heap_size);

        /* Re-map the nro/bss as code memory in the destination process. */
        R_TRY_CATCH(os::MapProcessCodeMemory(out_base_address, process_handle, regions, num_regions, ro::impl::GenerateSecureRandom)) {
            R_CONVERT(os::ResultOutOfAddressSpace, ro::ResultOutOfAddressSpace())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result SetNroPerms(os::NativeHandle process_handle, u64 base_address, u64 rx_size, u64 ro_size, u64 rw_size, bool is_aligned_header) {
        const u64 rx_offset = is_aligned_header ? os::MemoryPageSize : 0;
        const u64 ro_offset = rx_offset + rx_size;
        const u64 rw_offset = ro_offset + ro_size;

        if (is_aligned_header) {
            R_TRY(os::SetProcessMemoryPermission(process_handle, base_address, os::MemoryPageSize, os::MemoryPermission_ReadOnly));
        }

        R_TRY(os::SetProcessMemoryPermission(process_handle, base_address + rx_offset, rx_size, os::MemoryPermission_ReadExecute));
        R_TRY(os::SetProcessMemoryPermission(process_handle, base_address + ro_offset, ro_size, os::MemoryPermission_ReadOnly));
        R_TRY(os::SetProcessMemoryPermission(process_handle, base_address + rw_offset, rw_size, os::MemoryPermission_ReadWrite));

        R_SUCCEED();
    }

    Result UnmapNro(os::NativeHandle process_handle, u64 base_address, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size) {
        /* Set up the process memory regions. */
        os::ProcessMemoryRegion regions[2];
        const size_t num_regions = SetupNroProcessMemoryRegions(regions, nro_heap_address, nro_heap_size, bss_heap_address, bss_heap_size);

        /* Unmap the nro/bss. */
        R_RETURN(os::UnmapProcessCodeMemory(process_handle, base_address, regions, num_regions));
    }

}
