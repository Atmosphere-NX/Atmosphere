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
#include <stratosphere.hpp>
#include "ldr_process_manager.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"

Result ProcessManagerService::CreateProcess(Out<MovedHandle> proc_h, u64 index, u32 flags, CopiedHandle reslimit_h) {
    Result rc;
    Registration::TidSid tid_sid;
    LaunchQueue::LaunchItem *launch_item;
    char nca_path[FS_MAX_PATH] = {0};

    ON_SCOPE_EXIT {
        /* Loader doesn't persist the copied resource limit handle. */
        svcCloseHandle(reslimit_h.handle);
    };
    
    rc = Registration::GetRegisteredTidSid(index, &tid_sid);
    if (R_FAILED(rc)) {
        return rc;
    }
    
    if (tid_sid.storage_id != FsStorageId_None) {
        rc = ContentManagement::ResolveContentPathForTidSid(nca_path, &tid_sid);
        if (R_FAILED(rc)) {
            return rc;
        }
    }

    
    launch_item = LaunchQueue::GetItem(tid_sid.title_id);
    
    rc = ProcessCreation::CreateProcess(proc_h.GetHandlePointer(), index, nca_path, launch_item, flags, reslimit_h.handle);
    
    if (R_SUCCEEDED(rc)) {
        ContentManagement::SetCreatedTitle(tid_sid.title_id);
    }
    
    return rc;
}

Result ProcessManagerService::GetProgramInfo(OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info, Registration::TidSid tid_sid) {
    Result rc;
    char nca_path[FS_MAX_PATH] = {0};
    /* Zero output. */
    std::fill(out_program_info.pointer, out_program_info.pointer + out_program_info.num_elements, ProcessManagerService::ProgramInfo{});
    
    rc = PopulateProgramInfoBuffer(out_program_info.pointer, &tid_sid);
    
    if (R_FAILED(rc)) {
        return {rc};
    }
    
    if (tid_sid.storage_id != FsStorageId_None && tid_sid.title_id != out_program_info.pointer->title_id) {
        rc = ContentManagement::ResolveContentPathForTidSid(nca_path, &tid_sid);
        if (R_FAILED(rc)) {
            return {rc};
        }
        
        rc = ContentManagement::RedirectContentPath(nca_path, out_program_info.pointer->title_id, tid_sid.storage_id);
        if (R_FAILED(rc)) {
            return {rc};
        }
        
        rc = LaunchQueue::AddCopy(tid_sid.title_id, out_program_info.pointer->title_id);
    }
        
    return {rc};
}

Result ProcessManagerService::RegisterTitle(Out<u64> index, Registration::TidSid tid_sid) {
    return Registration::RegisterTidSid(&tid_sid, index.GetPointer()) ? 0 : ResultLoaderTooManyProcesses;
}

Result ProcessManagerService::UnregisterTitle(u64 index) {
    return Registration::UnregisterIndex(index) ? 0 : ResultLoaderProcessNotRegistered;
}


Result ProcessManagerService::PopulateProgramInfoBuffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid) {
    NpdmUtils::NpdmInfo info;
    Result rc;
    bool mounted_code = false;
    
    if (tid_sid->storage_id != FsStorageId_None) {
        rc = ContentManagement::MountCodeForTidSid(tid_sid);  
        if (R_FAILED(rc)) {
            return rc;
        }
        mounted_code = true;
    } else if (R_SUCCEEDED(ContentManagement::MountCodeNspOnSd(tid_sid->title_id))) {
        mounted_code = true;
    }
    
    rc = NpdmUtils::LoadNpdm(tid_sid->title_id, &info);
    
    if (mounted_code) {
        ContentManagement::UnmountCode();
    }
    
    if (R_FAILED(rc)) {
        return rc;
    }

    
    out->main_thread_priority = info.header->main_thread_prio;
    out->default_cpu_id = info.header->default_cpuid;
    out->main_thread_stack_size = info.header->main_stack_size;
    out->title_id = info.aci0->title_id;
    
    out->acid_fac_size = info.acid->fac_size;
    out->aci0_sac_size = info.aci0->sac_size;
    out->aci0_fah_size = info.aci0->fah_size;
    
    size_t offset = 0;
    rc = ResultLoaderInternalError;
    if (offset + info.acid->sac_size < sizeof(out->ac_buffer)) {
        out->acid_sac_size = info.acid->sac_size;
        std::memcpy(out->ac_buffer + offset, info.acid_sac, out->acid_sac_size);
        offset += out->acid_sac_size;
        if (offset + info.aci0->sac_size < sizeof(out->ac_buffer)) {
            out->aci0_sac_size = info.aci0->sac_size;
            std::memcpy(out->ac_buffer + offset, info.aci0_sac, out->aci0_sac_size);
            offset += out->aci0_sac_size;
            if (offset + info.acid->fac_size < sizeof(out->ac_buffer)) {
                out->acid_fac_size = info.acid->fac_size;
                std::memcpy(out->ac_buffer + offset, info.acid_fac, out->acid_fac_size);
                offset += out->acid_fac_size;
                if (offset + info.aci0->fah_size < sizeof(out->ac_buffer)) {
                    out->aci0_fah_size = info.aci0->fah_size;
                    std::memcpy(out->ac_buffer + offset, info.aci0_fah, out->aci0_fah_size);
                    offset += out->aci0_fah_size;
                    rc = ResultSuccess;
                }
            }
        }
    }
    
    /* Parse application type. */
    if (R_SUCCEEDED(rc)) {
        out->application_type = NpdmUtils::GetApplicationType((u32 *)info.acid_kac, info.acid->kac_size / sizeof(u32));
    }
    
    
    return rc;
}
