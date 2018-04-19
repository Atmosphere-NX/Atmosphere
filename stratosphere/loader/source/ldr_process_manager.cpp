#include <switch.h>
#include "ldr_process_manager.hpp"
#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"

Result ProcessManagerService::dispatch(IpcParsedCommand *r, IpcCommand *out_c, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count) {
    
    Result rc = 0xF601;
        
    switch ((ProcessManagerServiceCmd)cmd_id) {
        case Pm_Cmd_CreateProcess:
            /* TODO */          
            break;
        case Pm_Cmd_GetProgramInfo:
            /* TODO */          
            break;
        case Pm_Cmd_RegisterTitle:
            /* Validate arguments. */
            if (in_rawdata_size < 0x10 || r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 0) {
                break;
            }
            
            u64 out_index;
            rc = register_title((Registration::TidSid *)in_rawdata, &out_index);
            if (R_SUCCEEDED(rc)) {
                ((u64 *)out_rawdata)[0] = out_index;
                *out_raw_data_count = 8;
            } else {
                ((u64 *)out_rawdata)[0] = 0;
                *out_raw_data_count = 0;
            }
            
            break;
        case Pm_Cmd_UnregisterTitle:
            /* Validate arguments. */
            if (in_rawdata_size < 0x8 || r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 0) {
                break;
            }
            
            rc = unregister_title(((u64 *)in_rawdata)[0]);
            *out_raw_data_count = 0;
            
            break;
        default:
            break;
    }
    return rc;
}

Result ProcessManagerService::create_process() {
    /* TODO */
    return 0xF601;
}

Result ProcessManagerService::get_program_info() {
    /* TODO */
    return 0xF601;
}

Result ProcessManagerService::register_title(const Registration::TidSid *tid_sid, u64 *out_index) {
    if (Registration::register_tid_sid(tid_sid, out_index)) {
        return 0;
    } else {
        return 0xE09;
    }
}

Result ProcessManagerService::unregister_title(u64 index) {
    if (Registration::unregister_index(index)) {
        return 0;
    } else {
        return 0x1009;
    }
}