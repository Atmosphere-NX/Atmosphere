/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/erpt/erpt_types.hpp>
#include <stratosphere/erpt/erpt_multiple_category_context.hpp>
#include <stratosphere/time/time_steady_clock_time_point.hpp>

namespace ams::erpt::sf {

    class IContext : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                SubmitContext                            = 0,
                CreateReport                             = 1,
                SetInitialLaunchSettingsCompletionTime   = 2,
                ClearInitialLaunchSettingsCompletionTime = 3,
                UpdatePowerOnTime                        = 4,
                UpdateAwakeTime                          = 5,
                SubmitMultipleCategoryContext            = 6,
                UpdateApplicationLaunchTime              = 7,
                ClearApplicationLaunchTime               = 8,
                SubmitAttachment                         = 9,
                CreateReportWithAttachments              = 10,
            };
        public:
            /* Actual commands. */
            virtual Result SubmitContext(const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer) = 0;
            virtual Result CreateReport(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &meta_buffer) = 0;
            virtual Result SetInitialLaunchSettingsCompletionTime(const time::SteadyClockTimePoint &time_point) = 0;
            virtual Result ClearInitialLaunchSettingsCompletionTime() = 0;
            virtual Result UpdatePowerOnTime() = 0;
            virtual Result UpdateAwakeTime() = 0;
            virtual Result SubmitMultipleCategoryContext(const MultipleCategoryContextEntry &ctx_entry, const ams::sf::InBuffer &str_buffer) = 0;
            virtual Result UpdateApplicationLaunchTime() = 0;
            virtual Result ClearApplicationLaunchTime() = 0;
            virtual Result SubmitAttachment(ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data) = 0;
            virtual Result CreateReportWithAttachments(ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &attachment_ids_buffer) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(SubmitContext),
                MAKE_SERVICE_COMMAND_META(CreateReport),
                MAKE_SERVICE_COMMAND_META(SetInitialLaunchSettingsCompletionTime,   hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(ClearInitialLaunchSettingsCompletionTime, hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(UpdatePowerOnTime,                        hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(UpdateAwakeTime,                          hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(SubmitMultipleCategoryContext,            hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(UpdateApplicationLaunchTime,              hos::Version_6_0_0),
                MAKE_SERVICE_COMMAND_META(ClearApplicationLaunchTime,               hos::Version_6_0_0),
                MAKE_SERVICE_COMMAND_META(SubmitAttachment,                         hos::Version_8_0_0),
                MAKE_SERVICE_COMMAND_META(CreateReportWithAttachments,              hos::Version_8_0_0),
            };
    };

}