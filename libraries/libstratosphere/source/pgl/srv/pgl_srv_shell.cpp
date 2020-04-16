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

namespace ams::pgl::srv {

    namespace {

        constexpr inline size_t ProcessDataCount = 0x20;

        struct ProcessData {
            os::ProcessId process_id;
            u32 flags;
        };
        static_assert(std::is_pod<ProcessData>::value);

        enum ProcessDataFlag : u32 {
            ProcessDataFlag_None                        = 0,
            ProcessDataFlag_DetailedCrashReportAllowed  = (1 << 0),
            ProcessDataFlag_DetailedCrashReportEnabled  = (1 << 1),
            ProcessDataFlag_HasLogOption                = (1 << 2),
            ProcessDataFlag_OutputAllLog                = (1 << 3),
            ProcessDataFlag_EnableCrashReportScreenShot = (1 << 4),
        };

        bool g_is_production                  = true;
        bool g_enable_crash_report_screenshot = true;
        bool g_enable_jit_debug               = false;

        os::SdkMutex g_process_data_mutex;
        ProcessData g_process_data[ProcessDataCount];

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
            os::ProcessId dummy_process_id;
            return pm::shell::LaunchProgram(std::addressof(dummy_process_id), ncm::ProgramLocation::Make(ncm::SystemDebugAppletId::SnapShotDumper, ncm::StorageId::BuiltInSystem), pm::LaunchFlags_None);
        }

    }

    void InitializeProcessControlTask() {
        /* TODO */
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

        /* Launch the snapshot dumper. */
        return TriggerSnapShotDumper(process_id, dump_type, arg);
    }

}
