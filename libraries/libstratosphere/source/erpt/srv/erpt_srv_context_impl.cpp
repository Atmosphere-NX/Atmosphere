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
#include "erpt_srv_context_impl.hpp"
#include "erpt_srv_manager_impl.hpp"
#include "erpt_srv_context.hpp"
#include "erpt_srv_reporter.hpp"
#include "erpt_srv_journal.hpp"

namespace ams::erpt::srv {

    Result ContextImpl::SubmitContext(const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer) {
        const ContextEntry *ctx  = reinterpret_cast<const ContextEntry *>( ctx_buffer.GetPointer());
        const           u8 *data = reinterpret_cast<const           u8 *>(data_buffer.GetPointer());

        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());

        R_UNLESS(ctx_size == sizeof(ContextEntry), erpt::ResultInvalidArgument());
        R_UNLESS(data_size <= ArrayBufferSizeMax,  erpt::ResultInvalidArgument());

        return Context::SubmitContext(ctx, data, data_size);
    }

    Result ContextImpl::CreateReport(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result) {
        const   ContextEntry *ctx  = reinterpret_cast<const   ContextEntry *>( ctx_buffer.GetPointer());
        const             u8 *data = reinterpret_cast<const             u8 *>(data_buffer.GetPointer());
        const ReportMetaData *meta = reinterpret_cast<const ReportMetaData *>(meta_buffer.GetPointer());

        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());
        const u32 meta_size = static_cast<u32>(meta_buffer.GetSize());

        R_UNLESS(ctx_size == sizeof(ContextEntry),                      erpt::ResultInvalidArgument());
        R_UNLESS(meta_size == 0 || meta_size == sizeof(ReportMetaData), erpt::ResultInvalidArgument());

        Reporter reporter(report_type, ctx, data, data_size, meta_size != 0 ? meta : nullptr, nullptr, 0, result);
        R_TRY(reporter.CreateReport());

        ManagerImpl::NotifyAll();

        return ResultSuccess();
    }

    Result ContextImpl::CreateReportV0(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer) {
        return this->CreateReport(report_type, ctx_buffer, data_buffer, meta_buffer, ResultSuccess());
    }

    Result ContextImpl::SetInitialLaunchSettingsCompletionTime(const time::SteadyClockTimePoint &time_point) {
        Reporter::SetInitialLaunchSettingsCompletionTime(time_point);
        return ResultSuccess();
    }

    Result ContextImpl::ClearInitialLaunchSettingsCompletionTime() {
        Reporter::ClearInitialLaunchSettingsCompletionTime();
        return ResultSuccess();
    }

    Result ContextImpl::UpdatePowerOnTime() {
        Reporter::UpdatePowerOnTime();
        return ResultSuccess();
    }

    Result ContextImpl::UpdateAwakeTime() {
        Reporter::UpdateAwakeTime();
        return ResultSuccess();
    }

    Result ContextImpl::SubmitMultipleCategoryContext(const MultipleCategoryContextEntry &ctx_entry, const ams::sf::InBuffer &str_buffer) {
        R_UNLESS(0 <= ctx_entry.category_count && ctx_entry.category_count <= CategoriesPerMultipleCategoryContext, erpt::ResultInvalidArgument());

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

        return ResultSuccess();
    }

    Result ContextImpl::UpdateApplicationLaunchTime() {
        Reporter::UpdateApplicationLaunchTime();
        return ResultSuccess();
    }

    Result ContextImpl::ClearApplicationLaunchTime() {
        Reporter::ClearApplicationLaunchTime();
        return ResultSuccess();
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

        return JournalForAttachments::SubmitAttachment(out.GetPointer(), name_safe, data, data_size);
    }

    Result ContextImpl::CreateReportWithAttachments(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result) {
        const ContextEntry *ctx  = reinterpret_cast<const ContextEntry *>( ctx_buffer.GetPointer());
        const           u8 *data = reinterpret_cast<const           u8 *>(data_buffer.GetPointer());
        const u32 ctx_size  = static_cast<u32>(ctx_buffer.GetSize());
        const u32 data_size = static_cast<u32>(data_buffer.GetSize());

        const AttachmentId *attachments = reinterpret_cast<const AttachmentId *>(attachment_ids_buffer.GetPointer());
        const u32 num_attachments = attachment_ids_buffer.GetSize() / sizeof(*attachments);

        R_UNLESS(ctx_size == sizeof(ContextEntry),           erpt::ResultInvalidArgument());
        R_UNLESS(num_attachments <= AttachmentsPerReportMax, erpt::ResultInvalidArgument());

        Reporter reporter(report_type, ctx, data, data_size, nullptr, attachments, num_attachments, result);
        R_TRY(reporter.CreateReport());

        ManagerImpl::NotifyAll();

        return ResultSuccess();
    }

    Result ContextImpl::CreateReportWithAttachmentsDeprecated(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer) {
        return this->CreateReportWithAttachments(report_type, ctx_buffer, data_buffer, attachment_ids_buffer, ResultSuccess());
    }

}
