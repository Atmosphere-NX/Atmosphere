#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>
#include "ldr_debug_monitor.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_registration.hpp"

Result DebugMonitorService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
        
    switch ((DebugMonitorServiceCmd)cmd_id) {
        case Dmnt_Cmd_AddTitleToLaunchQueue:
            rc = WrapIpcCommandImpl<&DebugMonitorService::add_title_to_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Dmnt_Cmd_ClearLaunchQueue:
            rc = WrapIpcCommandImpl<&DebugMonitorService::clear_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Dmnt_Cmd_GetNsoInfo:
            rc = WrapIpcCommandImpl<&DebugMonitorService::get_nso_info>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

std::tuple<Result> DebugMonitorService::add_title_to_launch_queue(u64 tid, InPointer<char> args) {
    fprintf(stderr, "Add to launch queue: %p, %zX\n", args.pointer, args.num_elements);
    return {LaunchQueue::add(tid, args.pointer, args.num_elements)};
}

std::tuple<Result> DebugMonitorService::clear_launch_queue(u64 dat) {
    fprintf(stderr, "Clear launch queue: %lx\n", dat);
    LaunchQueue::clear();
    return {0};
}

std::tuple<Result, u32> DebugMonitorService::get_nso_info(u64 pid, OutPointerWithClientSize<Registration::NsoInfo> out) {
    u32 out_num_nsos = 0;
                    
    std::fill(out.pointer, out.pointer + out.num_elements, (const Registration::NsoInfo){0});
    
    Result rc = Registration::GetNsoInfosForProcessId(out.pointer, out.num_elements, pid, &out_num_nsos);
        
    return {rc, out_num_nsos};
}
