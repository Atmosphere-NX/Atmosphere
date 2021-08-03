#include "lm_sd_card_logging.hpp"

namespace ams::lm::impl {

    namespace {

        os::SystemEvent g_sd_card_detection_event;

        void UpdateSdCardStatus(bool *out_is_sd_inserted, bool *out_detection_changed) {
            std::unique_ptr<fs::IEventNotifier> sd_card_detection_event_notifier;
            R_ABORT_UNLESS(fs::OpenSdCardDetectionEventNotifier(std::addressof(sd_card_detection_event_notifier)));
            R_ABORT_UNLESS(sd_card_detection_event_notifier->BindEvent(g_sd_card_detection_event.GetBase(), os::EventClearMode_ManualClear));
            static bool is_sd_card_inserted = fs::IsSdCardInserted();

            if (g_sd_card_detection_event.TryWait()) {
                g_sd_card_detection_event.Clear();
                /* Something changed, thus update the values. */
                if (out_is_sd_inserted) {
                    is_sd_card_inserted = fs::IsSdCardInserted();
                    *out_is_sd_inserted = is_sd_card_inserted;
                }
                if (out_detection_changed) {
                    *out_detection_changed = true;
                }
            }
            else {
                /* Nothing changed, thus return cached value. */
                if (out_is_sd_inserted) {
                    *out_is_sd_inserted = is_sd_card_inserted;
                }
                if (out_detection_changed) {
                    *out_detection_changed = false;
                }
            }
        }

        bool PrepareLogPath(char *out_path, size_t len, const char *log_dir, size_t log_dir_size) {
            /* Get current time. */
            auto time_point = time::StandardUserSystemClock::now();
            const auto posix_time = time::StandardUserSystemClock::to_time_t(time_point);
            if (posix_time != 0) {
                auto local_time = std::gmtime(std::addressof(posix_time));

                /* Get the console's serial number. */
                settings::system::SerialNumber serial_number;
                settings::system::GetSerialNumber(std::addressof(serial_number));

                /* Log file format: sdmc:/<log_dir>/<serial_no>_<year><month><day><hour><min><sec>[_<extra_index>].nxbinlog */
                /* The extra index is used when multiple logs are logged at the same time */
                char log_file[0x80] = {};
                const auto log_file_len = static_cast<size_t>(util::SNPrintf(log_file, sizeof(log_file), "%s:/%s/%s_%04d%02d%02d%02d%02d%02d", "sdmc", log_dir, serial_number.str, local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec));
                if (log_file_len <= log_dir_size) {
                    u32 extra_index = 1;
                    while(true) {
                        /* Only add the extra index if we couldn't create the default file name. */
                        if (extra_index == 1) {
                            const auto log_file_ext_len = static_cast<size_t>(util::SNPrintf(out_path, len, "%s.%s", log_file, "nxbinlog"));
                            if (log_file_ext_len >= len) {
                                return false;
                            }
                        }
                        else {
                            const auto log_file_ext_len = static_cast<size_t>(util::SNPrintf(out_path, len, "%s_%d.%s", log_file, extra_index, "nxbinlog"));
                            if (log_file_ext_len >= len) {
                                return false;
                            }
                        }

                        auto rc = fs::CreateFile(out_path, 0);
                        if (R_SUCCEEDED(rc)) {
                            break;
                        }
                        else if (fs::ResultPathAlreadyExists::Includes(rc) && (++extra_index < 100)) {
                            /* If the log file already exists (multiple logs at the same time), retry over 100 times increasing the extra index. */
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
            /* Ensure that the settings entry is present and it's value is true. */
            bool enable_logging = false;
            const auto enable_logging_size = settings::fwdbg::GetSettingsItemValue(std::addressof(enable_logging), sizeof(enable_logging), "lm", "enable_sd_card_logging");
            return (enable_logging_size == sizeof(enable_logging)) && enable_logging;
        }

        inline bool EnsureLogDirectory(const char *log_dir) {
            /* Try to create the log directory. */
            /* If the directory already exists, there's no issue. */
            const auto result = fs::CreateDirectory(log_dir);
            return R_SUCCEEDED(result) || fs::ResultPathAlreadyExists::Includes(result);
        }

    }

    bool SdCardLogging::Initialize() {
        /* Skip mounting the SD card if we don't have to. */
        if (this->sd_card_ok) {
            return true;
        }
        return R_SUCCEEDED(fs::MountSdCard("sdmc"));
    }

    void SdCardLogging::Dispose() {
        this->SetEnabled(false);
        if (this->sd_card_mounted) {
            fs::Unmount("sdmc");
            this->sd_card_mounted = false;
        }
    }

    bool SdCardLogging::EnsureLogFile() {
        if (this->enabled) {
            return true;
        }

        auto is_sd_inserted = false;
        bool sd_detection_changed = false;
        UpdateSdCardStatus(std::addressof(is_sd_inserted), std::addressof(sd_detection_changed));
        if (sd_detection_changed) {
            /* Changes were detected (the SD card was ejected, etc.), thus we need to re-mount the SD card. */
            this->sd_card_ok = false;
        }

        if (is_sd_inserted && this->Initialize()) {
            this->sd_card_mounted = true;
            auto log_dir_size = settings::fwdbg::GetSettingsItemValueSize("lm", "sd_card_log_output_directory");
            char log_dir[0x80] = {};
            if (log_dir_size <= sizeof(log_dir)) {
                if (settings::fwdbg::GetSettingsItemValue(log_dir, sizeof(log_dir), "lm", "sd_card_log_output_directory") == log_dir_size) {
                    char log_dir_path[0x80] = {};
                    const auto log_dir_path_len = static_cast<size_t>(util::SNPrintf(log_dir_path, sizeof(log_dir_path), "%s:/%s", "sdmc", log_dir));
                    if (log_dir_path_len < sizeof(log_dir_path)) {
                        if (EnsureLogDirectory(log_dir_path)) {
                            if (PrepareLogPath(this->log_file_path, sizeof(this->log_file_path), log_dir, sizeof(log_dir))) {
                                fs::FileHandle log_file_h;
                                if (R_SUCCEEDED(fs::OpenFile(std::addressof(log_file_h), this->log_file_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                                    ON_SCOPE_EXIT { fs::CloseFile(log_file_h); };
                                    
                                    /* 8-byte binary log header, current version is 1. */
                                    const impl::BinaryLogHeader bin_log_header = { impl::BinaryLogHeader::Magic, 1 };
                                    if (R_SUCCEEDED(fs::WriteFile(log_file_h, 0, std::addressof(bin_log_header), sizeof(bin_log_header), fs::WriteOption::Flush))) {
                                        this->log_file_offset = sizeof(bin_log_header);
                                        return true;
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

    bool SdCardLogging::SaveLog(const void *buf, size_t size) {
        static bool should_log = IsSdCardLoggingEnabled();
        if (!should_log) {
            return false;
        }
        
        if (this->EnsureLogFile()) {
            fs::FileHandle log_file_h;
            if (R_SUCCEEDED(fs::OpenFile(std::addressof(log_file_h), this->log_file_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                ON_SCOPE_EXIT { fs::CloseFile(log_file_h); };
                /* Write log data, and set enabled if we succeed. */
                if (R_SUCCEEDED(fs::WriteFile(log_file_h, this->log_file_offset, buf, size, fs::WriteOption::Flush))) {
                    this->log_file_offset += size;
                    this->SetEnabled(true);
                    return true;
                }
            }
        }
        
        /* Failure, thus unmount the SD card and disable ourselves. */
        if (this->sd_card_mounted) {
            fs::Unmount("sdmc");
            this->sd_card_mounted = false;
            this->sd_card_ok = true;
            UpdateSdCardStatus(nullptr, nullptr);
        }
        this->SetEnabled(false);
        return false;
    }

    SdCardLogging *GetSdCardLogging() {
        static SdCardLogging sd_card_logging;
        return std::addressof(sd_card_logging);
    }

}