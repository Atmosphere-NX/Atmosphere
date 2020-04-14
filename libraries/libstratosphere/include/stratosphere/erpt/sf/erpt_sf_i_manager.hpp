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
#include <stratosphere/erpt/erpt_types.hpp>

namespace ams::erpt::sf {

    class IManager : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                GetReportList             = 0,
                GetEvent                  = 1,
                CleanupReports            = 2,
                DeleteReport              = 3,
                GetStorageUsageStatistics = 4,
                GetAttachmentList         = 5,
            };
        public:
            /* Actual commands. */
            virtual Result GetReportList(const ams::sf::OutBuffer &out_list, ReportType type_filter) = 0;
            virtual Result GetEvent(ams::sf::OutCopyHandle out) = 0;
            virtual Result CleanupReports() = 0;
            virtual Result DeleteReport(const ReportId &report_id) = 0;
            virtual Result GetStorageUsageStatistics(ams::sf::Out<StorageUsageStatistics> out) = 0;
            virtual Result GetAttachmentList(const ams::sf::OutBuffer &out_buf, const ReportId &report_id) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetReportList),
                MAKE_SERVICE_COMMAND_META(GetEvent),
                MAKE_SERVICE_COMMAND_META(CleanupReports,            hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(DeleteReport,              hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(GetStorageUsageStatistics, hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(GetAttachmentList,         hos::Version_8_0_0),
            };
    };

}