#include <switch.h>

#include "ldr_process_creation.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"
#include "ldr_nso.hpp"

Result ProcessCreation::CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 flags, Handle reslimit_h) {
    NpdmUtils::NpdmInfo info;
    Registration::Process *target_process;
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
    rc = NpdmUtils::LoadNpdmFromCache(target_process->tid_sid.title_id, &info);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Validate the title we're loading is what we expect. */
    if (info.aci0->title_id < info.acid->title_id_range_min || info.aci0->title_id > info.acid->title_id_range_max) {
        rc = 0x1209;
        goto CREATE_PROCESS_END;
    }
    
    /* Validate that the ACI0 Kernel Capabilities are valid and restricted by the ACID Kernel Capabilities. */
    rc = NpdmUtils::ValidateCapabilities((u32 *)info.acid_kac, info.acid->kac_size/sizeof(u32), (u32 *)info.aci0_kac, info.aci0->kac_size/sizeof(u32));
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Read in all NSO headers, see what NSOs are present. */
    rc = NsoUtils::LoadNsoHeaders(info.aci0->title_id);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* Validate that the set of NSOs to be loaded is correct. */
    rc = NsoUtils::ValidateNsoLoadSet();
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    /* TODO: Create the CreateProcessInfo. */
    
    /* TODO: Figure out where NSOs will be mapped, and how much space they (and arguments) will take up. */
    
    /* TODO: Call svcCreateProcessInfo(). */
    
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
