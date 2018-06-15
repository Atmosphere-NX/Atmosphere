#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

#include "pm_registration.hpp"

enum DmntCmd {
    Dmnt_Cmd_GetUnknownStub = 0,
    Dmnt_Cmd_GetDebugProcessIds = 1,
    Dmnt_Cmd_LaunchDebugProcess = 2,
    Dmnt_Cmd_GetTitleProcessId = 3,
    Dmnt_Cmd_EnableDebugForTitleId = 4,
    Dmnt_Cmd_GetApplicationProcessId = 5,
    Dmnt_Cmd_EnableDebugForApplication = 6,

    Dmnt_Cmd_AtmosphereGetProcessHandle = 65000
};

enum DmntCmd_5X {
    Dmnt_Cmd_5X_GetDebugProcessIds = 0,
    Dmnt_Cmd_5X_LaunchDebugProcess = 1,
    Dmnt_Cmd_5X_GetTitleProcessId = 2,
    Dmnt_Cmd_5X_EnableDebugForTitleId = 3,
    Dmnt_Cmd_5X_GetApplicationProcessId = 4,
    Dmnt_Cmd_5X_EnableDebugForApplication = 5,

    Dmnt_Cmd_5X_AtmosphereGetProcessHandle = 65000
};

class DebugMonitorService final : public IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override;
        
        DebugMonitorService *clone() override {
            return new DebugMonitorService(*this);
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, u32> get_unknown_stub(u64 unknown, OutBuffer<u8> out_unknown);
        std::tuple<Result, u32> get_debug_process_ids(OutBuffer<u64> out_processes);
        std::tuple<Result> launch_debug_process(u64 pid);
        std::tuple<Result, u64> get_title_process_id(u64 tid);
        std::tuple<Result, CopiedHandle> enable_debug_for_tid(u64 tid);
        std::tuple<Result, u64> get_application_process_id();
        std::tuple<Result, CopiedHandle> enable_debug_for_application();

        /* Atmosphere commands. */
        std::tuple<Result, CopiedHandle> get_process_handle(u64 pid);
};
