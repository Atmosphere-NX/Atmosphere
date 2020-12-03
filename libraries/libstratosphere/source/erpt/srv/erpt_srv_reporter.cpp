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

    constinit bool Reporter::s_redirect_new_reports    = true;
    constinit char Reporter::s_serial_number[24]       = "Unknown";
    constinit char Reporter::s_os_version[24]          = "Unknown";
    constinit char Reporter::s_private_os_version[96]  = "Unknown";
    constinit std::optional<os::Tick> Reporter::s_application_launch_time;
    constinit std::optional<os::Tick> Reporter::s_awake_time;
    constinit std::optional<os::Tick> Reporter::s_power_on_time;
    constinit std::optional<time::SteadyClockTimePoint> Reporter::s_initial_launch_settings_completion_time;

    namespace {

        constinit os::SdkMutex g_limit_mutex;
        constinit bool g_submitted_limit = false;

        Result PullErrorContext(size_t *out_total_size, size_t *out_size, void *dst, size_t dst_size, const err::ContextDescriptor &descriptor, Result result) {
            s32 unk0;
            u32 total_size, size;
            R_TRY(::ectxrPullContext(std::addressof(unk0), std::addressof(total_size), std::addressof(size), dst, dst_size, descriptor.value, result.GetValue()));

            *out_total_size = total_size;
            *out_size       = size;
            return ResultSuccess();
        }

        void SubmitErrorContext(ContextRecord *record, Result result) {
            /* Only support submitting context on 11.x. */
            if (hos::GetVersion() < hos::Version_11_0_0) {
                return;
            }

            /* Get the context descriptor. */
            const auto descriptor = err::GetContextDescriptorFromResult(result);
            if (descriptor == err::InvalidContextDescriptor) {
                return;
            }

            /* Pull the error context. */
            u8 error_context[0x200];
            size_t error_context_total_size;
            size_t error_context_size;
            if (R_FAILED(PullErrorContext(std::addressof(error_context_total_size), std::addressof(error_context_size), error_context, util::size(error_context), descriptor, result))) {
                return;
            }

            /* Set the total size. */
            if (error_context_total_size == 0) {
                return;
            }
            record->Add(FieldId_ErrorContextTotalSize, error_context_total_size);

            /* Set the context. */
            if (error_context_size == 0) {
                return;
            }
            record->Add(FieldId_ErrorContextSize, error_context_size);
            record->Add(FieldId_ErrorContext, error_context, error_context_size);
        }

        void SubmitResourceLimitLimitContext() {
            std::scoped_lock lk(g_limit_mutex);
            if (g_submitted_limit) {
                return;
            }

            ON_SCOPE_EXIT { g_submitted_limit = true; };

            /* Create and populate the record. */
            auto record = std::make_unique<ContextRecord>(CategoryId_ResourceLimitLimitInfo);
            if (record == nullptr) {
                return;
            }

            u64 reslimit_handle_value;
            if (R_FAILED(svc::GetInfo(std::addressof(reslimit_handle_value), svc::InfoType_ResourceLimit, svc::InvalidHandle, 0))) {
                return;
            }

            const auto handle = static_cast<svc::Handle>(reslimit_handle_value);
            ON_SCOPE_EXIT { R_ABORT_UNLESS(svc::CloseHandle(handle)); };

            #define ADD_RESOURCE(__RESOURCE__)                                                                                                        \
                do {                                                                                                                                  \
                    s64 limit_value;                                                                                                                  \
                    if (R_FAILED(svc::GetResourceLimitLimitValue(std::addressof(limit_value), handle, svc::LimitableResource_##__RESOURCE__##Max))) { \
                        return;                                                                                                                       \
                    }                                                                                                                                 \
                    if (R_FAILED(record->Add(FieldId_System##__RESOURCE__##Limit, limit_value))) {                                                    \
                        return;                                                                                                                       \
                    }                                                                                                                                 \
                } while (0)

            ADD_RESOURCE(PhysicalMemory);
            ADD_RESOURCE(ThreadCount);
            ADD_RESOURCE(EventCount);
            ADD_RESOURCE(TransferMemoryCount);
            ADD_RESOURCE(SessionCount);

            #undef ADD_RESOURCE

            Context::SubmitContextRecord(std::move(record));

            g_submitted_limit = true;
        }

        void SubmitResourceLimitPeakContext() {
            /* Create and populate the record. */
            auto record = std::make_unique<ContextRecord>(CategoryId_ResourceLimitPeakInfo);
            if (record == nullptr) {
                return;
            }

            u64 reslimit_handle_value;
            if (R_FAILED(svc::GetInfo(std::addressof(reslimit_handle_value), svc::InfoType_ResourceLimit, svc::InvalidHandle, 0))) {
                return;
            }

            const auto handle = static_cast<svc::Handle>(reslimit_handle_value);
            ON_SCOPE_EXIT { R_ABORT_UNLESS(svc::CloseHandle(handle)); };

            #define ADD_RESOURCE(__RESOURCE__)                                                                                                      \
                do {                                                                                                                                \
                    s64 peak_value;                                                                                                                 \
                    if (R_FAILED(svc::GetResourceLimitPeakValue(std::addressof(peak_value), handle, svc::LimitableResource_##__RESOURCE__##Max))) { \
                        return;                                                                                                                     \
                    }                                                                                                                               \
                    if (R_FAILED(record->Add(FieldId_System##__RESOURCE__##Peak, peak_value))) {                                                    \
                        return;                                                                                                                     \
                    }                                                                                                                               \
                } while (0)

            ADD_RESOURCE(PhysicalMemory);
            ADD_RESOURCE(ThreadCount);
            ADD_RESOURCE(EventCount);
            ADD_RESOURCE(TransferMemoryCount);
            ADD_RESOURCE(SessionCount);

            #undef ADD_RESOURCE

            Context::SubmitContextRecord(std::move(record));
        }

        void SubmitResourceLimitContexts() {
            if (hos::GetVersion() >= hos::Version_11_0_0 || svc::IsKernelMesosphere()) {
                SubmitResourceLimitLimitContext();
                SubmitResourceLimitPeakContext();
            }
        }

    }

    Reporter::Reporter(ReportType type, const ContextEntry *ctx, const u8 *data, u32 data_size, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, Result ctx_r)
        : type(type), ctx(ctx), data(data), data_size(data_size), meta(meta), attachments(attachments), num_attachments(num_attachments), occurrence_tick(), ctx_result(ctx_r)
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

        const bool found_error_code = util::range::any_of(MakeSpan(this->ctx->fields, this->ctx->field_count), [] (const FieldEntry &entry) {
            return entry.id == FieldId_ErrorCode;
        });
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
        auto record = std::make_unique<ContextRecord>(CategoryId_ErrorInfoDefaults);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

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

        if (!found_abort_flag) {
            record->Add(FieldId_AbortFlag, false);
        }

        if (!found_syslog_flag) {
            record->Add(FieldId_HasSyslogFlag, true);
        }

        R_TRY(Context::SubmitContextRecord(std::move(record)));

        return ResultSuccess();
    }

    Result Reporter::SubmitReportContexts() {
        auto record = std::make_unique<ContextRecord>(CategoryId_ErrorInfoAuto);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        /* Handle error context. */
        if (R_FAILED(this->ctx_result)) {
            SubmitErrorContext(record.get(), this->ctx_result);
        }

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

        R_TRY(Context::SubmitContextRecord(std::move(record)));

        R_TRY(Context::SubmitContext(this->ctx, this->data, this->data_size));

        /* Submit context for resource limits. */
        SubmitResourceLimitContexts();

        return ResultSuccess();
    }

    Result Reporter::LinkAttachments() {
        for (u32 i = 0; i < this->num_attachments; i++) {
            R_TRY(JournalForAttachments::SetOwner(this->attachments[i], this->report_id));
        }
        return ResultSuccess();
    }

    Result Reporter::CreateReportFile() {
        /* Define journal record deleter. */
        struct JournalRecordDeleter {
            void operator()(JournalRecord<ReportInfo> *record) {
                if (record != nullptr) {
                    if (record->RemoveReference()) {
                        delete record;
                    }
                }
            }
        };

        /* Make a journal record. */
        auto record = std::unique_ptr<JournalRecord<ReportInfo>, JournalRecordDeleter>{new JournalRecord<ReportInfo>, JournalRecordDeleter{}};
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());
        record->AddReference();

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

        auto report = std::make_unique<Report>(record.get(), s_redirect_new_reports);
        R_UNLESS(report != nullptr, erpt::ResultOutOfMemory());
        auto report_guard = SCOPE_GUARD { report->Delete(); };

        R_TRY(Context::WriteContextsToReport(report.get()));
        R_TRY(report->GetSize(std::addressof(record->info.report_size)));

        if (!s_redirect_new_reports) {
            /* If we're not redirecting new reports, then we want to store the report in the journal. */
            R_TRY(Journal::Store(record.get()));
        } else {
            /* If we are redirecting new reports, we don't want to store the report in the journal. */
            /* We should take this opportunity to delete any attachments associated with the report. */
            R_ABORT_UNLESS(JournalForAttachments::DeleteAttachments(this->report_id));
        }

        R_TRY(Journal::Commit());

        report_guard.Cancel();
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
