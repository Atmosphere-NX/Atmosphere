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
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_report.hpp"
#include "erpt_srv_journal.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_fs_info.hpp"

namespace ams::erpt::srv {

    constinit bool Reporter::s_redirect_new_reports    = true;
    constinit char Reporter::s_serial_number[24]       = "Unknown";
    constinit char Reporter::s_os_version[24]          = "Unknown";
    constinit char Reporter::s_private_os_version[96]  = "Unknown";
    constinit util::optional<os::Tick> Reporter::s_application_launch_time;
    constinit util::optional<os::Tick> Reporter::s_awake_time;
    constinit util::optional<os::Tick> Reporter::s_power_on_time;
    constinit util::optional<time::SteadyClockTimePoint> Reporter::s_initial_launch_settings_completion_time;

    namespace {

        class AppletActiveTimeInfoList {
            private:
                struct AppletActiveTimeInfo {
                    ncm::ProgramId program_id;
                    os::Tick register_tick;
                    TimeSpan suspended_duration;
                };
                static constexpr AppletActiveTimeInfo InvalidAppletActiveTimeInfo = { ncm::InvalidProgramId, os::Tick{}, TimeSpan::FromNanoSeconds(0) };
            private:
                std::array<AppletActiveTimeInfo, 8> m_list;
            public:
                constexpr AppletActiveTimeInfoList() : m_list{InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo, InvalidAppletActiveTimeInfo} {
                    m_list.fill(InvalidAppletActiveTimeInfo);
                }
            public:
                void Register(ncm::ProgramId program_id) {
                    /* Find an unused entry. */
                    auto entry = util::range::find_if(m_list, [](const AppletActiveTimeInfo &info) { return info.program_id == ncm::InvalidProgramId; });
                    AMS_ASSERT(entry != m_list.end());

                    /* Create the entry. */
                    *entry = { program_id, os::GetSystemTick(), TimeSpan::FromNanoSeconds(0) };
                }

                void Unregister(ncm::ProgramId program_id) {
                    /* Find a matching entry. */
                    auto entry = util::range::find_if(m_list, [&](const AppletActiveTimeInfo &info) { return info.program_id == program_id; });
                    AMS_ASSERT(entry != m_list.end());

                    /* Clear the entry. */
                    *entry = InvalidAppletActiveTimeInfo;
                }

                void UpdateSuspendedDuration(ncm::ProgramId program_id, TimeSpan suspended_duration) {
                    /* Find a matching entry. */
                    auto entry = util::range::find_if(m_list, [&](const AppletActiveTimeInfo &info) { return info.program_id == program_id; });
                    AMS_ASSERT(entry != m_list.end());

                    /* Set the suspended duration. */
                    entry->suspended_duration = suspended_duration;
                }

                util::optional<TimeSpan> GetActiveDuration(ncm::ProgramId program_id) const {
                    /* Try to find a matching entry. */
                    const auto entry = util::range::find_if(m_list, [&](const AppletActiveTimeInfo &info) { return info.program_id == program_id; });
                    if (entry != m_list.end()) {
                        return (os::GetSystemTick() - entry->register_tick).ToTimeSpan() - entry->suspended_duration;
                    } else {
                        return util::nullopt;
                    }
                }
        };

        constinit AppletActiveTimeInfoList g_applet_active_time_info_list;

        #if defined(ATMOSPHERE_OS_HORIZON)
        Result PullErrorContext(size_t *out_total_size, size_t *out_size, void *dst, size_t dst_size, const err::ContextDescriptor &descriptor, Result result) {
            s32 unk0;
            u32 total_size, size;
            R_TRY(::ectxrPullContext(std::addressof(unk0), std::addressof(total_size), std::addressof(size), dst, dst_size, descriptor.value, result.GetValue()));

            *out_total_size = total_size;
            *out_size       = size;
            R_SUCCEED();
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

        constinit os::SdkMutex g_limit_mutex;
        constinit bool g_submitted_limit = false;

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
                    if (R_FAILED(record->Add(FieldId_System##__RESOURCE__##Limit, limit_value))) {                                             \
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
                    if (R_FAILED(record->Add(FieldId_System##__RESOURCE__##Peak, peak_value))) {                                             \
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
            SubmitResourceLimitLimitContext();
            SubmitResourceLimitPeakContext();
        }
        #else
        void SubmitErrorContext(ContextRecord *record, Result result) {
            AMS_UNUSED(record, result);
        }
        #endif

        Result ValidateCreateReportContext(const ContextEntry *ctx) {
            R_UNLESS(ctx->category == CategoryId_ErrorInfo, erpt::ResultRequiredContextMissing());
            R_UNLESS(ctx->field_count <= FieldsPerContext,  erpt::ResultInvalidArgument());

            const bool found_error_code = util::range::any_of(MakeSpan(ctx->fields, ctx->field_count), [] (const FieldEntry &entry) {
                return entry.id == FieldId_ErrorCode;
            });
            R_UNLESS(found_error_code, erpt::ResultRequiredFieldMissing());

            R_SUCCEED();
        }

        Result SubmitReportDefaults(const ContextEntry *ctx) {
            AMS_ASSERT(ctx->category == CategoryId_ErrorInfo);

            auto record = std::make_unique<ContextRecord>(CategoryId_ErrorInfoDefaults);
            R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

            bool found_abort_flag = false, found_syslog_flag = false;
            for (u32 i = 0; i < ctx->field_count; i++) {
                if (ctx->fields[i].id == FieldId_AbortFlag) {
                    found_abort_flag = true;
                }
                if (ctx->fields[i].id == FieldId_HasSyslogFlag) {
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

            R_SUCCEED();
        }

        void SaveSyslogReportIfRequired(const ContextEntry *ctx, const ReportId &report_id) {
            bool needs_save_syslog = true;
            for (u32 i = 0; i < ctx->field_count; i++) {
                static_assert(FieldIndexToTypeMap[*FindFieldIndex(FieldId_HasSyslogFlag)] == FieldType_Bool);
                if (ctx->fields[i].id == FieldId_HasSyslogFlag && !ctx->fields[i].value_bool) {
                    needs_save_syslog = false;
                    break;
                }
            }

            if (needs_save_syslog) {
                /* Here nintendo sends a report to srepo:u (vtable offset 0xE8) with data report_id. */
                /* We will not send report ids to srepo:u. */
                AMS_UNUSED(report_id);
            }
        }

        void SubmitAppletActiveDurationForCrashReport(const ContextEntry *error_info_ctx, const void *data, u32 data_size, ContextRecord *error_info_auto_record) {
            /* Check pre-conditions. */
            AMS_ASSERT(error_info_ctx != nullptr);
            AMS_ASSERT(error_info_ctx->category == CategoryId_ErrorInfo);
            AMS_ASSERT(data != nullptr);
            AMS_ASSERT(error_info_auto_record != nullptr);

            /* Find the program id entry. */
            const auto fields_span = MakeSpan(error_info_ctx->fields, error_info_ctx->field_count);
            const auto program_id_entry = util::range::find_if(fields_span, [](const FieldEntry &entry) { return entry.id == FieldId_ProgramId; });
            if (program_id_entry == fields_span.end()) {
                return;
            }

            /* Check that the report has abort flag set. */
            AMS_ASSERT(util::range::any_of(fields_span, [](const FieldEntry &entry) { return entry.id == FieldId_AbortFlag && entry.value_bool; }));

            /* Check that the program id's value is a string. */
            AMS_ASSERT(program_id_entry->type  == FieldType_String);

            /* Check that the program id's length is valid/in bounds. */
            const auto program_id_ofs = program_id_entry->value_array.start_idx;
            const auto program_id_len = program_id_entry->value_array.size;
            AMS_ASSERT(16 <= program_id_len && program_id_len <= 17);
            AMS_ASSERT(program_id_ofs + program_id_len <= data_size);
            AMS_UNUSED(data_size);

            /* Get the program id string. */
            char program_id_str[17];
            std::memcpy(program_id_str, static_cast<const u8 *>(data) + program_id_ofs, std::min<size_t>(program_id_len, sizeof(program_id_str)));
            program_id_str[sizeof(program_id_str) - 1] = '\x00';

            /* Convert the string to an integer. */
            char *end_ptr = nullptr;
            const ncm::ProgramId program_id = { std::strtoull(program_id_str, std::addressof(end_ptr), 16) };
            AMS_ASSERT(*end_ptr == '\x00');

            /* Get the active duration. */
            const auto active_duration = g_applet_active_time_info_list.GetActiveDuration(program_id);
            if (!active_duration.has_value()) {
                return;
            }

            /* Add the active applet time. */
            const auto result = error_info_auto_record->Add(FieldId_AppletTotalActiveTime, (*active_duration).GetSeconds());
            R_ASSERT(result);
        }

        Result LinkAttachments(const ReportId &report_id, const AttachmentId *attachments, u32 num_attachments) {
            for (u32 i = 0; i < num_attachments; i++) {
                R_TRY(JournalForAttachments::SetOwner(attachments[i], report_id));
            }
            R_SUCCEED();
        }

        Result CreateReportFile(const ReportId &report_id, ReportType type, const ReportMetaData *meta, u32 num_attachments, const time::PosixTime &timestamp_user, const time::PosixTime &timestamp_network, bool redirect_new_reports) {
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

            record->m_info.type              = type;
            record->m_info.id                = report_id;
            record->m_info.flags             = erpt::srv::MakeNoReportFlags();
            record->m_info.timestamp_user    = timestamp_user;
            record->m_info.timestamp_network = timestamp_network;
            if (meta != nullptr) {
                record->m_info.meta_data = *meta;
            }
            if (num_attachments > 0) {
                record->m_info.flags.Set<ReportFlag::HasAttachment>();
            }

            auto report = std::make_unique<Report>(record.get(), redirect_new_reports);
            R_UNLESS(report != nullptr, erpt::ResultOutOfMemory());
            auto report_guard = SCOPE_GUARD { report->Delete(); };

            R_TRY(Context::WriteContextsToReport(report.get()));
            R_TRY(report->GetSize(std::addressof(record->m_info.report_size)));

            if (!redirect_new_reports) {
                /* If we're not redirecting new reports, then we want to store the report in the journal. */
                R_TRY(Journal::Store(record.get()));
            } else {
                /* If we are redirecting new reports, we don't want to store the report in the journal. */
                /* We should take this opportunity to delete any attachments associated with the report. */
                R_ABORT_UNLESS(JournalForAttachments::DeleteAttachments(report_id));
            }

            R_TRY(Journal::Commit());

            report_guard.Cancel();
            R_SUCCEED();
        }

    }

    Result Reporter::RegisterRunningApplet(ncm::ProgramId program_id) {
        g_applet_active_time_info_list.Register(program_id);
        R_SUCCEED();
    }

    Result Reporter::UnregisterRunningApplet(ncm::ProgramId program_id) {
        g_applet_active_time_info_list.Unregister(program_id);
        R_SUCCEED();
    }

    Result Reporter::UpdateAppletSuspendedDuration(ncm::ProgramId program_id, TimeSpan duration) {
        g_applet_active_time_info_list.UpdateSuspendedDuration(program_id, duration);
        R_SUCCEED();
    }

    Result Reporter::CreateReport(ReportType type, Result ctx_result, const ContextEntry *ctx, const u8 *data, u32 data_size, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, erpt::CreateReportOptionFlagSet flags, const ReportId *specified_report_id) {
        /* Create a context record for the report. */
        auto record = std::make_unique<ContextRecord>();
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        /* Initialize the record. */
        R_TRY(record->Initialize(ctx, data, data_size));

        /* Create the report. */
        R_RETURN(CreateReport(type, ctx_result, std::move(record), meta, attachments, num_attachments, flags, specified_report_id));
    }

    Result Reporter::CreateReport(ReportType type, Result ctx_result, std::unique_ptr<ContextRecord> record, const ReportMetaData *meta, const AttachmentId *attachments, u32 num_attachments, erpt::CreateReportOptionFlagSet flags, const ReportId *specified_report_id) {
        /* Clear the automatic categories, when we're done with our report. */
        ON_SCOPE_EXIT {
            Context::ClearContext(CategoryId_ErrorInfo);
            Context::ClearContext(CategoryId_ErrorInfoAuto);
            Context::ClearContext(CategoryId_ErrorInfoDefaults);
        };

        /* Get the context entry pointer. */
        const ContextEntry *ctx = record->GetContextEntryPtr();

        /* Validate the context. */
        R_TRY(ValidateCreateReportContext(ctx));

        /* Submit report defaults. */
        R_TRY(SubmitReportDefaults(ctx));

        /* Generate report id. */
        const ReportId report_id = specified_report_id ? *specified_report_id : ReportId{ .uuid = util::GenerateUuid() };

        /* Get posix timestamps. */
        time::PosixTime timestamp_user;
        time::PosixTime timestamp_network;
        R_TRY(time::StandardUserSystemClock::GetCurrentTime(std::addressof(timestamp_user)));
        if (R_FAILED(time::StandardNetworkSystemClock::GetCurrentTime(std::addressof(timestamp_network)))) {
            timestamp_network = {};
        }

        /* Save syslog report, if required. */
        SaveSyslogReportIfRequired(ctx, report_id);

        /* Submit report contexts. */
        R_TRY(SubmitReportContexts(report_id, type, ctx_result, std::move(record), timestamp_user, timestamp_network, flags));

        /* Link attachments to the report. */
        R_TRY(LinkAttachments(report_id, attachments, num_attachments));

        /* Create the report file. */
        R_TRY(CreateReportFile(report_id, type, meta, num_attachments, timestamp_user, timestamp_network, s_redirect_new_reports));

        R_SUCCEED();
    }

    Result Reporter::SubmitReportContexts(const ReportId &report_id, ReportType type, Result ctx_result, std::unique_ptr<ContextRecord> record, const time::PosixTime &timestamp_user, const time::PosixTime &timestamp_network, erpt::CreateReportOptionFlagSet flags) {
        /* Create automatic record. */
        auto auto_record = std::make_unique<ContextRecord>(CategoryId_ErrorInfoAuto, 0x300);
        R_UNLESS(auto_record != nullptr, erpt::ResultOutOfMemory());

        /* Handle error context. */
        if (R_FAILED(ctx_result)) {
            SubmitErrorContext(auto_record.get(), ctx_result);
        }

        /* Collect unique report fields. */
        char identifier_str[0x40];
        report_id.uuid.ToString(identifier_str, sizeof(identifier_str));

        const auto occurrence_tick = os::GetSystemTick();
        const s64 steady_clock_internal_offset_seconds = (hos::GetVersion() >= hos::Version_5_0_0) ? time::GetStandardSteadyClockInternalOffset().GetSeconds() : 0;

        time::SteadyClockTimePoint steady_clock_current_timepoint;
        R_ABORT_UNLESS(time::GetStandardSteadyClockCurrentTimePoint(std::addressof(steady_clock_current_timepoint)));

        /* Add automatic fields. */
        auto_record->Add(FieldId_OsVersion,                        s_os_version,                                 util::Strnlen(s_os_version, sizeof(s_os_version)));
        auto_record->Add(FieldId_PrivateOsVersion,                 s_private_os_version,                         util::Strnlen(s_private_os_version, sizeof(s_private_os_version)));
        auto_record->Add(FieldId_SerialNumber,                     s_serial_number,                              util::Strnlen(s_serial_number, sizeof(s_serial_number)));
        auto_record->Add(FieldId_ReportIdentifier,                 identifier_str,                               util::Strnlen(identifier_str, sizeof(identifier_str)));
        auto_record->Add(FieldId_OccurrenceTimestamp,              timestamp_user.value);
        auto_record->Add(FieldId_OccurrenceTimestampNet,           timestamp_network.value);
        auto_record->Add(FieldId_ReportVisibilityFlag,             type == ReportType_Visible);
        auto_record->Add(FieldId_OccurrenceTick,                   occurrence_tick.GetInt64Value());
        auto_record->Add(FieldId_SteadyClockInternalOffset,        steady_clock_internal_offset_seconds);
        auto_record->Add(FieldId_SteadyClockCurrentTimePointValue, steady_clock_current_timepoint.value);
        auto_record->Add(FieldId_ElapsedTimeSincePowerOn,          (occurrence_tick - *s_power_on_time).ToTimeSpan().GetSeconds());
        auto_record->Add(FieldId_ElapsedTimeSinceLastAwake,        (occurrence_tick - *s_awake_time).ToTimeSpan().GetSeconds());

        if (s_initial_launch_settings_completion_time) {
            s64 elapsed_seconds;
            if (R_SUCCEEDED(time::GetElapsedSecondsBetween(std::addressof(elapsed_seconds), *s_initial_launch_settings_completion_time, steady_clock_current_timepoint))) {
                auto_record->Add(FieldId_ElapsedTimeSinceInitialLaunch, elapsed_seconds);
            }
        }

        if (s_application_launch_time) {
            auto_record->Add(FieldId_ApplicationAliveTime, (occurrence_tick - *s_application_launch_time).ToTimeSpan().GetSeconds());
        }

        /* Submit applet active duration information. */
        {
            const auto *error_info_ctx = record->GetContextEntryPtr();
            SubmitAppletActiveDurationForCrashReport(error_info_ctx, error_info_ctx->array_buffer, error_info_ctx->array_buffer_size - error_info_ctx->array_free_count, auto_record.get());
        }

        /* Submit the auto record. */
        R_TRY(Context::SubmitContextRecord(std::move(auto_record)));

        /* Submit the info record. */
        R_TRY(Context::SubmitContextRecord(std::move(record)));

        /* Submit context for resource limits. */
        #if defined(ATMOSPHERE_OS_HORIZON)
        SubmitResourceLimitContexts();
        #endif

        /* If we should, submit fs info. */
        #if defined(ATMOSPHERE_OS_HORIZON)
        if (hos::GetVersion() >= hos::Version_17_0_0 && flags.Test<CreateReportOptionFlag::SubmitFsInfo>()) {
            /* NOTE: Nintendo ignores the result of this call. */
            SubmitFsInfo();
        }
        #else
        AMS_UNUSED(flags);
        #endif


        R_SUCCEED();
    }

}
