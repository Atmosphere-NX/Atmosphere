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
#include "erpt_srv_manager_impl.hpp"
#include "erpt_srv_journal.hpp"

namespace ams::erpt::srv {

    namespace {

        using ManagerList = util::IntrusiveListBaseTraits<ManagerImpl>::ListType;

        constinit ManagerList g_manager_list;

    }

    ManagerImpl::ManagerImpl() : m_system_event(os::EventClearMode_AutoClear, true) {
        g_manager_list.push_front(*this);
    }

    ManagerImpl::~ManagerImpl() {
        g_manager_list.erase(g_manager_list.iterator_to(*this));
    }

    void ManagerImpl::NotifyOne() {
        m_system_event.Signal();
    }

    Result ManagerImpl::NotifyAll() {
        for (auto &manager : g_manager_list) {
            manager.NotifyOne();
        }
        return ResultSuccess();
    }

    Result ManagerImpl::GetReportList(const ams::sf::OutBuffer &out_list, ReportType type_filter) {
        R_UNLESS(out_list.GetSize() == sizeof(ReportList), erpt::ResultInvalidArgument());

        return Journal::GetReportList(reinterpret_cast<ReportList *>(out_list.GetPointer()), type_filter);
    }

    Result ManagerImpl::GetEvent(ams::sf::OutCopyHandle out) {
        out.SetValue(m_system_event.GetReadableHandle(), false);
        return ResultSuccess();
    }

    Result ManagerImpl::CleanupReports() {
        Journal::CleanupReports();
        Journal::CleanupAttachments();
        return Journal::Commit();
    }

    Result ManagerImpl::DeleteReport(const ReportId &report_id) {
        R_TRY(Journal::Delete(report_id));
        return Journal::Commit();
    }

    Result ManagerImpl::GetStorageUsageStatistics(ams::sf::Out<StorageUsageStatistics> out) {
        StorageUsageStatistics stats = {};

        stats.journal_uuid = Journal::GetJournalId();
        stats.used_storage_size = Journal::GetUsedStorage();
        stats.max_report_size    = Journal::GetMaxReportSize();

        for (int i = ReportType_Start; i < ReportType_End; i++) {
            const auto type = static_cast<ReportType>(i);
            stats.report_count[i]        = Journal::GetStoredReportCount(type);
            stats.transmitted_count[i]   = Journal::GetTransmittedCount(type);
            stats.untransmitted_count[i] = Journal::GetUntransmittedCount(type);
        }

        out.SetValue(stats);
        return ResultSuccess();
    }

    Result ManagerImpl::GetAttachmentList(const ams::sf::OutBuffer &out_list, const ReportId &report_id) {
        R_UNLESS(out_list.GetSize() == sizeof(AttachmentList), erpt::ResultInvalidArgument());

        return Journal::GetAttachmentList(reinterpret_cast<AttachmentList *>(out_list.GetPointer()), report_id);
    }

}
