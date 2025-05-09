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
#include <vapours.hpp>
#include <stratosphere/erpt/erpt_types.hpp>

#define AMS_ERPT_I_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                                                     \
    AMS_SF_METHOD_INFO(C, H,  0, Result, GetReportList,               (const ams::sf::OutBuffer &out_list, erpt::ReportType type_filter),                                (out_list, type_filter))                                                   \
    AMS_SF_METHOD_INFO(C, H,  1, Result, GetEvent,                    (ams::sf::OutCopyHandle out),                                                                      (out))                                                                     \
    AMS_SF_METHOD_INFO(C, H,  2, Result, CleanupReports,              (),                                                                                                (),                              hos::Version_4_0_0)                       \
    AMS_SF_METHOD_INFO(C, H,  3, Result, DeleteReport,                (const erpt::ReportId &report_id),                                                                 (report_id),                     hos::Version_5_0_0)                       \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GetStorageUsageStatistics,   (ams::sf::Out<erpt::StorageUsageStatistics> out),                                                  (out),                           hos::Version_5_0_0)                       \
    AMS_SF_METHOD_INFO(C, H,  5, Result, GetAttachmentListDeprecated, (const ams::sf::OutBuffer &out_buf, const erpt::ReportId &report_id),                              (out_buf, report_id),            hos::Version_8_0_0,  hos::Version_19_0_1) \
    AMS_SF_METHOD_INFO(C, H,  6, Result, GetAttachmentList,           (ams::sf::Out<u32> out_count, const ams::sf::OutBuffer &out_buf, const erpt::ReportId &report_id), (out_count, out_buf, report_id), hos::Version_20_0_0)                      \
    AMS_SF_METHOD_INFO(C, H, 10, Result, GetReportSizeMax,            (ams::sf::Out<u32> out),                                                                           (out),                           hos::Version_20_0_0)



AMS_SF_DEFINE_INTERFACE(ams::erpt::sf, IManager, AMS_ERPT_I_MANAGER_INTERFACE_INFO, 0x5CFCC43F)
