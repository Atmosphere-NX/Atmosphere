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
#include <stratosphere.hpp>
#include <algorithm>
#include <cstdio>
#include <functional>
#include <cstring>
#include "ldr_nro.hpp"
#include "ldr_registration.hpp"
#include "ldr_map.hpp"
#include "ldr_random.hpp"

Result NroUtils::ValidateNrrHeader(NrrHeader *header, u64 size, u64 title_id_min) {
    if (header->magic != MAGIC_NRR0) {
        return ResultLoaderInvalidNrr;
    }
    if (header->nrr_size != size) {
        return ResultLoaderInvalidSize;
    }
    
    /* TODO: Check NRR signature. */
    if (false) {
        return ResultLoaderInvalidSignature;
    }
    
    if (header->title_id_min != title_id_min) {
        return ResultLoaderInvalidNrr;
    }
    
    return ResultSuccess;
}

Result NroUtils::LoadNro(Registration::Process *target_proc, Handle process_h, u64 nro_heap_address, u64 nro_heap_size, u64 bss_heap_address, u64 bss_heap_size, u64 *out_address) {
    NroHeader nro_hdr = {0};
    MappedCodeMemory mcm_nro = {0};
    MappedCodeMemory mcm_bss = {0};
    unsigned int i;
    Result rc = ResultSuccess;
    u8 nro_hash[0x20];

    /* Perform cleanup on failure. */
    ON_SCOPE_EXIT {
        if (R_FAILED(rc)) {
            mcm_nro.Close();
            mcm_bss.Close();
        }
    };

    /* Ensure there is an available NRO slot. */
    if (std::all_of(target_proc->nro_infos.begin(), target_proc->nro_infos.end(), std::mem_fn(&Registration::NroInfo::in_use))) {
        rc = ResultLoaderInsufficientNroRegistrations;
        return rc;
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
        rc = ResultLoaderInsufficientAddressSpace;
        return rc;
    }

    /* Map the NRO. */
    if (R_FAILED((rc = mcm_nro.Map()))) {
        return rc;
    }
    
    /* Read data from NRO while it's mapped. */
    {
        nro_hdr = *((NroHeader *)mcm_nro.mapped_address);

        if (nro_hdr.magic != MAGIC_NRO0) {
            rc = ResultLoaderInvalidNro;
            return rc;
        }
        if (nro_hdr.nro_size != nro_heap_size || nro_hdr.bss_size != bss_heap_size) {
            rc = ResultLoaderInvalidNro;
            return rc;
        }
        if ((nro_hdr.text_size & 0xFFF) || (nro_hdr.ro_size & 0xFFF) || (nro_hdr.rw_size & 0xFFF) || (nro_hdr.bss_size & 0xFFF)) {
            rc = ResultLoaderInvalidNro;
            return rc;
        }
        if (nro_hdr.text_offset != 0 || nro_hdr.text_offset + nro_hdr.text_size != nro_hdr.ro_offset || nro_hdr.ro_offset + nro_hdr.ro_size != nro_hdr.rw_offset || nro_hdr.rw_offset + nro_hdr.rw_size != nro_hdr.nro_size) {
            rc = ResultLoaderInvalidNro;
            return rc;
        }
        
        sha256CalculateHash(nro_hash, mcm_nro.mapped_address, nro_hdr.nro_size);
    }

    /* Unmap the NRO. */
    if (R_FAILED((rc = mcm_nro.Unmap()))) {
        return rc;
    }
    
    if (!Registration::IsNroHashPresent(target_proc->index, nro_hash)) {
        rc = ResultLoaderInvalidSignature;
        return rc;
    }

    if (Registration::IsNroAlreadyLoaded(target_proc->index, nro_hash)) {
        rc = ResultLoaderNroAlreadyLoaded;
        return rc;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address, nro_hdr.text_size, 5)))) {
        return rc;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address + nro_hdr.ro_offset, nro_hdr.ro_size, 1)))) {
        return rc;
    }
    
    if (R_FAILED((rc = svcSetProcessMemoryPermission(process_h, mcm_nro.code_memory_address + nro_hdr.rw_offset, nro_hdr.rw_size + nro_hdr.bss_size, 3)))) {
        return rc;
    }
    
    Registration::AddNroToProcess(target_proc->index, &mcm_nro, &mcm_bss, nro_hdr.text_size, nro_hdr.ro_size, nro_hdr.rw_size, nro_hdr.build_id);
    *out_address = mcm_nro.code_memory_address;
    rc = ResultSuccess;

    return rc;
}
