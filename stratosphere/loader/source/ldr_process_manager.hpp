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
    struct ProgramInfo {
        u8 main_thread_priority;
        u8 default_cpu_id;
        u16 application_type;
        u32 main_thread_stack_size;
        u64 title_id_min;
        u32 acid_sac_size;
        u32 aci0_sac_size;
        u32 acid_fac_size;
        u32 aci0_fah_size;
        u8 ac_buffer[0x3E0];
    };
    
    static_assert(sizeof(ProcessManagerService::ProgramInfo) == 0x400, "Incorrect ProgramInfo definition.");
    
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        
    private:
        /* Actual commands. */
        std::tuple<Result, MovedHandle> create_process(u64 flags, u64 title_id, CopiedHandle reslimit_h);
        std::tuple<Result> get_program_info(Registration::TidSid tid_sid, OutPointerWithServerSize<ProcessManagerService::ProgramInfo, 0x1> out_program_info);
        std::tuple<Result, u64> register_title(Registration::TidSid tid_sid);
        std::tuple<Result> unregister_title(u64 index);
        
        /* Utilities */
        Result populate_program_info_buffer(ProcessManagerService::ProgramInfo *out, Registration::TidSid *tid_sid);
};