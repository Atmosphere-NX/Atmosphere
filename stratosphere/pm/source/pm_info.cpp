#include <switch.h>
#include "pm_registration.hpp"
#include "pm_info.hpp"

Result InformationService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    
    switch ((InformationCmd)cmd_id) {
        case Information_Cmd_GetTitleId:
            rc = WrapIpcCommandImpl<&InformationService::get_title_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    
    return rc;
}

Result InformationService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

std::tuple<Result, u64> InformationService::get_title_id(u64 pid) {
    Registration::AutoProcessListLock auto_lock;
    
    Registration::Process *proc = Registration::GetProcess(pid);
    if (proc != NULL) {
        return {0x0, proc->tid_sid.title_id};
    } else {
        return {0x20F, 0x0};
    }
}
