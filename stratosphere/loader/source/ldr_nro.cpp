/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include <algorithm>
#include <cstdio>
#include <functional>
#include <cstring>
#include "sha256.h"
#include "ldr_nro.hpp"
#include "ldr_registration.hpp"
#include "ldr_map.hpp"
#include "ldr_random.hpp"

Result NroUtils::ValidateNrrHeader(NrrHeader *header, u64 size, u64 title_id_min) {
    if (header->magic != MAGIC_NRR0) {
        return 0x6A09;
    }
    if (header->nrr_size != size) {
        return 0xA409;
    }
    
    /* TODO: Check NRR signature. */
    if (false) {
        return 0x6C09;
    }
    
    if (header->title_id_min != title_id_min) {
        return 0x6A09;
    }
    
    return 0x0;
}

Result NroUtils::LoadNro(Registration::Process *target_proc, Handle process_h, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size, u64 *out_address) {
    NroHeader *nro;
    MappedCodeMemory mcm_nro = {0};
    MappedCodeMemory mcm_bss = {0};
    unsigned int i;
    Result rc;
    u8 nro_hash[0x20];
    struct sha256_state sha_ctx;
    /* Ensure there is an available NRO slot. */
    if (std::all_of(target_proc->nro_infos.begin(), target_proc->nro_infos.end(), std::mem_fn(&Registration::NroInfo::in_use))) {
        return 0x6E09;
    }
    for (i = 0; i < 0x200; i++) {
        if (R_SUCCEEDED(mcm_nro.Open(process_h, target_proc->is_64_bit_addspace, nro_heap_address, nro_heap_size))) {
            if (R_SUCCEEDED(mcm_bss.OpenAtAddress(process_h, bss_heap_address, bss_heap_size, mcm_nro.code_memory_address + nro_heap_size))) {
                break;
            } else {
                mcm_nro.Close();
            }
        }
    }
    if (i >= 0x200) {
        return 0x6609;
    }
    if (R_FAILED((rc = mcm_nro.Map()))) {
        goto LOAD_NRO_END;
    }
    
    nro = (NroHeader *)mcm_nro.mapped_address;
    if (nro->magic != MAGIC_NRO0) {
        rc = 0x6809;
        goto LOAD_NRO_END;
    }
    if (nro->nro_size != nro_heap_size || nro->bss_size != bss_heap_size) {
        rc = 0x6809;
        goto LOAD_NRO_END;
    }
    if ((nro->text_size & 0xFFF) || (nro->ro_size & 0xFFF) || (nro->rw_size & 0xFFF) || (nro->bss_size & 0xFFF)) {
        rc = 0x6809;
        goto LOAD_NRO_END;
    }
    if (nro->text_offset != 0 || nro->text_offset + nro->text_size != nro->ro_offset || nro->ro_offset + nro->ro_size != nro->rw_offset || nro->rw_offset + nro->rw_size != nro->nro_size) {
        rc = 0x6809;
        goto LOAD_NRO_END;
    }
    
    
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, (u8 *)nro, nro->nro_size);
    sha256_finalize(&sha_ctx);
    sha256_finish(&sha_ctx, nro_hash);
    
    if (!Registration::IsNroHashPresent(target_proc->index, nro_hash)) {
        rc = 0x6C09;
        goto LOAD_NRO_END;
    }

    if (Registration::IsNroAlreadyLoaded(target_proc->index, nro_hash)) {
        rc = 0x7209;
        goto LOAD_NRO_END;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address, nro->text_size, 5)))) {
        goto LOAD_NRO_END;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address + nro->ro_offset, nro->ro_size, 1)))) {
        goto LOAD_NRO_END;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address + nro->rw_offset, nro->rw_size + nro->bss_size, 3)))) {
        goto LOAD_NRO_END;
    }
    
    Registration::AddNroToProcess(target_proc->index, &mcm_nro, &mcm_bss, nro->text_size, nro->ro_size, nro->rw_size, nro->build_id);
    *out_address = mcm_nro.code_memory_address;
    mcm_nro.Unmap();
    mcm_bss.Unmap();
    rc = 0x0;

LOAD_NRO_END:
    if (R_FAILED(rc)) {
        mcm_nro.Close();
        mcm_bss.Close();
    }
    return 0x0;
}
