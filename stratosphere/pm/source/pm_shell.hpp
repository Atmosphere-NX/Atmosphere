#pragma once
#include <switch.h>
#include <stratosphere/iserviceobject.hpp>

#include "pm_registration.hpp"

#define BOOT2_TITLE_ID (0x0100000000000008ULL)

enum ShellCmd {
    Shell_Cmd_LaunchProcess = 0,
    Shell_Cmd_TerminateProcessId = 1,
    Shell_Cmd_TerminateTitleId = 2,
    Shell_Cmd_GetProcessWaitEvent = 3,
    Shell_Cmd_GetProcessEventType = 4,
    Shell_Cmd_FinalizeExitedProcess = 5,
    Shell_Cmd_ClearProcessNotificationFlag = 6,
    Shell_Cmd_NotifyBootFinished = 7,
    Shell_Cmd_GetApplicationProcessId = 8,
    Shell_Cmd_BoostSystemMemoryResourceLimit = 9
};

enum ShellCmd_5X {
    Shell_Cmd_5X_LaunchProcess = 0,
    Shell_Cmd_5X_TerminateProcessId = 1,
    Shell_Cmd_5X_TerminateTitleId = 2,
    Shell_Cmd_5X_GetProcessWaitEvent = 3,
    Shell_Cmd_5X_GetProcessEventType = 4,
    Shell_Cmd_5X_NotifyBootFinished = 5,
    Shell_Cmd_5X_GetApplicationProcessId = 6,
    Shell_Cmd_5X_BoostSystemMemoryResourceLimit = 7
};

class ShellService : IServiceObject {
    public:
        virtual Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size);
        virtual Result handle_deferred();
        
    private:
        /* Actual commands. */
        std::tuple<Result, u64> launch_process(u64 launch_flags, Registration::TidSid tid_sid);
        std::tuple<Result> terminate_process_id(u64 pid);
        std::tuple<Result> terminate_title_id(u64 tid);
        std::tuple<Result, CopiedHandle> get_process_wait_event();
        std::tuple<Result, u64, u64> get_process_event_type();
        std::tuple<Result> finalize_exited_process(u64 pid);
        std::tuple<Result> clear_process_notification_flag(u64 pid);
        std::tuple<Result> notify_boot_finished();
        std::tuple<Result, u64> get_application_process_id();
        std::tuple<Result> boost_system_memory_resource_limit(u64 sysmem_size);
};