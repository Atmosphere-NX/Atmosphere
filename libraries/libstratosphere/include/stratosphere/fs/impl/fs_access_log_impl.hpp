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
#pragma once
#include <vapours.hpp>
#include <stratosphere/fs/fs_access_log.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_priority.hpp>
#include <stratosphere/os/os_tick.hpp>

namespace ams::fs::impl {

    enum AccessLogTarget : u32 {
        AccessLogTarget_None        = (0 << 0),
        AccessLogTarget_Application = (1 << 0),
        AccessLogTarget_System      = (1 << 1),
    };

    struct IdentifyAccessLogHandle {
        void *handle;
        public:
            static constexpr IdentifyAccessLogHandle MakeHandle(void *h) {
                return IdentifyAccessLogHandle{h};
            }
    };

    bool IsEnabledAccessLog(u32 target);
    bool IsEnabledAccessLog();

    bool IsEnabledHandleAccessLog(fs::FileHandle handle);
    bool IsEnabledHandleAccessLog(fs::DirectoryHandle handle);
    bool IsEnabledHandleAccessLog(fs::impl::IdentifyAccessLogHandle handle);
    bool IsEnabledHandleAccessLog(const void *handle);

    bool IsEnabledFileSystemAccessorAccessLog(const char *mount_name);
    void EnableFileSystemAccessorAccessLog(const char *mount_name);

    using AccessLogPrinterCallback = int (*)(char *buffer, size_t buffer_size);
    void RegisterStartAccessLogPrinterCallback(AccessLogPrinterCallback callback);

    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, fs::impl::IdentifyAccessLogHandle handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLog(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLog(Result result, fs::Priority priority, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) __attribute__((format (printf, 7, 8)));
    void OutputAccessLog(Result result, fs::PriorityRaw priority_raw, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) __attribute__((format (printf, 7, 8)));

    void OutputAccessLogToOnlySdCard(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::FileHandle handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, fs::DirectoryHandle handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));
    void OutputAccessLogUnlessResultSuccess(Result result, os::Tick start, os::Tick end, const char *name, const void *handle, const char *fmt, ...) __attribute__((format (printf, 6, 7)));

    class IdString {
        private:
            char buffer[0x20];
        private:
            const char *ToValueString(int id);
        public:
            template<typename T>
            const char *ToString(T id);
    };

    template<typename T> requires (requires { T{}; })
    inline T DereferenceOutValue(T *out_value, Result result) {
        if (R_SUCCEEDED(result) && out_value != nullptr) {
            return *out_value;
        } else {
            return T{};
        }
    }

}

/* Access log result name. */
#define AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME __tmp_ams_fs_access_log_result
/* Access log utils. */
#define AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__VALUE__) ::ams::fs::impl::DereferenceOutValue(__VALUE__, AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME)

/* Access log components. */
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE                  ", size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_SIZE             ", read_size: %zu"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE       ", offset: %" PRId64 ", size: %zu"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID             ", thread_id: %" PRIu64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT                 ", name: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_COUNT           ", entry_count: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_BUFFER_COUNT    ", entry_buffer_count: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_OPEN_MODE             ", open_mode: 0x%" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH                  ", path: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_SIZE         ", path: \"%s\", size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE    ", path: \"%s\", open_mode: 0x%" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME                ", path: \"%s\", new_path: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_DIRECTORY_ENTRY_TYPE  ", entry_type: %s"

/* Access log formats. */
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE ""

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_FILE(__OUT_READ_SIZE__, __OFFSET__, __SIZE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_SIZE, __OFFSET__, __SIZE__, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_READ_SIZE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION    AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_FLUSH_OPTION AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION ", write_option: Flush"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE(__OPTION__)       ((__OPTION__).HasFlushFlag() ? AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_FLUSH_OPTION : AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_FILE_SIZE(__OUT_SIZE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_SIZE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_DIRECTORY(__OUT_ENTRY_COUNT__, __ENTRY_BUFFER_COUNT__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_BUFFER_COUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_COUNT, __ENTRY_BUFFER_COUNT__, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_ENTRY_COUNT__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_DIRECTORY_ENTRY_COUNT(__OUT_ENTRY_COUNT__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_COUNT, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_ENTRY_COUNT__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_ENTRY_TYPE(__OUT_ENTRY_TYPE__, __PATH__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_DIRECTORY_ENTRY_TYPE, __PATH__, ::ams::fs::impl::IdString().ToString(AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_ENTRY_TYPE__))

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GET_SPACE_SIZE(__OUT_SIZE__, __NAME__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE, __NAME__, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__OUT_SIZE__)

/* Access log invocation lambdas. */
#define AMS_FS_IMPL_ACCESS_LOG_IMPL(__EXPR__, __HANDLE__, __ENABLED__, __NAME__, ...)                                        \
    [&](const char *name) {                                                                                                  \
        if (!(__ENABLED__)) {                                                                                                \
            return (__EXPR__);                                                                                               \
        } else {                                                                                                             \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                                                        \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                      \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                                                          \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, start, end, name, __HANDLE__, __VA_ARGS__); \
            return AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME;                                                                       \
        }                                                                                                                    \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_WITH_PRIORITY_IMPL(__EXPR__, __PRIORITY__, __HANDLE__, __ENABLED__, __NAME__, ...)                           \
    [&](const char *name) {                                                                                                                 \
        if (!(__ENABLED__)) {                                                                                                               \
            return (__EXPR__);                                                                                                              \
        } else {                                                                                                                            \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                                                                       \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                                     \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                                                                         \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __PRIORITY__, start, end, name, __HANDLE__, __VA_ARGS__);  \
            return AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME;                                                                                      \
        }                                                                                                                                   \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_EXPLICIT_IMPL(__RESULT__, __START__, __END__, __HANDLE__, __ENABLED__, __NAME__, ...)                 \
    [&](const char *name) {                                                                                                          \
        if (!(__ENABLED__)) {                                                                                                        \
            return __RESULT__;                                                                                                       \
        } else {                                                                                                                     \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__RESULT__);                                                            \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __START__, __END__, name, __HANDLE__, __VA_ARGS__); \
            return AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME;                                                                               \
        }                                                                                                                            \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL(__EXPR__, __ENABLED__, __NAME__, ...)                                                 \
    [&](const char *name) {                                                                                                                  \
        if (!(__ENABLED__)) {                                                                                                                \
            return (__EXPR__);                                                                                                               \
        } else {                                                                                                                             \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                                                                        \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                                      \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                                                                          \
            ::ams::fs::impl::OutputAccessLogUnlessResultSuccess(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, start, end, name, nullptr, __VA_ARGS__); \
            return AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME;                                                                                       \
        }                                                                                                                                    \
    }(__NAME__)


/* Access log api. */
#define AMS_FS_IMPL_ACCESS_LOG(__EXPR__, __HANDLE__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_WITH_NAME(__EXPR__, __HANDLE__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_EXPLICIT(__RESULT__, __START__, __END__, __HANDLE__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_EXPLICIT_IMPL((__RESULT__), __START__, __END__, __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(__EXPR__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL((__EXPR__), ::ams::fs::impl::IsEnabledAccessLog(), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

/* Specific utilities. */
#define AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(__EXPR__, __HANDLE__, __FILESYSTEM__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && (__FILESYSTEM__)->IsEnabledAccessLog(), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM_WITH_NAME(__EXPR__, __HANDLE__, __FILESYSTEM__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && (__FILESYSTEM__)->IsEnabledAccessLog(), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_UNMOUNT(__EXPR__, __MOUNT_NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), nullptr, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledFileSystemAccessorAccessLog(__MOUNT_NAME__), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)
