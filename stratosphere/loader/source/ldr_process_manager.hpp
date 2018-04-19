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
        Result dispatch(IpcParsedCommand *r, IpcCommand *out_c, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count);
        
    private:
        /* Actual commands. */
        Result create_process();
        Result get_program_info();
        Result register_title(const Registration::TidSid *tid_sid, u64 *out_index);
        Result unregister_title(u64 index);
};