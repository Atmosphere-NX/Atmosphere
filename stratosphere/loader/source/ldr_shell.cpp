#include <switch.h>
#include "ldr_shell.hpp"
#include "ldr_launch_queue.hpp"

Result ShellService::dispatch(IpcParsedCommand *r, IpcCommand *out_c, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count) {
    
    Result rc = 0xF601;
    
    /* TODO: Prepare SFCO. */
    
    switch ((ShellServiceCmd)cmd_id) {
        case Cmd_AddTitleToLaunchQueue:
            /* Validate arguments. */
            if (in_rawdata_size < 0x10 || r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 1) {
                break;
            }
            
            rc = add_title_to_launch_queue(((u64 *)in_rawdata)[0], (const char *)r->Statics[0], r->StaticSizes[0]);
            
            *out_raw_data_count = 0;
            
            break;
        case Cmd_ClearLaunchQueue:
            if (r->HasPid || r->NumHandles != 0 || r->NumBuffers != 0 || r->NumStatics != 0) {
                break;
            }
            
            rc = clear_launch_queue();
            *out_raw_data_count = 0;
            
            break;
        default:
            break;
    }
    return rc;
}

Result ShellService::add_title_to_launch_queue(u64 tid, const char *args, size_t args_size) {
    return LaunchQueue::add(tid, args, args_size);
}

Result ShellService::clear_launch_queue() {
    LaunchQueue::clear();
    return 0;
}