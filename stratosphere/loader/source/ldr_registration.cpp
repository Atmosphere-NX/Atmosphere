#include <switch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "ldr_registration.hpp"
#include "ldr_nro.hpp"

static Registration::List g_registration_list = {0};
static u64 g_num_registered = 1;

Registration::Process *Registration::GetFreeProcess() {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX; i++) {
        if (!g_registration_list.processes[i].in_use) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::GetProcess(u64 index) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX; i++) {
        if (g_registration_list.processes[i].in_use && g_registration_list.processes[i].index == index) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::GetProcessByProcessId(u64 pid) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX; i++) {
        if (g_registration_list.processes[i].in_use && g_registration_list.processes[i].process_id == pid) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::GetProcessByRoService(void *service) {
    for (unsigned int i = 0; i < REGISTRATION_LIST_MAX; i++) {
        if (g_registration_list.processes[i].in_use && g_registration_list.processes[i].owner_ro_service == service) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

bool Registration::RegisterTidSid(const TidSid *tid_sid, u64 *out_index) { 
    Registration::Process *free_process = GetFreeProcess();
    if (free_process == NULL) {
        return false;
    }
    
    /* Reset the process. */
    *free_process = {0};
    free_process->tid_sid = *tid_sid;
    free_process->in_use = true;
    free_process->index = g_num_registered++;
    *out_index = free_process->index;
    return true;
}

bool Registration::UnregisterIndex(u64 index) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return false;
    }
    
    /* Reset the process. */
    *target_process = {0};
    return true;
}


Result Registration::GetRegisteredTidSid(u64 index, Registration::TidSid *out) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return 0x1009;
    }
    
    *out = target_process->tid_sid;
    
    return 0;
}

void Registration::SetProcessIdTidAndIs64BitAddressSpace(u64 index, u64 process_id, u64 tid, bool is_64_bit_addspace) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return;
    }
    
    target_process->process_id = process_id;
    target_process->title_id = tid;
    target_process->is_64_bit_addspace = is_64_bit_addspace;
}

void Registration::AddNsoInfo(u64 index, u64 base_address, u64 size, const unsigned char *build_id) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return;
    }
    
    for (unsigned int i = 0; i < NSO_INFO_MAX; i++) {
        if (!target_process->nso_infos[i].in_use) {
            target_process->nso_infos[i].info.base_address = base_address;
            target_process->nso_infos[i].info.size = size;
            std::copy(build_id, build_id + sizeof(target_process->nso_infos[i].info.build_id), target_process->nso_infos[i].info.build_id);
            target_process->nso_infos[i].in_use = true;
            return;
        }
    }
}

void Registration::CloseRoService(void *service, Handle process_h) {
    Registration::Process *target_process = GetProcessByRoService(service);
    if (target_process == NULL) {
        return;
    }
    for (unsigned int i = 0; i < NRR_INFO_MAX; i++) {
        if (target_process->nrr_infos[i].IsActive() && target_process->nrr_infos[i].process_handle == process_h) {
            target_process->nrr_infos[i].Close();
        }
    }
    target_process->owner_ro_service = NULL;
}

Result Registration::AddNrrInfo(u64 index, MappedCodeMemory *nrr_info) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        /* TODO: panic() */
        return 0x7009;
    }
    
    for (unsigned int i = 0; i < NRR_INFO_MAX; i++) {
        if (!target_process->nrr_infos[i].IsActive()) {
            target_process->nrr_infos[i] = *nrr_info;
            return 0;
        }
    }
    return 0x7009;
}

Result Registration::RemoveNrrInfo(u64 index, u64 base_address) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        /* Despite the fact that this should really be a panic condition, Nintendo returns 0x1009 in this case. */
        return 0x1009;
    }
    
    for (unsigned int i = 0; i < NRR_INFO_MAX; i++) {
        if (target_process->nrr_infos[i].IsActive() && target_process->nrr_infos[i].base_address == base_address) {
            target_process->nrr_infos[i].Close();
            return 0;
        }
    }
    return 0xAA09;
}


bool Registration::IsNroHashPresent(u64 index, u8 *nro_hash) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        /* TODO: panic */
        return false;
    }
    
    for (unsigned int i = 0; i < NRR_INFO_MAX; i++) {
        if (target_process->nrr_infos[i].IsActive()) {
            NroUtils::NrrHeader *nrr = (NroUtils::NrrHeader *)target_process->nrr_infos[i].mapped_address;
            /* Binary search. */
            int low = 0, high = (int)(nrr->num_hashes - 1);
            while (low <= high) {
                int mid = (low + high) / 2;
                u8 *hash_in_nrr = (u8 *)nrr + nrr->hash_offset + 0x20 * mid;
                int ret = std::memcmp(hash_in_nrr, nro_hash, 0x20);
                if (ret == 0) {
                    return true;
                } else if (ret > 0) {
                    high = mid - 1;
                } else {
                    low = mid + 1;
                }
            }
        }
    }
    return false;
}

bool Registration::IsNroAlreadyLoaded(u64 index, u8 *build_id) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        /* TODO: panic */
        return true;
    }
    
    for (unsigned int i = 0; i < NRO_INFO_MAX; i++) {
        if (target_process->nro_infos[i].in_use && std::memcmp(target_process->nro_infos[i].build_id, build_id, 0x20) == 0) {
            return true;
        }
    }
    return false;
}

void Registration::AddNroToProcess(u64 index, MappedCodeMemory *nro, MappedCodeMemory *bss, u32 text_size, u32 ro_size, u32 rw_size, u8 *build_id) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        /* TODO: panic */
        return;
    }
    
    for (unsigned int i = 0; i < NRO_INFO_MAX; i++) {
        if (!target_process->nro_infos[i].in_use) {
            target_process->nro_infos[i].base_address = nro->code_memory_address;
            target_process->nro_infos[i].nro_heap_address = nro->base_address;
            target_process->nro_infos[i].nro_heap_size = nro->size;
            target_process->nro_infos[i].bss_heap_address = bss->base_address;
            target_process->nro_infos[i].bss_heap_size = bss->size;
            target_process->nro_infos[i].text_size = text_size;
            target_process->nro_infos[i].ro_size = ro_size;
            target_process->nro_infos[i].rw_size = rw_size;
            std::copy(build_id, build_id + sizeof(target_process->nro_infos[i].build_id), target_process->nro_infos[i].build_id);
            target_process->nro_infos[i].in_use = true;
            break;
        }
    }
}

Result Registration::RemoveNroInfo(u64 index, Handle process_h, u64 nro_heap_address) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return 0xA809;
    }
    
    for (unsigned int i = 0; i < NRO_INFO_MAX; i++) {
        if (target_process->nro_infos[i].in_use && target_process->nro_infos[i].nro_heap_address == nro_heap_address) {
            NroInfo *info = &target_process->nro_infos[i];
            Result rc = svcUnmapProcessCodeMemory(process_h, info->base_address + info->text_size + info->ro_size + info->rw_size, info->bss_heap_address, info->bss_heap_size);
            if (R_SUCCEEDED(rc)) {
                rc = svcUnmapProcessCodeMemory(process_h, info->base_address + info->text_size + info->ro_size, nro_heap_address + info->text_size + info->ro_size, info->rw_size);
                if (R_SUCCEEDED(rc)) {
                    rc = svcUnmapProcessCodeMemory(process_h, info->base_address, nro_heap_address, info->text_size + info->ro_size);
                }
            }
            target_process->nro_infos[i] = (const NroInfo ){0};
            return rc;
        }
    }
    return 0xA809;
}

Result Registration::GetNsoInfosForProcessId(Registration::NsoInfo *out, u32 max_out, u64 process_id, u32 *num_written) {
    Registration::Process *target_process = GetProcessByProcessId(process_id);
    if (target_process == NULL) {
        return 0x1009;
    }
    u32 cur = 0;
    
    if (max_out > 0) {
        for (unsigned int i = 0; i < NSO_INFO_MAX && cur < max_out; i++) {
            if (target_process->nso_infos[i].in_use) {
                out[cur++] = target_process->nso_infos[i].info;
            }
        }
    }
    
    *num_written = cur;

    return 0;
}
