#include "lm_a_logger.hpp"
#include "../detail/lm_log_packet.hpp"

namespace ams::lm::impl {

    namespace {

        bool PrepareLogFile(char *out_path, size_t len, const char *log_dir) {
            char log_file[0x80] = {};
            auto time_point = time::StandardUserSystemClock::now();
            const auto posix_time = time::StandardUserSystemClock::to_time_t(time_point);
            if (posix_time != 0) {
                auto local_time = std::localtime(&posix_time);

                settings::system::SerialNumber serial_number;
                settings::system::GetSerialNumber(&serial_number);
                /* Log file format: sdmc:/<log_dir>/<serial_no>_<year><month><day><hour><min><sec>[_<extra_index>].nxbinlog */
                /* The extra index is used when multiple logs are logged at the same time */
                const auto log_file_len = static_cast<size_t>(snprintf(log_file, sizeof(log_file), "%s:/%s/%s_%04d%02d%02d%02d%02d%02d", "sdmc", log_dir, serial_number.str, local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec));
                if (log_file_len <= sizeof(log_dir)) {
                    auto tmp_index = 1;
                    while(true) {
                        /* Only add the index if we couldn't create the default file name. */
                        if (tmp_index == 1) {
                            const auto log_file_ext_len = static_cast<size_t>(snprintf(out_path, len, "%s.%s", log_file, "nxbinlog"));
                            if (log_file_ext_len >= len) {
                                return false;
                            }
                        }
                        else {
                            const auto log_file_ext_len = static_cast<size_t>(snprintf(out_path, len, "%s_%d.%s", log_file, tmp_index, "nxbinlog"));
                            if (log_file_ext_len >= len) {
                                return false;
                            }
                        }

                        auto rc = fs::CreateFile(out_path, 0);
                        if (R_SUCCEEDED(rc)) {
                            break;
                        }
                        else if (fs::ResultPathAlreadyExists::Includes(rc) && (++tmp_index < 100)) {
                            /* If the log file already exists (multiple logs at the same time), retry over 100 times and increase the index */
                            continue;
                        }

                        return false;
                    }
                    return true;
                }
            }
            return false;
        }

        inline bool IsSdCardLoggingEnabled() {
            bool logging_enabled = false;
            auto enable_logging_size = settings::fwdbg::GetSettingsItemValue(&logging_enabled, sizeof(logging_enabled), "lm", "enable_sd_card_logging");
            return (enable_logging_size == sizeof(logging_enabled)) && logging_enabled;
        }

        inline bool EnsureLogDirectory(const char *log_dir) {
            /* Try to create the log directory. */
            /* If the directory already exists, there's no issue. */
            const auto result = fs::CreateDirectory(log_dir);
            return R_SUCCEEDED(result) || fs::ResultPathAlreadyExists::Includes(result);
        }

    }

    void ALogger::Dispose() {
        if (this->some_flag) {
            this->some_flag = false;
            this->CallSomeFunction(false);
        }
        if (this->sd_card_mounted) {
            fs::Unmount("sdmc");
            this->sd_card_mounted = false;
        }
    }

    bool ALogger::EnsureLogFile() {
        if (this->some_flag) {
            return true;
        }

        bool unk_1 = false;
        bool unk_2 = false; /* TODO: what are these? */
        if (unk_1) {
            this->some_flag_3 = false;
        }

        if (unk_2) {
            if (!this->some_flag_3 && R_SUCCEEDED(fs::MountSdCard("sdmc"))) {
                this->sd_card_mounted = true;
                auto log_dir_size = settings::fwdbg::GetSettingsItemValueSize("lm", "sd_card_log_output_directory");
                char log_dir[0x80] = {};
                if (log_dir_size <= sizeof(log_dir)) {
                    if (settings::fwdbg::GetSettingsItemValue(log_dir, sizeof(log_dir), "lm", "sd_card_log_output_directory") == log_dir_size) {
                        char log_dir_path[0x80] = {};
                        const auto log_dir_path_len = static_cast<size_t>(snprintf(log_dir_path, sizeof(log_dir_path), "%s:/%s", "sdmc", log_dir));
                        if (log_dir_path_len < sizeof(log_dir_path)) {
                            if (EnsureLogDirectory(log_dir_path)) {
                                if (PrepareLogFile(this->log_file_path, sizeof(this->log_file_path), log_dir_path)) {
                                    fs::FileHandle log_file_h;
                                    if (R_SUCCEEDED(fs::OpenFile(&log_file_h, this->log_file_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                                        ON_SCOPE_EXIT { fs::CloseFile(log_file_h); };
                                        
                                        /* 8-byte header, current version is 1 */
                                        detail::LogBinaryHeader bin_header = { detail::LogBinaryHeader::Magic, 1 };
                                        if (R_SUCCEEDED(fs::WriteFile(log_file_h, 0, &bin_header, sizeof(bin_header), fs::WriteOption::Flush))) {
                                            this->log_file_offset = sizeof(bin_header);
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    bool ALogger::SaveLog(void *buf, size_t size) {
        static bool should_log = IsSdCardLoggingEnabled();
        auto ret_val = false;
        if (!should_log) {
            return ret_val;
        }
        
        if (this->EnsureLogFile()) {
            fs::FileHandle log_file_h;
            if(R_SUCCEEDED(fs::OpenFile(&log_file_h, this->log_file_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                ON_SCOPE_EXIT { fs::CloseFile(log_file_h); };
                /* Write log data. */
                if (R_SUCCEEDED(fs::WriteFile(log_file_h, this->log_file_offset, buf, size, fs::WriteOption::Flush))) {
                    this->log_file_offset += size;
                    ret_val = true;
                    if (this->some_flag) {
                        return ret_val;
                    }
                }
            }
        }
        if (this->sd_card_mounted) {
            fs::Unmount("sdmc");
            this->sd_card_mounted = false;
            this->some_flag_3 = true;
            // loc_71000388E0 (something related to SD card event notifier)
        }
        if (this->some_flag) {
            this->some_flag = ret_val;
            this->CallSomeFunction(ret_val);
        }
        return ret_val;
    }

    ALogger *GetALogger() {
        static ALogger a_logger;
        return &a_logger;
    }

}