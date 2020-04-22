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
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_report.hpp"
#include "erpt_srv_journal.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_context.hpp"

namespace ams::erpt::srv {

    bool Reporter::s_redirect_new_reports    = true;
    char Reporter::s_serial_number[24]       = "Unknown";
    char Reporter::s_os_version[24]          = "Unknown";
    char Reporter::s_private_os_version[96]  = "Unknown";
    std::optional<os::Tick> Reporter::s_application_launch_time;
    std::optional<os::Tick> Reporter::s_awake_time;
    std::optional<os::Tick> Reporter::s_power_on_time;
    std::optional<time::SteadyClockTimePoint> Reporter::s_initial_launch_settings_completion_time;

    Reporter::Reporter(ReportType type, const ContextEntry *ctx, const u8 *data, u32 data_size, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments)
        : type(type), ctx(ctx), data(data), data_size(data_size), meta(meta), attachments(attachments), num_attachments(num_attachments), occurrence_tick()
    {
        /* ... */
    }

    Result Reporter::CreateReport() {
        R_TRY(this->ValidateReportContext());
        R_TRY(this->CollectUniqueReportFields());
        R_TRY(this->SubmitReportDefaults());
        R_TRY(this->SubmitReportContexts());
        R_TRY(this->LinkAttachments());
        R_TRY(this->CreateReportFile());

        this->SaveSyslogReportIfRequired();
        return ResultSuccess();
    }

    Result Reporter::ValidateReportContext() {
        R_UNLESS(this->ctx->category == CategoryId_ErrorInfo, erpt::ResultRequiredContextMissing());
        R_UNLESS(this->ctx->field_count <= FieldsPerContext,  erpt::ResultInvalidArgument());

        bool found_error_code = false;
        for (u32 i = 0; i < this->ctx->field_count; i++) {
            if (this->ctx->fields[i].id == FieldId_ErrorCode) {
                found_error_code = true;
                break;
            }
        }
        R_UNLESS(found_error_code, erpt::ResultRequiredFieldMissing());

        return ResultSuccess();
    }

    Result Reporter::CollectUniqueReportFields() {
        this->occurrence_tick = os::GetSystemTick();
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            this->steady_clock_internal_offset_seconds = time::GetStandardSteadyClockInternalOffset().GetSeconds();
        } else {
            this->steady_clock_internal_offset_seconds = 0;
        }
        this->report_id.uuid = util::GenerateUuid();
        this->report_id.uuid.ToString(this->identifier_str, sizeof(this->identifier_str));
        if (R_FAILED(time::StandardNetworkSystemClock::GetCurrentTime(std::addressof(this->timestamp_network)))) {
            this->timestamp_network = {0};
        }
        R_ABORT_UNLESS(time::GetStandardSteadyClockCurrentTimePoint(std::addressof(this->steady_clock_current_timepoint)));
        R_TRY(time::StandardUserSystemClock::GetCurrentTime(std::addressof(this->timestamp_user)));
        return ResultSuccess();
    }

    Result Reporter::SubmitReportDefaults() {
        bool found_abort_flag = false, found_syslog_flag = false;
        for (u32 i = 0; i < this->ctx->field_count; i++) {
            if (this->ctx->fields[i].id == FieldId_AbortFlag) {
                found_abort_flag = true;
            }
            if (this->ctx->fields[i].id == FieldId_HasSyslogFlag) {
                found_syslog_flag = true;
            }
            if (found_abort_flag && found_syslog_flag) {
                break;
            }
        }

        ContextRecord *record = new ContextRecord(CategoryId_ErrorInfoDefaults);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());
        auto record_guard = SCOPE_GUARD { delete record; };

        if (!found_abort_flag) {
            record->Add(FieldId_AbortFlag, false);
        }

        if (!found_syslog_flag) {
            record->Add(FieldId_HasSyslogFlag, true);
        }

        R_TRY(Context::SubmitContextRecord(record));

        record_guard.Cancel();
        return ResultSuccess();
    }

    Result Reporter::SubmitReportContexts() {
        ContextRecord *record = new ContextRecord(CategoryId_ErrorInfoAuto);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());
        auto record_guard = SCOPE_GUARD { delete record; };

        record->Add(FieldId_OsVersion,                        s_os_version,                                 strnlen(s_os_version, sizeof(s_os_version)));
        record->Add(FieldId_PrivateOsVersion,                 s_private_os_version,                         strnlen(s_private_os_version, sizeof(s_private_os_version)));
        record->Add(FieldId_SerialNumber,                     s_serial_number,                              strnlen(s_serial_number, sizeof(s_serial_number)));
        record->Add(FieldId_ReportIdentifier,                 this->identifier_str,                         sizeof(this->identifier_str));
        record->Add(FieldId_OccurrenceTimestamp,              this->timestamp_user.value);
        record->Add(FieldId_OccurrenceTimestampNet,           this->timestamp_network.value);
        record->Add(FieldId_ReportVisibilityFlag,             this->type == ReportType_Visible);
        record->Add(FieldId_OccurrenceTick,                   this->occurrence_tick.GetInt64Value());
        record->Add(FieldId_SteadyClockInternalOffset,        this->steady_clock_internal_offset_seconds);
        record->Add(FieldId_SteadyClockCurrentTimePointValue, this->steady_clock_current_timepoint.value);

        if (s_initial_launch_settings_completion_time) {
            s64 elapsed_seconds;
            if (R_SUCCEEDED(time::GetElapsedSecondsBetween(std::addressof(elapsed_seconds), *s_initial_launch_settings_completion_time, this->steady_clock_current_timepoint))) {
                record->Add(FieldId_ElapsedTimeSinceInitialLaunch, elapsed_seconds);
            }
        }

        if (s_power_on_time) {
            record->Add(FieldId_ElapsedTimeSincePowerOn, (this->occurrence_tick - *s_power_on_time).ToTimeSpan().GetSeconds());
        }

        if (s_awake_time) {
            record->Add(FieldId_ElapsedTimeSincePowerOn, (this->occurrence_tick - *s_awake_time).ToTimeSpan().GetSeconds());
        }

        if (s_application_launch_time) {
            record->Add(FieldId_ApplicationAliveTime, (this->occurrence_tick - *s_application_launch_time).ToTimeSpan().GetSeconds());
        }

        R_TRY(Context::SubmitContextRecord(record));
        record_guard.Cancel();

        R_TRY(Context::SubmitContext(this->ctx, this->data, this->data_size));

        return ResultSuccess();
    }

    Result Reporter::LinkAttachments() {
        for (u32 i = 0; i < this->num_attachments; i++) {
            R_TRY(JournalForAttachments::SetOwner(this->attachments[i], this->report_id));
        }
        return ResultSuccess();
    }

    Result Reporter::CreateReportFile() {
        /* Make a journal record. */
        auto *record = new JournalRecord<ReportInfo>;
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        record->AddReference();
        ON_SCOPE_EXIT {
            if (record->RemoveReference()) {
                delete record;
            }
        };

        record->info.type              = this->type;
        record->info.id                = this->report_id;
        record->info.flags             = erpt::srv::MakeNoReportFlags();
        record->info.timestamp_user    = this->timestamp_user;
        record->info.timestamp_network = this->timestamp_network;
        if (this->meta != nullptr) {
            record->info.meta_data = *this->meta;
        }
        if (this->num_attachments > 0) {
            record->info.flags.Set<ReportFlag::HasAttachment>();
        }

        auto report = std::make_unique<Report>(record, s_redirect_new_reports);
        R_UNLESS(report != nullptr, erpt::ResultOutOfMemory());

        R_TRY(Context::WriteContextsToReport(report.get()));
        R_TRY(report->GetSize(std::addressof(record->info.report_size)));

        if (!s_redirect_new_reports) {
            /* If we're not redirecting new reports, then we want to store the report in the journal. */
            R_TRY(Journal::Store(record));
        } else {
            /* If we are redirecting new reports, we don't want to store the report in the journal. */
            /* We should take this opportunity to delete any attachments associated with the report. */
            R_ABORT_UNLESS(JournalForAttachments::DeleteAttachments(this->report_id));
        }

        R_TRY(Journal::Commit());

        return ResultSuccess();
    }

    void Reporter::SaveSyslogReportIfRequired() {
        bool needs_save_syslog = true;
        for (u32 i = 0; i < this->ctx->field_count; i++) {
            static_assert(FieldToTypeMap[FieldId_HasSyslogFlag] == FieldType_Bool);
            if (this->ctx->fields[i].id == FieldId_HasSyslogFlag && (this->ctx->fields[i].value_bool == false)) {
                needs_save_syslog = false;
                break;
            }
        }
        if (needs_save_syslog) {
            /* Here nintendo sends a report to srepo:u (vtable offset 0xE8) with data this->report_id. */
            /* We will not send report ids to srepo:u. */
        }
    }

}
