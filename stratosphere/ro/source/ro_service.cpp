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

#include "ro_service.hpp"
#include "ro_registration.hpp"

RelocatableObjectsService::~RelocatableObjectsService() {
    if (this->IsInitialized()) {
        Registration::UnregisterProcess(this->context);
        this->context = nullptr;
    }
}

bool RelocatableObjectsService::IsProcessIdValid(u64 process_id) {
    if (!this->IsInitialized()) {
        return false;
    }
    
    return this->context->process_id == process_id;
}

u64 RelocatableObjectsService::GetTitleId(Handle process_handle) {
    u64 title_id = 0;
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_300) {
        /* 3.0.0+: Use svcGetInfo. */
        if (R_FAILED(svcGetInfo(&title_id, 18, process_handle, 0))) {
            std::abort();
        }
    } else {
        /* 1.0.0-2.3.0: We're not inside loader, so ask pm. */
        u64 process_id = 0;
        if (R_FAILED(svcGetProcessId(&process_id, process_handle))) {
            std::abort();
        }
        if (R_FAILED(pminfoGetTitleId(&title_id, process_id))) {
            std::abort();
        }
    }
    return title_id;
}

Result RelocatableObjectsService::LoadNro(Out<u64> load_address, PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    if (!this->IsProcessIdValid(pid_desc.pid)) {
        return ResultRoInvalidProcess;
    }
    
    return Registration::LoadNro(load_address.GetPointer(), this->context, nro_address, nro_size, bss_address, bss_size);
}

Result RelocatableObjectsService::UnloadNro(PidDescriptor pid_desc, u64 nro_address) {
    if (!this->IsProcessIdValid(pid_desc.pid)) {
        return ResultRoInvalidProcess;
    }
    
    return Registration::UnloadNro(this->context, nro_address);
}

Result RelocatableObjectsService::LoadNrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size) {
    if (!this->IsProcessIdValid(pid_desc.pid)) {
        return ResultRoInvalidProcess;
    }

    return Registration::LoadNrr(this->context, GetTitleId(this->context->process_handle), nrr_address, nrr_size, RoModuleType_ForSelf, true);
}

Result RelocatableObjectsService::UnloadNrr(PidDescriptor pid_desc, u64 nrr_address) {
    if (!this->IsProcessIdValid(pid_desc.pid)) {
        return ResultRoInvalidProcess;
    }

    return Registration::UnloadNrr(this->context, nrr_address);
}

Result RelocatableObjectsService::Initialize(PidDescriptor pid_desc, CopiedHandle process_h) {
    /* Validate the input pid/process handle. */
    u64 handle_pid = 0;
    if (R_FAILED(svcGetProcessId(&handle_pid, process_h.handle)) || handle_pid != pid_desc.pid) {
        return ResultRoInvalidProcess;
    }

    return Registration::RegisterProcess(&this->context, process_h.handle, pid_desc.pid);
}

Result RelocatableObjectsService::LoadNrrEx(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size, CopiedHandle process_h) {
    if (!this->IsProcessIdValid(pid_desc.pid)) {
        return ResultRoInvalidProcess;
    }

    return Registration::LoadNrr(this->context, GetTitleId(process_h.handle), nrr_address, nrr_size, this->type, this->type == RoModuleType_ForOthers);
}