#include <switch.h>
#include <algorithm>

#include "ldr_process_creation.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"
#include "ldr_nso.hpp"

Result ProcessCreation::InitializeProcessInfo(NpdmUtils::NpdmInfo *npdm, Handle reslimit_h, u64 arg_flags, ProcessInfo *out_proc_info) {
    /* Initialize a ProcessInfo using an npdm. */
    *out_proc_info = (const ProcessCreation::ProcessInfo){0};
    
    /* Copy all but last char of name, insert NULL terminator. */
    std::copy(npdm->header->title_name, npdm->header->title_name + sizeof(out_proc_info->name) - 1, out_proc_info->name);
    out_proc_info->name[sizeof(out_proc_info->name) - 1] = 0;
    
    /* Set title id. */
    out_proc_info->title_id = npdm->aci0->title_id;
    
    /* Set process category. */
    out_proc_info->process_category = npdm->header->process_category;
    
    /* Copy reslimit handle raw. */
    out_proc_info->reslimit_h = reslimit_h;
    
    /* Set IsAddressSpace64Bit, AddressSpaceType. */
    if (npdm->header->mmu_flags & 8) {
        /* Invalid Address Space Type. */
        return 0x809;
    }
    out_proc_info->process_flags = (npdm->header->mmu_flags & 0xF);
    /* Set Bit 4 (?) and EnableAslr based on argument flags. */
    out_proc_info->process_flags |= ((arg_flags & 3) << 4) ^ 0x20;
    /* Set UseSystemMemBlocks if application type is 1. */
    u32 application_type = NpdmUtils::GetApplicationType((u32 *)npdm->aci0_kac, npdm->aci0->kac_size / sizeof(u32));
    if ((application_type & 3) == 1) {
        out_proc_info->process_flags |= 0x40;
    }
    
    /* 3.0.0+ System Resource Size. */
    if (kernelAbove300()) {
        if (npdm->header->system_resource_size & 0x1FFFFF) {
            return 0xA409;
        }
        if (npdm->header->system_resource_size) {
            if ((out_proc_info->process_flags & 6) == 0) {
                return 0x809;
            }
            if ((application_type & 3) != 1) {
                return 0x809;
            }
            if (npdm->header->system_resource_size > 0x1FE00000) {
                return 0x809;
            }
        }
        out_proc_info->system_resource_num_pages = npdm->header->system_resource_size >> 12;
    } else {
        out_proc_info->system_resource_num_pages = 0;
    }
    
    /* 5.0.0+ Pool Partition. */
    if (kernelAbove500()) {
        u32 pool_partition_id = (npdm->acid->is_retail >> 2) & 0xF;
        switch (pool_partition_id) {
            case 0: /* Application. */
                if ((application_type & 3) == 2) {
                    out_proc_info->process_flags |= 0x80;
                }
                break;
            case 1: /* Applet. */
                out_proc_info->process_flags |= 0x80;
                break;
            case 2: /* Sysmodule. */
                out_proc_info->process_flags |= 0x100;
                break;
            case 3: /* nvservices. */
                out_proc_info->process_flags |= 0x180;
                break;
            default:
                return 0x809;
        }
    }
    
    return 0x0;
}

Result ProcessCreation::CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 arg_flags, Handle reslimit_h) {
    NpdmUtils::NpdmInfo npdm_info = {0};
    ProcessInfo process_info = {0};
    NsoUtils::NsoLoadExtents nso_extents = {0};
    Registration::Process *target_process;
    Handle process_h = 0;
    Result rc;
    
    /* Get the process from the registration queue. */
    target_process = Registration::get_process(index);
    if (target_process == NULL) {
        return 0x1009;
    }
    
    /* Mount the title's exefs. */
    rc = ContentManagement::MountCodeForTidSid(&target_process->tid_sid);  
    if (R_FAILED(rc)) {
        return rc;
    }
    
    /* Load the process's NPDM. */
    rc = NpdmUtils::LoadNpdmFromCache(target_process->tid_sid.title_id, &npdm_info);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Validate the title we're loading is what we expect. */
    if (npdm_info.aci0->title_id < npdm_info.acid->title_id_range_min || npdm_info.aci0->title_id > npdm_info.acid->title_id_range_max) {
        rc = 0x1209;
        goto CREATE_PROCESS_END;
    }
    
    /* Validate that the ACI0 Kernel Capabilities are valid and restricted by the ACID Kernel Capabilities. */
    rc = NpdmUtils::ValidateCapabilities((u32 *)npdm_info.acid_kac, npdm_info.acid->kac_size/sizeof(u32), (u32 *)npdm_info.aci0_kac, npdm_info.aci0->kac_size/sizeof(u32));
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Read in all NSO headers, see what NSOs are present. */
    rc = NsoUtils::LoadNsoHeaders(npdm_info.aci0->title_id);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Validate that the set of NSOs to be loaded is correct. */
    rc = NsoUtils::ValidateNsoLoadSet();
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Initialize the ProcessInfo. */
    rc = ProcessCreation::InitializeProcessInfo(&npdm_info, reslimit_h, arg_flags, &process_info);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Figure out where NSOs will be mapped, and how much space they (and arguments) will take up. */
    rc = NsoUtils::CalculateNsoLoadExtents(launch_item != NULL ? launch_item->arg_size : 0, process_info.process_flags, &nso_extents);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Set Address Space information in ProcessInfo. */
    process_info.code_addr = nso_extents.base_address;
    process_info.code_num_pages = nso_extents.total_size + 0xFFF;
    process_info.code_num_pages >>= 12;
    
    /* Call svcCreateProcess(). */
    rc = svcCreateProcess(&process_h, &process_info, (u32 *)npdm_info.aci0_kac, npdm_info.aci0->kac_size/sizeof(u32));
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* TODO: For each NSO, call svcMapProcessMemory, load the NSO into memory there (validating it), and then svcUnmapProcessMemory. */
    
    /* TODO: svcSetProcessMemoryPermission for each memory segment in the new process. */
    
    /* TODO: Map and load arguments to the process, if relevant. */
    
    /* TODO: Update the list of registered processes with the new process. */
    
    rc = 0;
CREATE_PROCESS_END:
    if (R_SUCCEEDED(rc)) {
        rc = ContentManagement::UnmountCode();
    } else {
        ContentManagement::UnmountCode();
    }
    return rc;
}
