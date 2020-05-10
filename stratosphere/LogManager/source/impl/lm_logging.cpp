#include "lm_logging.hpp"

namespace ams::lm::impl {

    namespace {

        LogInfo g_last_log_info;
        bool g_can_access_fs = true;
        os::Mutex g_logging_lock(true);
        os::SystemEvent g_logging_event(os::EventClearMode_AutoClear, true);

        void UpdateLastLogInfo(LogInfo info) {
            std::scoped_lock lk(g_logging_lock);
            g_last_log_info = info;
        }

        bool CanAccessFs() {
            std::scoped_lock lk(g_logging_lock);
            return g_can_access_fs;
        }

        void SignalLogEvent() {
            std::scoped_lock lk(g_logging_lock);
            g_logging_event.Signal();
        }

    }

    void SetCanAccessFs(bool can_access) {
        std::scoped_lock lk(g_logging_lock);
        g_can_access_fs = can_access;
    }

    Result WriteLogPackets(std::vector<LogPacketBuffer> &packet_list) {
        std::scoped_lock lk(g_logging_lock);
        if(CanAccessFs() && !packet_list.empty()) {
            /* Ensure log directory exists. */
            fs::CreateDirectory(DebugLogDirectory);

            const auto program_id = packet_list.front().GetProgramId();

            /* Ensure process's directory for debug logs exists. */
            char process_dir[FS_MAX_PATH] = {};
            std::snprintf(process_dir, sizeof(process_dir), "%s/0x%016lX", DebugLogDirectory, program_id);
            fs::CreateDirectory(process_dir);
            
            /* Use current system tick as the binary log file's name. */
            const LogInfo info = {
                .log_id = os::GetSystemTick().GetInt64Value(),
                .program_id = program_id,
            };

            /* Generate binary log path. */
            /* All current log packets will be written to the same file. */
            char bin_log_path[FS_MAX_PATH] = {};
            std::snprintf(bin_log_path, sizeof(bin_log_path), "%s/0x%016lX.bin", process_dir, info.log_id);

            /* Ensure the file exists. */
            fs::CreateFile(bin_log_path, 0);

            /* Now, open the file. */
            fs::FileHandle bin_log_file;
            R_TRY(fs::OpenFile(&bin_log_file, bin_log_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
            ON_SCOPE_EXIT { fs::CloseFile(bin_log_file); };
            
            s64 offset = 0;
            for(const auto &packet_buf: packet_list) {
                /* Write each packet. Don't write the entire buffer since it might have garbage from previous packets at the end. */
                const size_t size = packet_buf.GetPacketSize();
                R_TRY(fs::WriteFile(bin_log_file, offset, packet_buf.buf.get(), size, fs::WriteOption::Flush));
                offset += size;
            }

            /* Update the last log's info and signal log event. */
            UpdateLastLogInfo(info);
            SignalLogEvent();
        }
        return ResultSuccess();
    }

    LogInfo GetLastLogInfo() {
        std::scoped_lock lk(g_logging_lock);
        return g_last_log_info;
    }

    Handle GetLogEventHandle() {
        std::scoped_lock lk(g_logging_lock);
        return g_logging_event.GetReadableHandle();
    }

}