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
#pragma once
#include <vapours.hpp>
#include <stratosphere/fs/fs_access_log.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/fs/fs_file.hpp>
#include <stratosphere/fs/fs_priority.hpp>
#include <stratosphere/os/os_tick.hpp>

namespace ams::fs::impl {

    /* ACCURATE_TO_VERSION: Unknown */
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
            char m_buffer[0x20];
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

    static_assert(sizeof(size_t) == sizeof(u64));

}

/* Access log result name. */
#define AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME __tmp_ams_fs_access_log_result
/* Access log utils. */
#define AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__VALUE__) ::ams::fs::impl::DereferenceOutValue(__VALUE__, AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME)

/* Access log components. */
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SIZE                   ", size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_READ_SIZE              ", read_size: %" PRIuZ ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_QUERY_SIZE             ", read_size: %" PRIuZ ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_OFFSET_AND_SIZE        ", offset: %" PRId64 ", size: %" PRIuZ ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_THREAD_ID              ", thread_id: %" PRIu64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT                  ", name: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_COUNT            ", entry_count: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_ENTRY_BUFFER_COUNT     ", entry_buffer_count: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_OPEN_MODE              ", open_mode: 0x%" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH                   ", path: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_SIZE          ", path: \"%s\", size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH_AND_OPEN_MODE     ", path: \"%s\", open_mode: 0x%" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_RENAME                 ", path: \"%s\", new_path: \"%s\""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_DIRECTORY_ENTRY_TYPE   ", entry_type: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_TYPE           ", content_type: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_OPTION      ", mount_host_option: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_ROOT_PATH              ", root_path: %s"

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_APPLICATION_ID         ", applicationid: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_BIS_PARTITION_ID       ", bispartitionid: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_STORAGE_ID     ", contentstorageid: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SYSTEM_DATA_ID         ", systemdataid: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_DATA_ID                ", dataid: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GAME_CARD_HANDLE       ", gamecard_handle: 0x%" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_GAME_CARD_PARTITION    ", gamecard_partition: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_IMAGE_DIRECTORY_ID     ", imagedirectoryid: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_PROGRAM_ID             ", programid: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID           ", savedataid: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID     ", savedataspaceid: %s"
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_OWNER_ID     ", save_data_owner_id: 0x%" PRIx64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_USER_ID                ", userid: 0x%016" PRIx64 "%016" PRIx64 ""

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_FLAGS        ", save_data_flags: 0x%08" PRIX32 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_JOURNAL_SIZE ", save_data_journal_size: %" PRId64 ""
#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SIZE         ", save_data_size: %" PRId64 ""

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

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_APPLICATION_PACKAGE(__NAME__, __PATH__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, (__NAME__), (__PATH__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_BIS(__NAME__, __ID__, __PATH__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_BIS_PARTITION_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH, (__NAME__), ::ams::fs::impl::IdString().ToString(__ID__), (__PATH__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CODE(__NAME__, __PATH__, __ID__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_PROGRAM_ID, (__NAME__), (__PATH__), (__ID__).value

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH(__NAME__, __PATH__, __TYPE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_TYPE, (__NAME__), (__PATH__), ::ams::fs::impl::IdString().ToString(__TYPE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PROGRAM_ID(__NAME__, __ID__, __TYPE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PROGRAM_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_TYPE, (__NAME__), (__ID__), ::ams::fs::impl::IdString().ToString(__TYPE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH_AND_PROGRAM_ID(__NAME__, __PATH__, __ID__, __TYPE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_PROGRAM_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_TYPE, (__NAME__), (__PATH__), (__ID__).value, ::ams::fs::impl::IdString().ToString(__TYPE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_PATH_AND_DATA_ID(__NAME__, __PATH__, __ID__, __TYPE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_TYPE, (__NAME__), (__PATH__), (__ID__).value, ::ams::fs::impl::IdString().ToString(__TYPE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_CONTENT_STORAGE(__NAME__, __ID__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_CONTENT_STORAGE_ID, (__NAME__), ::ams::fs::impl::IdString().ToString(__ID__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_DEVICE_SAVE_DATA_APPLICATION_ID(__NAME__, __ID__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_APPLICATION_ID, (__NAME__), (__ID__).value

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_GAME_CARD_PARTITION(__NAME__, __GCHANDLE__, __PARTITION__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_GAME_CARD_HANDLE AMS_FS_IMPL_ACCESS_LOG_FORMAT_GAME_CARD_PARTITION, (__NAME__), __GCHANDLE__, ::ams::fs::impl::IdString().ToString(__PARTITION__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT() \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT, (AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT_WITH_OPTION(__OPTION__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_OPTION, (AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME), ::ams::fs::impl::IdString().ToString(__OPTION__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST(__NAME__, __ROOT_PATH__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_ROOT_PATH, (__NAME__), (__ROOT_PATH__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_WITH_OPTION(__NAME__, __ROOT_PATH__, __OPTION__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_ROOT_PATH AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_OPTION, (__NAME__), (__ROOT_PATH__), ::ams::fs::impl::IdString().ToString(__OPTION__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_IMAGE_DIRECTORY(__NAME__, __ID__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_IMAGE_DIRECTORY_ID, (__NAME__), ::ams::fs::impl::IdString().ToString(__ID__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_SYSTEM_DATA(__NAME__, __ID__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_SYSTEM_DATA_ID, (__NAME__), (__ID__).value

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_SYSTEM_SAVE_DATA(__NAME__, __SPACE__, __SAVE__, __USER__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_USER_ID, \
    (__NAME__), ::ams::fs::impl::IdString().ToString(__SPACE__), (__SAVE__), (__USER__).data[0], (__USER__).data[1]

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_CREATE_SYSTEM_SAVE_DATA(__SPACE__, __SAVE__, __USER__, __OWNER__, __SIZE__, __JOURNAL_SIZE__, __FLAGS__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_USER_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_OWNER_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SIZE AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_JOURNAL_SIZE AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_FLAGS, \
    ::ams::fs::impl::IdString().ToString(__SPACE__), (__SAVE__), (__USER__).data[0], (__USER__).data[1], (__OWNER__), (__SIZE__), (__JOURNAL_SIZE__), (__FLAGS__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_DELETE_SAVE_DATA(__SPACE__, __SAVE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID, \
    ::ams::fs::impl::IdString().ToString(__SPACE__), (__SAVE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_DELETE_SYSTEM_SAVE_DATA(__SPACE__, __SAVE__, __USER__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_USER_ID, \
    ::ams::fs::impl::IdString().ToString(__SPACE__), (__SAVE__), (__USER__).data[0], (__USER__).data[1]

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_EXTEND_SAVE_DATA(__SPACE__, __SAVE__, __SIZE__, __JOURNAL_SIZE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SPACE_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_SIZE AMS_FS_IMPL_ACCESS_LOG_FORMAT_SAVE_DATA_JOURNAL_SIZE, \
    ::ams::fs::impl::IdString().ToString(__SPACE__), (__SAVE__), (__SIZE__), (__JOURNAL_SIZE__)

#define AMS_FS_IMPL_ACCESS_LOG_FORMAT_QUERY_MOUNT_SYSTEM_DATA_CACHE_SIZE(__ID__, __SIZE__) \
    AMS_FS_IMPL_ACCESS_LOG_FORMAT_SYSTEM_DATA_ID AMS_FS_IMPL_ACCESS_LOG_FORMAT_QUERY_SIZE, (__ID__).value, AMS_FS_IMPL_ACCESS_LOG_DEREFERENCE_OUT_VALUE(__SIZE__)

/* Access log invocation lambdas. */
#define AMS_FS_IMPL_ACCESS_LOG_IMPL(__EXPR__, __HANDLE__, __ENABLED__, __NAME__, ...)                                                                       \
    [&](const char *__fs_func_name_) -> Result {                                                                                                            \
        if (!(__ENABLED__)) {                                                                                                                               \
            R_RETURN(__EXPR__);                                                                                                                             \
        } else {                                                                                                                                            \
            const ::ams::os::Tick __fs_start_tick = ::ams::os::GetSystemTick();                                                                             \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                                                     \
            const ::ams::os::Tick __fs_end_tick = ::ams::os::GetSystemTick();                                                                               \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __fs_start_tick, __fs_end_tick, __fs_func_name_, __HANDLE__, __VA_ARGS__); \
            R_RETURN( AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME );                                                                                                 \
        }                                                                                                                                                   \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_WITH_PRIORITY_IMPL(__EXPR__, __PRIORITY__, __HANDLE__, __ENABLED__, __NAME__, ...)                                                         \
    [&](const char *__fs_func_name_) -> Result {                                                                                                                          \
        if (!(__ENABLED__)) {                                                                                                                                             \
            R_RETURN(__EXPR__);                                                                                                                                           \
        } else {                                                                                                                                                          \
            const ::ams::os::Tick __fs_start_tick = ::ams::os::GetSystemTick();                                                                                           \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                                                                   \
            const ::ams::os::Tick __fs_end_tick = ::ams::os::GetSystemTick();                                                                                             \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __PRIORITY__, __fs_start_tick, __fs_end_tick, __fs_func_name_, __HANDLE__, __VA_ARGS__); \
            R_RETURN( AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME );                                                                                                               \
        }                                                                                                                                                                 \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_EXPLICIT_IMPL(__RESULT__, __START__, __END__, __HANDLE__, __ENABLED__, __NAME__, ...)                            \
    [&](const char *__fs_func_name_) -> Result {                                                                                                \
        if (!(__ENABLED__)) {                                                                                                                   \
            R_RETURN(__RESULT__);                                                                                                               \
        } else {                                                                                                                                \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__RESULT__);                                                                       \
            ::ams::fs::impl::OutputAccessLog(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __START__, __END__, __fs_func_name_, __HANDLE__, __VA_ARGS__); \
            R_RETURN( AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME );                                                                                     \
        }                                                                                                                                       \
    }(__NAME__)

#define AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL(__EXPR__, __ENABLED__, __NAME__, ...)                                                                                \
    [&](const char *__fs_func_name_) -> Result {                                                                                                                            \
        if (!(__ENABLED__)) {                                                                                                                                               \
            R_RETURN(__EXPR__);                                                                                                                                             \
        } else {                                                                                                                                                            \
            const ::ams::os::Tick __fs_start_tick = ::ams::os::GetSystemTick();                                                                                             \
            const auto AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME = (__EXPR__);                                                                                                     \
            const ::ams::os::Tick __fs_end_tick = ::ams::os::GetSystemTick();                                                                                               \
            ::ams::fs::impl::OutputAccessLogUnlessResultSuccess(AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME, __fs_start_tick, __fs_end_tick, __fs_func_name_, nullptr, __VA_ARGS__); \
            R_RETURN( AMS_FS_IMPL_ACCESS_LOG_RESULT_NAME );                                                                                                                 \
        }                                                                                                                                                                   \
    }(__NAME__)

/* Access log api. */
#define AMS_FS_IMPL_ACCESS_LOG(__EXPR__, __HANDLE__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_SYSTEM(__EXPR__, __HANDLE__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_System) && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_WITH_NAME(__EXPR__, __HANDLE__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_EXPLICIT(__RESULT__, __START__, __END__, __HANDLE__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_EXPLICIT_IMPL((__RESULT__), __START__, __END__, __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledHandleAccessLog(__HANDLE__), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED(__EXPR__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL((__EXPR__), ::ams::fs::impl::IsEnabledAccessLog(), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

/* FS Accessor logging. */
#define AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE_IMPL(__NAME__, __ENABLED__) \
    do {                                                                          \
        if (static_cast<bool>(__ENABLED__)) {                                     \
            ::ams::fs::impl::EnableFileSystemAccessorAccessLog((__NAME__));       \
        }                                                                         \
    } while (false)

#define AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(__NAME__) \
    AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE_IMPL((__NAME__), ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_Application))

// DEBUG
#define AMS_FS_FORCE_ENABLE_SYSTEM_MOUNT_ACCESS_LOG

/* System access log api. */
#if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING) || defined(AMS_FS_FORCE_ENABLE_SYSTEM_MOUNT_ACCESS_LOG)

#define AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(__EXPR__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), nullptr, ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_System), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(__NAME__) \
    AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE_IMPL((__NAME__), ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_System))

#else

#define AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(__EXPR__, __NAME__, ...) (__EXPR__)

#define AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(__NAME__) static_cast<void>(0)

#endif

/* Specific utilities. */
#define AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM(__EXPR__, __HANDLE__, __FILESYSTEM__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && (__FILESYSTEM__)->IsEnabledAccessLog(), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_FILESYSTEM_WITH_NAME(__EXPR__, __HANDLE__, __FILESYSTEM__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), __HANDLE__, ::ams::fs::impl::IsEnabledAccessLog() && (__FILESYSTEM__)->IsEnabledAccessLog(), __NAME__, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_MOUNT(__EXPR__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), nullptr, ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_Application), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(__EXPR__, __NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_UNLESS_R_SUCCEEDED_IMPL((__EXPR__), ::ams::fs::impl::IsEnabledAccessLog(::ams::fs::impl::AccessLogTarget_Application), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)

#define AMS_FS_IMPL_ACCESS_LOG_UNMOUNT(__EXPR__, __MOUNT_NAME__, ...) \
    AMS_FS_IMPL_ACCESS_LOG_IMPL((__EXPR__), nullptr, ::ams::fs::impl::IsEnabledAccessLog() && ::ams::fs::impl::IsEnabledFileSystemAccessorAccessLog(__MOUNT_NAME__), AMS_CURRENT_FUNCTION_NAME, __VA_ARGS__)
