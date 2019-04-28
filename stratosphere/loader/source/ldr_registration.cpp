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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include "ldr_registration.hpp"

static Registration::List g_registration_list = {};
static u64 g_num_registered = 1;

Registration::Process *Registration::GetFreeProcess() {
    auto process_it = std::find_if_not(g_registration_list.processes.begin(), g_registration_list.processes.end(), std::mem_fn(&Registration::Process::in_use));
    if (process_it == g_registration_list.processes.end()) {
        return nullptr;
    }
    return &*process_it;
}

Registration::Process *Registration::GetProcess(u64 index) {
    for (unsigned int i = 0; i < Registration::MaxProcesses; i++) {
        if (g_registration_list.processes[i].in_use && g_registration_list.processes[i].index == index) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::GetProcessByProcessId(u64 pid) {
    for (unsigned int i = 0; i < Registration::MaxProcesses; i++) {
        if (g_registration_list.processes[i].in_use && g_registration_list.processes[i].process_id == pid) {
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
    *free_process = {};
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
    *target_process = {};
    return true;
}


Result Registration::GetRegisteredTidSid(u64 index, Registration::TidSid *out) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return ResultLoaderProcessNotRegistered;
    }
    
    *out = target_process->tid_sid;
    
    return ResultSuccess;
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

void Registration::AddModuleInfo(u64 index, u64 base_address, u64 size, const unsigned char *build_id) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return;
    }

    auto nso_info_it = std::find_if_not(target_process->module_infos.begin(), target_process->module_infos.end(), std::mem_fn(&Registration::ModuleInfoHolder::in_use));
    if (nso_info_it != target_process->module_infos.end()) {
        nso_info_it->info.base_address = base_address;
        nso_info_it->info.size = size;
        memcpy(nso_info_it->info.build_id, build_id, sizeof(nso_info_it->info.build_id));
        nso_info_it->in_use = true;
    }
}

Result Registration::GetProcessModuleInfo(LoaderModuleInfo *out, u32 max_out, u64 process_id, u32 *num_written) {
    Registration::Process *target_process = GetProcessByProcessId(process_id);
    if (target_process == NULL) {
        return ResultLoaderProcessNotRegistered;
    }
    u32 cur = 0;
    
    for (unsigned int i = 0; i < Registration::MaxModuleInfos && cur < max_out; i++) {
        if (target_process->module_infos[i].in_use) {
            out[cur++] = target_process->module_infos[i].info;
        }
    }
    
    *num_written = cur;

    return ResultSuccess;
}

void Registration::AssociatePidTidForMitM(u64 index) {
    Registration::Process *target_process = GetProcess(index);
    if (target_process == NULL) {
        return;
    }
    
    Handle sm_hnd;
    Result rc = svcConnectToNamedPort(&sm_hnd, "sm:");
    if (R_SUCCEEDED(rc)) {
        /* Initialize. */
        {
            IpcCommand c;
            ipcInitialize(&c);
            ipcSendPid(&c);

            struct {
                u64 magic;
                u64 cmd_id;
                u64 zero;
                u64 reserved[2];
            } *raw = (decltype(raw))ipcPrepareHeader(&c, sizeof(*raw));

            raw->magic = SFCI_MAGIC;
            raw->cmd_id = 0;
            raw->zero = 0;

            rc = ipcDispatch(sm_hnd);

            if (R_SUCCEEDED(rc)) {
                IpcParsedCommand r;
                ipcParse(&r);

                struct {
                    u64 magic;
                    u64 result;
                } *resp = (decltype(resp))r.Raw;

                rc = resp->result;
            }
        }
        /* Associate. */
        if (R_SUCCEEDED(rc)) {
            IpcCommand c;
            ipcInitialize(&c);
            struct {
                u64 magic;
                u64 cmd_id;
                u64 process_id;
                u64 title_id;
            } *raw = (decltype(raw))ipcPrepareHeader(&c, sizeof(*raw));
            
            raw->magic = SFCI_MAGIC;
            raw->cmd_id = 65002;
            raw->process_id = target_process->process_id;
            raw->title_id = target_process->tid_sid.title_id;
            
            ipcDispatch(sm_hnd);
        }
        svcCloseHandle(sm_hnd);
    }
}