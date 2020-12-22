/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
                TYPED_STORAGE(ProcessInfo) process_info_storages[MaxProcessInfos];
                bool process_info_allocated[MaxProcessInfos];
                os::Mutex lock;
            private:
                constexpr inline size_t GetProcessInfoIndex(ProcessInfo *process_info) const {
                    return process_info - GetPointer(this->process_info_storages[0]);
                }
            public:
                constexpr ProcessInfoAllocator() : lock(false) {
                    std::memset(this->process_info_storages, 0, sizeof(this->process_info_storages));
                    std::memset(this->process_info_allocated, 0, sizeof(this->process_info_allocated));
                }

                void *AllocateProcessInfoStorage() {
                    std::scoped_lock lk(this->lock);
                    for (size_t i = 0; i < MaxProcessInfos; i++) {
                        if (!this->process_info_allocated[i]) {
                            this->process_info_allocated[i] = true;
                            std::memset(&this->process_info_storages[i], 0, sizeof(this->process_info_storages[i]));
                            return GetPointer(this->process_info_storages[i]);
                        }
                    }
                    return nullptr;
                }

                void FreeProcessInfo(ProcessInfo *process_info) {
                    std::scoped_lock lk(this->lock);

                    const size_t index = this->GetProcessInfoIndex(process_info);
                    AMS_ABORT_UNLESS(index < MaxProcessInfos);
                    AMS_ABORT_UNLESS(this->process_info_allocated[index]);

                    process_info->~ProcessInfo();
                    this->process_info_allocated[index] = false;
                }
        };

        /* Process Tracking globals. */
        void ProcessTrackingMain(void *arg);

        os::ThreadType g_process_track_thread;
        alignas(os::ThreadStackAlignment) u8 g_process_track_thread_stack[16_KB];

        /* Process lists. */
        ProcessList g_process_list;
        ProcessList g_dead_process_list;

        /* Process Info Allocation. */
        /* Note: The kernel slabheap is size 0x50 -- we allow slightly larger to account for the dead process list. */
        constexpr size_t MaxProcessCount = 0x60;
        ProcessInfoAllocator<MaxProcessCount> g_process_info_allocator;

        /* Global events. */
        os::SystemEventType g_process_event;
        os::SystemEventType g_hook_to_create_process_event;
        os::SystemEventType g_hook_to_create_application_process_event;
        os::SystemEventType g_boot_finished_event;

        /* Process Launch synchronization globals. */
        os::Mutex g_launch_program_lock(false);
        os::Event g_process_launch_start_event(os::EventClearMode_AutoClear);
        os::Event g_process_launch_finish_event(os::EventClearMode_AutoClear);
        Result g_process_launch_result = ResultSuccess();
        LaunchProcessArgs g_process_launch_args = {};

        /* Hook globals. */
        std::atomic<ncm::ProgramId> g_program_id_hook;
        std::atomic<bool> g_application_hook;

        /* Forward declarations. */
        Result LaunchProcess(os::WaitableManagerType &waitable_manager, const LaunchProcessArgs &args);
        void   OnProcessSignaled(ProcessListAccessor &list, ProcessInfo *process_info);

        /* Helpers. */
        void ProcessTrackingMain(void *arg) {
            /* This is the main loop of the process tracking thread. */

            /* Setup waitable manager. */
            os::WaitableManagerType process_waitable_manager;
            os::WaitableHolderType start_event_holder;
            os::InitializeWaitableManager(std::addressof(process_waitable_manager));
            os::InitializeWaitableHolder(std::addressof(start_event_holder), g_process_launch_start_event.GetBase());
            os::LinkWaitableHolder(std::addressof(process_waitable_manager), std::addressof(start_event_holder));

            while (true) {
                auto signaled_holder = os::WaitAny(std::addressof(process_waitable_manager));
                if (signaled_holder == &start_event_holder) {
                    /* Launch start event signaled. */
                    /* TryWait will clear signaled, preventing duplicate notifications. */
                    if (g_process_launch_start_event.TryWait()) {
                        g_process_launch_result = LaunchProcess(process_waitable_manager, g_process_launch_args);
                        g_process_launch_finish_event.Signal();
                    }
                } else {
                    /* Some process was signaled. */
                    ProcessListAccessor list(g_process_list);
                    OnProcessSignaled(list, reinterpret_cast<ProcessInfo *>(os::GetWaitableHolderUserData(signaled_holder)));
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
            R_TRY(svcStartProcess(process_info->GetHandle(), program_info->main_thread_priority, program_info->default_cpu_id, program_info->main_thread_stack_size));
            process_info->SetState(svc::ProcessState_Running);
            return ResultSuccess();
        }

        void CleanupProcessInfo(ProcessListAccessor &list, ProcessInfo *process_info) {
            /* Remove the process from the list. */
            list->Remove(process_info);

            /* Delete the process. */
            g_process_info_allocator.FreeProcessInfo(process_info);
        }

        Result LaunchProcess(os::WaitableManagerType &waitable_manager, const LaunchProcessArgs &args) {
            /* Get Program Info. */
            ldr::ProgramInfo program_info;
            cfg::OverrideStatus override_status;
            R_TRY(ldr::pm::AtmosphereGetProgramInfo(&program_info, &override_status, args.location));
            const bool is_application = (program_info.flags & ldr::ProgramInfoFlag_ApplicationTypeMask) == ldr::ProgramInfoFlag_Application;
            const bool allow_debug    = (program_info.flags & ldr::ProgramInfoFlag_AllowDebug) || hos::GetVersion() < hos::Version_2_0_0;

            /* Ensure we only try to run one application. */
            R_UNLESS(!is_application || !HasApplicationProcess(), pm::ResultApplicationRunning());

            /* Fix the program location to use the right program id. */
            const ncm::ProgramLocation location = ncm::ProgramLocation::Make(program_info.program_id, static_cast<ncm::StorageId>(args.location.storage_id));

            /* Pin the program with loader. */
            ldr::PinId pin_id;
            R_TRY(ldr::pm::AtmospherePinProgram(&pin_id, location, override_status));

            /* Ensure resources are available. */
            resource::WaitResourceAvailable(&program_info);

            /* Actually create the process. */
            Handle process_handle;
            {
                auto pin_guard = SCOPE_GUARD { ldr::pm::UnpinProgram(pin_id); };
                R_TRY(ldr::pm::CreateProcess(&process_handle, pin_id, GetLoaderCreateProcessFlags(args.flags), resource::GetResourceLimitHandle(&program_info)));
                pin_guard.Cancel();
            }

            /* Get the process id. */
            os::ProcessId process_id = os::GetProcessId(process_handle);

            /* Make new process info. */
            void *process_info_storage = g_process_info_allocator.AllocateProcessInfoStorage();
            AMS_ABORT_UNLESS(process_info_storage != nullptr);
            ProcessInfo *process_info = new (process_info_storage) ProcessInfo(process_handle, process_id, pin_id, location, override_status);

            /* Link new process info. */
            {
                ProcessListAccessor list(g_process_list);
                list->push_back(*process_info);
                process_info->LinkToWaitableManager(waitable_manager);
            }

            /* Prevent resource leakage if register fails. */
            auto cleanup_guard = SCOPE_GUARD {
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
                R_TRY(StartProcess(process_info, &program_info));
            }

            /* We succeeded, so we can cancel our cleanup. */
            cleanup_guard.Cancel();

            *args.out_process_id = process_id;
            return ResultSuccess();
        }

        void OnProcessSignaled(ProcessListAccessor &list, ProcessInfo *process_info) {
            /* Reset the process's signal. */
            svcResetSignal(process_info->GetHandle());

            /* Update the process's state. */
            const svc::ProcessState old_state = process_info->GetState();
            {
                s64 tmp = 0;
                R_ABORT_UNLESS(svc::GetProcessInfo(&tmp, process_info->GetHandle(), svc::ProcessInfoType_ProcessState));
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
                    /* Free process resources, unlink from waitable manager. */
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

        return ResultSuccess();
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

        return g_process_launch_result;
    }

    Result StartProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,     pm::ResultProcessNotFound());
        R_UNLESS(!process_info->HasStarted(), pm::ResultAlreadyStarted());

        ldr::ProgramInfo program_info;
        R_TRY(ldr::pm::GetProgramInfo(&program_info, process_info->GetProgramLocation()));
        return StartProcess(process_info, &program_info);
    }

    Result TerminateProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        return svcTerminateProcess(process_info->GetHandle());
    }

    Result TerminateProgram(ncm::ProgramId program_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        return svcTerminateProcess(process_info->GetHandle());
    }

    Result GetProcessEventHandle(Handle *out) {
        *out = os::GetReadableHandleOfSystemEvent(std::addressof(g_process_event));
        return ResultSuccess();
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
                    return ResultSuccess();
                }
                if (process.HasSuspendedStateChanged()) {
                    process.ClearSuspendedStateChanged();
                    if (process.IsSuspended()) {
                        out->event = GetProcessEventValue(ProcessEvent::DebugBreak);
                    } else {
                        out->event = GetProcessEventValue(ProcessEvent::DebugRunning);
                    }
                    out->process_id = process.GetProcessId();
                    return ResultSuccess();
                }
                if (process.HasExceptionOccurred()) {
                    process.ClearExceptionOccurred();
                    out->event = GetProcessEventValue(ProcessEvent::Exception);
                    out->process_id = process.GetProcessId();
                    return ResultSuccess();
                }
                if (hos::GetVersion() < hos::Version_5_0_0 && process.ShouldSignalOnExit() && process.HasTerminated()) {
                    out->event = GetProcessEventValue(ProcessEvent::Exited);
                    out->process_id = process.GetProcessId();
                    return ResultSuccess();
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

                CleanupProcessInfo(dead_list, &process_info);
                return ResultSuccess();
            }
        }

        out->process_id = os::ProcessId{};
        out->event = GetProcessEventValue(ProcessEvent::None);
        return ResultSuccess();
    }

    Result CleanupProcess(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr,       pm::ResultProcessNotFound());
        R_UNLESS(process_info->HasTerminated(), pm::ResultNotTerminated());

        CleanupProcessInfo(list, process_info);
        return ResultSuccess();
    }

    Result ClearExceptionOccurred(os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        process_info->ClearExceptionOccurred();
        return ResultSuccess();
    }

    /* Information Getters. */
    Result GetModuleIdList(u32 *out_count, u8 *out_buf, size_t max_out_count, u64 unused) {
        /* This function was always stubbed... */
        *out_count = 0;
        return ResultSuccess();
    }

    Result GetExceptionProcessIdList(u32 *out_count, os::ProcessId *out_process_ids, size_t max_out_count) {
        ProcessListAccessor list(g_process_list);

        size_t count = 0;
        for (auto &process : *list) {
            if (process.HasExceptionWaitingAttach()) {
                out_process_ids[count++] = process.GetProcessId();
            }
        }
        *out_count = static_cast<u32>(count);
        return ResultSuccess();
    }

    Result GetProcessId(os::ProcessId *out, ncm::ProgramId program_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(program_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProcessId();
        return ResultSuccess();
    }

    Result GetProgramId(ncm::ProgramId *out, os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out = process_info->GetProgramLocation().program_id;
        return ResultSuccess();
    }

    Result GetApplicationProcessId(os::ProcessId *out_process_id) {
        ProcessListAccessor list(g_process_list);

        for (auto &process : *list) {
            if (process.IsApplication()) {
                *out_process_id = process.GetProcessId();
                return ResultSuccess();
            }
        }

        return pm::ResultProcessNotFound();
    }

    Result AtmosphereGetProcessInfo(Handle *out_process_handle, ncm::ProgramLocation *out_loc, cfg::OverrideStatus *out_status, os::ProcessId process_id) {
        ProcessListAccessor list(g_process_list);

        auto process_info = list->Find(process_id);
        R_UNLESS(process_info != nullptr, pm::ResultProcessNotFound());

        *out_process_handle = process_info->GetHandle();
        *out_loc = process_info->GetProgramLocation();
        *out_status = process_info->GetOverrideStatus();
        return ResultSuccess();
    }

    /* Hook API. */
    Result HookToCreateProcess(Handle *out_hook, ncm::ProgramId program_id) {
        *out_hook = INVALID_HANDLE;

        {
            ncm::ProgramId old_value = ncm::InvalidProgramId;
            R_UNLESS(g_program_id_hook.compare_exchange_strong(old_value, program_id), pm::ResultDebugHookInUse());
        }

        *out_hook = os::GetReadableHandleOfSystemEvent(std::addressof(g_hook_to_create_process_event));
        return ResultSuccess();
    }

    Result HookToCreateApplicationProcess(Handle *out_hook) {
        *out_hook = INVALID_HANDLE;

        {
            bool old_value = false;
            R_UNLESS(g_application_hook.compare_exchange_strong(old_value, true), pm::ResultDebugHookInUse());
        }

        *out_hook = os::GetReadableHandleOfSystemEvent(std::addressof(g_hook_to_create_application_process_event));
        return ResultSuccess();
    }

    Result ClearHook(u32 which) {
        if (which & HookType_ProgramId) {
            g_program_id_hook = ncm::InvalidProgramId;
        }
        if (which & HookType_Application) {
            g_application_hook = false;
        }
        return ResultSuccess();
    }

    /* Boot API. */
    Result NotifyBootFinished() {
        static bool g_has_boot_finished = false;
        if (!g_has_boot_finished) {
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
            g_has_boot_finished = true;
            os::SignalSystemEvent(std::addressof(g_boot_finished_event));
        }
        return ResultSuccess();
    }

    Result GetBootFinishedEventHandle(Handle *out) {
        /* In 8.0.0, Nintendo added this command, which signals that the boot sysmodule has finished. */
        /* Nintendo only signals it in safe mode FIRM, and this function aborts on normal FIRM. */
        /* We will signal it always, but only allow this function to succeed on safe mode. */
        AMS_ABORT_UNLESS(spl::IsRecoveryBoot());
        *out = os::GetReadableHandleOfSystemEvent(std::addressof(g_boot_finished_event));
        return ResultSuccess();
    }

    /* Resource Limit API. */
    Result BoostSystemMemoryResourceLimit(u64 boost_size) {
        return resource::BoostSystemMemoryResourceLimit(boost_size);
    }

    Result BoostApplicationThreadResourceLimit() {
        return resource::BoostApplicationThreadResourceLimit();
    }

    Result AtmosphereGetCurrentLimitInfo(s64 *out_cur_val, s64 *out_lim_val, u32 group, u32 resource) {
        return resource::GetResourceLimitValues(out_cur_val, out_lim_val, static_cast<ResourceLimitGroup>(group), static_cast<svc::LimitableResource>(resource));
    }

}
