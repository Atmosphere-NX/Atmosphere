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
#include <stratosphere.hpp>
#include "erpt_srv_context_record.hpp"

namespace ams::erpt::srv {

    class Reporter {
        private:
            static bool s_redirect_new_reports;
            static char s_serial_number[24];
            static char s_os_version[24];
            static char s_private_os_version[96];
            static util::optional<os::Tick> s_application_launch_time;
            static util::optional<os::Tick> s_awake_time;
            static util::optional<os::Tick> s_power_on_time;
            static util::optional<time::SteadyClockTimePoint> s_initial_launch_settings_completion_time;
        public:
            static void ClearApplicationLaunchTime() { s_application_launch_time = util::nullopt; }
            static void ClearInitialLaunchSettingsCompletionTime() { s_initial_launch_settings_completion_time = util::nullopt; }

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
                R_SUCCEED();
            }

            static Result RegisterRunningApplet(ncm::ProgramId program_id);
            static Result UnregisterRunningApplet(ncm::ProgramId program_id);
            static Result UpdateAppletSuspendedDuration(ncm::ProgramId program_id, TimeSpan duration);

            static void SetRedirectNewReportsToSdCard(bool en) { s_redirect_new_reports = en; }
        private:
            static Result SubmitReportContexts(const ReportId &report_id, ReportType type, Result ctx_result, std::unique_ptr<ContextRecord> record, const time::PosixTime &user_timestamp, const time::PosixTime &network_timestamp, erpt::CreateReportOptionFlagSet flags);
        public:
            static Result CreateReport(ReportType type, Result ctx_result, const ContextEntry *ctx, const u8 *data, u32 data_size, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, erpt::CreateReportOptionFlagSet flags, const ReportId *specified_report_id);
            static Result CreateReport(ReportType type, Result ctx_result, std::unique_ptr<ContextRecord> record, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, erpt::CreateReportOptionFlagSet flags, const ReportId *specified_report_id);
    };

}
