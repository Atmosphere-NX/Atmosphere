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
#include "erpt_srv_context_impl.hpp"
#include "erpt_srv_manager_impl.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_journal.hpp"
#include "erpt_srv_forced_shutdown.hpp"

namespace ams::erpt::srv {

    namespace {

        ContextEntry MakeContextEntry(const CategoryEntry &cat_entry, Span<const FieldEntry> field_entries) {
            /* Check pre-conditions. */
            AMS_ASSERT(cat_entry.field_count <= field_entries.size());

            /* Make the entry. */
            ContextEntry entry = {};

            entry.version     = 0;
            entry.category    = cat_entry.category;
            entry.field_count = cat_entry.field_count;

            for (size_t i = 0; i < cat_entry.field_count; ++i) {
                entry.fields[i] = field_entries[i];
            }

            return entry;
        }

        Result SubmitMultipleContextImpl(Span<const CategoryEntry> category_entries, Span<const FieldEntry> field_entries, Span<const u8> array_buf) {
            /* Iterate over all category entries. */
            size_t field_entry_offset = 0;
            size_t array_buf_offset   = 0;
            for (const auto &category_entry : category_entries) {
                /* Check that the category is valid. */
                R_UNLESS(erpt::srv::IsValidCategory(category_entry.category),  erpt::ResultInvalidArgument());

                /* Check that there aren't too many fields for the category. */
                R_UNLESS(category_entry.field_count <= FieldsPerContext,          erpt::ResultInvalidArgument());

                /* Check that there isn't too much data in the array buf. */
                R_UNLESS(category_entry.array_buffer_count <= ArrayBufferSizeMax, erpt::ResultInvalidArgument());

                /* Check that the fields/data fit into the provided buffer. */
                R_UNLESS(category_entry.field_count + field_entry_offset <= field_entries.size(),        erpt::ResultInvalidArgument());
                R_UNLESS(category_entry.array_buffer_count + array_buf_offset <= array_buf.size_bytes(), erpt::ResultInvalidArgument());

                /* Make the entry. */
                const auto ctx_entry = MakeContextEntry(category_entry, field_entries.subspan(field_entry_offset, category_entry.field_count));
                R_TRY(Context::SubmitContext(std::addressof(ctx_entry), array_buf.data() + array_buf_offset, category_entry.array_buffer_count));

                /* Advance. */
                field_entry_offset += category_entry.field_count;
                array_buf_offset   += category_entry.array_buffer_count;
            }

            /* We succeeded. */
            R_SUCCEED();
        }

    }

    Result ContextImpl::SubmitContext(const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer) {
        const ContextEntry *ctx  = reinterpret_cast<const ContextEntry *>( ctx_buffer.GetPointer());
        const           u8 *data = reinterpret_cast<const           u8 *>(data_buffer.GetPointer());

        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());

        R_UNLESS(ctx_size == sizeof(ContextEntry), erpt::ResultInvalidArgument());
        R_UNLESS(data_size <= ArrayBufferSizeMax,  erpt::ResultInvalidArgument());

        SubmitContextForForcedShutdownDetection(ctx, data, data_size);

        R_RETURN(Context::SubmitContext(ctx, data, data_size));
    }

    Result ContextImpl::CreateReport(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result, erpt::CreateReportOptionFlagSet flags) {
        const   ContextEntry *ctx  = reinterpret_cast<const   ContextEntry *>( ctx_buffer.GetPointer());
        const             u8 *data = reinterpret_cast<const             u8 *>(data_buffer.GetPointer());
        const ReportMetaData *meta = reinterpret_cast<const ReportMetaData *>(meta_buffer.GetPointer());

        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());
        const u32 meta_size = static_cast<u32>(meta_buffer.GetSize());

        R_UNLESS(ctx_size == sizeof(ContextEntry),                      erpt::ResultInvalidArgument());
        R_UNLESS(meta_size == 0 || meta_size == sizeof(ReportMetaData), erpt::ResultInvalidArgument());

        R_TRY(Reporter::CreateReport(report_type, result, ctx, data, data_size, meta_size != 0 ? meta : nullptr, nullptr, 0, flags, nullptr));

        ManagerImpl::NotifyAll();

        R_SUCCEED();
    }

    Result ContextImpl::CreateReportV1(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result) {
        R_RETURN(this->CreateReport(report_type, ctx_buffer, data_buffer, meta_buffer, result, erpt::srv::MakeNoCreateReportOptionFlags()));
    }

    Result ContextImpl::CreateReportV0(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer) {
        R_RETURN(this->CreateReportV1(report_type, ctx_buffer, data_buffer, meta_buffer, ResultSuccess()));
    }

    Result ContextImpl::SetInitialLaunchSettingsCompletionTime(const time::SteadyClockTimePoint &time_point) {
        Reporter::SetInitialLaunchSettingsCompletionTime(time_point);
        R_SUCCEED();
    }

    Result ContextImpl::ClearInitialLaunchSettingsCompletionTime() {
        Reporter::ClearInitialLaunchSettingsCompletionTime();
        R_SUCCEED();
    }

    Result ContextImpl::UpdatePowerOnTime() {
        /* NOTE: Prior to 12.0.0, this set the power on time, but now erpt does it during initialization. */
        R_SUCCEED();
    }

    Result ContextImpl::UpdateAwakeTime() {
        /* NOTE: Prior to 12.0.0, this set the power on time, but now erpt does it during initialization. */
        R_SUCCEED();
    }

    Result ContextImpl::CreateReportWithAdditionalContext(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result, erpt::CreateReportOptionFlagSet flags, const ams::sf::InMapAliasArray<erpt::CategoryEntry> &category_entries, const ams::sf::InMapAliasArray<erpt::FieldEntry> &field_entries, const ams::sf::InBuffer &array_buffer) {
        /* Submit the additional context. */
        R_TRY(SubmitMultipleContextImpl(category_entries.ToSpan(), field_entries.ToSpan(), MakeSpan<const u8>(array_buffer.GetPointer(), array_buffer.GetSize())));

        /* Clear the additional context when we're done. */
        ON_SCOPE_EXIT {
            const auto category_span = category_entries.ToSpan();

            for (const auto &entry : category_span) {
                if (erpt::srv::IsValidCategory(entry.category)) {
                    static_cast<void>(Context::ClearContext(entry.category));
                }
            }
        };

        /* Create the report. */
        R_TRY(this->CreateReport(report_type, ctx_buffer, data_buffer, meta_buffer, result, flags));

        /* We succeeded. */
        R_SUCCEED();
    }

    Result ContextImpl::SubmitMultipleCategoryContext(const MultipleCategoryContextEntry &ctx_entry, const ams::sf::InBuffer &str_buffer) {
        R_UNLESS(ctx_entry.category_count <= CategoriesPerMultipleCategoryContext, erpt::ResultInvalidArgument());

        const u8 *str      = reinterpret_cast<const u8 *>(str_buffer.GetPointer());
        const u32 str_size = static_cast<u32>(str_buffer.GetSize());

        u32 total_field_count = 0, total_arr_count = 0;
        for (u32 i = 0; i < ctx_entry.category_count; i++) {
            ContextEntry entry = {
                .version     = ctx_entry.version,
                .field_count = ctx_entry.field_counts[i],
                .category    = ctx_entry.categories[i],
            };
            R_UNLESS(entry.field_count <= erpt::FieldsPerContext,                                     erpt::ResultInvalidArgument());
            R_UNLESS(entry.field_count + total_field_count <= erpt::FieldsPerMultipleCategoryContext, erpt::ResultInvalidArgument());
            R_UNLESS(ctx_entry.array_buf_counts[i] <= ArrayBufferSizeMax,                             erpt::ResultInvalidArgument());
            R_UNLESS(ctx_entry.array_buf_counts[i] + total_arr_count <= str_size,                     erpt::ResultInvalidArgument());

            std::memcpy(entry.fields, ctx_entry.fields + total_field_count, entry.field_count * sizeof(FieldEntry));

            R_TRY(Context::SubmitContext(std::addressof(entry), str + total_arr_count, ctx_entry.array_buf_counts[i]));

            total_field_count += entry.field_count;
            total_arr_count   += ctx_entry.array_buf_counts[i];
        }

        R_SUCCEED();
    }

    Result ContextImpl::SubmitMultipleContext(const ams::sf::InMapAliasArray<erpt::CategoryEntry> &category_entries, const ams::sf::InMapAliasArray<erpt::FieldEntry> &field_entries, const ams::sf::InBuffer &array_buffer) {
        R_RETURN(SubmitMultipleContextImpl(category_entries.ToSpan(), field_entries.ToSpan(), MakeSpan<const u8>(array_buffer.GetPointer(), array_buffer.GetSize())));
    }

    Result ContextImpl::UpdateApplicationLaunchTime() {
        Reporter::UpdateApplicationLaunchTime();
        R_SUCCEED();
    }

    Result ContextImpl::ClearApplicationLaunchTime() {
        Reporter::ClearApplicationLaunchTime();
        R_SUCCEED();
    }

    Result ContextImpl::RegisterRunningApplicationInfo(ncm::ApplicationId app_id, ncm::ProgramId program_id) {
        Reporter::RegisterRunningApplicationInfo(app_id, program_id);
        R_SUCCEED();
    }

    Result ContextImpl::UnregisterRunningApplicationInfo() {
        Reporter::UnregisterRunningApplicationInfo();
        R_SUCCEED();
    }

    Result ContextImpl::SubmitAttachment(ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data) {
        const char *name = reinterpret_cast<const char *>(attachment_name.GetPointer());
        const   u8 *data = reinterpret_cast<const   u8 *>(attachment_data.GetPointer());

        const u32 name_size = static_cast<u32>(attachment_name.GetSize());
        const u32 data_size = static_cast<u32>(attachment_data.GetSize());

        R_UNLESS(data != nullptr,                    erpt::ResultInvalidArgument());
        R_UNLESS(data_size <= AttachmentSizeMax,     erpt::ResultInvalidArgument());
        R_UNLESS(name != nullptr,                    erpt::ResultInvalidArgument());
        R_UNLESS(name_size <= AttachmentNameSizeMax, erpt::ResultInvalidArgument());

        char name_safe[AttachmentNameSizeMax];
        util::Strlcpy(name_safe, name, sizeof(name_safe));

        R_RETURN(JournalForAttachments::SubmitAttachment(out.GetPointer(), name_safe, data, data_size));
    }

    Result ContextImpl::SubmitAttachmentWithLz4Compression(ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data) {
        /* TODO: Implement LZ4 compression on attachments. */
        R_RETURN(this->SubmitAttachment(out, attachment_name, attachment_data));
    }

    Result ContextImpl::CreateReportWithAttachments(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result, erpt::CreateReportOptionFlagSet flags) {
        const ContextEntry *ctx  = reinterpret_cast<const ContextEntry *>( ctx_buffer.GetPointer());
        const           u8 *data = reinterpret_cast<const           u8 *>(data_buffer.GetPointer());
        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());

        const AttachmentId *attachments = reinterpret_cast<const AttachmentId *>(attachment_ids_buffer.GetPointer());
        const u32 num_attachments = attachment_ids_buffer.GetSize() / sizeof(*attachments);

        R_UNLESS(ctx_size == sizeof(ContextEntry),           erpt::ResultInvalidArgument());
        R_UNLESS(num_attachments <= AttachmentsPerReportMax, erpt::ResultInvalidArgument());

        R_TRY(Reporter::CreateReport(report_type, result, ctx, data, data_size, nullptr, attachments, num_attachments, flags, nullptr));

        ManagerImpl::NotifyAll();

        R_SUCCEED();
    }

    Result ContextImpl::CreateReportWithAttachmentsDeprecated2(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result) {
        R_RETURN(this->CreateReportWithAttachments(report_type, ctx_buffer, data_buffer, attachment_ids_buffer, result, erpt::srv::MakeNoCreateReportOptionFlags()));
    }

    Result ContextImpl::CreateReportWithAttachmentsDeprecated(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer) {
        R_RETURN(this->CreateReportWithAttachmentsDeprecated2(report_type, ctx_buffer, data_buffer, attachment_ids_buffer, ResultSuccess()));
    }

    Result ContextImpl::CreateReportWithSpecifiedReprotId(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result, erpt::CreateReportOptionFlagSet flags, const ReportId &report_id) {
        const   ContextEntry *ctx  = reinterpret_cast<const   ContextEntry *>( ctx_buffer.GetPointer());
        const             u8 *data = reinterpret_cast<const             u8 *>(data_buffer.GetPointer());
        const ReportMetaData *meta = reinterpret_cast<const ReportMetaData *>(meta_buffer.GetPointer());

        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());
        const u32 meta_size = static_cast<u32>(meta_buffer.GetSize());

        const AttachmentId *attachments = reinterpret_cast<const AttachmentId *>(attachment_ids_buffer.GetPointer());
        const u32 num_attachments = attachment_ids_buffer.GetSize() / sizeof(*attachments);

        R_UNLESS(ctx_size == sizeof(ContextEntry),                      erpt::ResultInvalidArgument());
        R_UNLESS(meta_size == 0 || meta_size == sizeof(ReportMetaData), erpt::ResultInvalidArgument());
        R_UNLESS(num_attachments <= AttachmentsPerReportMax,            erpt::ResultInvalidArgument());

        R_TRY(Reporter::CreateReport(report_type, result, ctx, data, data_size, meta_size != 0 ? meta : nullptr, attachments, num_attachments, flags, std::addressof(report_id)));

        ManagerImpl::NotifyAll();

        R_SUCCEED();
    }

    Result ContextImpl::RegisterRunningApplet(ncm::ProgramId program_id) {
        R_RETURN(Reporter::RegisterRunningApplet(program_id));
    }

    Result ContextImpl::UnregisterRunningApplet(ncm::ProgramId program_id) {
        R_RETURN(Reporter::UnregisterRunningApplet(program_id));
    }

    Result ContextImpl::UpdateAppletSuspendedDuration(ncm::ProgramId program_id, TimeSpanType duration) {
        R_RETURN(Reporter::UpdateAppletSuspendedDuration(program_id, duration));
    }

    Result ContextImpl::InvalidateForcedShutdownDetection() {
        /* NOTE: Nintendo does not check the result here. */
        static_cast<void>(erpt::srv::InvalidateForcedShutdownDetection());
        R_SUCCEED();
    }

    Result ContextImpl::WaitForReportCreation() {
        /* This function currently does nothing. Maybe it only waits on Ounce? */
        R_SUCCEED();
    }

}
