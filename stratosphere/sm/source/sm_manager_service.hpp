#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

enum ManagerServiceCmd {
    Manager_Cmd_RegisterProcess = 0,
    Manager_Cmd_UnregisterProcess = 1
};

class ManagerService : IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        
    private:
        /* Actual commands. */
        std::tuple<Result> register_process(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac);
        std::tuple<Result> unregister_process(u64 pid);
};