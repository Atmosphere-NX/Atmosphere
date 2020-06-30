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

}

/* Access log components. */
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE            ", size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE ", offset: %" PRId64 ", size: %zu"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID       ", thread_id: %" PRIu64 ""

/* Access log formats. */
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_NONE                         ""

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION    AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_FLUSH_OPTION AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION ", write_option: Flush"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE(__OPTION__)       ((__OPTION__).HasFlushFlag() ? AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_FLUSH_OPTION : AMS_FS_IMPL_ACCESS_LOG_FORMAT_WRITE_FILE_WITH_NO_OPTION)

/* Access log invocation lambdas. */
#define AMS_FS_IMPL_ACCESS_LOG_IMPL(__EXPR__, __HANDLE__, __ENABLED__, __NAME__, ...)            \
    [&](const char *name) {                                                                      \
        if (!(__ENABLED__)) {                                                                    \
            return (__EXPR__);                                                                   \
        } else {                                                                                 \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                            \
            const auto result = (__EXPR__);                                                      \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                              \
            ::ams::fs::impl::OutputAccessLog(result, start, end, name, __HANDLE__, __VA_ARGS__); \
            return result;                                                                       \
        }                                                                                        \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_WITH_PRIORITY_IMPL(__EXPR__, __PRIORITY__, __HANDLE__, __ENABLED__, __NAME__, ...) \
    [&](const char *name) {                                                                                       \
        if (!(__ENABLED__)) {                                                                                     \
            return (__EXPR__);                                                                                    \
        } else {                                                                                                  \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                                             \
            const auto result = (__EXPR__);                                                                       \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                                               \
            ::ams::fs::impl::OutputAccessLog(result, __PRIORITY__, start, end, name, __HANDLE__, __VA_ARGS__);    \
            return result;                                                                                        \
        }                                                                                                         \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_EXPLICIT_IMPL(__RESULT__, __START__, __END__, __HANDLE__, __ENABLED__, __NAME__, ...) \
    [&](const char *name) {                                                                                          \
        if (!(__ENABLED__)) {                                                                                        \
            return __RESULT__;                                                                                       \
        } else {                                                                                                     \
            ::ams::fs::impl::OutputAccessLog(__RESULT__, __START__, __END__, name, __HANDLE__, __VA_ARGS__);         \
            return __RESULT__;                                                                                       \
        }                                                                                                            \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL(__EXPR__, __ENABLED__, __NAME__, ...)                     \
    [&](const char *name) {                                                                                      \
        if (!(__ENABLED__)) {                                                                                    \
            return (__EXPR__);                                                                                   \
        } else {                                                                                                 \
            const ::ams::os::Tick start = ::ams::os::GetSystemTick();                                            \
            const auto result = (__EXPR__);                                                                      \
            const ::ams::os::Tick end = ::ams::os::GetSystemTick();                                              \
            ::ams::fs::impl::OutputAccessLogUnlessResultSuccess(result, start, end, name, nullptr, __VA_ARGS__); \
            return result;                                                                                       \
        }                                                                                                        \
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
