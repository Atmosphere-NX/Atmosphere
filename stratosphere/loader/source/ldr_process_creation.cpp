#include <switch.h>

#include "ldr_process_creation.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_content_management.hpp"
#include "ldr_npdm.hpp"

Result ProcessCreation::CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 flags, Handle reslimit_h) {
    NpdmUtils::NpdmInfo info;
    Registration::Process *target_process;
    Result rc;
    
    target_process = Registration::get_process(index);
    if (target_process == NULL) {
        return 0x1009;
    }
    
    rc = ContentManagement::MountCodeForTidSid(&target_process->tid_sid);  
    if (R_FAILED(rc)) {
        return rc;
    }
    
    rc = NpdmUtils::LoadNpdmFromCache(target_process->tid_sid.title_id, &info);
    if (R_FAILED(rc)) {
        goto CREATE_PROCESS_END;
    }
    
    if (info.aci0->title_id < info.acid->title_id_range_min || info.aci0->title_id > info.acid->title_id_range_max) {
        rc = 0x1209;
        goto CREATE_PROCESS_END;
    }
    
    /* TODO: Parse and verify ACI0 kernel caps vs ACID kernel caps. */
    
    /* TODO: Read in all NSO headers, see what NSOs are present. */
    
    /* TODO: Validate that the set of NSOs to be loaded is correct. */
    
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
