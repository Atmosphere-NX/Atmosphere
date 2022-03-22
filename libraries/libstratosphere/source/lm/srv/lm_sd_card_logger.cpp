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
#include <stratosphere.hpp>
#include "lm_sd_card_logger.hpp"
#include "lm_time_util.hpp"

namespace ams::lm::srv {

    namespace {

        constexpr const char SdCardMountName[]  = "sdcard";
        constexpr const char LogFileExtension[] = "nxbinlog";

        constexpr const char SettingName[]               = "lm";
        constexpr const char SettingKeyLoggingEnabled[]  = "enable_sd_card_logging";
        constexpr const char SettingKeyOutputDirectory[] = "sd_card_log_output_directory";

        constexpr inline size_t LogFileHeaderSize = 8;
        constexpr inline u32 LogFileHeaderMagic   = util::ReverseFourCC<'p','h','p','h'>::Code;
        constexpr inline u8 LogFileHeaderVersion  = 1;

        struct LogFileHeader {
            u32 magic;
            u8 version;
            u8 reserved[3];
        };
        static_assert(sizeof(LogFileHeader) == LogFileHeaderSize);

        constinit os::SdkMutex g_sd_card_logging_enabled_mutex;
        constinit bool g_determined_sd_card_logging_enabled = false;
        constinit bool g_sd_card_logging_enabled = false;

        constinit os::SdkMutex g_sd_card_detection_event_mutex;
        constinit bool g_sd_card_inserted_cache = false;
        constinit bool g_sd_card_detection_event_initialized = false;
        constinit std::unique_ptr<fs::IEventNotifier> g_sd_card_detection_event_notifier;
        os::SystemEvent g_sd_card_detection_event;

        bool GetSdCardLoggingEnabledImpl() {
            bool enabled;
            const auto size = settings::fwdbg::GetSettingsItemValue(std::addressof(enabled), sizeof(enabled), SettingName, SettingKeyLoggingEnabled);
            if (size != sizeof(enabled)) {
                AMS_ASSERT(size == sizeof(enabled));
                return false;
            }

            return enabled;
        }

        bool GetSdCardLoggingEnabled() {
            if (AMS_UNLIKELY(!g_determined_sd_card_logging_enabled)) {
                std::scoped_lock lk(g_sd_card_logging_enabled_mutex);

                if (AMS_LIKELY(!g_determined_sd_card_logging_enabled)) {
                    g_sd_card_logging_enabled = GetSdCardLoggingEnabledImpl();
                    g_determined_sd_card_logging_enabled = true;
                }
            }

            return g_sd_card_logging_enabled;
        }

        void EnsureSdCardDetectionEventInitialized() {
            if (AMS_UNLIKELY(!g_sd_card_detection_event_initialized)) {
                std::scoped_lock lk(g_sd_card_detection_event_mutex);

                if (AMS_LIKELY(!g_sd_card_detection_event_initialized)) {
                    /* Create SD card detection event notifier. */
                    R_ABORT_UNLESS(fs::OpenSdCardDetectionEventNotifier(std::addressof(g_sd_card_detection_event_notifier)));
                    R_ABORT_UNLESS(g_sd_card_detection_event_notifier->BindEvent(g_sd_card_detection_event.GetBase(), os::EventClearMode_ManualClear));

                    /* Get initial inserted value. */
                    g_sd_card_inserted_cache = fs::IsSdCardInserted();

                    g_sd_card_detection_event_initialized = true;
                }
            }
        }

        void GetSdCardStatus(bool *out_inserted, bool *out_status_changed) {
            /* Ensure that we can detect the sd card. */
            EnsureSdCardDetectionEventInitialized();

            /* Check if there's a detection event. */
            const bool status_changed = g_sd_card_detection_event.TryWait();
            if (status_changed) {
                g_sd_card_detection_event.Clear();

                /* Update the inserted cache. */
                g_sd_card_inserted_cache = fs::IsSdCardInserted();
            }

            *out_inserted       = g_sd_card_inserted_cache;
            *out_status_changed = status_changed;
        }

        bool GetSdCardLogOutputDirectory(char *dst, size_t size) {
            /* Get the output directory size. */
            const auto value_size = settings::fwdbg::GetSettingsItemValueSize(SettingName, SettingKeyOutputDirectory);
            if (value_size > size) {
                AMS_ASSERT(value_size <= size);
                return false;
            }

            /* Get the output directory. */
            const auto read_size = settings::fwdbg::GetSettingsItemValue(dst, size, SettingName, SettingKeyOutputDirectory);
            AMS_ASSERT(read_size == value_size);

            return read_size == value_size;
        }

        bool EnsureLogDirectory(const char *dir) {
            /* Generate the log directory path. */
            char path[0x80];
            const size_t len = util::SNPrintf(path, sizeof(path), "%s:/%s", SdCardMountName, dir);
            if (len >= sizeof(path)) {
                AMS_ASSERT(len < sizeof(path));
                return false;
            }

            /* Ensure the directory. */
            /* NOTE: Nintendo does not perform recusrive directory ensure, only a single CreateDirectory level. */
            return R_SUCCEEDED(fs::EnsureDirectoryRecursively(path));
        }

        bool MakeLogFilePathWithoutExtension(char *dst, size_t size, const char *dir) {
            /* Get the current time. */
            const auto cur_time = time::ToCalendarTimeInUtc(lm::srv::GetCurrentTime());

            /* Get the device serial number. */
            settings::system::SerialNumber serial_number;
            settings::system::GetSerialNumber(std::addressof(serial_number));

            /* Print the path. */
            const size_t len = util::SNPrintf(dst, size, "%s:/%s/%s_%04d%02d%02d%02d%02d%02d", SdCardMountName, dir, serial_number.str, cur_time.year, cur_time.month, cur_time.day, cur_time.hour, cur_time.minute, cur_time.second);

            AMS_ASSERT(len < size);
            return len < size;
        }

        bool GenerateLogFile(char *dst, size_t size, const char *dir) {
            /* Generate the log file path. */
            char path_without_ext[0x80];
            if (!MakeLogFilePathWithoutExtension(path_without_ext, sizeof(path_without_ext), dir)) {
                return false;
            }

            /* Try to find an available log file path. */
            constexpr auto MaximumLogIndex = 99;
            for (auto i = 1; i <= MaximumLogIndex; ++i) {
                /* Print the current log file path. */
                const size_t len = (i == 1) ? util::SNPrintf(dst, size, "%s.%s", path_without_ext, LogFileExtension) : util::SNPrintf(dst, size, "%s_%d.%s", path_without_ext, i, LogFileExtension);
                if (len >= size) {
                    AMS_ASSERT(len < size);
                    return false;
                }

                /* Try to create the log file. */
                const auto result = fs::CreateFile(dst, 0);
                if (R_SUCCEEDED(result)) {
                    return true;
                } else if (fs::ResultPathAlreadyExists::Includes(result)) {
                    /* The log file already exists, so try the next index. */
                    continue;
                } else {
                    /* We failed to create a log file. */
                    return false;
                }
            }

            /* We ran out of log file indices. */
            return false;
        }

        Result WriteLogFileHeaderImpl(const char *path) {
            /* Open the log file. */
            fs::FileHandle file;
            R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Write the log file header. */
            const LogFileHeader header = {
                .magic   = LogFileHeaderMagic,
                .version = LogFileHeaderVersion
            };

            R_TRY(fs::WriteFile(file, 0, std::addressof(header), sizeof(header), fs::WriteOption::Flush));

            return ResultSuccess();
        }

        bool WriteLogFileHeader(const char *path) {
            return R_SUCCEEDED(WriteLogFileHeaderImpl(path));
        }

        Result WriteLogFileBodyImpl(const char *path, s64 offset, const u8 *data, size_t size) {
            /* Open the log file. */
            fs::FileHandle file;
            R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Write the data. */
            R_TRY(fs::WriteFile(file, offset, data, size, fs::WriteOption::Flush));

            return ResultSuccess();
        }

        bool WriteLogFileBody(const char *path, s64 offset, const u8 *data, size_t size) {
            return R_SUCCEEDED(WriteLogFileBodyImpl(path, offset, data, size));
        }

    }

    SdCardLogger::SdCardLogger() : m_logging_observer_mutex(), m_is_enabled(false), m_is_sd_card_mounted(false), m_is_sd_card_status_unknown(false), m_log_file_offset(0), m_logging_observer(nullptr) {
        /* ... */
    }

    bool SdCardLogger::GetEnabled() const {
        return m_is_enabled;
    }

    void SdCardLogger::SetEnabled(bool enabled) {
        /* Only update if we need to. */
        if (m_is_enabled == enabled) {
            return;
        }

        /* Set enabled. */
        m_is_enabled = enabled;

        /* Invoke our observer. */
        std::scoped_lock lk(m_logging_observer_mutex);

        if (m_logging_observer) {
            m_logging_observer(enabled);
        }
    }

    void SdCardLogger::SetLoggingObserver(LoggingObserver observer) {
        std::scoped_lock lk(m_logging_observer_mutex);

        m_logging_observer = observer;
    }

    bool SdCardLogger::Initialize() {
        /* If we're already enabled, nothing to do. */
        if (this->GetEnabled()) {
            return true;
        }

        /* Get the sd card status. */
        bool inserted = false, status_changed = false;
        GetSdCardStatus(std::addressof(inserted), std::addressof(status_changed));

        /* Update whether status is known. */
        if (status_changed) {
            m_is_sd_card_status_unknown = false;
        }

        /* If the SD isn't inserted, we can't initialize. */
        if (!inserted) {
            return false;
        }

        /* If the status is unknown, we can't initialize. */
        if (m_is_sd_card_status_unknown) {
            return false;
        }

        /* Mount the SD card. */
        if (R_FAILED(fs::MountSdCard(SdCardMountName))) {
            return false;
        }

        /* Note that the SD card is mounted. */
        m_is_sd_card_mounted = true;

        /* Get the output directory. */
        char output_dir[0x80];
        if (!GetSdCardLogOutputDirectory(output_dir, sizeof(output_dir))) {
            return false;
        }

        /* Ensure the output directory exists. */
        if (!EnsureLogDirectory(output_dir)) {
            return false;
        }

        /* Ensure that a log file exists for us to write to. */
        if (!GenerateLogFile(m_log_file_path, sizeof(m_log_file_path), output_dir)) {
            return false;
        }

        /* Write the log file header. */
        if (!WriteLogFileHeader(m_log_file_path)) {
            return false;
        }

        /* Set our initial offset. */
        m_log_file_offset = LogFileHeaderSize;

        return true;
    }

    void SdCardLogger::Finalize() {
        this->SetEnabled(false);
        if (m_is_sd_card_mounted) {
            fs::Unmount(SdCardMountName);
            m_is_sd_card_mounted = false;
        }
    }

    bool SdCardLogger::Write(const u8 *data, size_t size) {
        /* Only write if sd card logging is enabled. */
        if (!GetSdCardLoggingEnabled()) {
            return false;
        }

        /* Ensure we keep our pre and post-conditions in check. */
        bool success = false;
        ON_SCOPE_EXIT {
            if (!success && m_is_sd_card_mounted) {
                fs::Unmount(SdCardMountName);
                m_is_sd_card_mounted        = false;
                m_is_sd_card_status_unknown = true;
            }
            this->SetEnabled(success);
        };

        /* Try to initialize. */
        if (!this->Initialize()) {
            return false;
        }

        /* Try to write the log file. */
        if (!WriteLogFileBody(m_log_file_path, m_log_file_offset, data, size)) {
            return false;
        }

        /* Advance. */
        m_log_file_offset += size;

        /* We succeeded. */
        success = true;
        return true;
    }

}
