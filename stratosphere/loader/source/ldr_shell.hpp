#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

enum ShellServiceCmd {
    Shell_Cmd_AddTitleToLaunchQueue = 0,
    Shell_Cmd_ClearLaunchQueue = 1
};

class ShellService : IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        
    private:
        /* Actual commands. */
        std::tuple<Result> add_title_to_launch_queue(u64 tid, InPointer<char> args);
        std::tuple<Result> clear_launch_queue(u64 dat);
};