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
        R_SUCCEED();
    }

    Result ManagerImpl::GetReportList(const ams::sf::OutBuffer &out_list, ReportType type_filter) {
        R_UNLESS(out_list.GetSize() == sizeof(ReportList), erpt::ResultInvalidArgument());

        R_RETURN(Journal::GetReportList(reinterpret_cast<ReportList *>(out_list.GetPointer()), type_filter));
    }

    Result ManagerImpl::GetEvent(ams::sf::OutCopyHandle out) {
        out.SetValue(m_system_event.GetReadableHandle(), false);
        R_SUCCEED();
    }

    Result ManagerImpl::CleanupReports() {
        Journal::CleanupReports();
        Journal::CleanupAttachments();
        R_RETURN(Journal::Commit());
    }

    Result ManagerImpl::DeleteReport(const ReportId &report_id) {
        R_TRY(Journal::Delete(report_id));
        R_RETURN(Journal::Commit());
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
        R_SUCCEED();
    }

    Result ManagerImpl::GetAttachmentListDeprecated(const ams::sf::OutBuffer &out_list, const ReportId &report_id) {
        R_UNLESS(out_list.GetSize() == sizeof(AttachmentList), erpt::ResultInvalidArgument());

        auto *attachment_list = reinterpret_cast<AttachmentList *>(out_list.GetPointer());

        R_RETURN(Journal::GetAttachmentList(std::addressof(attachment_list->attachment_count), attachment_list->attachments, util::size(attachment_list->attachments), report_id));
    }

    Result ManagerImpl::GetAttachmentList(ams::sf::Out<u32> out_count, const ams::sf::OutBuffer &out_buf, const ReportId &report_id) {
        R_RETURN(Journal::GetAttachmentList(out_count.GetPointer(), reinterpret_cast<AttachmentInfo *>(out_buf.GetPointer()), out_buf.GetSize() / sizeof(AttachmentInfo), report_id));
    }

    Result ManagerImpl::GetReportSizeMax(ams::sf::Out<u32> out) {
        /* TODO: Where is this size defined? */
        constexpr size_t ReportSizeMax = 0x3FF4F;
        *out = ReportSizeMax;
        R_SUCCEED();
    }

}
