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
#include <stratosphere.hpp>
#include "ro_nro_utils.hpp"

namespace ams::ro::impl {

    namespace {

        constexpr size_t MaxMapRetries = 0x200;

    }

    Result MapNro(u64 *out_base_address, Handle process_handle, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size) {
        map::MappedCodeMemory nro_mcm(ResultInternalError{});
        map::MappedCodeMemory bss_mcm(ResultInternalError{});
        u64 base_address;

        /* Map the NRO, and map the BSS immediately after it. */
        size_t i;
        for (i = 0; i < MaxMapRetries; i++) {
            map::MappedCodeMemory tmp_nro_mcm(ResultInternalError{});
            R_TRY(map::MapCodeMemoryInProcess(tmp_nro_mcm, process_handle, nro_heap_address, nro_heap_size));
            base_address = tmp_nro_mcm.GetDstAddress();

            if (bss_heap_size > 0) {
                map::MappedCodeMemory tmp_bss_mcm(process_handle, base_address + nro_heap_size, bss_heap_address, bss_heap_size);
                R_TRY_CATCH(tmp_bss_mcm.GetResult()) {
                    R_CATCH(svc::ResultInvalidCurrentMemory) {
                        continue;
                    }
                } R_END_TRY_CATCH;

                if (!map::CanAddGuardRegionsInProcess(process_handle, base_address, nro_heap_size + bss_heap_size)) {
                    continue;
                }

                bss_mcm = std::move(tmp_bss_mcm);
            } else {
                if (!map::CanAddGuardRegionsInProcess(process_handle, base_address, nro_heap_size)) {
                    continue;
                }
            }
            nro_mcm = std::move(tmp_nro_mcm);
            break;
        }
        R_UNLESS(i < MaxMapRetries, ResultOutOfAddressSpace());

        /* Invalidation here actually prevents them from unmapping at scope exit. */
        nro_mcm.Invalidate();
        bss_mcm.Invalidate();

        *out_base_address = base_address;
        return ResultSuccess();
    }

    Result SetNroPerms(Handle process_handle, u64 base_address, u64 rx_size, u64 ro_size, u64 rw_size) {
        const u64 rx_offset = 0;
        const u64 ro_offset = rx_offset + rx_size;
        const u64 rw_offset = ro_offset + ro_size;

        R_TRY(svcSetProcessMemoryPermission(process_handle, base_address + rx_offset, rx_size, Perm_Rx));
        R_TRY(svcSetProcessMemoryPermission(process_handle, base_address + ro_offset, ro_size, Perm_R ));
        R_TRY(svcSetProcessMemoryPermission(process_handle, base_address + rw_offset, rw_size, Perm_Rw));

        return ResultSuccess();
    }

    Result UnmapNro(Handle process_handle, u64 base_address, u64 nro_heap_address, u64 bss_heap_address, u64 bss_heap_size, u64 code_size, u64 rw_size) {
        /* First, unmap bss. */
        if (bss_heap_size > 0) {
            R_TRY(svcUnmapProcessCodeMemory(process_handle, base_address + code_size + rw_size, bss_heap_address, bss_heap_size));
        }

        /* Next, unmap .rwdata */
        if (rw_size > 0) {
            R_TRY(svcUnmapProcessCodeMemory(process_handle, base_address + code_size, nro_heap_address + code_size, rw_size));
        }

        /* Finally, unmap .text + .rodata. */
        R_TRY(svcUnmapProcessCodeMemory(process_handle, base_address, nro_heap_address, code_size));

        return ResultSuccess();
    }

}
