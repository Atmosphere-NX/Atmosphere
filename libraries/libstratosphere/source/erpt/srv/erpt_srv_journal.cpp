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
#include "erpt_srv_journal.hpp"

namespace ams::erpt::srv {

    void Journal::CleanupAttachments() {
        return JournalForAttachments::CleanupAttachments();
    }

    void Journal::CleanupReports() {
        return JournalForReports::CleanupReports();
    }

    Result Journal::Commit() {
        /* Open the stream. */
        Stream stream;
        R_TRY(stream.OpenStream(JournalFileName, StreamMode_Write, JournalStreamBufferSize));

        /* Commit the reports. */
        R_TRY(JournalForReports::CommitJournal(std::addressof(stream)));

        /* Commit the meta. */
        R_TRY(JournalForMeta::CommitJournal(std::addressof(stream)));

        /* Commit the attachments. */
        R_TRY(JournalForAttachments::CommitJournal(std::addressof(stream)));

        /* Close and commit the stream. */
        stream.CloseStream();
        stream.CommitStream();

        return ResultSuccess();
    }

    Result Journal::Delete(ReportId report_id) {
        return JournalForReports::DeleteReport(report_id);
    }

    Result Journal::GetAttachmentList(AttachmentList *out, ReportId report_id) {
        return JournalForAttachments::GetAttachmentList(out, report_id);
    }

    util::Uuid Journal::GetJournalId() {
        return JournalForMeta::GetJournalId();
    }

    s64 Journal::GetMaxReportSize() {
        return JournalForReports::GetMaxReportSize();
    }

    Result Journal::GetReportList(ReportList *out, ReportType type_filter) {
        return JournalForReports::GetReportList(out, type_filter);
    }

    u32 Journal::GetStoredReportCount(ReportType type) {
        return JournalForReports::GetStoredReportCount(type);
    }

    u32 Journal::GetTransmittedCount(ReportType type) {
        return JournalForMeta::GetTransmittedCount(type);
    }

    u32 Journal::GetUntransmittedCount(ReportType type) {
        return JournalForMeta::GetUntransmittedCount(type);
    }

    u32 Journal::GetUsedStorage() {
        return JournalForReports::GetUsedStorage() + JournalForAttachments::GetUsedStorage();
    }

    Result Journal::Restore() {
        /* Open the stream. */
        Stream stream;
        R_TRY(stream.OpenStream(JournalFileName, StreamMode_Read, JournalStreamBufferSize));

        /* Restore the reports. */
        R_TRY(JournalForReports::RestoreJournal(std::addressof(stream)));

        /* Restore the meta. */
        R_TRY(JournalForMeta::RestoreJournal(std::addressof(stream)));

        /* Restore the attachments. */
        R_TRY(JournalForAttachments::RestoreJournal(std::addressof(stream)));

        return ResultSuccess();
    }

    JournalRecord<ReportInfo> *Journal::Retrieve(ReportId report_id) {
        return JournalForReports::RetrieveRecord(report_id);
    }

    JournalRecord<AttachmentInfo> *Journal::Retrieve(AttachmentId attachment_id) {
        return JournalForAttachments::RetrieveRecord(attachment_id);
    }

    Result Journal::Store(JournalRecord<ReportInfo> *record) {
        return JournalForReports::StoreRecord(record);
    }

    Result Journal::Store(JournalRecord<AttachmentInfo> *record) {
        return JournalForAttachments::StoreRecord(record);
    }

}
