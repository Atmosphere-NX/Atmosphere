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
#include <stratosphere.hpp>

namespace ams::erpt::srv {

    class Reporter {
        private:
            static bool s_redirect_new_reports;
            static char s_serial_number[24];
            static char s_os_version[24];
            static char s_private_os_version[96];
            static std::optional<os::Tick> s_application_launch_time;
            static std::optional<os::Tick> s_awake_time;
            static std::optional<os::Tick> s_power_on_time;
            static std::optional<time::SteadyClockTimePoint> s_initial_launch_settings_completion_time;
        private:
            const ReportType type;
            const ContextEntry * const ctx;
            const u8 * const data;
            const u32 data_size;
            const ReportMetaData * const meta;
            const AttachmentId * const attachments;
            const u32 num_attachments;
            char identifier_str[0x40];
            time::PosixTime timestamp_user;
            time::PosixTime timestamp_network;
            os::Tick occurrence_tick;
            s64 steady_clock_internal_offset_seconds;
            ReportId report_id;
            Result ctx_result;
            time::SteadyClockTimePoint steady_clock_current_timepoint;
        public:
            static void ClearApplicationLaunchTime() { s_application_launch_time = std::nullopt; }
            static void ClearInitialLaunchSettingsCompletionTime() { s_initial_launch_settings_completion_time = std::nullopt; }

            static void SetInitialLaunchSettingsCompletionTime(const time::SteadyClockTimePoint &time) { s_initial_launch_settings_completion_time = time; }

            static void UpdateApplicationLaunchTime() { s_application_launch_time = os::GetSystemTick(); }
            static void UpdateAwakeTime() { s_awake_time = os::GetSystemTick(); }
            static void UpdatePowerOnTime() { s_power_on_time = os::GetSystemTick(); }

            static Result SetSerialNumberAndOsVersion(const char *sn, u32 sn_len, const char *os, u32 os_len, const char *os_priv, u32 os_priv_len) {
                R_UNLESS(sn_len <= sizeof(s_serial_number),           erpt::ResultInvalidArgument());
                R_UNLESS(os_len <= sizeof(s_os_version),              erpt::ResultInvalidArgument());
                R_UNLESS(os_priv_len <= sizeof(s_private_os_version), erpt::ResultInvalidArgument());

                std::memcpy(s_serial_number, sn, sn_len);
                std::memcpy(s_os_version, os, os_len);
                std::memcpy(s_private_os_version, os_priv, os_priv_len);
                return ResultSuccess();
            }

            static void SetRedirectNewReportsToSdCard(bool en) { s_redirect_new_reports = en; }
        private:
            Result ValidateReportContext();
            Result CollectUniqueReportFields();
            Result SubmitReportDefaults();
            Result SubmitReportContexts();
            Result LinkAttachments();
            Result CreateReportFile();
            void   SaveSyslogReportIfRequired();
            void   SaveSyslogReport();
        public:
            Reporter(ReportType type, const ContextEntry *ctx, const u8 *data, u32 data_size, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, Result ctx_result);

            Result CreateReport();
    };

}
