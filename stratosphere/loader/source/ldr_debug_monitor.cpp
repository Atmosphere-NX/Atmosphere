#include <switch.h>
#include "ldr_debug_monitor.hpp"
#include "ldr_launch_queue.hpp"

Result DebugMonitorService::dispatch(IpcParsedCommand *r, IpcCommand *out_c, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count) {
    
    Result rc = 0xF601;
    
    /* TODO: Prepare SFCO. */
    
    switch ((DebugMonitorServiceCmd)cmd_id) {
        case Dmnt_Cmd_AddTitleToLaunchQueue:
            /* Validate arguments. */
            if (in_rawdata_size < 0x10 || r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 1) {
                break;
            }
            
            rc = add_title_to_launch_queue(((u64 *)in_rawdata)[0], (const char *)r->Statics[0], r->StaticSizes[0]);
            
            *out_raw_data_count = 0;
            
            break;
        case Dmnt_Cmd_ClearLaunchQueue:
            if (r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 0) {
                break;
            }
            
            rc = clear_launch_queue();
            *out_raw_data_count = 0;
            
            break;
        case Dmnt_Cmd_GetNsoInfo:
            if (in_rawdata_size < 0x8 || r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 1) {
                break;
            }
            
            
            rc = get_nso_info(((u64 *)in_rawdata)[0], r->Statics[0], r->StaticSizes[0], out_rawdata);
            
            if (R_SUCCEEDED(rc)) {  
                *out_raw_data_count = 4;
            } else {                
                *out_raw_data_count = 0;
            }
        
            break;
        default:
            break;
    }
    return rc;
}

Result DebugMonitorService::add_title_to_launch_queue(u64 tid, const char *args, size_t args_size) {
    return LaunchQueue::add(tid, args, args_size);
}

Result DebugMonitorService::clear_launch_queue() {
    LaunchQueue::clear();
    return 0;
}

Result DebugMonitorService::get_nso_info(u64 pid, void *out, size_t out_size, u32 *out_num_nsos) {
    /* TODO, once I've defined struct NsoInfo elsewhere (in ldr_RegisteredProcesses.hpp) */
    return 0;
}