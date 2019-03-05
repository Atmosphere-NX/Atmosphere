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
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "ldr_ro_service.hpp"
#include "ldr_registration.hpp"
#include "ldr_map.hpp"
#include "ldr_nro.hpp"

Result RelocatableObjectsService::LoadNro(Out<u64> load_address, PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    Registration::Process *target_proc = NULL;
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        return 0xAE09;
    }
    if (nro_address & 0xFFF) {
        return 0xA209;
    }
    if (nro_address + nro_size <= nro_address || !nro_size || (nro_size & 0xFFF)) {
        return 0xA409;
    }
    if (bss_size && bss_address + bss_size <= bss_address) {
        return 0xA409;
    }
    /* Ensure no overflow for combined sizes. */
    if (U64_MAX - nro_size < bss_size) {
        return 0xA409;
    }
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        return 0xAC09;
    }
    target_proc->owner_ro_service = this;
    
    return NroUtils::LoadNro(target_proc, this->process_handle, nro_address, nro_size, bss_address, bss_size, load_address.GetPointer());
}

Result RelocatableObjectsService::UnloadNro(PidDescriptor pid_desc, u64 nro_address) {
    Registration::Process *target_proc = NULL;
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        return 0xAE09;
    }
    if (nro_address & 0xFFF) {
        return 0xA209;
    }
    
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        return 0xAC09;
    }
    target_proc->owner_ro_service = this;
    
    return Registration::RemoveNroInfo(target_proc->index, this->process_handle, nro_address);
}

Result RelocatableObjectsService::LoadNrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size) {
    Result rc = 0;
    Registration::Process *target_proc = NULL;
    MappedCodeMemory nrr_info = {0};
    ON_SCOPE_EXIT {
        if (R_FAILED(rc) && nrr_info.IsActive()) {
            nrr_info.Close();
        }
    };
    
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        rc = 0xAE09;
        return rc;
    }
    if (nrr_address & 0xFFF) {
        rc = 0xA209;
        return rc;
    }
    if (nrr_address + nrr_size <= nrr_address || !nrr_size || (nrr_size & 0xFFF)) {
        rc = 0xA409;
        return rc;
    }
    
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        rc = 0xAC09;
        return rc;
    }
    target_proc->owner_ro_service = this;
    
    if (R_FAILED((rc = nrr_info.Open(this->process_handle, target_proc->is_64_bit_addspace, nrr_address, nrr_size)))) {
        return rc;
    }
        
    if (R_FAILED((rc = nrr_info.Map()))) {
        return rc;
    }
    
    rc = NroUtils::ValidateNrrHeader((NroUtils::NrrHeader *)nrr_info.mapped_address, nrr_size, target_proc->title_id);
    if (R_SUCCEEDED(rc)) {
        Registration::AddNrrInfo(target_proc->index, &nrr_info);
    }
    
    return rc;
}

Result RelocatableObjectsService::UnloadNrr(PidDescriptor pid_desc, u64 nrr_address) {
    Registration::Process *target_proc = NULL;
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        return 0xAE09;
    }
    if (nrr_address & 0xFFF) {
        return 0xA209;
    }
    
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        return 0xAC09;
    }
    target_proc->owner_ro_service = this;
    
    return Registration::RemoveNrrInfo(target_proc->index, nrr_address);
}

Result RelocatableObjectsService::Initialize(PidDescriptor pid_desc, CopiedHandle process_h) {
    u64 handle_pid;
    if (R_SUCCEEDED(svcGetProcessId(&handle_pid, process_h.handle)) && handle_pid == pid_desc.pid) {
        if (this->has_initialized) {
            svcCloseHandle(this->process_handle);
        }
        this->process_handle = process_h.handle;
        this->process_id = handle_pid;
        this->has_initialized = true;
        return 0;
    }
    return 0xAE09;
}
