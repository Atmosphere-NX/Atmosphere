#include <switch.h>
#include <stratosphere.hpp>
#include "pm_registration.hpp"
#include "pm_resource_limits.hpp"
#include "pm_shell.hpp"

static bool g_has_boot_finished = false;

Result ShellService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;

    if (kernelAbove500()) {
        switch ((ShellCmd_5X)cmd_id) {
            case Shell_Cmd_5X_LaunchProcess:
                rc = WrapIpcCommandImpl<&ShellService::launch_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_TerminateProcessId:
                rc = WrapIpcCommandImpl<&ShellService::terminate_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_TerminateTitleId:
                rc = WrapIpcCommandImpl<&ShellService::terminate_title_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_GetProcessWaitEvent:
                rc = WrapIpcCommandImpl<&ShellService::get_process_wait_event>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_GetProcessEventType:
                rc = WrapIpcCommandImpl<&ShellService::get_process_event_type>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_NotifyBootFinished:
                rc = WrapIpcCommandImpl<&ShellService::notify_boot_finished>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_GetApplicationProcessId:
                rc = WrapIpcCommandImpl<&ShellService::get_application_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_5X_BoostSystemMemoryResourceLimit:
                rc = WrapIpcCommandImpl<&ShellService::boost_system_memory_resource_limit>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            default:
                break;
        }
    } else {
        switch ((ShellCmd)cmd_id) {
            case Shell_Cmd_LaunchProcess:
                rc = WrapIpcCommandImpl<&ShellService::launch_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_TerminateProcessId:
                rc = WrapIpcCommandImpl<&ShellService::terminate_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_TerminateTitleId:
                rc = WrapIpcCommandImpl<&ShellService::terminate_title_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_GetProcessWaitEvent:
                rc = WrapIpcCommandImpl<&ShellService::get_process_wait_event>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_GetProcessEventType:
                rc = WrapIpcCommandImpl<&ShellService::get_process_event_type>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_FinalizeExitedProcess:
                rc = WrapIpcCommandImpl<&ShellService::finalize_exited_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_ClearProcessNotificationFlag:
                rc = WrapIpcCommandImpl<&ShellService::clear_process_notification_flag>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_NotifyBootFinished:
                rc = WrapIpcCommandImpl<&ShellService::notify_boot_finished>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_GetApplicationProcessId:
                rc = WrapIpcCommandImpl<&ShellService::get_application_process_id>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            case Shell_Cmd_BoostSystemMemoryResourceLimit:
                rc = WrapIpcCommandImpl<&ShellService::boost_system_memory_resource_limit>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                break;
            default:
                break;
        }
    }
    
    return rc;
}

Result ShellService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

std::tuple<Result, u64> ShellService::launch_process(u64 launch_flags, Registration::TidSid tid_sid) {
    u64 pid = 0;
    Result rc = Registration::LaunchProcessByTidSid(tid_sid, launch_flags, &pid);
    return {rc, pid};
}

std::tuple<Result> ShellService::terminate_process_id(u64 pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcess(pid);
    if (proc != NULL) {
        return {svcTerminateProcess(proc->handle)};
    } else {
        return {0x20F};
    }
}

std::tuple<Result> ShellService::terminate_title_id(u64 tid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcessByTitleId(tid);
    if (proc != NULL) {
        return {svcTerminateProcess(proc->handle)};
    } else {
        return {0x20F};
    }
}

std::tuple<Result, CopiedHandle> ShellService::get_process_wait_event() {
    return {0x0, Registration::GetProcessEventHandle()};
}

std::tuple<Result, u64, u64> ShellService::get_process_event_type() {
    u64 type, pid;
    Registration::GetProcessEventType(&pid, &type);
    return {0x0, type, pid};
}

std::tuple<Result> ShellService::finalize_exited_process(u64 pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcess(pid);
    if (proc == NULL) {
        return {0x20F};
    } else if (proc->state != ProcessState_Exited) {
        return {0x60F};
    } else {
        Registration::FinalizeExitedProcess(proc);
        return {0x0};
    }
}

std::tuple<Result> ShellService::clear_process_notification_flag(u64 pid) {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> proc = Registration::GetProcess(pid);
    if (proc != NULL) {
        proc->flags &= ~PROCESSFLAGS_CRASHED;
        return {0x0};
    } else {
        return {0x20F};
    }
}

std::tuple<Result> ShellService::notify_boot_finished() {
    u64 boot2_pid;
    if (!g_has_boot_finished) {
        g_has_boot_finished = true;
        return {Registration::LaunchProcess(BOOT2_TITLE_ID, FsStorageId_NandSystem, 0, &boot2_pid)};
    }
    return {0};
}

std::tuple<Result, u64> ShellService::get_application_process_id() {
    auto auto_lock = Registration::GetProcessListUniqueLock();
    
    std::shared_ptr<Registration::Process> app_proc;
    if (Registration::HasApplicationProcess(&app_proc)) {
        return {0, app_proc->pid};
    }
    return {0x20F, 0};
}

std::tuple<Result> ShellService::boost_system_memory_resource_limit(u64 sysmem_size) {
    if (!kernelAbove400()) {
        return {0xF601};
    }
    
    /* TODO */
    return {ResourceLimitUtils::BoostSystemMemoryResourceLimit(sysmem_size)};
}
