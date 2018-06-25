#include <switch.h>
#include <stratosphere.hpp>
#include "ldr_shell.hpp"
#include "ldr_launch_queue.hpp"

Result ShellService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    
    Result rc = 0xF601;
        
    switch ((ShellServiceCmd)cmd_id) {
        case Shell_Cmd_AddTitleToLaunchQueue:
            rc = WrapIpcCommandImpl<&ShellService::add_title_to_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Shell_Cmd_ClearLaunchQueue:
            rc = WrapIpcCommandImpl<&ShellService::clear_launch_queue>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

std::tuple<Result> ShellService::add_title_to_launch_queue(u64 args_size, u64 tid, InPointer<char> args) {
    fprintf(stderr, "Add to launch queue: %p, %zX\n", args.pointer, std::min(args_size, args.num_elements));
    return {LaunchQueue::add(tid, args.pointer, std::min(args_size, args.num_elements))};
}

std::tuple<Result> ShellService::clear_launch_queue(u64 dat) {
    fprintf(stderr, "Clear launch queue: %lx\n", dat);
    LaunchQueue::clear();
    return {0};
}
