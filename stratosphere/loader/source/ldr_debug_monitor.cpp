#include <switch.h>
#include "ldr_debug_monitor.hpp"
#include "ldr_launch_queue.hpp"

void DebugMonitorService::dispatch(IpcParsedCommand *r, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count) {
    /* TODO */
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