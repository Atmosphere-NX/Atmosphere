#pragma once
#include <switch.h>

#include "iserviceobject.hpp"
#include "ldr_registration.hpp"

enum ProcessManagerServiceCmd {
    Pm_Cmd_CreateProcess = 0,
    Pm_Cmd_GetProgramInfo = 1,
    Pm_Cmd_RegisterTitle = 2,
    Pm_Cmd_UnregisterTitle = 3
};

class ProcessManagerService : IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        
    private:
        /* Actual commands. */
        std::tuple<Result> create_process();
        std::tuple<Result> get_program_info();
        std::tuple<Result, u64> register_title(Registration::TidSid tid_sid);
        std::tuple<Result> unregister_title(u64 index);
};