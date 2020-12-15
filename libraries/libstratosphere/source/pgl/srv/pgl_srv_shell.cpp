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
#include "pgl_srv_shell.hpp"
#include "pgl_srv_shell_event_observer.hpp"

namespace ams::pgl::srv {

    namespace {

        constexpr inline size_t ProcessDataCount = 0x20;

        struct ProcessData {
            os::ProcessId process_id;
            u32 flags;
        };
        static_assert(util::is_pod<ProcessData>::value);

        enum ProcessDataFlag : u32 {
            ProcessDataFlag_None                        = 0,
            ProcessDataFlag_DetailedCrashReportAllowed  = (1 << 0),
            ProcessDataFlag_DetailedCrashReportEnabled  = (1 << 1),
            ProcessDataFlag_HasLogOption                = (1 << 2),
            ProcessDataFlag_OutputAllLog                = (1 << 3),
            ProcessDataFlag_EnableCrashReportScreenShot = (1 << 4),
        };

        constinit bool g_is_production                  = true;
        constinit bool g_enable_crash_report_screenshot = true;
        constinit bool g_enable_jit_debug               = false;

        constexpr inline size_t ProcessControlTaskStackSize = 8_KB;
        constinit os::ThreadType g_process_control_task_thread;
        alignas(os::ThreadStackAlignment) constinit u8 g_process_control_task_stack[ProcessControlTaskStackSize];

        constinit os::SdkMutex g_observer_list_mutex;
        constinit util::IntrusiveListBaseTraits<ShellEventObserverHolder>::ListType g_observer_list;

        constinit os::SdkMutex g_process_data_mutex;
        constinit ProcessData g_process_data[ProcessDataCount];

        constinit os::ProcessId g_crashed_process_id = os::InvalidProcessId;
        constinit os::ProcessId g_creport_process_id = os::InvalidProcessId;
        constinit os::ProcessId g_ssd_process_id     = os::InvalidProcessId;

        ProcessData *FindProcessData(os::ProcessId process_id) {
            for (auto &data : g_process_data) {
                if (data.process_id == process_id) {
                    return std::addressof(data);
                }
            }
            return nullptr;
        }

        u32 ConvertToProcessDataFlags(u8 pgl_flags) {
            if ((pgl_flags & pgl::LaunchFlags_EnableDetailedCrashReport) == 0) {
                /* If we shouldn't generate detailed crash reports, set no flags. */
                return ProcessDataFlag_None;
            } else {
                /* We can and should generate detailed crash reports. */
                u32 data_flags = ProcessDataFlag_DetailedCrashReportAllowed | ProcessDataFlag_DetailedCrashReportEnabled;

                /* If we should enable crash report screenshots, check the correct flag. */
                if (g_enable_crash_report_screenshot) {
                    const u32 test_flag = g_is_production ? pgl::LaunchFlags_EnableCrashReportScreenShotForProduction : pgl::LaunchFlags_EnableCrashReportScreenShotForDevelop;
                    if ((pgl_flags & test_flag) != 0) {
                            data_flags |= ProcessDataFlag_EnableCrashReportScreenShot;
                    }
                }

                return data_flags;
            }
        }

        std::optional<os::ProcessId> GetRunningApplicationProcessId() {
            os::ProcessId process_id;
            if (R_SUCCEEDED(pm::shell::GetApplicationProcessIdForShell(std::addressof(process_id)))) {
                return process_id;
            } else {
                return std::nullopt;
            }
        }

        s32 ConvertDumpTypeToArgument(SnapShotDumpType dump_type) {
            switch (dump_type) {
                case SnapShotDumpType::None: return -1;
                case SnapShotDumpType::Auto: return 0;
                case SnapShotDumpType::Full: return 1;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        bool GetSnapShotDumpOutputAllLog(os::ProcessId process_id) {
            /* Check if we have an option set for the process. */
            {
                std::scoped_lock lk(g_process_data_mutex);
                if (ProcessData *data = FindProcessData(process_id); data != nullptr) {
                    if ((data->flags & ProcessDataFlag_HasLogOption) != 0) {
                        return ((data->flags & ProcessDataFlag_OutputAllLog) != 0);
                    }
                }
            }

            /* If we don't have an option for the process, fall back to settings. */
            u8 log_option;
            const size_t option_size = settings::fwdbg::GetSettingsItemValue(std::addressof(log_option), sizeof(log_option), "snap_shot_dump", "output_all_log");
            return (option_size == sizeof(log_option) && log_option != 0);
        }

        size_t CreateSnapShotDumpArguments(char *dst, size_t dst_size, os::ProcessId process_id, SnapShotDumpType dump_type, const char *str_arg) {
            const s32 dump_arg = ConvertDumpTypeToArgument(dump_type);
            const s32 log_arg  = GetSnapShotDumpOutputAllLog(process_id) ? 1 : 0;
            if (str_arg != nullptr) {
                return std::snprintf(dst, dst_size, "D %010llu \"%s\" -log %d -dump %d", static_cast<unsigned long long>(static_cast<u64>(process_id)), str_arg, log_arg, dump_arg);
            } else {
                return std::snprintf(dst, dst_size, "D %010llu -log %d -dump %d", static_cast<unsigned long long>(static_cast<u64>(process_id)), log_arg, dump_arg);
            }
        }

        Result TriggerSnapShotDumper(os::ProcessId process_id, SnapShotDumpType dump_type, const char *arg) {
            /* Create the arguments. */
            char process_arguments[800];
            const size_t arg_len = CreateSnapShotDumpArguments(process_arguments, sizeof(process_arguments), process_id, dump_type, arg);

            /* Set the arguments. */
            R_TRY(ldr::SetProgramArgument(ncm::SystemDebugAppletId::SnapShotDumper, process_arguments, arg_len + 1));

            /* Launch the process. */
            os::ProcessId ssd_process_id = os::InvalidProcessId;
            R_TRY(pm::shell::LaunchProgram(std::addressof(ssd_process_id), ncm::ProgramLocation::Make(ncm::SystemDebugAppletId::SnapShotDumper, ncm::StorageId::BuiltInSystem), pm::LaunchFlags_None));

            /* Set the globals. */
            g_crashed_process_id = process_id;
            g_ssd_process_id     = ssd_process_id;
            return ResultSuccess();
        }

        bool ShouldSnapShotAutoDump() {
            bool dump;
            const size_t sz = settings::fwdbg::GetSettingsItemValue(std::addressof(dump), sizeof(dump), "snap_shot_dump", "auto_dump");
            return sz == sizeof(dump) && dump;
        }

        bool ShouldSnapShotFullDump() {
            bool dump;
            const size_t sz = settings::fwdbg::GetSettingsItemValue(std::addressof(dump), sizeof(dump), "snap_shot_dump", "full_dump");
            return sz == sizeof(dump) && dump;
        }

        SnapShotDumpType GetSnapShotDumpType() {
            if (ShouldSnapShotAutoDump()) {
                if (ShouldSnapShotFullDump()) {
                    return SnapShotDumpType::Full;
                } else {
                    return SnapShotDumpType::Auto;
                }
            } else {
                return SnapShotDumpType::None;
            }
        }

        void TriggerSnapShotDumper(os::ProcessId process_id) {
            TriggerSnapShotDumper(process_id, GetSnapShotDumpType(), nullptr);
        }

        s32 GetCrashReportDetailedArgument(u32 data_flags) {
            if (((data_flags & ProcessDataFlag_DetailedCrashReportAllowed) != 0) && ((data_flags & ProcessDataFlag_DetailedCrashReportEnabled) != 0)) {
                return 1;
            } else {
                return 0;
            }
        }

        s32 GetCrashReportScreenShotArgument(u32 data_flags) {
            if (settings::system::GetErrorReportSharePermission() == settings::system::ErrorReportSharePermission_Granted) {
                return ((data_flags & ProcessDataFlag_EnableCrashReportScreenShot) != 0) ? 1 : 0;
            } else {
                return 0;
            }
        }

        void TriggerCrashReport(os::ProcessId process_id) {
            /* If the program that crashed is creport, we should just terminate both processes and return. */
            if (process_id == g_creport_process_id) {
                TerminateProcess(g_crashed_process_id);
                TerminateProcess(g_creport_process_id);
                g_crashed_process_id = os::InvalidProcessId;
                g_creport_process_id = os::InvalidProcessId;
                return;
            }

            /* Get the data flags for the process. */
            u32 data_flags;
            {
                std::scoped_lock lk(g_process_data_mutex);
                if (auto *data = FindProcessData(process_id); data != nullptr) {
                    data_flags = data->flags;
                } else {
                    data_flags = ProcessDataFlag_None;
                }
            }

            /* Generate arguments. */
            char arguments[0x40];
            const size_t len = std::snprintf(arguments, sizeof(arguments), "%ld %d %d %d", static_cast<s64>(static_cast<u64>(process_id)), GetCrashReportDetailedArgument(data_flags), GetCrashReportScreenShotArgument(data_flags), g_enable_jit_debug);
            if (R_FAILED(ldr::SetProgramArgument(ncm::SystemProgramId::Creport, arguments, len + 1))) {
                return;
            }

            /* Launch creport. */
            os::ProcessId creport_process_id;
            if (R_FAILED(pm::shell::LaunchProgram(std::addressof(creport_process_id), ncm::ProgramLocation::Make(ncm::SystemProgramId::Creport, ncm::StorageId::BuiltInSystem), pm::LaunchFlags_None))) {
                return;
            }

            /* Set the globals. */
            g_crashed_process_id = process_id;
            g_creport_process_id = creport_process_id;
        }

        void HandleException(os::ProcessId process_id) {
            if (g_enable_jit_debug) {
                /* If jit debug is enabled, we want to try to launch snap shot dumper. */
                ProcessData *data = nullptr;
                {
                    std::scoped_lock lk(g_process_data_mutex);
                    data = FindProcessData(process_id);
                }

                /* If we're tracking the process, we can launch dumper. Otherwise we should just terminate. */
                if (data != nullptr) {
                    TriggerSnapShotDumper(process_id);
                } else {
                    TerminateProcess(process_id);
                }
            } else {
                /* Otherwise, we want to launch creport. */
                TriggerCrashReport(process_id);
            }
        }

        void HandleExit(os::ProcessId process_id) {
            std::scoped_lock lk(g_process_data_mutex);
            if (auto *data = FindProcessData(process_id); data != nullptr) {
                data->process_id = os::InvalidProcessId;
            }
        }

        void OnProcessEvent(const pm::ProcessEventInfo &event_info) {
            /* Determine if we're tracking the process. */
            ProcessData *data = nullptr;
            {
                std::scoped_lock lk(g_process_data_mutex);
                data = FindProcessData(event_info.process_id);
            }

            /* If we are, we're going to want to notify our listeners. */
            if (data != nullptr) {
                /* If we closed the process, note that. */
                if (static_cast<pm::ProcessEvent>(event_info.event) == pm::ProcessEvent::Exited) {
                    HandleExit(event_info.process_id);
                }

                /* Notify all observers. */
                std::scoped_lock lk(g_observer_list_mutex);
                for (auto &observer : g_observer_list) {
                    observer.Notify(event_info);
                }
            }

            /* If the process crashed, handle that. */
            if (static_cast<pm::ProcessEvent>(event_info.event) == pm::ProcessEvent::Exception) {
                HandleException(event_info.process_id);
            }
        }

        void ProcessControlTask(void *) {
            /* Get the process event event from pm. */
            os::SystemEvent process_event;
            R_ABORT_UNLESS(pm::shell::GetProcessEventEvent(std::addressof(process_event)));

            while (true) {
                /* Wait for an event to come in, and clear our signal. */
                process_event.Wait();
                process_event.Clear();

                bool continue_getting_event = true;
                while (continue_getting_event) {
                    /* Try to get an event info. */
                    pm::ProcessEventInfo event_info;
                    if (R_FAILED(pm::shell::GetProcessEventInfo(std::addressof(event_info)))) {
                        break;
                    }

                    /* Process the event. */
                    switch (static_cast<pm::ProcessEvent>(event_info.event)) {
                        case pm::ProcessEvent::None:
                            continue_getting_event = false;
                            break;
                        case pm::ProcessEvent::Exited:
                            {
                                /* If SnapShotDumper terminated, trigger a crash report. */
                                if (event_info.process_id == g_ssd_process_id && g_crashed_process_id != os::InvalidProcessId) {
                                    TriggerCrashReport(g_crashed_process_id);
                                }
                            }
                            [[fallthrough]];
                        case pm::ProcessEvent::Started:
                        case pm::ProcessEvent::Exception:
                        case pm::ProcessEvent::DebugRunning:
                        case pm::ProcessEvent::DebugBreak:
                            OnProcessEvent(event_info);
                            break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }
            }
        }

    }

    void InitializeProcessControlTask() {
        /* Create the task thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_process_control_task_thread), ProcessControlTask, nullptr, g_process_control_task_stack, sizeof(g_process_control_task_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(pgl, ProcessControlTask)));
        os::SetThreadNamePointer(std::addressof(g_process_control_task_thread), AMS_GET_SYSTEM_THREAD_NAME(pgl, ProcessControlTask));

        /* Retrieve settings. */
        settings::fwdbg::GetSettingsItemValue(std::addressof(g_enable_jit_debug), sizeof(g_enable_jit_debug), "jit_debug", "enable_jit_debug");
        settings::fwdbg::GetSettingsItemValue(std::addressof(g_enable_crash_report_screenshot), sizeof(g_enable_crash_report_screenshot), "creport", "crash_screen_shot");
        g_is_production = !settings::fwdbg::IsDebugModeEnabled();

        /* Clear all process data. */
        {
            for (size_t i = 0; i < util::size(g_process_data); i++) {
                g_process_data[i].process_id = os::InvalidProcessId;
            }
        }

        /* Start the thread. */
        os::StartThread(std::addressof(g_process_control_task_thread));
    }

    void RegisterShellEventObserver(ShellEventObserverHolder *holder) {
        std::scoped_lock lk(g_observer_list_mutex);

        g_observer_list.push_back(*holder);
    }

    void UnregisterShellEventObserver(ShellEventObserverHolder *holder) {
        std::scoped_lock lk(g_observer_list_mutex);

        for (auto &observer : g_observer_list) {
            if (std::addressof(observer) == holder) {
                g_observer_list.erase(g_observer_list.iterator_to(observer));
                break;
            }
        }
    }

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 pm_flags, u8 pgl_flags) {
        /* Convert the input flags to the internal format. */
        const u32 data_flags = ConvertToProcessDataFlags(pgl_flags);

        /* If jit debug is enabled, we want to be signaled on crash. */
        if (g_enable_jit_debug) {
            pm_flags |= pm::LaunchFlags_SignalOnException;
        }

        /* Launch the process. */
        os::ProcessId process_id;
        R_TRY(pm::shell::LaunchProgram(std::addressof(process_id), loc, pm_flags & pm::LaunchFlagsMask));

        /* Create a ProcessData for the process. */
        {
            std::scoped_lock lk(g_process_data_mutex);
            ProcessData *new_data = FindProcessData(os::InvalidProcessId);
            AMS_ABORT_UNLESS(new_data != nullptr);

            new_data->process_id = process_id;
            new_data->flags      = data_flags;
        }

        /* We succeeded. */
        *out = process_id;
        return ResultSuccess();
    }

    Result TerminateProcess(os::ProcessId process_id) {
        /* Ask PM to terminate the process. */
        return pm::shell::TerminateProcess(process_id);
    }

    Result GetApplicationProcessId(os::ProcessId *out) {
        /* Get the application process id. */
        auto application_process_id = GetRunningApplicationProcessId();
        R_UNLESS(application_process_id, pgl::ResultApplicationNotRunning());

        /* Return the id. */
        *out = *application_process_id;
        return ResultSuccess();
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        /* Ask PM to boost the limit. */
        return pm::shell::BoostSystemMemoryResourceLimit(size);
    }

    bool IsProcessTracked(os::ProcessId process_id) {
        /* Check whether a ProcessData exists for the process. */
        std::scoped_lock lk(g_process_data_mutex);
        return FindProcessData(process_id) != nullptr;
    }

    void EnableApplicationCrashReport(bool enabled) {
        /* Get the application process id. */
        auto application_process_id = GetRunningApplicationProcessId();
        if (application_process_id) {
            /* Find the data for the application process. */
            std::scoped_lock lk(g_process_data_mutex);
            ProcessData *data = FindProcessData(*application_process_id);

            /* It's okay if we aren't tracking the process. */
            if (data != nullptr) {
                /* Set or clear the flag. */
                if (enabled) {
                    data->flags |= ProcessDataFlag_DetailedCrashReportEnabled;
                } else {
                    data->flags &= ~ProcessDataFlag_DetailedCrashReportEnabled;
                }
            }
        }
    }

    bool IsApplicationCrashReportEnabled() {
        /* Get the application process id. */
        auto application_process_id = GetRunningApplicationProcessId();
        if (!application_process_id) {
            return false;
        }

        /* Find the data for the process. */
        std::scoped_lock lk(g_process_data_mutex);
        if (ProcessData *data = FindProcessData(*application_process_id); data != nullptr) {
            return (data->flags & ProcessDataFlag_DetailedCrashReportEnabled) != 0;
        } else {
            return false;
        }
    }

    void EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        /* Get the application process id. */
        auto application_process_id = GetRunningApplicationProcessId();
        if (application_process_id) {
            /* Find the data for the application process. */
            std::scoped_lock lk(g_process_data_mutex);
            ProcessData *data = FindProcessData(*application_process_id);

            /* It's okay if we aren't tracking the process. */
            if (data != nullptr) {
                /* Set or clear the flag. */
                if (enabled) {
                    data->flags |= ProcessDataFlag_OutputAllLog;
                } else {
                    data->flags &= ~ProcessDataFlag_OutputAllLog;
                }

                /* NOTE: Here Nintendo releases the lock, re-takes the lock, and re-finds the process data. */
                /* This is unnecessary and less efficient, so we will not bother. */

                /* Note that the flag bit has a meaningful value. */
                data->flags |= ProcessDataFlag_HasLogOption;
            }
        }
    }

    Result TriggerApplicationSnapShotDumper(SnapShotDumpType dump_type, const char *arg) {
        /* Try to get the application process id. */
        os::ProcessId process_id;
        R_TRY(pm::shell::GetApplicationProcessIdForShell(std::addressof(process_id)));

        /* Launch the snapshot dumper, clearing the global tracker process id. */
        ON_SCOPE_EXIT { g_ssd_process_id = os::InvalidProcessId; };
        return TriggerSnapShotDumper(process_id, dump_type, arg);
    }

}
