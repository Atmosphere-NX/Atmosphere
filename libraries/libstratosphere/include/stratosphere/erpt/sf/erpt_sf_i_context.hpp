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

    #define AMS_ERPT_I_CONTEXT_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H,  0, Result, SubmitContext,                             (const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer))                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H,  1, Result, CreateReportV0,                            (ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &meta_buffer))                                                                  \
        AMS_SF_METHOD_INFO(C, H,  2, Result, SetInitialLaunchSettingsCompletionTime,    (const time::SteadyClockTimePoint &time_point),                                                                                                                    hos::Version_3_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  3, Result, ClearInitialLaunchSettingsCompletionTime,  (),                                                                                                                                                                hos::Version_3_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  4, Result, UpdatePowerOnTime,                         (),                                                                                                                                                                hos::Version_3_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  5, Result, UpdateAwakeTime,                           (),                                                                                                                                                                hos::Version_3_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  6, Result, SubmitMultipleCategoryContext,             (const MultipleCategoryContextEntry &ctx_entry, const ams::sf::InBuffer &str_buffer),                                                                              hos::Version_5_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  7, Result, UpdateApplicationLaunchTime,               (),                                                                                                                                                                hos::Version_6_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  8, Result, ClearApplicationLaunchTime,                (),                                                                                                                                                                hos::Version_6_0_0)                       \
        AMS_SF_METHOD_INFO(C, H,  9, Result, SubmitAttachment,                          (ams::sf::Out<AttachmentId> out, const ams::sf::InBuffer &attachment_name, const ams::sf::InBuffer &attachment_data),                                              hos::Version_8_0_0)                       \
        AMS_SF_METHOD_INFO(C, H, 10, Result, CreateReportWithAttachmentsDeprecated,     (ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &attachment_ids_buffer),                hos::Version_8_0_0,  hos::Version_10_2_0) \
        AMS_SF_METHOD_INFO(C, H, 10, Result, CreateReportWithAttachments,               (ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &attachment_ids_buffer, Result result), hos::Version_11_0_0)                      \
        AMS_SF_METHOD_INFO(C, H, 11, Result, CreateReport,                              (ReportType report_type, const ams::sf::InBuffer &ctx_buffer, const ams::sf::InBuffer &str_buffer, const ams::sf::InBuffer &meta_buffer, Result result),           hos::Version_11_0_0)


    AMS_SF_DEFINE_INTERFACE(IContext, AMS_ERPT_I_CONTEXT_INTERFACE_INFO)

}