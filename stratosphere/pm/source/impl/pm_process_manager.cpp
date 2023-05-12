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
#include "pm_resource_manager.hpp"

#include "pm_process_info.hpp"

namespace ams::pm::impl {

    namespace {

        /* Types. */
        enum HookType {
            HookType_ProgramId     = (1 << 0),
            HookType_Application = (1 << 1),
        };

        struct LaunchProcessArgs {
            os::ProcessId *out_process_id;
            ncm::ProgramLocation location;
            u32 flags;
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

        template<size_t MaxProcessInfos>
        class ProcessInfoAllocator {
            NON_COPYABLE(ProcessInfoAllocator);
            NON_MOVEABLE(ProcessInfoAllocator);
            static_assert(MaxProcessInfos >= 0x40, "MaxProcessInfos is too small.");
            private:
                util::TypedStorage<ProcessInfo> m_process_info_storages[MaxProcessInfos]{};
                bool m_process_info_allocated[MaxProcessInfos]{};
                os::SdkMutex m_lock{};
            private:
                constexpr inline size_t GetProcessInfoIndex(ProcessInfo *process_info) const {
                    return process_info - GetPointer(m_process_info_storages[0]);
                }
            public:
                constexpr ProcessInfoAllocator() = default;

                template<typename... Args>
                ProcessInfo *AllocateProcessInfo(Args &&... args) {
                    std::scoped_lock lk(m_lock);

                    for (size_t i = 0; i < MaxProcessInfos; i++) {
                        if (!m_process_info_allocated[i]) {
                            m_process_info_allocated[i] = true;

                            std::memset(m_process_info_storages + i, 0, sizeof(m_process_info_storages[i]));

                            return util::ConstructAt(m_process_info_storages[i], std::forward<Args>(args)...);
                        }
                    }

                    return nullptr;
                }

                void FreeProcessInfo(ProcessInfo *process_info) {
                    std::scoped_lock lk(m_lock);

                    const size_t index = this->GetProcessInfoIndex(process_info);
                    AMS_ABORT_UNLESS(index < MaxProcessInfos);
                    AMS_ABORT_UNLESS(m_process_info_allocated[index]);

                    util::DestroyAt(m_process_info_storages[index]);
                    m_process_info_allocated[index] = false;
                }
        };

        /* Process Tracking globals. */
        void ProcessTrackingMain(void *);

        constinit os::ThreadType g_process_track_thread;
        alignas(os::ThreadStackAlignment) constinit u8 g_process_track_thread_stack[16_KB];

        /* Process lists. */
        constinit ProcessList g_process_list;
        constinit ProcessList g_dead_process_list;

        /* Process Info Allocation. */
        /* Note: The kernel slabheap is size 0x50 -- we allow slightly larger to account for the dead process list. */
        constexpr size_t MaxProcessCount = 0x60;
        constinit ProcessInfoAllocator<MaxProcessCount> g_process_info_allocator;

        /* Global events. */
        constinit os::SystemEventType g_process_event;
        constinit os::SystemEventType g_hook_to_create_process_event;
        constinit os::SystemEventType g_hook_to_create_application_process_event;
        constinit os::SystemEventType g_boot_finished_event;

        /* Process Launch synchronization globals. */
        constinit os::SdkMutex g_launch_program_lock;
        os::Event g_process_launch_start_event(os::EventClearMode_AutoClear);
        os::Event g_process_launch_finish_event(os::EventClearMode_AutoClear);
        constinit Result g_process_launch_result = ResultSuccess();
        constinit LaunchProcessArgs g_process_launch_args = {};

        /* Hook globals. */
        constinit std::atomic<ncm::ProgramId> g_program_id_hook;
        constinit std::atomic<bool> g_application_hook;

        /* Forward declarations. */
        Result LaunchProcess(os::MultiWaitType &multi_wait, const LaunchProcessArgs &args);
        void   OnProcessSignaled(ProcessListAccessor &list, ProcessInfo *process_info);

        /* Helpers. */
        void ProcessTrackingMain(void *) {
            /* This is the main loop of the process tracking thread. */

            /* Setup multi wait/holders. */
            os::MultiWaitType process_multi_wait;
            os::MultiWaitHolderType start_event_holder;
            os::InitializeMultiWait(std::addressof(process_multi_wait));
            os::InitializeMultiWaitHolder(std::addressof(start_event_holder), g_process_launch_start_event.GetBase());
            os::LinkMultiWaitHolder(std::addressof(process_multi_wait), std::addressof(start_event_holder));

            while (true) {
                auto signaled_holder = os::WaitAny(std::addressof(process_multi_wait));
                if (signaled_holder == std::addressof(start_event_holder)) {
                    /* Launch start event signaled. */
                    /* TryWait will clear signaled, preventing duplicate notifications. */
                    if (g_process_launch_start_event.TryWait()) {
                        g_process_launch_result = LaunchProcess(process_multi_wait, g_process_launch_args);
                        g_process_launch_finish_event.Signal();
                    }
                } else {
                    /* Some process was signaled. */
                    ProcessListAccessor list(g_process_list);
                    OnProcessSignaled(list, reinterpret_cast<ProcessInfo *>(os::GetMultiWaitHolderUserData(signaled_holder)));
                }
            }
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
            ProcessListAccessor list(g_process_list);

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

        void CleanupProcessInfo(ProcessListAccessor &list, ProcessInfo *process_info) {
            /* Remove the process from the list. */
            list->Remove(process_info);

            /* Delete the process. */
            g_process_info_allocator.FreeProcessInfo(process_info);
        }

        Result LaunchProcess(os::MultiWaitType &multi_wait, const LaunchProcessArgs &args) {
            /* Get Program Info. */
            ldr::ProgramInfo program_info;
            cfg::OverrideStatus override_status;
            R_TRY(ldr::pm::AtmosphereGetProgramInfo(std::addressof(program_info), std::addressof(override_status), args.location));
            const bool is_application = (program_info.flags & ldr::ProgramInfoFlag_ApplicationTypeMask) == ldr::ProgramInfoFlag_Application;
            const bool allow_debug    = (program_info.flags & ldr::ProgramInfoFlag_AllowDebug) || hos::GetVersion() < hos::Version_2_0_0;

            /* Ensure we only try to run one application. */
            R_UNLESS(!is_application || !HasApplicationProcess(), pm::ResultApplicationRunning());

            /* Fix the program location to use the right program id. */
            const ncm::ProgramLocation location = ncm::ProgramLocation::Make(program_info.program_id, static_cast<ncm::StorageId>(args.location.storage_id));

            /* Pin and create the process. */
            os::NativeHandle process_handle;
            ldr::PinId pin_id;
            {
                /* Pin the program with loader. */
                R_TRY(ldr::pm::AtmospherePinProgram(std::addressof(pin_id), location, override_status));

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
                resource::WaitResourceAvailable(std::addressof(program_info));

                /* Actually create the process. */
                R_TRY(ldr::pm::CreateProcess(std::addressof(process_handle), pin_id, GetLoaderCreateProcessFlags(args.flags), resource::GetResourceLimitHandle(std::addressof(program_info))));
            }

            /* Get the process id. */
            os::ProcessId process_id = os::GetProcessId(process_handle);

            /* Make new process info. */
            ProcessInfo *process_info  = g_process_info_allocator.AllocateProcessInfo(process_handle, process_id, pin_id, location, override_status);
            AMS_ABORT_UNLESS(process_info != nullptr);

            /* Link new process info. */
            {
                ProcessListAccessor list(g_process_list);
                list->push_back(*process_info);
                process_info->LinkToMultiWait(multi_wait);
            }

            /* Prevent resource leakage if register fails. */
            ON_RESULT_FAILURE {
                ProcessListAccessor list(g_process_list);
                process_info->Cleanup();
                CleanupProcessInfo(list, process_info);
            };

            const u8 *acid_sac = program_info.ac_buffer;
            const u8 *aci_sac  = acid_sac + program_info.acid_sac_size;
            const u8 *acid_fac = aci_sac  + program_info.aci_sac_size;
            const u8 *aci_fah  = acid_fac + program_info.acid_fac_size;

            /* Register with FS and SM. */
            R_TRY(fsprRegisterProgram(static_cast<u64>(process_id), static_cast<u64>(location.program_id), static_cast<NcmStorageId>(location.storage_id), aci_fah, program_info.aci_fah_size, acid_fac, program_info.acid_fac_size));
            R_TRY(sm::manager::RegisterProcess(process_id, location.program_id, override_status, acid_sac, program_info.acid_sac_size, aci_sac, program_info.aci_sac_size));

            /* Set flags. */
            if (is_application) {
                process_info->SetApplication();
            }
            if (ShouldSignalOnStart(args.flags) && allow_debug) {
                process_info->SetSignalOnStart();
            }
            if (ShouldSignalOnExit(args.flags)) {
                process_info->SetSignalOnExit();
            }
            if (ShouldSignalOnDebugEvent(args.flags) && allow_debug) {
                process_info->SetSignalOnDebugEvent();
            }

            /* Process hooks/signaling. */
            if (location.program_id == g_program_id_hook) {
                os::SignalSystemEvent(std::addressof(g_hook_to_create_process_event));
                g_program_id_hook = ncm::InvalidProgramId;
            } else if (is_application && g_application_hook) {
                os::SignalSystemEvent(std::addressof(g_hook_to_create_application_process_event));
                g_application_hook = false;
            } else if (!ShouldStartSuspended(args.flags)) {
                R_TRY(StartProcess(process_info, std::addressof(program_info)));
            }

            *args.out_process_id = process_id;
            R_SUCCEED();
        }

        void OnProcessSignaled(ProcessListAccessor &list, ProcessInfo *process_info) {
            /* Reset the process's signal. */
            svc::ResetSignal(process_info->GetHandle());

            /* Update the process's state. */
            const svc::ProcessState old_state = process_info->GetState();
            {
                s64 tmp = 0;
                R_ABORT_UNLESS(svc::GetProcessInfo(std::addressof(tmp), process_info->GetHandle(), svc::ProcessInfoType_ProcessState));
                process_info->SetState(static_cast<svc::ProcessState>(tmp));
            }
            const svc::ProcessState new_state = process_info->GetState();

            /* If we're transitioning away from crashed, clear waiting attached. */
            if (old_state == svc::ProcessState_Crashed && new_state != svc::ProcessState_Crashed) {
                process_info->ClearExceptionWaitingAttach();
            }

            switch (new_state) {
                case svc::ProcessState_Created:
                case svc::ProcessState_CreatedAttached:
                case svc::ProcessState_Terminating:
                    break;
                case svc::ProcessState_Running:
                    if (process_info->ShouldSignalOnDebugEvent()) {
                        process_info->ClearSuspended();
                        process_info->SetSuspendedStateChanged();
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    } else if (hos::GetVersion() >= hos::Version_2_0_0 && process_info->ShouldSignalOnStart()) {
                        process_info->SetStartedStateChanged();
                        process_info->ClearSignalOnStart();
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    }
                    process_info->ClearUnhandledException();
                    break;
                case svc::ProcessState_Crashed:
                    if (!process_info->HasUnhandledException()) {
                        process_info->SetExceptionOccurred();
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    }
                    process_info->SetExceptionWaitingAttach();
                    break;
                case svc::ProcessState_RunningAttached:
                    if (process_info->ShouldSignalOnDebugEvent()) {
                        process_info->ClearSuspended();
                        process_info->SetSuspendedStateChanged();
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    }
                    process_info->ClearUnhandledException();
                    break;
                case svc::ProcessState_Terminated:
                    /* Free process resources, unlink from multi wait. */
                    process_info->Cleanup();

                    if (hos::GetVersion() < hos::Version_5_0_0 && process_info->ShouldSignalOnExit()) {
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    } else {
                        /* Handle the case where we need to keep the process alive some time longer. */
                        if (hos::GetVersion() >= hos::Version_5_0_0 && process_info->ShouldSignalOnExit()) {
                            /* Remove from the living list. */
                            list->Remove(process_info);

                            /* Add the process to the list of dead processes. */
                            {
                                ProcessListAccessor dead_list(g_dead_process_list);
                                dead_list->push_back(*process_info);
                            }

                            /* Signal. */
                            os::SignalSystemEvent(std::addressof(g_process_event));
                        } else {
                            /* Actually delete process. */
                            CleanupProcessInfo(list, process_info);
                        }
                    }
                    break;
                case svc::ProcessState_DebugBreak:
                    if (process_info->ShouldSignalOnDebugEvent()) {
                        process_info->SetSuspended();
                        process_info->SetSuspendedStateChanged();
                        os::SignalSystemEvent(std::addressof(g_process_event));
                    }
                    break;
            }
        }

    }

    /* Initialization. */
    Result InitializeProcessManager() {
        /* Create events. */
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_process_event),                            os::EventClearMode_AutoClear, true));
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_hook_to_create_process_event),             os::EventClearMode_AutoClear, true));
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_hook_to_create_application_process_event), os::EventClearMode_AutoClear, true));
        R_ABORT_UNLESS(os::CreateSystemEvent(std::addressof(g_boot_finished_event),                      os::EventClearMode_AutoClear, true));

        /* Initialize resource limits. */
        R_TRY(resource::InitializeResourceManager());

        /* Create thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_process_track_thread), ProcessTrackingMain, nullptr, g_process_track_thread_stack, sizeof(g_process_track_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(pm, ProcessTrack)));
        os::SetThreadNamePointer(std::addressof(g_process_track_thread), AMS_GET_SYSTEM_THREAD_NAME(pm, ProcessTrack));

        /* Start thread. */
        os::StartThread(std::addressof(g_process_track_thread));

        R_SUCCEED();
    }

    /* Process Management. */
    Result LaunchProgram(os::ProcessId *out_process_id, const ncm::ProgramLocation &loc, u32 flags) {
        /* Ensure we only try to launch one program at a time. */
        std::scoped_lock lk(g_launch_program_lock);

        /* Set global arguments, signal, wait. */
        g_process_launch_args = {
            .out_process_id = out_process_id,
            .location = loc,
            .flags = flags,
        };
        g_process_launch_start_event.Signal();
        g_process_launch_finish_event.Wait();

        R_RETURN(g_process_launch_result);
    }

    Result StartProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,     pm::ResultProcessNotFound());
        R_UNLESS(!process_info->HasStarted(), pm::ResultAlreadyStarted());

        ldr::ProgramInfo program_info;
        R_TRY(ldr::pm::GetProgramInfo(std::addressof(program_info), process_info->GetProgramLocation()));
        R_RETURN(StartProcess(process_info, std::addressof(program_info)));
    }

    Result TerminateProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        R_RETURN(svc::TerminateProcess(process_info->GetHandle()));
    }

    Result TerminateProgram(ncm::ProgramId program_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        R_RETURN(svc::TerminateProcess(process_info->GetHandle()));
    }

    Result GetProcessEventHandle(os::NativeHandle *out) {
        *out = os::GetReadableHandleOfSystemEvent(std::addressof(g_process_event));
        R_SUCCEED();
    }

    Result GetProcessEventInfo(ProcessEventInfo *out) {
        /* Check for event from current process. */
        {
            ProcessListAccessor list(g_process_list);


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
            ProcessListAccessor dead_list(g_dead_process_list);

            if (!dead_list->empty()) {
                auto &process_info = dead_list->front();
                out->event = GetProcessEventValue(ProcessEvent::Exited);
                out->process_id = process_info.GetProcessId();

                CleanupProcessInfo(dead_list, std::addressof(process_info));
                R_SUCCEED();
            }
        }

        out->process_id = os::ProcessId{};
        out->event = GetProcessEventValue(ProcessEvent::None);
        R_SUCCEED();
    }

    Result CleanupProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,       pm::ResultProcessNotFound());
        R_UNLESS(process_info->HasTerminated(), pm::ResultNotTerminated());

        CleanupProcessInfo(list, process_info);
        R_SUCCEED();
    }

    Result ClearExceptionOccurred(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

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
        ProcessListAccessor list(g_process_list);

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
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProcessId();
        R_SUCCEED();
    }

    Result GetProgramId(ncm::ProgramId *out, os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProgramLocation().program_id;
        R_SUCCEED();
    }

    Result GetApplicationProcessId(os::ProcessId *out_process_id) {
        ProcessListAccessor list(g_process_list);

        for (auto &process : *list) {
            if (process.IsApplication()) {
                *out_process_id = process.GetProcessId();
                R_SUCCEED();
            }
        }

        R_THROW(pm::ResultProcessNotFound());
    }

    Result AtmosphereGetProcessInfo(os::NativeHandle *out_process_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

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

    /* Resource Limit API. */
    Result BoostSystemMemoryResourceLimit(u64 boost_size) {
        R_RETURN(resource::BoostSystemMemoryResourceLimit(boost_size));
    }

    Result BoostApplicationThreadResourceLimit() {
        R_RETURN(resource::BoostApplicationThreadResourceLimit());
    }

    Result BoostSystemThreadResourceLimit() {
        R_RETURN(resource::BoostSystemThreadResourceLimit());
    }

    Result GetAppletCurrentResourceLimitValues(pm::ResourceLimitValues *out) {
        R_RETURN(resource::GetCurrentResourceLimitValues(ResourceLimitGroup_Applet, out));
    }

    Result GetAppletPeakResourceLimitValues(pm::ResourceLimitValues *out) {
        R_RETURN(resource::GetPeakResourceLimitValues(ResourceLimitGroup_Applet, out));
    }

    Result AtmosphereGetCurrentLimitInfo(s64 *out_cur_val, s64 *out_lim_val, u32 group, u32 resource) {
        R_RETURN(resource::GetResourceLimitValues(out_cur_val, out_lim_val, static_cast<ResourceLimitGroup>(group), static_cast<svc::LimitableResource>(resource)));
    }

    Result BoostSystemMemoryResourceLimitForMitm(u64 boost_size) {
        R_RETURN(resource::BoostSystemMemoryResourceLimitForMitm(boost_size));
    }

}
