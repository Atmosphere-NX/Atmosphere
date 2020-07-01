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
#include "fsa/fs_user_mount_table.hpp"
#include "fsa/fs_directory_accessor.hpp"
#include "fsa/fs_file_accessor.hpp"
#include "fsa/fs_filesystem_accessor.hpp"

#define AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION "ams_version: " STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MAJOR) "." STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MINOR) "." STRINGIZE(ATMOSPHERE_RELEASE_VERSION_MICRO)

/* TODO: Other boards? */
#define AMS_FS_IMPL_ACCESS_LOG_SPEC "spec: NX"

namespace ams::fs {

    /* Forward declare priority getter. */
    fs::PriorityRaw GetPriorityRawOnCurrentThreadInternal();

    namespace {

        constinit u32 g_global_access_log_mode  = fs::AccessLogMode_None;
        constinit u32 g_local_access_log_target = fs::impl::AccessLogTarget_None;

        constinit std::atomic_bool g_access_log_initialized = false;
        constinit os::SdkMutex g_access_log_initialization_mutex;

        void SetLocalAccessLogImpl(bool enabled) {
            if (enabled) {
                g_local_access_log_target |= fs::impl::AccessLogTarget_Application;
            } else {
                g_local_access_log_target &= ~fs::impl::AccessLogTarget_Application;
            }
        }

    }

    Result GetGlobalAccessLogMode(u32 *out) {
        /* Use libnx bindings. */
        return ::fsGetGlobalAccessLogMode(out);
    }

    Result SetGlobalAccessLogMode(u32 mode) {
        /* Use libnx bindings. */
        return ::fsSetGlobalAccessLogMode(mode);
    }

    void SetLocalAccessLog(bool enabled) {
        SetLocalAccessLogImpl(enabled);
    }

    void SetLocalApplicationAccessLog(bool enabled) {
        SetLocalAccessLogImpl(enabled);
    }

    void SetLocalSystemAccessLogForDebug(bool enabled) {
        #if defined(AMS_BUILD_FOR_DEBUGGING)
            if (enabled) {
                g_local_access_log_target |= (fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
            } else {
                g_local_access_log_target &= ~(fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
            }
        #endif
    }

}

namespace ams::fs::impl {

    const char *IdString::ToValueString(int id) {
        const int len = std::snprintf(this->buffer, sizeof(this->buffer), "%d", id);
        AMS_ASSERT(static_cast<size_t>(len) < sizeof(this->buffer));
        return this->buffer;
    }

    template<> const char *IdString::ToString<fs::Priority>(fs::Priority id) {
        switch (id) {
            case fs::Priority_Realtime: return "Realtime";
            case fs::Priority_Normal:   return "Normal";
            case fs::Priority_Low:      return "Low";
            default:                    return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::PriorityRaw>(fs::PriorityRaw id) {
        switch (id) {
            case fs::PriorityRaw_Realtime:   return "Realtime";
            case fs::PriorityRaw_Normal:     return "Normal";
            case fs::PriorityRaw_Low:        return "Low";
            case fs::PriorityRaw_Background: return "Realtime";
            default:                         return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::ContentStorageId>(fs::ContentStorageId id) {
        switch (id) {
            case fs::ContentStorageId::User:   return "User";
            case fs::ContentStorageId::System: return "System";
            case fs::ContentStorageId::SdCard: return "SdCard";
            default:                           return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::SaveDataSpaceId>(fs::SaveDataSpaceId id) {
        switch (id) {
            case fs::SaveDataSpaceId::System:       return "System";
            case fs::SaveDataSpaceId::User:         return "User";
            case fs::SaveDataSpaceId::SdSystem:     return "SdSystem";
            case fs::SaveDataSpaceId::ProperSystem: return "ProperSystem";
            default:                                return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::ContentType>(fs::ContentType id) {
        switch (id) {
            case fs::ContentType_Meta:    return "Meta";
            case fs::ContentType_Control: return "Control";
            case fs::ContentType_Manual:  return "Manual";
            case fs::ContentType_Logo:    return "Logo";
            case fs::ContentType_Data:    return "Data";
            default:                      return ToValueString(static_cast<int>(id));
        }
    }

    template<> const char *IdString::ToString<fs::BisPartitionId>(fs::BisPartitionId id) {
        switch (id) {
            case fs::BisPartitionId::BootPartition1Root:         return "BootPartition1Root";
            case fs::BisPartitionId::BootPartition2Root:         return "BootPartition2Root";
            case fs::BisPartitionId::UserDataRoot:               return "UserDataRoot";
            case fs::BisPartitionId::BootConfigAndPackage2Part1: return "BootConfigAndPackage2Part1";
            case fs::BisPartitionId::BootConfigAndPackage2Part2: return "BootConfigAndPackage2Part2";
            case fs::BisPartitionId::BootConfigAndPackage2Part3: return "BootConfigAndPackage2Part3";
            case fs::BisPartitionId::BootConfigAndPackage2Part4: return "BootConfigAndPackage2Part4";
            case fs::BisPartitionId::BootConfigAndPackage2Part5: return "BootConfigAndPackage2Part5";
            case fs::BisPartitionId::BootConfigAndPackage2Part6: return "BootConfigAndPackage2Part6";
            case fs::BisPartitionId::CalibrationBinary:          return "CalibrationBinary";
            case fs::BisPartitionId::CalibrationFile:            return "CalibrationFile";
            case fs::BisPartitionId::SafeMode:                   return "SafeMode";
            case fs::BisPartitionId::User:                       return "User";
            case fs::BisPartitionId::System:                     return "System";
            case fs::BisPartitionId::SystemProperEncryption:     return "SystemProperEncryption";
            case fs::BisPartitionId::SystemProperPartition:      return "SystemProperPartition";
            default:                                             return ToValueString(static_cast<int>(id));
        }
    }

    namespace {

        class AccessLogPrinterCallbackManager {
            private:
                AccessLogPrinterCallback callback;
            public:
                constexpr AccessLogPrinterCallbackManager() : callback(nullptr) { /* ... */ }

                constexpr bool IsRegisteredCallback() const { return this->callback != nullptr; }

                constexpr void RegisterCallback(AccessLogPrinterCallback c) {
                    AMS_ASSERT(this->callback == nullptr);
                    this->callback = c;
                }

                constexpr int InvokeCallback(char *buf, size_t size) const {
                    AMS_ASSERT(this->callback != nullptr);
                    return this->callback(buf, size);
                }
        };

        constinit AccessLogPrinterCallbackManager g_access_log_manager_printer_callback_manager;

        ALWAYS_INLINE AccessLogPrinterCallbackManager &GetStartAccessLogPrinterCallbackManager() {
            return g_access_log_manager_printer_callback_manager;
        }

        const char *GetPriorityRawName(fs::impl::IdString &id_string) {
            return id_string.ToString(fs::GetPriorityRawOnCurrentThreadInternal());
        }

        Result OutputAccessLogToSdCardImpl(const char *log, size_t size) {
            /* Use libnx bindings. */
            return ::fsOutputAccessLogToSdCard(log, size);
        }

        void OutputAccessLogToSdCard(const char *format, std::va_list vl) {
            if ((g_global_access_log_mode & AccessLogMode_SdCard) != 0) {
                /* Create a buffer to hold the log's input string. */
                int log_buffer_size = 1_KB;
                auto log_buffer = fs::impl::MakeUnique<char[]>(log_buffer_size);
                while (true) {
                    if (log_buffer == nullptr) {
                        return;
                    }

                    const auto size = std::vsnprintf(log_buffer.get(), log_buffer_size, format, vl);
                    if (size < log_buffer_size) {
                        break;
                    }

                    log_buffer_size = size + 1;
                    log_buffer = fs::impl::MakeUnique<char[]>(log_buffer_size);
                }

                /* Output. */
                OutputAccessLogToSdCardImpl(log_buffer.get(), log_buffer_size - 1);
            }
        }

        void OutputAccessLogImpl(const char *log, size_t size) {
            if ((g_global_access_log_mode & AccessLogMode_Log) != 0) {
                /* TODO: Support logging. */
            } else if ((g_global_access_log_mode & AccessLogMode_SdCard) != 0) {
                OutputAccessLogToSdCardImpl(log, size - 1);
            }
        }

        void OutputAccessLog(Result result, const char *priority, os::Tick start, os::Tick end, const char *name, const void *handle, const char *format, std::va_list vl) {
            /* Create a buffer to hold the log's input string. */
            int str_buffer_size = 1_KB;
            auto str_buffer = fs::impl::MakeUnique<char[]>(str_buffer_size);
            while (true) {
                if (str_buffer == nullptr) {
                    return;
                }

                const auto size = std::vsnprintf(str_buffer.get(), str_buffer_size, format, vl);
                if (size < str_buffer_size) {
                    break;
                }

                str_buffer_size = size + 1;
                str_buffer = fs::impl::MakeUnique<char[]>(str_buffer_size);
            }

            /* Create a buffer to hold the log. */
            int log_buffer_size = 0;
            decltype(str_buffer) log_buffer;
            {
                /* Declare format string. */
                constexpr const char FormatString[] = "FS_ACCESS { "
                                                      "start: %9" PRId64 ", "
                                                      "end: %9" PRId64 ", "
                                                      "result: 0x%08" PRIX32 ", "
                                                      "handle: 0x%p, "
                                                      "priority: %s, "
                                                      "function: \"%s\""
                                                      "%s"
                                                      " }\n";

                /* Convert the timing to ms. */
                const s64 start_ms = start.ToTimeSpan().GetMilliSeconds();
                const s64 end_ms   = end.ToTimeSpan().GetMilliSeconds();

                /* Print the log. */
                int try_size = std::max<int>(str_buffer_size + sizeof(FormatString) + 0x100, 1_KB);
                while (true) {
                    log_buffer = fs::impl::MakeUnique<char[]>(try_size);
                    if (log_buffer == nullptr) {
                        return;
                    }

                    log_buffer_size = 1 + std::snprintf(log_buffer.get(), try_size, FormatString, start_ms, end_ms, result.GetValue(), handle, priority, name, str_buffer.get());
                    if (log_buffer_size <= try_size) {
                        break;
                    }
                    try_size = log_buffer_size;
                }
            }

            OutputAccessLogImpl(log_buffer.get(), log_buffer_size);
        }

        void GetProgramIndexFortAccessLog(u32 *out_index, u32 *out_count) {
            if (hos::GetVersion() >= hos::Version_7_0_0) {
                /* Use libnx bindings if available. */
                R_ABORT_UNLESS(::fsGetProgramIndexForAccessLog(out_index, out_count));
            } else {
                /* Use hardcoded defaults. */
                *out_index = 0;
                *out_count = 0;
            }
        }

        void OutputAccessLogStart() {
            /* Get the program index. */
            u32 program_index = 0, program_count = 0;
            GetProgramIndexFortAccessLog(std::addressof(program_index), std::addressof(program_count));

            /* Print the log buffer. */
            if (program_count < 2) {
                constexpr const char StartLog[] = "FS_ACCESS: { "
                                                  AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                                  AMS_FS_IMPL_ACCESS_LOG_SPEC
                                                  " }\n";

                OutputAccessLogImpl(StartLog, sizeof(StartLog));
            } else {
                constexpr const char StartLog[] = "FS_ACCESS: { "
                                                  AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                                  AMS_FS_IMPL_ACCESS_LOG_SPEC ", "
                                                  "program_index: %d"
                                                  " }\n";

                char log_buffer[0x80];
                const int len = 1 + std::snprintf(log_buffer, sizeof(log_buffer), StartLog, static_cast<int>(program_index));
                if (static_cast<size_t>(len) <= sizeof(log_buffer)) {
                    OutputAccessLogImpl(log_buffer, len);
                }
            }
        }

        [[maybe_unused]] void OutputAccessLogStartForSystem() {
            constexpr const char StartLog[] = "FS_ACCESS: { "
                                              AMS_FS_IMPL_ACCESS_LOG_AMS_API_VERSION ", "
                                              AMS_FS_IMPL_ACCESS_LOG_SPEC ", "
                                              "for_system: true"
                                              " }\n";
            OutputAccessLogImpl(StartLog, sizeof(StartLog));
        }

        void OutputAccessLogStartGeneratedByCallback() {
            /* Get the manager. */
            const auto &manager = GetStartAccessLogPrinterCallbackManager();
            if (manager.IsRegisteredCallback()) {
                /* Invoke the callback. */
                char log_buffer[0x80];
                const int len = 1 + manager.InvokeCallback(log_buffer, sizeof(log_buffer));

                /* Print, if we fit. */
                if (static_cast<size_t>(len) <= sizeof(log_buffer)) {
                    OutputAccessLogImpl(log_buffer, len);
                }
            }
        }

    }

    bool IsEnabledAccessLog(u32 target) {
        /* If we don't need to log to the target, return false. */
        if ((g_local_access_log_target & target) == 0) {
            return false;
        }

        /* Ensure we've initialized. */
        if (!g_access_log_initialized) {
            std::scoped_lock lk(g_access_log_initialization_mutex);
            if (!g_access_log_initialized) {

                #if defined (AMS_BUILD_FOR_DEBUGGING)
                if ((g_local_access_log_target & fs::impl::AccessLogTarget_System) != 0)
                {
                    g_global_access_log_mode = AccessLogMode_Log;
                    OutputAccessLogStartForSystem();
                    OutputAccessLogStartGeneratedByCallback();
                }
                else
                #endif
                {
                    AMS_FS_R_ABORT_UNLESS(GetGlobalAccessLogMode(std::addressof(g_global_access_log_mode)));
                    if (g_global_access_log_mode != AccessLogMode_None) {
                        OutputAccessLogStart();
                        OutputAccessLogStartGeneratedByCallback();
                    }
                }

                g_access_log_initialized = true;
            }
        }

        return g_global_access_log_mode != AccessLogMode_None;
    }

    bool IsEnabledAccessLog() {
        return IsEnabledAccessLog(fs::impl::AccessLogTarget_Application | fs::impl::AccessLogTarget_System);
    }

    void RegisterStartAccessLogPrinterCallback(AccessLogPrinterCallback callback) {
        GetStartAccessLogPrinterCallbackManager().RegisterCallback(callback);
    }

    void OutputAccessLog(Result result, fs::Priority priority, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLog(result, fs::impl::IdString().ToString(priority), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, fs::PriorityRaw priority_raw, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...){
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLog(result, fs::impl::IdString().ToString(priority_raw), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::impl::IdentifyAccessLogHandle handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        fs::impl::IdString id_string;
        OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle, fmt, vl);
        va_end(vl);
    }

    void OutputAccessLogToOnlySdCard(const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        OutputAccessLogToSdCard(fmt, vl);
        va_end(vl);
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
            va_end(vl);
        }
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle.handle, fmt, vl);
            va_end(vl);
        }
    }

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) {
        if (R_FAILED(result)) {
            std::va_list vl;
            va_start(vl, fmt);
            fs::impl::IdString id_string;
            OutputAccessLog(result, GetPriorityRawName(id_string), start, end, name, handle, fmt, vl);
            va_end(vl);
        }
    }

    bool IsEnabledHandleAccessLog(fs::FileHandle handle) {
        /* Get the file accessor. */
        impl::FileAccessor *accessor = reinterpret_cast<impl::FileAccessor *>(handle.handle);
        if (accessor == nullptr) {
            return true;
        }

        /* Check the parent. */
        if (auto *parent = accessor->GetParent(); parent != nullptr) {
            return parent->IsEnabledAccessLog();
        } else {
            return false;
        }
    }

    bool IsEnabledHandleAccessLog(fs::DirectoryHandle handle) {
        /* Get the file accessor. */
        impl::DirectoryAccessor *accessor = reinterpret_cast<impl::DirectoryAccessor *>(handle.handle);
        if (accessor == nullptr) {
            return true;
        }

        /* Check the parent. */
        if (auto *parent = accessor->GetParent(); parent != nullptr) {
            return parent->IsEnabledAccessLog();
        } else {
            return false;
        }
    }

    bool IsEnabledHandleAccessLog(fs::impl::IdentifyAccessLogHandle handle) {
        return true;
    }

    bool IsEnabledHandleAccessLog(const void *handle) {
        if (handle == nullptr) {
            return true;
        }

        /* We should never receive non-null here. */
        AMS_ASSERT(handle == nullptr);
        return false;
    }

    bool IsEnabledFileSystemAccessorAccessLog(const char *mount_name) {
        /* Get the accessor. */
        impl::FileSystemAccessor *accessor;
        if (R_FAILED(impl::Find(std::addressof(accessor), mount_name))) {
            return true;
        }

        return accessor->IsEnabledAccessLog();
    }

    void EnableFileSystemAccessorAccessLog(const char *mount_name) {
        /* Get the accessor. */
        impl::FileSystemAccessor *accessor;
        AMS_FS_R_ABORT_UNLESS(impl::Find(std::addressof(accessor), mount_name));
        accessor->SetAccessLogEnabled(true);
    }

}
