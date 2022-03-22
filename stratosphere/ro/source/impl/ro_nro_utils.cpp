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
#include "ro_map_utils.hpp"

namespace ams::ro::impl {

    Result MapNro(u64 *out_base_address, os::NativeHandle process_handle, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size) {
        /* Re-map the NRO/BSS as code memory in the destination process. */
        MappedCodeMemory nro_mcm;
        MappedCodeMemory bss_mcm;
        ProcessRegionInfo region_info(process_handle);
        u64 base_address;
        {
            const u64 memory_size = nro_heap_size + bss_heap_size;
            int i;
            for (i = 0; i < RetrySearchCount; ++i) {
                /* Get a random address for the nro. */
                base_address = region_info.GetAslrRegion(memory_size);
                R_UNLESS(base_address != 0, ro::ResultOutOfAddressSpace());

                /* Map the NRO, retrying if random address was invalid. */
                MappedCodeMemory tmp_nro_mcm(process_handle, base_address, nro_heap_address, nro_heap_size);
                R_TRY_CATCH(tmp_nro_mcm.GetResult()) {
                    R_CATCH(svc::ResultInvalidCurrentMemory) { continue; }
                } R_END_TRY_CATCH;

                /* Handle bss. */
                if (bss_heap_size > 0) {
                    /* Map BSS, retrying if random address was invalid. */
                    MappedCodeMemory tmp_bss_mcm(process_handle, base_address + nro_heap_size, bss_heap_address, bss_heap_size);
                    R_TRY_CATCH(tmp_bss_mcm.GetResult()) {
                        R_CATCH(svc::ResultInvalidCurrentMemory) { continue; }
                    } R_END_TRY_CATCH;

                    /* Check that we can have guard spaces. */
                    if (!region_info.CanEmplaceGuardSpaces(process_handle, base_address, memory_size)) {
                        continue;
                    }

                    /* We succeeded, so save the bss memory. */
                    bss_mcm = std::move(tmp_bss_mcm);
                } else {
                    /* Check that we can have guard spaces. */
                    if (!region_info.CanEmplaceGuardSpaces(process_handle, base_address, memory_size)) {
                        continue;
                    }
                }

                /* We succeeded, so save the code memory. */
                nro_mcm = std::move(tmp_nro_mcm);
                break;
            }

            R_UNLESS(i != RetrySearchCount, ro::ResultOutOfAddressSpace());
        }

        /* Cancel the automatic closing of our mappings. */
        nro_mcm.Cancel();
        bss_mcm.Cancel();

        *out_base_address = base_address;
        return ResultSuccess();
    }

    Result SetNroPerms(os::NativeHandle process_handle, u64 base_address, u64 rx_size, u64 ro_size, u64 rw_size) {
        const u64 rx_offset = 0;
        const u64 ro_offset = rx_offset + rx_size;
        const u64 rw_offset = ro_offset + ro_size;

        R_TRY(svc::SetProcessMemoryPermission(process_handle, base_address + rx_offset, rx_size, svc::MemoryPermission_ReadExecute));
        R_TRY(svc::SetProcessMemoryPermission(process_handle, base_address + ro_offset, ro_size, svc::MemoryPermission_Read));
        R_TRY(svc::SetProcessMemoryPermission(process_handle, base_address + rw_offset, rw_size, svc::MemoryPermission_ReadWrite));

        return ResultSuccess();
    }

    Result UnmapNro(os::NativeHandle process_handle, u64 base_address, u64 nro_heap_address, u64 bss_heap_address, u64 bss_heap_size, u64 code_size, u64 rw_size) {
        /* First, unmap bss. */
        if (bss_heap_size > 0) {
            R_TRY(svc::UnmapProcessCodeMemory(process_handle, base_address + code_size + rw_size, bss_heap_address, bss_heap_size));
        }

        /* Next, unmap .rwdata */
        if (rw_size > 0) {
            R_TRY(svc::UnmapProcessCodeMemory(process_handle, base_address + code_size, nro_heap_address + code_size, rw_size));
        }

        /* Finally, unmap .text + .rodata. */
        R_TRY(svc::UnmapProcessCodeMemory(process_handle, base_address, nro_heap_address, code_size));

        return ResultSuccess();
    }

}
