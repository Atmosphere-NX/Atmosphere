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
    Registration::TidSid tid_sid;
    LaunchQueue::LaunchItem *launch_item;
    char nca_path[FS_MAX_PATH] = {0};

    ON_SCOPE_EXIT {
        /* Loader doesn't persist the copied resource limit handle. */
        svcCloseHandle(reslimit_h.handle);
    };

    R_TRY(Registration::GetRegisteredTidSid(index, &tid_sid));

    if (tid_sid.storage_id != FsStorageId_None) {
        R_TRY(ContentManagement::ResolveContentPathForTidSid(nca_path, &tid_sid));
    }

    launch_item = LaunchQueue::GetItem(tid_sid.title_id);
    R_TRY(ProcessCreation::CreateProcess(proc_h.GetHandlePointer(), index, nca_path, launch_item, flags, reslimit_h.handle));

    ContentManagement::SetCreatedTitle(tid_sid.title_id);
    return ResultSuccess;
}

Result ProcessManagerService::GetProgramInfo(OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info, Registration::TidSid tid_sid) {
    char nca_path[FS_MAX_PATH] = {0};

    /* Zero output. */
    std::fill(out_program_info.pointer, out_program_info.pointer + out_program_info.num_elements, ProcessManagerService::ProgramInfo{});

    R_TRY(PopulateProgramInfoBuffer(out_program_info.pointer, &tid_sid));

    if (tid_sid.storage_id != FsStorageId_None && tid_sid.title_id != out_program_info.pointer->title_id) {
        R_TRY(ContentManagement::ResolveContentPathForTidSid(nca_path, &tid_sid));
        R_TRY(ContentManagement::RedirectContentPath(nca_path, out_program_info.pointer->title_id, tid_sid.storage_id));
        R_TRY(LaunchQueue::AddCopy(tid_sid.title_id, out_program_info.pointer->title_id));
    }

    return ResultSuccess;
}

Result ProcessManagerService::RegisterTitle(Out<u64> index, Registration::TidSid tid_sid) {
    return Registration::RegisterTidSid(&tid_sid, index.GetPointer()) ? 0 : ResultLoaderTooManyProcesses;
}

Result ProcessManagerService::UnregisterTitle(u64 index) {
    return Registration::UnregisterIndex(index) ? 0 : ResultLoaderProcessNotRegistered;
}


Result ProcessManagerService::PopulateProgramInfoBuffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid) {
    NpdmUtils::NpdmInfo info;

    /* Mount code, load NPDM. */
    {
        bool mounted_code = false;
        if (tid_sid->storage_id != FsStorageId_None) {
            R_TRY(ContentManagement::MountCodeForTidSid(tid_sid));
            mounted_code = true;
        } else if (R_SUCCEEDED(ContentManagement::MountCodeNspOnSd(tid_sid->title_id))) {
            mounted_code = true;
        }
        ON_SCOPE_EXIT {
            if (mounted_code) {
                ContentManagement::UnmountCode();
            }
        };

        R_TRY(NpdmUtils::LoadNpdm(tid_sid->title_id, &info));
    }

    out->main_thread_priority = info.header->main_thread_prio;
    out->default_cpu_id = info.header->default_cpuid;
    out->main_thread_stack_size = info.header->main_stack_size;
    out->title_id = info.aci0->title_id;

    out->acid_fac_size = info.acid->fac_size;
    out->aci0_sac_size = info.aci0->sac_size;
    out->aci0_fah_size = info.aci0->fah_size;

    size_t offset = 0;

    /* Copy ACID Service Access Control. */
    if (offset + info.acid->sac_size >= sizeof(out->ac_buffer)) {
        return ResultLoaderInternalError;
    }
    out->acid_sac_size = info.acid->sac_size;
    std::memcpy(out->ac_buffer + offset, info.acid_sac, out->acid_sac_size);
    offset += out->acid_sac_size;

    /* Copy ACI0 Service Access Control. */
    if (offset + info.aci0->sac_size >= sizeof(out->ac_buffer)) {
        return ResultLoaderInternalError;
    }
    out->aci0_sac_size = info.aci0->sac_size;
    std::memcpy(out->ac_buffer + offset, info.aci0_sac, out->aci0_sac_size);
    offset += out->aci0_sac_size;

    /* Copy ACID Filesystem Access Control. */
    if (offset + info.acid->fac_size >= sizeof(out->ac_buffer)) {
        return ResultLoaderInternalError;
    }
    out->acid_fac_size = info.acid->fac_size;
    std::memcpy(out->ac_buffer + offset, info.acid_fac, out->acid_fac_size);
    offset += out->acid_fac_size;

    /* Copy ACI0 Filesystem Access Header. */
    if (offset + info.aci0->fah_size >= sizeof(out->ac_buffer)) {
        return ResultLoaderInternalError;
    }
    out->aci0_fah_size = info.aci0->fah_size;
    std::memcpy(out->ac_buffer + offset, info.aci0_fah, out->aci0_fah_size);
    offset += out->aci0_fah_size;

    /* Parse application type. */
    out->application_type = NpdmUtils::GetApplicationType(reinterpret_cast<const u32 *>(info.acid_kac), info.acid->kac_size / sizeof(u32));

    return ResultSuccess;
}
