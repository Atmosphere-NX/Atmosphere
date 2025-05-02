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
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_ref_count.hpp"
#include "erpt_srv_journal_record.hpp"
#include "erpt_srv_stream.hpp"

namespace ams::erpt::srv {

    constexpr inline s32 JournalVersion = 1;

    constexpr inline u32 JournalStreamBufferSize = 4_KB;

    struct JournalMeta {
        s32 version;
        u32 transmitted_count[ReportType_Count];
        u32 untransmitted_count[ReportType_Count];
        util::Uuid journal_id;
        u32 reserved[4];
    };
    static_assert(sizeof(JournalMeta) == 0x34);

    class JournalForMeta {
        private:
            static JournalMeta s_journal_meta;
        public:
            static void InitializeJournal();
            static Result CommitJournal(Stream *stream);
            static Result RestoreJournal(Stream *stream);
            static u32 GetTransmittedCount(ReportType type);
            static u32 GetUntransmittedCount(ReportType type);
            static void IncrementCount(bool transmitted, ReportType type);
            static util::Uuid GetJournalId();
    };

    class JournalForReports {
        private:
            using RecordListType = util::IntrusiveListBaseTraits<JournalRecord<ReportInfo>>::ListType;
            static RecordListType s_record_list;
            static u32 s_record_count;
            static u32 s_record_count_by_type[ReportType_Count];
            static u32 s_used_storage;
        private:
            static void EraseReportImpl(JournalRecord<ReportInfo> *record, bool increment_count, bool force_delete_attachments);
        public:
            static void   CleanupReports();
            static Result CommitJournal(Stream *stream);
            static Result DeleteReport(ReportId report_id);
            static Result DeleteReportWithAttachments();
            static s64    GetMaxReportSize();
            static Result GetReportList(ReportList *out, ReportType type_filter);
            static u32    GetStoredReportCount(ReportType type);
            static u32    GetUsedStorage();
            static Result RestoreJournal(Stream *stream);

            static JournalRecord<ReportInfo> *RetrieveRecord(ReportId report_id);
            static Result StoreRecord(JournalRecord<ReportInfo> *record);
    };

    class JournalForAttachments {
        private:
            using AttachmentListType = util::IntrusiveListBaseTraits<JournalRecord<AttachmentInfo>>::ListType;
            static AttachmentListType s_attachment_list;
            static u32 s_attachment_count;
            static u32 s_used_storage;
        public:
            static void CleanupAttachments();
            static Result CommitJournal(Stream *stream);
            static Result DeleteAttachments(ReportId report_id);
            static Result GetAttachmentList(u32 *out_count, AttachmentInfo *out_infos, size_t max_out_infos, ReportId report_id);
            static u32    GetUsedStorage();
            static Result RestoreJournal(Stream *stream);

            static JournalRecord<AttachmentInfo> *RetrieveRecord(AttachmentId attachment_id);
            static Result SetOwner(AttachmentId attachment_id, ReportId report_id);
            static Result StoreRecord(JournalRecord<AttachmentInfo> *record);

            static Result SubmitAttachment(AttachmentId *out, char *name, const u8 *data, u32 data_size);
    };

    class Journal {
        public:
            static void       CleanupAttachments();
            static void       CleanupReports();
            static Result     Commit();
            static Result     Delete(ReportId report_id);
            static Result     GetAttachmentList(u32 *out_count, AttachmentInfo *out_infos, size_t max_out_infos, ReportId report_id);
            static util::Uuid GetJournalId();
            static s64        GetMaxReportSize();
            static Result     GetReportList(ReportList *out, ReportType type_filter);
            static u32        GetStoredReportCount(ReportType type);
            static u32        GetTransmittedCount(ReportType type);
            static u32        GetUntransmittedCount(ReportType type);
            static u32        GetUsedStorage();
            static Result     Restore();

            static JournalRecord<ReportInfo> *Retrieve(ReportId report_id);
            static JournalRecord<AttachmentInfo> *Retrieve(AttachmentId attachment_id);

            static Result Store(JournalRecord<ReportInfo> *record);
            static Result Store(JournalRecord<AttachmentInfo> *record);
    };

}
