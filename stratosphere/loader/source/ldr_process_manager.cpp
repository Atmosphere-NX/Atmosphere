#include <switch.h>
#include <stratosphere.hpp>
#include "ldr_process_manager.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"

Result ProcessManagerService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
                
    switch ((ProcessManagerServiceCmd)cmd_id) {
        case Pm_Cmd_CreateProcess:
            rc = WrapIpcCommandImpl<&ProcessManagerService::create_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_GetProgramInfo:
            rc = WrapIpcCommandImpl<&ProcessManagerService::get_program_info>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_RegisterTitle:            
            rc = WrapIpcCommandImpl<&ProcessManagerService::register_title>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Pm_Cmd_UnregisterTitle:
            rc = WrapIpcCommandImpl<&ProcessManagerService::unregister_title>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

std::tuple<Result, MovedHandle> ProcessManagerService::create_process(u64 flags, u64 index, CopiedHandle reslimit_h) {
    Result rc;
    Registration::TidSid tid_sid;
    LaunchQueue::LaunchItem *launch_item;
    char nca_path[FS_MAX_PATH] = {0};
    Handle process_h = 0;
    
    fprintf(stderr, "CreateProcess(%016lx, %016lx, %08x);\n", flags, index, reslimit_h.handle);
    
    rc = Registration::GetRegisteredTidSid(index, &tid_sid);
    if (R_FAILED(rc)) {
        return {rc, MovedHandle{process_h}};
    }
    
    if (tid_sid.storage_id != FsStorageId_None) {
        rc = ContentManagement::ResolveContentPathForTidSid(nca_path, &tid_sid);
        if (R_FAILED(rc)) {
            return {rc, MovedHandle{process_h}};
        }
    }

    
    launch_item = LaunchQueue::get_item(tid_sid.title_id);
    
    rc = ProcessCreation::CreateProcess(&process_h, index, nca_path, launch_item, flags, reslimit_h.handle);
    
    if (R_SUCCEEDED(rc)) {
        ContentManagement::SetCreatedTitle(tid_sid.title_id);
    }
    
    return {rc, MovedHandle{process_h}};
}

std::tuple<Result> ProcessManagerService::get_program_info(Registration::TidSid tid_sid, OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info) {
    Result rc;
    char nca_path[FS_MAX_PATH] = {0};
    /* Zero output. */
    std::fill(out_program_info.pointer, out_program_info.pointer + out_program_info.num_elements, (const ProcessManagerService::ProgramInfo){0});
    
    rc = populate_program_info_buffer(out_program_info.pointer, &tid_sid);
    
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
        
        rc = LaunchQueue::add_copy(tid_sid.title_id, out_program_info.pointer->title_id);
    }
        
    return {rc};
}

std::tuple<Result, u64> ProcessManagerService::register_title(Registration::TidSid tid_sid) {
    u64 out_index = 0;
    if (Registration::RegisterTidSid(&tid_sid, &out_index)) {
        return {0, out_index};
    } else {
        return {0xE09, out_index};
    }
}

std::tuple<Result> ProcessManagerService::unregister_title(u64 index) {
    if (Registration::UnregisterIndex(index)) {
        return {0};
    } else {
        return {0x1009};
    }
}


Result ProcessManagerService::populate_program_info_buffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid) {
    NpdmUtils::NpdmInfo info;
    Result rc;
    
    if (tid_sid->storage_id != FsStorageId_None) {
        rc = ContentManagement::MountCodeForTidSid(tid_sid);  
        if (R_FAILED(rc)) {
            return rc;
        }
    }
    
    rc = NpdmUtils::LoadNpdm(tid_sid->title_id, &info);
    
    if (tid_sid->storage_id != FsStorageId_None) {
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
    rc = 0x19009;
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
                    rc = 0;
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
