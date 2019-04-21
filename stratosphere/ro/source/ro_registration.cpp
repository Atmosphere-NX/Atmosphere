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

/* Declare process contexts as static to this function. */
static Registration::RoProcessContext g_process_contexts[Registration::MaxSessions] = {};

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

Result Registration::UnmapNrr(Handle process_handle, const NrrHeader *header, u64 nrr_heap_address, u64 nrr_heap_size, u64 mapped_code_address) {
    Result rc = svcUnmapProcessMemory((void *)header, process_handle, mapped_code_address, nrr_heap_size);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    return svcUnmapProcessCodeMemory(process_handle, mapped_code_address, nrr_heap_address, nrr_heap_size);
}
