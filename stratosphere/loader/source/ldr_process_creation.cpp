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

static inline bool IsDisallowedVersion810(const u64 title_id, const u32 version) {
    return version == 0 &&
    (title_id == TitleId_Settings ||
     title_id == TitleId_Bus ||
     title_id == TitleId_Audio ||
     title_id == TitleId_NvServices ||
     title_id == TitleId_Ns ||
     title_id == TitleId_Ssl ||
     title_id == TitleId_Es ||
     title_id == TitleId_Creport ||
     title_id == TitleId_Ro);
}

Result ProcessCreation::ValidateProcessVersion(u64 title_id, u32 version) {
    if (GetRuntimeFirmwareVersion() < FirmwareVersion_810) {
        return ResultSuccess;
    } else {
#ifdef LDR_VALIDATE_PROCESS_VERSION
        if (IsDisallowedVersion810(title_id, version)) {
            return ResultLoaderInvalidVersion;
        } else {
            return ResultSuccess;
        }
#else
        return ResultSuccess;
#endif
    }
}

Result ProcessCreation::InitializeProcessInfo(NpdmUtils::NpdmInfo *npdm, Handle reslimit_h, u64 arg_flags, ProcessInfo *out_proc_info) {
    /* Initialize a ProcessInfo using an npdm. */
    *out_proc_info = {};

    /* Copy all but last char of name, insert NULL terminator. */
    std::copy(npdm->header->title_name, npdm->header->title_name + sizeof(out_proc_info->name) - 1, out_proc_info->name);
    out_proc_info->name[sizeof(out_proc_info->name) - 1] = 0;

    /* Set title id. */
    out_proc_info->title_id = npdm->aci0->title_id;

    /* Set version. */
    out_proc_info->version = npdm->header->version;

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
    if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_300)) {
        if (npdm->header->system_resource_size & 0x1FFFFF) {
            return ResultLoaderInvalidSize;
        }
        if (npdm->header->system_resource_size) {
            if ((out_proc_info->process_flags & 6) == 0) {
                return ResultLoaderInvalidMeta;
            }
            if (!(((application_type & 3) == 1) || ((GetRuntimeFirmwareVersion() >= FirmwareVersion_600) && (application_type & 3) == 2))) {
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
    if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_500)) {
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

    /* Get the process from the registration queue. */
    target_process = Registration::GetProcess(index);
    if (target_process == nullptr) {
        return ResultLoaderProcessNotRegistered;
    }

    /* Mount the title's exefs. */
    bool mounted_code = false;
    if (target_process->tid_sid.storage_id != FsStorageId_None) {
        R_TRY(ContentManagement::MountCodeForTidSid(&target_process->tid_sid));
        mounted_code = true;
    } else {
        if (R_SUCCEEDED(ContentManagement::MountCodeNspOnSd(target_process->tid_sid.title_id))) {
            mounted_code = true;
        }
    }
    ON_SCOPE_EXIT {
        if (mounted_code) {
            const Result unmount_res = ContentManagement::UnmountCode();
            if (target_process->tid_sid.storage_id != FsStorageId_None) {
                R_ASSERT(unmount_res);
            }
        }
    };

    /* Load the process's NPDM. */
    R_TRY(NpdmUtils::LoadNpdmFromCache(target_process->tid_sid.title_id, &npdm_info));

    /* Validate version. */
    R_TRY(ValidateProcessVersion(target_process->tid_sid.title_id, npdm_info.header->version));

    /* Validate the title we're loading is what we expect. */
    if (npdm_info.aci0->title_id < npdm_info.acid->title_id_range_min || npdm_info.aci0->title_id > npdm_info.acid->title_id_range_max) {
        return ResultLoaderInvalidProgramId;
    }

    /* Validate that the ACI0 Kernel Capabilities are valid and restricted by the ACID Kernel Capabilities. */
    const u32 *acid_caps = reinterpret_cast<u32 *>(npdm_info.acid_kac);
    const u32 *aci0_caps = reinterpret_cast<u32 *>(npdm_info.aci0_kac);
    const size_t num_acid_caps = npdm_info.acid->kac_size / sizeof(*acid_caps);
    const size_t num_aci0_caps = npdm_info.aci0->kac_size / sizeof(*aci0_caps);
    R_TRY(NpdmUtils::ValidateCapabilities(acid_caps, num_acid_caps, aci0_caps, num_aci0_caps));

    /* Read in all NSO headers, see what NSOs are present. */
    R_TRY(NsoUtils::LoadNsoHeaders(npdm_info.aci0->title_id));

    /* Validate that the set of NSOs to be loaded is correct. */
    R_TRY(NsoUtils::ValidateNsoLoadSet());

    /* Initialize the ProcessInfo. */
    R_TRY(ProcessCreation::InitializeProcessInfo(&npdm_info, reslimit_h, arg_flags, &process_info));

    /* Figure out where NSOs will be mapped, and how much space they (and arguments) will take up. */
    R_TRY(NsoUtils::CalculateNsoLoadExtents(process_info.process_flags, launch_item != nullptr ? launch_item->arg_size : 0, &nso_extents));

    /* Set Address Space information in ProcessInfo. */
    process_info.code_addr = nso_extents.base_address;
    process_info.code_num_pages = nso_extents.total_size + 0xFFF;
    process_info.code_num_pages >>= 12;

    /* Call svcCreateProcess(). */
    R_TRY(svcCreateProcess(&process_h, &process_info, (u32 *)npdm_info.aci0_kac, npdm_info.aci0->kac_size/sizeof(u32)));
    auto proc_handle_guard = SCOPE_GUARD {
        svcCloseHandle(process_h);
    };


    /* Load all NSOs into Process memory, and set permissions accordingly. */
    {
        const u8 *launch_args = nullptr;
        size_t launch_args_size = 0;
        if (launch_item != nullptr) {
            launch_args = reinterpret_cast<const u8 *>(launch_item->args);
            launch_args_size = launch_item->arg_size;
        }

        R_TRY(NsoUtils::LoadNsosIntoProcessMemory(process_h, npdm_info.aci0->title_id, &nso_extents, launch_args, launch_args_size));
    }

    /* Update the list of registered processes with the new process. */
    svcGetProcessId(&process_id, process_h);
    bool is_64_bit_addspace;
    if ((GetRuntimeFirmwareVersion() >= FirmwareVersion_200)) {
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

    /* If HBL, override HTML document path. */
    if (ContentManagement::ShouldOverrideContentsWithHBL(target_process->tid_sid.title_id)) {
        ContentManagement::RedirectHtmlDocumentPathForHbl(target_process->tid_sid.title_id, target_process->tid_sid.storage_id);
    }

    /* ECS is a one-shot operation, but we don't clear on failure. */
    ContentManagement::ClearExternalContentSource(target_process->tid_sid.title_id);

    /* Cancel the process handle guard. */
    proc_handle_guard.Cancel();

    /* Write process handle to output. */
    *out_process_h = process_h;
    return ResultSuccess;
}
