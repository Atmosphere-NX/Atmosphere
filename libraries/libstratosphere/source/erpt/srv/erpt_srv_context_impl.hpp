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

namespace ams::erpt::srv {

    class ContextImpl {
        public:
            Result SubmitContext(const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer);
            Result CreateReportV0(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer);
            Result SetInitialLaunchSettingsCompletionTime(const time::SteadyClockTimePoint &time_point);
            Result ClearInitialLaunchSettingsCompletionTime();
            Result UpdatePowerOnTime();
            Result UpdateAwakeTime();
            Result SubmitMultipleCategoryContext(const MultipleCategoryContextEntry &ctx_entry, const ams::sf::InBuffer &str_buffer);
            Result UpdateApplicationLaunchTime();
            Result ClearApplicationLaunchTime();
            Result SubmitAttachment(ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data);
            Result CreateReportWithAttachmentsDeprecated(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer);
            Result CreateReportWithAttachmentsDeprecated2(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result);
            Result CreateReportWithAttachments(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result, erpt::CreateReportOptionFlagSet flags);
            Result CreateReportV1(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result);
            Result CreateReport(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, Result result, erpt::CreateReportOptionFlagSet flags);
            Result SubmitAttachmentWithLz4Compression(ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data);
            Result CreateReportWithSpecifiedReprotId(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &data_buffer, const ams::sf::InBuffer &meta_buffer, const ams::sf::InBuffer &attachment_data, Result result, erpt::CreateReportOptionFlagSet flags, const ReportId &report_id);
            Result RegisterRunningApplet(ncm::ProgramId program_id);
            Result UnregisterRunningApplet(ncm::ProgramId program_id);
            Result UpdateAppletSuspendedDuration(ncm::ProgramId program_id, TimeSpanType duration);
            Result InvalidateForcedShutdownDetection();
    };
    static_assert(erpt::sf::IsIContext<ContextImpl>);

}
