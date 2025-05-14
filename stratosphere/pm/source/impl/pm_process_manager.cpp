/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "pm_process_manager.hpp"
#include "pm_process_tracker.hpp"
#include "pm_process_info.hpp"
#include "pm_spec.hpp"

namespace ams::pm::impl {

    namespace {

        /* Types. */
        enum HookType {
            HookType_ProgramId     = (1 << 0),
            HookType_Application = (1 << 1),
        };

        #define GET_FLAG_MASK(flag) (hos_version >= hos::Version_5_0_0 ? static_cast<u32>(LaunchFlags_##flag) : static_cast<u32>(LaunchFlagsDeprecated_##flag))

        inline bool ShouldSignalOnExit(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnExit);
        }

        inline bool ShouldSignalOnStart(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            if (hos_version < hos::Version_2_0_0) {
                return false;
            }
            return launch_flags & GET_FLAG_MASK(SignalOnStart);
        }

        inline bool ShouldSignalOnException(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnException);
        }

        inline bool ShouldSignalOnDebugEvent(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            return launch_flags & GET_FLAG_MASK(SignalOnDebugEvent);
        }

        inline bool ShouldStartSuspended(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            return launch_flags & GET_FLAG_MASK(StartSuspended);
        }

        inline bool ShouldDisableAslr(u32 launch_flags) {
            const auto hos_version = hos::GetVersion();
            return launch_flags & GET_FLAG_MASK(DisableAslr);
        }

        #undef GET_FLAG_MASK

        /* Process Tracking globals. */
        constinit ProcessTracker g_process_tracker;
        alignas(os::ThreadStackAlignment) constinit u8 g_process_track_thread_stack[8_KB];

        /* Global events. */
        constinit os::SystemEventType g_hook_to_create_process_event;
        constinit os::SystemEventType g_hook_to_create_application_process_event;
        constinit os::SystemEventType g_boot_finished_event;

        /* Hook globals. */
        constinit std::atomic<ncm::ProgramId> g_program_id_hook;
        constinit std::atomic<bool> g_application_hook;

        /* Helpers. */
        void CreateDebuggerEvent() {
            /* Create debugger hook events. */
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_hook_to_create_process_event),             os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_hook_to_create_application_process_event), os::EventClearMode_AutoClear, true));
        }

        inline u32 GetLoaderCreateProcessFlags(u32 launch_flags) {
            u32 ldr_flags = 0;

            if (ShouldSignalOnException(launch_flags) || (hos::GetVersion() >= hos::Version_2_0_0 && !ShouldStartSuspended(launch_flags))) {
                ldr_flags |= ldr::CreateProcessFlag_EnableDebug;
            }
            if (ShouldDisableAslr(launch_flags)) {
                ldr_flags |= ldr::CreateProcessFlag_DisableAslr;
            }

            return ldr_flags;
        }

        bool HasApplicationProcess() {
            auto list = GetProcessList();

            for (auto &process : *list) {
                if (process.IsApplication()) {
                    return true;
                }
            }

            return false;
        }

        Result StartProcess(ProcessInfo *process_info, const ldr::ProgramInfo *program_info) {
            R_TRY(svc::StartProcess(process_info->GetHandle(), program_info->main_thread_priority, program_info->default_cpu_id, program_info->main_thread_stack_size));
            process_info->SetState(svc::ProcessState_Running);
            R_SUCCEED();
        }

        Result LaunchProgramImpl(ProcessInfo **out_process_info, os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 flags, const ProcessAttributes &attrs) {
            /* Set the output to nullptr, if we fail. */
            *out_process_info = nullptr;

            /* Get Program Info. */
            ldr::ProgramInfo program_info;
            cfg::OverrideStatus override_status;
            R_TRY(ldr::pm::AtmosphereGetProgramInfo(std::addressof(program_info), std::addressof(override_status), loc, attrs.program_attrs));
            const bool is_application = (program_info.flags & ldr::ProgramInfoFlag_ApplicationTypeMask) == ldr::ProgramInfoFlag_Application;
            const bool allow_debug    = (program_info.flags & ldr::ProgramInfoFlag_AllowDebug) || hos::GetVersion() < hos::Version_2_0_0;

            /* Ensure we only try to run one application. */
            R_UNLESS(!is_application || !HasApplicationProcess(), pm::ResultApplicationRunning());

            /* Fix the program location to use the right program id. */
            const ncm::ProgramLocation fixed_location = ncm::ProgramLocation::Make(program_info.program_id, static_cast<ncm::StorageId>(loc.storage_id));

            /* Pin and create the process. */
            os::NativeHandle process_handle;
            ldr::PinId pin_id;
            {
                /* Pin the program with loader. */
                R_TRY(ldr::pm::AtmospherePinProgram(std::addressof(pin_id), fixed_location, override_status));

                /* If we fail after now, unpin. */
                ON_RESULT_FAILURE { ldr::pm::UnpinProgram(pin_id); };

                /* Ensure we can talk to mitm services. */
                {
                    AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_initialized_mitm, false);
                    if (!s_initialized_mitm) {
                        mitm::pm::Initialize();
                        s_initialized_mitm = true;
                    }
                }

                /* Determine boost size for mitm. */
                u64 mitm_boost_size = 0;
                R_TRY(mitm::pm::PrepareLaunchProgram(std::addressof(mitm_boost_size), program_info.program_id, override_status, is_application));

                if (mitm_boost_size > 0 || is_application) {
                    R_ABORT_UNLESS(BoostSystemMemoryResourceLimitForMitm(mitm_boost_size));
                }
                ON_RESULT_FAILURE_2 { if (mitm_boost_size > 0 || is_application) { R_ABORT_UNLESS(BoostSystemMemoryResourceLimitForMitm(0)); } };

                /* Ensure resources are available. */
                WaitResourceAvailable(std::addressof(program_info));

                /* Actually create the process. */
                R_TRY(ldr::pm::CreateProcess(std::addressof(process_handle), pin_id, GetLoaderCreateProcessFlags(flags), GetResourceLimitHandle(std::addressof(program_info)), attrs.program_attrs));
            }

            /* Get the process id. */
            os::ProcessId process_id = os::GetProcessId(process_handle);

            /* Make new process info. */
            ProcessInfo *process_info = AllocateProcessInfo(process_handle, process_id, pin_id, fixed_location, override_status, attrs);
            AMS_ABORT_UNLESS(process_info != nullptr);

            /* Add the new process info to the process list. */
            {
                auto list = GetProcessList();
                list->push_back(*process_info);
            }

            /* Prevent resource leakage if register fails. */
            ON_RESULT_FAILURE {
                auto list = GetProcessList();
                process_info->Cleanup();
                CleanupProcessInfo(list, process_info);
            };

            const u8 *acid_sac = program_info.ac_buffer;
            const u8 *aci_sac  = acid_sac + program_info.acid_sac_size;
            const u8 *acid_fac = aci_sac  + program_info.aci_sac_size;
            const u8 *aci_fah  = acid_fac + program_info.acid_fac_size;

            /* Register with FS and SM. */
            R_TRY(fsprRegisterProgram(static_cast<u64>(process_id), static_cast<u64>(fixed_location.program_id), static_cast<NcmStorageId>(fixed_location.storage_id), aci_fah, program_info.aci_fah_size, acid_fac, program_info.acid_fac_size, 0));
            R_TRY(sm::manager::RegisterProcess(process_id, fixed_location.program_id, override_status, acid_sac, program_info.acid_sac_size, aci_sac, program_info.aci_sac_size));

            /* Set flags. */
            if (is_application) {
                process_info->SetApplication();
            }
            if (ShouldSignalOnStart(flags) && allow_debug) {
                process_info->SetSignalOnStart();
            }
            if (ShouldSignalOnExit(flags)) {
                process_info->SetSignalOnExit();
            }
            if (ShouldSignalOnDebugEvent(flags) && allow_debug) {
                process_info->SetSignalOnDebugEvent();
            }

            /* Process hooks/signaling. */
            if (fixed_location.program_id == g_program_id_hook) {
                os::SignalSystemEvent(std::addressof(g_hook_to_create_process_event));
                g_program_id_hook = ncm::InvalidProgramId;
            } else if (is_application && g_application_hook) {
                os::SignalSystemEvent(std::addressof(g_hook_to_create_application_process_event));
                g_application_hook = false;
            } else if (!ShouldStartSuspended(flags)) {
                R_TRY(StartProcess(process_info, std::addressof(program_info)));
            }

            *out_process_id   = process_id;
            *out_process_info = process_info;
            R_SUCCEED();
        }

    }

    /* Initialization. */
    Result InitializeProcessManager() {
        /* Create events. */
        CreateProcessEvent();
        CreateDebuggerEvent();
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_boot_finished_event), os::EventClearMode_AutoClear, true));

        /* Initialize resource limits. */
        R_TRY(InitializeSpec());

        /* Initialize the process tracker. */
        g_process_tracker.Initialize(g_process_track_thread_stack, sizeof(g_process_track_thread_stack));

        /* Start the process tracker thread. */
        g_process_tracker.StartThread();

        R_SUCCEED();
    }

    /* Process Management. */
    Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 flags) {
        /* Launch the program. */
        ProcessInfo *process_info = nullptr;
        R_TRY(LaunchProgramImpl(std::addressof(process_info), out_process_id, loc, flags, ProcessAttributes_Nx));

        /* Register the process info with the tracker. */
        g_process_tracker.QueueEntry(process_info);
        R_SUCCEED();
    }

    Result StartProcess(os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,     pm::ResultProcessNotFound());
        R_UNLESS(!process_info->HasStarted(), pm::ResultAlreadyStarted());

        ldr::ProgramInfo program_info;
        R_TRY(ldr::pm::GetProgramInfo(std::addressof(program_info), process_info->GetProgramLocation(), process_info->GetProcessAttributes().program_attrs));
        R_RETURN(StartProcess(process_info, std::addressof(program_info)));
    }

    Result TerminateProcess(os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        R_RETURN(svc::TerminateProcess(process_info->GetHandle()));
    }

    Result TerminateProgram(ncm::ProgramId program_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        R_RETURN(svc::TerminateProcess(process_info->GetHandle()));
    }

    Result GetProcessEventInfo(ProcessEventInfo *out) {
        /* Check for event from current process. */
        {
            auto list = GetProcessList();


            for (auto &process : *list) {
                if (process.HasStarted() && process.HasStartedStateChanged()) {
                    process.ClearStartedStateChanged();
                    out->event = GetProcessEventValue(ProcessEvent::Started);
                    out->process_id = process.GetProcessId();
                    R_SUCCEED();
                }
                if (process.HasSuspendedStateChanged()) {
                    process.ClearSuspendedStateChanged();
                    if (process.IsSuspended()) {
                        out->event = GetProcessEventValue(ProcessEvent::DebugBreak);
                    } else {
                        out->event = GetProcessEventValue(ProcessEvent::DebugRunning);
                    }
                    out->process_id = process.GetProcessId();
                    R_SUCCEED();
                }
                if (process.HasExceptionOccurred()) {
                    process.ClearExceptionOccurred();
                    out->event = GetProcessEventValue(ProcessEvent::Exception);
                    out->process_id = process.GetProcessId();
                    R_SUCCEED();
                }
                if (hos::GetVersion() < hos::Version_5_0_0 && process.ShouldSignalOnExit() && process.HasTerminated()) {
                    out->event = GetProcessEventValue(ProcessEvent::Exited);
                    out->process_id = process.GetProcessId();
                    R_SUCCEED();
                }
            }
        }

        /* Check for event from exited process. */
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            auto exit_list = GetExitList();

            if (!exit_list->empty()) {
                auto &process_info = exit_list->front();
                out->event      = GetProcessEventValue(ProcessEvent::Exited);
                out->process_id = process_info.GetProcessId();

                CleanupProcessInfo(exit_list, std::addressof(process_info));
                R_SUCCEED();
            }
        }

        out->process_id = os::ProcessId{};
        out->event = GetProcessEventValue(ProcessEvent::None);
        R_SUCCEED();
    }

    Result CleanupProcess(os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,       pm::ResultProcessNotFound());
        R_UNLESS(process_info->HasTerminated(), pm::ResultNotTerminated());

        CleanupProcessInfo(list, process_info);
        R_SUCCEED();
    }

    Result ClearExceptionOccurred(os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        process_info->ClearExceptionOccurred();
        R_SUCCEED();
    }

    /* Information Getters. */
    Result GetModuleIdList(u32 *out_count, u8 *out_buf, size_t max_out_count, u64 unused) {
        /* This function was always stubbed... */
        AMS_UNUSED(out_buf, max_out_count, unused);
        *out_count = 0;
        R_SUCCEED();
    }

    Result GetExceptionProcessIdList(u32 *out_count, os::ProcessId *out_process_ids, size_t max_out_count) {
        auto list = GetProcessList();

        size_t count = 0;

        if (max_out_count > 0) {
            for (auto &process : *list) {
                if (process.HasExceptionWaitingAttach()) {
                    out_process_ids[count++] = process.GetProcessId();

                    if (count >= max_out_count) {
                        break;
                    }
                }
            }
        }

        *out_count = static_cast<u32>(count);
        R_SUCCEED();
    }

    Result GetProcessId(os::ProcessId *out, ncm::ProgramId program_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProcessId();
        R_SUCCEED();
    }

    Result GetProgramId(ncm::ProgramId *out, os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProgramLocation().program_id;
        R_SUCCEED();
    }

    Result GetApplicationProcessId(os::ProcessId *out_process_id) {
        auto list = GetProcessList();

        for (auto &process : *list) {
            if (process.IsApplication()) {
                *out_process_id = process.GetProcessId();
                R_SUCCEED();
            }
        }

        R_THROW(pm::ResultProcessNotFound());
    }

    Result AtmosphereGetProcessInfo(os::NativeHandle *out_process_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        auto list = GetProcessList();

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out_process_handle = process_info->GetHandle();
        *out_loc = process_info->GetProgramLocation();
        *out_status = process_info->GetOverrideStatus();
        R_SUCCEED();
    }

    /* Hook API. */
    Result HookToCreateProcess(os::NativeHandle *out_hook, ncm::ProgramId program_id) {
        *out_hook = os::InvalidNativeHandle;

        {
            ncm::ProgramId old_value = ncm::InvalidProgramId;
            R_UNLESS(g_program_id_hook.compare_exchange_strong(old_value, program_id), pm::ResultDebugHookInUse());
        }

        *out_hook = os::GetReadableHandleOfSystemEvent(std::addressof(g_hook_to_create_process_event));
        R_SUCCEED();
    }

    Result HookToCreateApplicationProcess(os::NativeHandle *out_hook) {
        *out_hook = os::InvalidNativeHandle;

        {
            bool old_value = false;
            R_UNLESS(g_application_hook.compare_exchange_strong(old_value, true), pm::ResultDebugHookInUse());
        }

        *out_hook = os::GetReadableHandleOfSystemEvent(std::addressof(g_hook_to_create_application_process_event));
        R_SUCCEED();
    }

    Result ClearHook(u32 which) {
        if (which & HookType_ProgramId) {
            g_program_id_hook = ncm::InvalidProgramId;
        }
        if (which & HookType_Application) {
            g_application_hook = false;
        }
        R_SUCCEED();
    }

    /* Boot API. */
    Result NotifyBootFinished() {
        AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(bool, s_has_boot_finished, false);
        if (!s_has_boot_finished) {
            /* Set program verification disabled, if we should. */
            /* NOTE: Nintendo does not check the result of this. */
            if (spl::IsDisabledProgramVerification()) {
                if (hos::GetVersion() >= hos::Version_10_0_0) {
                    ldr::pm::SetEnabledProgramVerification(false);
                } else {
                    fsprSetEnabledProgramVerification(false);
                }
            }

            boot2::LaunchPreSdCardBootProgramsAndBoot2();

            s_has_boot_finished = true;
            os::SignalSystemEvent(std::addressof(g_boot_finished_event));
        }
        R_SUCCEED();
    }

    Result GetBootFinishedEventHandle(os::NativeHandle *out) {
        /* In 8.0.0, Nintendo added this command, which signals that the boot sysmodule has finished. */
        /* Nintendo only signals it in safe mode FIRM, and this function aborts on normal FIRM. */
        /* We will signal it always, but only allow this function to succeed on safe mode. */
        AMS_ABORT_UNLESS(spl::IsRecoveryBoot());
        *out = os::GetReadableHandleOfSystemEvent(std::addressof(g_boot_finished_event));
        R_SUCCEED();
    }

}
