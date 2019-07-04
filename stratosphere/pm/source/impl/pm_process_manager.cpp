/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <atomic>
#include <stratosphere/spl.hpp>
#include <stratosphere/ldr/ldr_pm_api.hpp>
#include <stratosphere/sm/sm_manager_api.hpp>

#include "pm_process_manager.hpp"
#include "pm_resource_manager.hpp"

#include "pm_process_info.hpp"

#include "../boot2/boot2_api.hpp"

namespace sts::pm::impl {

    namespace {

        /* Types. */
        enum HookType {
            HookType_TitleId     = (1 << 0),
            HookType_Application = (1 << 1),
        };

        struct LaunchProcessArgs {
            u64 *out_process_id;
            ncm::TitleLocation location;
            u32 flags;
        };

        enum LaunchFlags {
            LaunchFlags_None                = 0,
            LaunchFlags_SignalOnExit        = (1 << 0),
            LaunchFlags_SignalOnStart       = (1 << 1),
            LaunchFlags_SignalOnException   = (1 << 2),
            LaunchFlags_SignalOnDebugEvent  = (1 << 3),
            LaunchFlags_StartSuspended      = (1 << 4),
            LaunchFlags_DisableAslr         = (1 << 5),
        };

        enum LaunchFlagsDeprecated {
            LaunchFlagsDeprecated_None                = 0,
            LaunchFlagsDeprecated_SignalOnExit        = (1 << 0),
            LaunchFlagsDeprecated_StartSuspended      = (1 << 1),
            LaunchFlagsDeprecated_SignalOnException   = (1 << 2),
            LaunchFlagsDeprecated_DisableAslr         = (1 << 3),
            LaunchFlagsDeprecated_SignalOnDebugEvent  = (1 << 4),
            LaunchFlagsDeprecated_SignalOnStart       = (1 << 5),
        };

#define GET_FLAG_MASK(flag) (firmware_version >= FirmwareVersion_500 ? static_cast<u32>(LaunchFlags_##flag) : static_cast<u32>(LaunchFlagsDeprecated_##flag))

        inline bool ShouldSignalOnExit(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnExit);
        }

        inline bool ShouldSignalOnStart(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            if (firmware_version < FirmwareVersion_200) {
                return false;
            }
            return launch_flags & GET_FLAG_MASK(SignalOnStart);
        }

        inline bool ShouldSignalOnException(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnException);
        }

        inline bool ShouldSignalOnDebugEvent(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnDebugEvent);
        }

        inline bool ShouldStartSuspended(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            return launch_flags & GET_FLAG_MASK(StartSuspended);
        }

        inline bool ShouldDisableAslr(u32 launch_flags) {
            const auto firmware_version = GetRuntimeFirmwareVersion();
            return launch_flags & GET_FLAG_MASK(DisableAslr);
        }

#undef GET_FLAG_MASK

        enum class ProcessEvent {
            None           = 0,
            Exited         = 1,
            Started        = 2,
            Exception      = 3,
            DebugRunning   = 4,
            DebugSuspended = 5,
        };

        enum class ProcessEventDeprecated {
            None           = 0,
            Exception      = 1,
            Exited         = 2,
            DebugRunning   = 3,
            DebugSuspended = 4,
            Started        = 5,
        };

        inline u32 GetProcessEventValue(ProcessEvent event) {
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
                return static_cast<u32>(event);
            }
            switch (event) {
                case ProcessEvent::None:
                    return static_cast<u32>(ProcessEventDeprecated::None);
                case ProcessEvent::Exited:
                    return static_cast<u32>(ProcessEventDeprecated::Exited);
                case ProcessEvent::Started:
                    return static_cast<u32>(ProcessEventDeprecated::Started);
                case ProcessEvent::Exception:
                    return static_cast<u32>(ProcessEventDeprecated::Exception);
                case ProcessEvent::DebugRunning:
                    return static_cast<u32>(ProcessEventDeprecated::DebugRunning);
                case ProcessEvent::DebugSuspended:
                    return static_cast<u32>(ProcessEventDeprecated::DebugSuspended);
                default:
                    std::abort();
            }
        }

        /* Process Tracking globals. */
        HosThread g_process_track_thread;
        auto g_process_waitable_manager = WaitableManager(1);

        /* Process lists. */
        ProcessList g_process_list;
        ProcessList g_dead_process_list;

        /* Global events. */
        IEvent *g_process_event = CreateWriteOnlySystemEvent();
        IEvent *g_hook_to_create_process_event = CreateWriteOnlySystemEvent();
        IEvent *g_hook_to_create_application_process_event = CreateWriteOnlySystemEvent();
        IEvent *g_boot_finished_event = CreateWriteOnlySystemEvent();

        /* Process Launch synchronization globals. */
        IEvent *g_process_launch_start_event = CreateWriteOnlySystemEvent();
        HosSignal g_process_launch_finish_signal;
        Result g_process_launch_result = ResultSuccess;
        LaunchProcessArgs g_process_launch_args = {};

        /* Hook globals. */
        std::atomic<ncm::TitleId> g_title_id_hook;
        std::atomic<bool> g_application_hook;

        /* Helpers. */
        void ProcessTrackingMain(void *arg) {
            /* This is the main loop of the process tracking thread. */

            /* Service processes. */
            g_process_waitable_manager.AddWaitable(g_process_launch_start_event);
            g_process_waitable_manager.Process();
        }

        inline u32 GetLoaderCreateProcessFlags(u32 launch_flags) {
            u32 ldr_flags = 0;

            if (ShouldSignalOnException(launch_flags) || (GetRuntimeFirmwareVersion() >= FirmwareVersion_200 && !ShouldStartSuspended(launch_flags))) {
                ldr_flags |= ldr::CreateProcessFlag_EnableDebug;
            }
            if (ShouldDisableAslr(launch_flags)) {
                ldr_flags |= ldr::CreateProcessFlag_DisableAslr;
            }

            return ldr_flags;
        }

        bool HasApplicationProcess() {
            ProcessListAccessor list(g_process_list);

            for (size_t i = 0; i < list->GetSize(); i++) {
                if (list[i]->IsApplication()) {
                    return true;
                }
            }

            return false;
        }

        Result StartProcess(std::shared_ptr<ProcessInfo> process_info, const ldr::ProgramInfo *program_info) {
            R_TRY(svcStartProcess(process_info->GetHandle(), program_info->main_thread_priority, program_info->default_cpu_id, program_info->main_thread_stack_size));
            process_info->SetState(ProcessState_Running);
            return ResultSuccess;
        }

        Result LaunchProcess(const LaunchProcessArgs *args) {
            /* Get Program Info. */
            ldr::ProgramInfo program_info;
            R_TRY(ldr::pm::GetProgramInfo(&program_info, args->location));
            const bool is_application = (program_info.flags & ldr::ProgramInfoFlag_ApplicationTypeMask) == ldr::ProgramInfoFlag_Application;
            const bool allow_debug    = (program_info.flags & ldr::ProgramInfoFlag_AllowDebug) || GetRuntimeFirmwareVersion() < FirmwareVersion_200;

            /* Ensure we only try to run one application. */
            if (is_application && HasApplicationProcess()) {
                return ResultPmApplicationRunning;
            }

            /* Fix the title location to use the right title id. */
            const ncm::TitleLocation location = ncm::TitleLocation::Make(program_info.title_id, static_cast<ncm::StorageId>(args->location.storage_id));

            /* Pin the program with loader. */
            ldr::PinId pin_id;
            R_TRY(ldr::pm::PinTitle(&pin_id, location));

            /* Ensure resources are available. */
            resource::WaitResourceAvailable(&program_info);

            /* Actually create the process. */
            Handle process_handle;
            R_TRY_CLEANUP(ldr::pm::CreateProcess(&process_handle, pin_id, GetLoaderCreateProcessFlags(args->flags), resource::GetResourceLimitHandle(&program_info)), {
                ldr::pm::UnpinTitle(pin_id);
            });

            /* Get the process id. */
            u64 process_id;
            R_ASSERT(svcGetProcessId(&process_id, process_handle));

            /* Make new process info. */
            auto process_info = std::make_shared<ProcessInfo>(process_handle, process_id, pin_id, location);

            const u8 *acid_sac = program_info.ac_buffer;
            const u8 *aci_sac  = acid_sac + program_info.acid_sac_size;
            const u8 *acid_fac = aci_sac  + program_info.aci_sac_size;
            const u8 *aci_fah  = acid_fac + program_info.acid_fac_size;

            /* Register with FS and SM. */
            R_TRY(fsprRegisterProgram(process_id, static_cast<u64>(location.title_id), static_cast<FsStorageId>(location.storage_id), aci_fah, program_info.aci_fah_size, acid_fac, program_info.acid_fac_size));
            R_TRY(sm::manager::RegisterProcess(process_id, location.title_id, acid_sac, program_info.acid_sac_size, aci_sac, program_info.aci_sac_size));

            /* Set flags. */
            if (is_application) {
                process_info->SetApplication();
            }
            if (ShouldSignalOnStart(args->flags) && allow_debug) {
                process_info->SetSignalOnStart();
            }
            if (ShouldSignalOnExit(args->flags)) {
                process_info->SetSignalOnExit();
            }
            if (ShouldSignalOnDebugEvent(args->flags) && allow_debug) {
                process_info->SetSignalOnDebugEvent();
            }

            /* Process hooks/signaling. */
            if (location.title_id == g_title_id_hook) {
                g_hook_to_create_process_event->Signal();
                g_title_id_hook = ncm::TitleId::Invalid;
            } else if (is_application && g_application_hook) {
                g_hook_to_create_application_process_event->Signal();
                g_application_hook = false;
            } else if (!ShouldStartSuspended(args->flags)) {
                R_TRY(StartProcess(process_info, &program_info));
            }

            /* Add process to list. */
            {
                ProcessListAccessor list(g_process_list);
                list->Add(process_info);
                g_process_waitable_manager.AddWaitable(new ProcessInfoWaiter(process_info));
            }

            *args->out_process_id = process_id;
            return ResultSuccess;
        }

        Result LaunchProcessEventCallback(u64 timeout) {
            g_process_launch_start_event->Clear();
            g_process_launch_result = LaunchProcess(&g_process_launch_args);
            g_process_launch_finish_signal.Signal();
            return ResultSuccess;
        }

        void CleanupProcess(ProcessListAccessor &list, std::shared_ptr<ProcessInfo> process_info) {
            /* Remove the process from the list. */
            list->Remove(process_info->GetProcessId());

            /* Close process resources. */
            process_info->Cleanup();

            /* Handle the case where we need to keep the process alive some time longer. */
            if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500 && process_info->ShouldSignalOnExit()) {
                /* Add the process to the list of dead processes. */
                {
                    ProcessListAccessor dead_list(g_dead_process_list);
                    dead_list->Add(process_info);
                }
                /* Signal. */
                g_process_event->Signal();
            }
        }

    }

    /* Initialization. */
    Result InitializeProcessManager() {
        /* Create events. */
        g_process_event = CreateWriteOnlySystemEvent();
        g_hook_to_create_process_event = CreateWriteOnlySystemEvent();
        g_hook_to_create_application_process_event = CreateWriteOnlySystemEvent();
        g_boot_finished_event = CreateWriteOnlySystemEvent();

        /* Process launch is signaled via non-system event. */
        g_process_launch_start_event = CreateHosEvent(&LaunchProcessEventCallback);

        /* Initialize resource limits. */
        R_TRY(resource::InitializeResourceManager());

        /* Start thread. */
        R_ASSERT(g_process_track_thread.Initialize(&ProcessTrackingMain, nullptr, 0x4000, 0x15));
        R_ASSERT(g_process_track_thread.Start());

        return ResultSuccess;
    }

    /* Process Info API. */
    Result OnProcessSignaled(std::shared_ptr<ProcessInfo> process_info) {
        /* Resest the process's signal. */
        svcResetSignal(process_info->GetHandle());

        /* Update the process's state. */
        const ProcessState old_state = process_info->GetState();
        {
            u64 tmp = 0;
            R_ASSERT(svcGetProcessInfo(&tmp, process_info->GetHandle(), ProcessInfoType_ProcessState));
            process_info->SetState(static_cast<ProcessState>(tmp));
        }
        const ProcessState new_state = process_info->GetState();

        /* If we're transitioning away from crashed, clear waiting attached. */
        if (old_state == ProcessState_Crashed && new_state != ProcessState_Crashed) {
            process_info->ClearExceptionWaitingAttach();
        }

        switch (new_state) {
            case ProcessState_Created:
            case ProcessState_CreatedAttached:
            case ProcessState_Exiting:
                break;
            case ProcessState_Running:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->ClearSuspended();
                    process_info->SetSuspendedStateChanged();
                    g_process_event->Signal();
                } else if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200 && process_info->ShouldSignalOnStart()) {
                    process_info->SetStartedStateChanged();
                    process_info->ClearSignalOnStart();
                    g_process_event->Signal();
                }
                break;
            case ProcessState_Crashed:
                process_info->SetExceptionOccurred();
                g_process_event->Signal();
                break;
            case ProcessState_RunningAttached:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->ClearSuspended();
                    process_info->SetSuspendedStateChanged();
                    g_process_event->Signal();
                }
                break;
            case ProcessState_Exited:
                if (GetRuntimeFirmwareVersion() < FirmwareVersion_500 && process_info->ShouldSignalOnExit()) {
                    g_process_event->Signal();
                } else {
                    ProcessListAccessor list(g_process_list);
                    CleanupProcess(list, process_info);
                }
                /* Return ConnectionClosed to cause libstratosphere to stop waiting on the process. */
                return ResultKernelConnectionClosed;
            case ProcessState_DebugSuspended:
                if (process_info->ShouldSignalOnDebugEvent()) {
                    process_info->SetSuspended();
                    process_info->SetSuspendedStateChanged();
                    g_process_event->Signal();
                }
                break;
        }

        return ResultSuccess;
    }

    /* Process Management. */
    Result LaunchTitle(u64 *out_process_id, const ncm::TitleLocation &loc, u32 flags) {
        /* Ensure we only try to launch one title at a time. */
        static HosMutex s_lock;
        std::scoped_lock lk(s_lock);

        /* Set global arguments, signal, wait. */
        g_process_launch_args = {
            .out_process_id = out_process_id,
            .location = loc,
            .flags = flags,
        };
        g_process_launch_finish_signal.Reset();
        g_process_launch_start_event->Signal();
        g_process_launch_finish_signal.Wait();

        return g_process_launch_result;
    }

    Result StartProcess(u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        if (process_info->HasStarted()) {
            return ResultPmAlreadyStarted;
        }

        ldr::ProgramInfo program_info;
        R_TRY(ldr::pm::GetProgramInfo(&program_info, process_info->GetTitleLocation()));
        return StartProcess(process_info, &program_info);
    }

    Result TerminateProcess(u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        return svcTerminateProcess(process_info->GetHandle());
    }

    Result TerminateTitle(ncm::TitleId title_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(title_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        return svcTerminateProcess(process_info->GetHandle());
    }

    Result GetProcessEventHandle(Handle *out) {
        *out = g_process_event->GetHandle();
        return ResultSuccess;
    }

    Result GetProcessEventInfo(ProcessEventInfo *out) {
        /* Check for event from current process. */
        {
            ProcessListAccessor list(g_process_list);

            for (size_t i = 0; i < list->GetSize(); i++) {
                auto process_info = list[i];
                if (process_info->HasStarted() && process_info->HasStartedStateChanged()) {
                    process_info->ClearStartedStateChanged();
                    out->event = GetProcessEventValue(ProcessEvent::Started);
                    out->process_id = process_info->GetProcessId();
                    return ResultSuccess;
                }
                if (process_info->HasSuspendedStateChanged()) {
                    process_info->ClearSuspendedStateChanged();
                    if (process_info->IsSuspended()) {
                        out->event = GetProcessEventValue(ProcessEvent::DebugSuspended);
                    } else {
                        out->event = GetProcessEventValue(ProcessEvent::DebugRunning);
                    }
                    out->process_id = process_info->GetProcessId();
                    return ResultSuccess;
                }
                if (process_info->HasExceptionOccurred()) {
                    process_info->ClearExceptionOccurred();
                    out->event = GetProcessEventValue(ProcessEvent::Exception);
                    out->process_id = process_info->GetProcessId();
                    return ResultSuccess;
                }
                if (GetRuntimeFirmwareVersion() < FirmwareVersion_500 && process_info->ShouldSignalOnExit() && process_info->HasExited()) {
                    out->event = GetProcessEventValue(ProcessEvent::Exited);
                    out->process_id = process_info->GetProcessId();
                    return ResultSuccess;
                }
            }
        }

        /* Check for event from exited process. */
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
            ProcessListAccessor dead_list(g_dead_process_list);

            if (dead_list->GetSize() > 0) {
                auto process_info = dead_list->Pop();
                out->event = GetProcessEventValue(ProcessEvent::Exited);
                out->process_id = process_info->GetProcessId();
                return ResultSuccess;
            }
        }

        out->process_id = 0;
        out->event = GetProcessEventValue(ProcessEvent::None);
        return ResultSuccess;
    }

    Result CleanupProcess(u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        if (!process_info->HasExited()) {
            return ResultPmNotExited;
        }

        CleanupProcess(list, process_info);
        return ResultSuccess;
    }

    Result ClearExceptionOccurred(u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        process_info->ClearExceptionOccurred();
        return ResultSuccess;
    }

    /* Information Getters. */
    Result GetModuleIdList(u32 *out_count, u8 *out_buf, size_t max_out_count, u64 unused) {
        /* This function was always stubbed... */
        *out_count = 0;
        return ResultSuccess;
    }

    Result GetExceptionProcessIdList(u32 *out_count, u64 *out_process_ids, size_t max_out_count) {
        ProcessListAccessor list(g_process_list);

        size_t count = 0;
        for (size_t i = 0; i < list->GetSize() && count < max_out_count; i++) {
            auto process_info = list[i];
            if (process_info->HasExceptionWaitingAttach()) {
                out_process_ids[count++] = process_info->GetProcessId();
            }
        }

        *out_count = static_cast<u32>(count);
        return ResultSuccess;
    }

    Result GetProcessId(u64 *out, ncm::TitleId title_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(title_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        *out = process_info->GetProcessId();
        return ResultSuccess;
    }

    Result GetTitleId(ncm::TitleId *out, u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        *out = process_info->GetTitleLocation().title_id;
        return ResultSuccess;
    }

    Result GetApplicationProcessId(u64 *out_process_id) {
        ProcessListAccessor list(g_process_list);

        for (size_t i = 0; i < list->GetSize(); i++) {
            auto process_info = list[i];
            if (process_info->IsApplication()) {
                *out_process_id = process_info->GetProcessId();
                return ResultSuccess;
            }
        }

        return ResultPmProcessNotFound;
    }

    Result AtmosphereGetProcessInfo(Handle *out_process_handle, ncm::TitleLocation *out_loc, u64 process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        if (process_info == nullptr) {
            return ResultPmProcessNotFound;
        }

        *out_process_handle = process_info->GetHandle();
        *out_loc = process_info->GetTitleLocation();
        return ResultSuccess;
    }

    /* Hook API. */
    Result HookToCreateProcess(Handle *out_hook, ncm::TitleId title_id) {
        *out_hook = INVALID_HANDLE;

        ncm::TitleId old_value = ncm::TitleId::Invalid;
        if (!g_title_id_hook.compare_exchange_strong(old_value, title_id)) {
            return ResultPmDebugHookInUse;
        }

        *out_hook = g_hook_to_create_process_event->GetHandle();
        return ResultSuccess;
    }

    Result HookToCreateApplicationProcess(Handle *out_hook) {
        *out_hook = INVALID_HANDLE;

        bool old_value = false;
        if (!g_application_hook.compare_exchange_strong(old_value, true)) {
            return ResultPmDebugHookInUse;
        }

        *out_hook = g_hook_to_create_application_process_event->GetHandle();
        return ResultSuccess;
    }

    Result ClearHook(u32 which) {
        if (which & HookType_TitleId) {
            g_title_id_hook = ncm::TitleId::Invalid;
        }
        if (which & HookType_Application) {
            g_application_hook = false;
        }
        return ResultSuccess;
    }

    /* Boot API. */
    Result NotifyBootFinished() {
        static bool g_has_boot_finished = false;
        if (!g_has_boot_finished) {
            boot2::LaunchBootPrograms();
            g_has_boot_finished = true;
            g_boot_finished_event->Signal();
        }
        return ResultSuccess;
    }

    Result GetBootFinishedEventHandle(Handle *out) {
        /* In 8.0.0, Nintendo added this command, which signals that the boot sysmodule has finished. */
        /* Nintendo only signals it in safe mode FIRM, and this function aborts on normal FIRM. */
        /* We will signal it always, but only allow this function to succeed on safe mode. */
        if (!spl::IsRecoveryBoot()) {
            std::abort();
        }
        *out = g_boot_finished_event->GetHandle();
        return ResultSuccess;
    }

    /* Resource Limit API. */
    Result BoostSystemMemoryResourceLimit(u64 boost_size) {
        return resource::BoostSystemMemoryResourceLimit(boost_size);
    }

    Result BoostApplicationThreadResourceLimit() {
        return resource::BoostApplicationThreadResourceLimit();
    }

    Result AtmosphereGetCurrentLimitInfo(u64 *out_cur_val, u64 *out_lim_val, u32 group, u32 resource) {
        return resource::GetResourceLimitValues(out_cur_val, out_lim_val, static_cast<ResourceLimitGroup>(group), static_cast<LimitableResource>(resource));
    }

}
