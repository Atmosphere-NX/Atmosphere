#include <switch.h>
#include <cstdio>
#include <algorithm>

#include "ldr_ro_service.hpp"
#include "ldr_registration.hpp"
#include "ldr_map.hpp"
#include "ldr_nro.hpp"

Result RelocatableObjectsService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((RoServiceCmd)cmd_id) {
        case Ro_Cmd_LoadNro:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::load_nro>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_UnloadNro:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::unload_nro>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_LoadNrr:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::load_nrr>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_UnloadNrr:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::unload_nrr>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Ro_Cmd_Initialize:
            rc = WrapIpcCommandImpl<&RelocatableObjectsService::initialize>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}


std::tuple<Result, u64> RelocatableObjectsService::load_nro(PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    Result rc;
    u64 out_address = 0;
    Registration::Process *target_proc = NULL;
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        rc = 0xAE09;
        goto LOAD_NRO_END;
    }
    if (nro_address & 0xFFF) {
        rc = 0xA209;
        goto LOAD_NRO_END;
    }
    if (nro_address + nro_size <= nro_address || !nro_size || (nro_size & 0xFFF)) {
        rc = 0xA409;
        goto LOAD_NRO_END;
    }
    if (bss_size && bss_address + bss_size <= bss_address) {
        rc = 0xA409;
        goto LOAD_NRO_END;
    }
    /* Ensure no overflow for combined sizes. */
    if (U64_MAX - nro_size < bss_size) {
        rc = 0xA409;
        goto LOAD_NRO_END;
    }
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        rc = 0xAC09;
        goto LOAD_NRO_END;
    }
    target_proc->owner_ro_service = this;
    
    rc = NroUtils::LoadNro(target_proc, this->process_handle, nro_address, nro_size, bss_address, bss_size, &out_address);
LOAD_NRO_END:
    return std::make_tuple(rc, out_address);
}

std::tuple<Result> RelocatableObjectsService::unload_nro(PidDescriptor pid_desc, u64 nro_address) {
    /* TODO */
    return std::make_tuple(0xF601);
}

std::tuple<Result> RelocatableObjectsService::load_nrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size) {
    Result rc;
    Registration::Process *target_proc = NULL;
    MappedCodeMemory nrr_info = {0};
    if (!this->has_initialized || this->process_id != pid_desc.pid) {
        rc = 0xAE09;
        goto LOAD_NRR_END;
    }
    if (nrr_address & 0xFFF) {
        rc = 0xA209;
        goto LOAD_NRR_END;
    }
    if (nrr_address + nrr_size <= nrr_address || !nrr_size || (nrr_size & 0xFFF)) {
        rc = 0xA409;
        goto LOAD_NRR_END;
    }
    
    target_proc = Registration::GetProcessByProcessId(pid_desc.pid);
    if (target_proc == NULL || (target_proc->owner_ro_service != NULL && (RelocatableObjectsService *)(target_proc->owner_ro_service) != this)) {
        rc = 0xAC09;
        goto LOAD_NRR_END;
    }
    target_proc->owner_ro_service = this;
    
    if (R_FAILED((rc = nrr_info.Open(this->process_handle, target_proc->is_64_bit_addspace, nrr_address, nrr_size)))) {
        goto LOAD_NRR_END;
    }
    
    if (R_FAILED((rc = nrr_info.Map()))) {
        goto LOAD_NRR_END;
    }
    
    rc = NroUtils::ValidateNrrHeader((NroUtils::NrrHeader *)nrr_info.mapped_address, nrr_size, target_proc->title_id_min);
    if (R_SUCCEEDED(rc)) {
        Registration::AddNrrInfo(target_proc->index, &nrr_info);
    }
    
LOAD_NRR_END:
    if (R_FAILED(rc)) {
        if (nrr_info.IsActive()) {
            nrr_info.Close();
        }
    }
    return std::make_tuple(rc);
}

std::tuple<Result> RelocatableObjectsService::unload_nrr(PidDescriptor pid_desc, u64 nrr_address) {
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

std::tuple<Result> RelocatableObjectsService::initialize(PidDescriptor pid_desc, CopiedHandle process_h) {
    u64 handle_pid;
    Result rc = 0xAE09;
    if (R_SUCCEEDED(svcGetProcessId(&handle_pid, process_h.handle)) && handle_pid == pid_desc.pid) {
        if (this->has_initialized) {
            svcCloseHandle(this->process_handle);
        }
        this->process_handle = process_h.handle;
        this->process_id = handle_pid;
        this->has_initialized = true;
        rc = 0;
    }
    return std::make_tuple(rc);
}
