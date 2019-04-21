/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "ro_registration.hpp"
#include "ro_map.hpp"
#include "ro_nrr.hpp"

/* Declare process contexts as global array. */
static Registration::RoProcessContext g_process_contexts[Registration::MaxSessions] = {};

static bool g_is_development_hardware, g_is_development_function_enabled;

void Registration::Initialize() {
    if (R_FAILED(splInitialize())) {
        std::abort();
    }
    ON_SCOPE_EXIT { splExit(); };
    
    if (R_FAILED(splIsDevelopment(&g_is_development_hardware))) {
        std::abort();
    }
    
    {
        u64 out_val = 0;
        if (R_FAILED(splGetConfig(SplConfigItem_IsDebugMode, &out_val))) {
            std::abort();
        }
        g_is_development_function_enabled = out_val != 0;
    }
}

bool Registration::ShouldEaseNroRestriction() {
    bool should_ease = false;
    
    if (R_FAILED(setsysGetSettingsItemValue("ro", "ease_nro_restriction", &should_ease, sizeof(should_ease)))) {
        return false;
    }
    
    /* Nintendo only allows easing restriction on dev, we will allow on production, as well. */
    /* should_ease &= g_is_development_function_enabled; */
    return should_ease;
}

Result Registration::RegisterProcess(RoProcessContext **out_context, Handle process_handle, u64 process_id) {
    /* Check if a process context already exists. */
    for (size_t i = 0; i < Registration::MaxSessions; i++) {
        if (g_process_contexts[i].process_id == process_id) {
            return ResultRoInvalidSession;
        }
    }
    
    /* Find a free process context. */
    for (size_t i = 0; i < Registration::MaxSessions; i++) {
        if (!g_process_contexts[i].in_use) {
            g_process_contexts[i].process_id = process_id;
            g_process_contexts[i].process_handle = process_handle;
            g_process_contexts[i].in_use = true;
            *out_context = &g_process_contexts[i];
            return ResultSuccess;
        }
    }
    
    /* Failure to find a free context is actually an abort condition. */
    /* TODO: Should this return an unofficial error code? */
    std::abort();
}

void Registration::UnregisterProcess(RoProcessContext *context) {
    if (context->process_handle != INVALID_HANDLE) {
        for (size_t i = 0; i < Registration::MaxNrrInfos; i++) {
            if (context->nrr_in_use[i]) {
                UnmapNrr(context->process_handle, context->nrr_infos[i].header, context->nrr_infos[i].nrr_heap_address, context->nrr_infos[i].nrr_heap_size, context->nrr_infos[i].mapped_code_address);
            }
        }
    }
    std::memset(context, 0, sizeof(*context));
}

Result Registration::GetProcessModuleInfo(u32 *out_count, LoaderModuleInfo *out_infos, size_t max_out_count, u64 process_id) {
    size_t count = 0;
    for (size_t sess = 0; sess < Registration::MaxSessions; sess++) {
        if (g_process_contexts[sess].process_id == process_id) {
            /* For convenience, helper. */
            const RoProcessContext *context = &g_process_contexts[sess];

            for (size_t i = 0; i < Registration::MaxNroInfos && count < max_out_count; i++) {
                if (!context->nro_in_use[i]) {
                    continue;
                }

                /* Just copy out the info. */
                LoaderModuleInfo *out_info = &out_infos[count++];
                memcpy(out_info->build_id, &context->nro_infos[i].module_id, sizeof(context->nro_infos[i].module_id));
                out_info->base_address = context->nro_infos[i].base_address;
                out_info->size = context->nro_infos[i].nro_heap_size + context->nro_infos[i].bss_heap_size;
            }

            break;
        }
    }

    *out_count = static_cast<u32>(count);
    return ResultSuccess;
}

Result Registration::LoadNrr(RoProcessContext *context, u64 title_id, u64 nrr_address, u64 nrr_size, RoModuleType expected_type, bool enforce_type) {
    /* Validate address/size. */
    if (nrr_address & 0xFFF) {
        return ResultRoInvalidAddress;
    }
    if (nrr_size == 0 || (nrr_size & 0xFFF) || !(nrr_address < nrr_address + nrr_size)) {
        return ResultRoInvalidSize;
    }
    
    /* Check we have space for a new NRR. */
    size_t slot = 0;
    for (slot = 0; slot < Registration::MaxNrrInfos; slot++) {
        if (!context->nrr_in_use[slot]) {
            break;
        }
    }
    if (slot == Registration::MaxNrrInfos) {
        return ResultRoTooManyNrr;
    }
    
    NrrInfo *nrr_info = &context->nrr_infos[slot];
    
    /* Map. */
    NrrHeader *header = nullptr;
    u64 mapped_code_address = 0;
    Result rc = MapAndValidateNrr(&header, &mapped_code_address, context->process_handle, title_id, nrr_address, nrr_size, expected_type, enforce_type);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* Set NRR info. */
    nrr_info->header = header;
    nrr_info->nrr_heap_address = nrr_address;
    nrr_info->nrr_heap_size = nrr_size;
    nrr_info->mapped_code_address = mapped_code_address;
    context->nrr_in_use[slot] = true;
    
    /* TODO. */
    return ResultSuccess;
}

Result Registration::UnloadNrr(RoProcessContext *context, u64 nrr_address) {
    /* Validate address. */
    if (nrr_address & 0xFFF) {
        return ResultRoInvalidAddress;
    }

    /* Check the NRR is loaded. */
    size_t slot = 0;
    for (slot = 0; slot < Registration::MaxNrrInfos; slot++) {
        if (!context->nrr_in_use[slot]) {
            continue;
        }
        
        if (context->nrr_infos[slot].nrr_heap_address == nrr_address) {
            break;
        }
    }
    if (slot == Registration::MaxNrrInfos) {
        return ResultRoNotRegistered;
    }

    /* Unmap. */
    const NrrInfo nrr_info = context->nrr_infos[slot];
    {
        /* Nintendo does this unconditionally, whether or not the actual unmap succeeds. */
        context->nrr_in_use[slot] = false;
        std::memset(&context->nrr_infos[slot], 0, sizeof(context->nrr_infos[slot]));
    }
    return UnmapNrr(context->process_handle, nrr_info.header, nrr_info.nrr_heap_address, nrr_info.nrr_heap_size, nrr_info.mapped_code_address);
}

Result Registration::MapAndValidateNrr(NrrHeader **out_header, u64 *out_mapped_code_address, Handle process_handle, u64 title_id, u64 nrr_heap_address, u64 nrr_heap_size, RoModuleType expected_type, bool enforce_type) {
    Result rc;
    MappedCodeMemory nrr_mcm;
    
    /* First, map the NRR. */
    if (R_FAILED((rc = MapUtils::MapCodeMemoryForProcess(nrr_mcm, process_handle, true, nrr_heap_address, nrr_heap_size)))) {
        if (GetRuntimeFirmwareVersion() < FirmwareVersion_300) {
            /* Try mapping as 32-bit, since we might have guessed wrong on < 3.0.0. */
            rc = MapUtils::MapCodeMemoryForProcess(nrr_mcm, process_handle, false, nrr_heap_address, nrr_heap_size);
        }
        if (R_FAILED(rc)) {
            return rc;
        }
    }
    
    const u64 code_address = nrr_mcm.GetDstAddress();
    u64 map_address;
    if (R_FAILED(MapUtils::LocateSpaceForMap(&map_address, nrr_heap_size))) {
        return ResultRoInsufficientAddressSpace;
    }
    
    /* Nintendo...does not check the return value of this map. We will check, instead of aborting if it fails. */
    AutoCloseMap nrr_map(map_address, process_handle, code_address, nrr_heap_size);
    if (!nrr_map.IsSuccess()) {
        return nrr_map.GetResult();
    }
    
    NrrHeader *nrr_header = reinterpret_cast<NrrHeader *>(map_address);
    if (R_FAILED((rc = NrrUtils::ValidateNrr(nrr_header, nrr_heap_size, title_id, expected_type, enforce_type)))) {
        return rc;
    }
    
    /* Invalidation here actually prevents them from unmapping at scope exit. */
    nrr_map.Invalidate();
    nrr_mcm.Invalidate();
    
    *out_header = nrr_header;
    *out_mapped_code_address = code_address;
    return ResultSuccess;
}

Result Registration::UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address) {
    Result rc = svcUnmapProcessMemory((void *)header, process_handle, mapped_code_address, nrr_heap_size);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    return svcUnmapProcessCodeMemory(process_handle, mapped_code_address, nrr_heap_address, nrr_heap_size);
}
