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
#include <stratosphere.hpp>

#include "ldr_process_creation.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"
#include "ldr_nso.hpp"

Result ProcessCreation::InitializeProcessInfo(NpdmUtils::NpdmInfo *npdm, Handle reslimit_h, u64 arg_flags, ProcessInfo *out_proc_info) {
    /* Initialize a ProcessInfo using an npdm. */
    *out_proc_info = {};
    
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
        return ResultLoaderInvalidMeta;
    }
    out_proc_info->process_flags = (npdm->header->mmu_flags & 0xF);
    
    /* Set Bit 4 (?) and EnableAslr based on argument flags. */
    out_proc_info->process_flags |= ((arg_flags & 3) << 4) ^ 0x20;
    /* Set UseSystemMemBlocks if application type is 1. */
    u32 application_type = NpdmUtils::GetApplicationType((u32 *)npdm->aci0_kac, npdm->aci0->kac_size / sizeof(u32));
    if ((application_type & 3) == 1) {
        out_proc_info->process_flags |= 0x40;
        /* 7.0.0+: Set unknown bit related to system resource heap if relevant. */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_700) {
            if ((npdm->header->mmu_flags & 0x10)) {
                out_proc_info->process_flags |= 0x800;
            }
        }
    }
    
    /* 3.0.0+ System Resource Size. */
    if (kernelAbove300()) {
        if (npdm->header->system_resource_size & 0x1FFFFF) {
            return ResultLoaderInvalidSize;
        }
        if (npdm->header->system_resource_size) {
            if ((out_proc_info->process_flags & 6) == 0) {
                return ResultLoaderInvalidMeta;
            }
            if (!(((application_type & 3) == 1) || (kernelAbove600() && (application_type & 3) == 2))) {
                return ResultLoaderInvalidMeta;
            }
            if (npdm->header->system_resource_size > 0x1FE00000) {
                return ResultLoaderInvalidMeta;
            }
        }
        out_proc_info->system_resource_num_pages = npdm->header->system_resource_size >> 12;
    } else {
        out_proc_info->system_resource_num_pages = 0;
    }
    
    /* 5.0.0+ Pool Partition. */
    if (kernelAbove500()) {
        u32 pool_partition_id = (npdm->acid->flags >> 2) & 0xF;
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
                return ResultLoaderInvalidMeta;
        }
    }
    
    return ResultSuccess;
}

Result ProcessCreation::CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 arg_flags, Handle reslimit_h) {
    NpdmUtils::NpdmInfo npdm_info = {};
    ProcessInfo process_info = {};
    NsoUtils::NsoLoadExtents nso_extents = {};
    Registration::Process *target_process;
    Handle process_h = 0;
    u64 process_id = 0;
    bool mounted_code = false;
    Result rc;
    
    /* Get the process from the registration queue. */
    target_process = Registration::GetProcess(index);
    if (target_process == NULL) {
        return ResultLoaderProcessNotRegistered;
    }
    
    /* Mount the title's exefs. */
    if (target_process->tid_sid.storage_id != FsStorageId_None) {
        rc = ContentManagement::MountCodeForTidSid(&target_process->tid_sid);  
        if (R_FAILED(rc)) {
            return rc;
        }
        mounted_code = true;
    } else {
        if (R_SUCCEEDED(ContentManagement::MountCodeNspOnSd(target_process->tid_sid.title_id))) {
            mounted_code = true;
        }
    }
    
    /* Load the process's NPDM. */
    rc = NpdmUtils::LoadNpdmFromCache(target_process->tid_sid.title_id, &npdm_info);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Validate the title we're loading is what we expect. */
    if (npdm_info.aci0->title_id < npdm_info.acid->title_id_range_min || npdm_info.aci0->title_id > npdm_info.acid->title_id_range_max) {
        rc = ResultLoaderInvalidProgramId;
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
    rc = NsoUtils::CalculateNsoLoadExtents(process_info.process_flags, launch_item != NULL ? launch_item->arg_size : 0, &nso_extents);
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
    
    
    /* Load all NSOs into Process memory, and set permissions accordingly. */
    if (launch_item != NULL) {
        rc = NsoUtils::LoadNsosIntoProcessMemory(process_h, npdm_info.aci0->title_id, &nso_extents, (u8 *)launch_item->args, launch_item->arg_size); 
    } else {
        rc = NsoUtils::LoadNsosIntoProcessMemory(process_h, npdm_info.aci0->title_id, &nso_extents, NULL, 0);    
    }
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Update the list of registered processes with the new process. */
    svcGetProcessId(&process_id, process_h);
    bool is_64_bit_addspace;
    if (kernelAbove200()) {
        is_64_bit_addspace = (((npdm_info.header->mmu_flags >> 1) & 5) | 2) == 3;
    } else {
        is_64_bit_addspace = (npdm_info.header->mmu_flags & 0xE) == 0x2;
    }
    Registration::SetProcessIdTidAndIs64BitAddressSpace(index, process_id, npdm_info.aci0->title_id, is_64_bit_addspace);
    for (unsigned int i = 0; i < NSO_NUM_MAX; i++) {
        if (NsoUtils::IsNsoPresent(i)) {   
            Registration::AddModuleInfo(index, nso_extents.nso_addresses[i], nso_extents.nso_sizes[i], NsoUtils::GetNsoBuildId(i));
        }
    }
    
    /* Send the pid/tid pair to anyone interested in man-in-the-middle-attacking it. */
    Registration::AssociatePidTidForMitM(index);
    
    rc = ResultSuccess;

    /* If HBL, override HTML document path. */
    if (ContentManagement::ShouldOverrideContentsWithHBL(target_process->tid_sid.title_id)) {
        ContentManagement::RedirectHtmlDocumentPathForHbl(target_process->tid_sid.title_id, target_process->tid_sid.storage_id);
    }

    /* ECS is a one-shot operation, but we don't clear on failure. */
    ContentManagement::ClearExternalContentSource(target_process->tid_sid.title_id);


CREATE_PROCESS_END:
    if (mounted_code) {
        if (R_SUCCEEDED(rc) && target_process->tid_sid.storage_id != FsStorageId_None) {
            rc = ContentManagement::UnmountCode();
        } else {
            ContentManagement::UnmountCode();
        }
    }
    
    if (R_SUCCEEDED(rc)) {
        *out_process_h = process_h;
    } else {
        svcCloseHandle(process_h);
    }
    return rc;
}
