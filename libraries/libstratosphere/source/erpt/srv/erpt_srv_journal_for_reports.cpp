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
#include "erpt_srv_journal.hpp"
#include "erpt_srv_report.hpp"

namespace ams::erpt::srv {

    util::IntrusiveListBaseTraits<JournalRecord<ReportInfo>>::ListType JournalForReports::s_record_list;
    u32 JournalForReports::s_record_count = 0;
    u32 JournalForReports::s_record_count_by_type[ReportType_Count] = {};
    u32 JournalForReports::s_used_storage = 0;

    void JournalForReports::CleanupReports() {
        for (auto it = s_record_list.begin(); it != s_record_list.end(); /* ... */) {
            auto *record = std::addressof(*it);
            it = s_record_list.erase(s_record_list.iterator_to(*record));
            if (record->RemoveReference()) {
                Stream::DeleteStream(Report::FileName(record->info.id, false).name);
                delete record;
            }
        }
        AMS_ASSERT(s_record_list.empty());

        s_record_count = 0;
        s_used_storage = 0;

        std::memset(s_record_count_by_type, 0, sizeof(s_record_count_by_type));
    }

    Result JournalForReports::CommitJournal(Stream *stream) {
        R_TRY(stream->WriteStream(reinterpret_cast<const u8 *>(std::addressof(s_record_count)), sizeof(s_record_count)));
        for (auto it = s_record_list.crbegin(); it != s_record_list.crend(); it++) {
            R_TRY(stream->WriteStream(reinterpret_cast<const u8 *>(std::addressof(it->info)), sizeof(it->info)));
        }
        return ResultSuccess();
    }

    void JournalForReports::EraseReportImpl(JournalRecord<ReportInfo> *record, bool increment_count, bool force_delete_attachments) {
        /* Erase from the list. */
        s_record_list.erase(s_record_list.iterator_to(*record));

        /* Update storage tracking counts. */
        --s_record_count;
        --s_record_count_by_type[record->info.type];
        s_used_storage -= static_cast<u32>(record->info.report_size);

        /* If we should increment count, do so. */
        if (increment_count) {
            JournalForMeta::IncrementCount(record->info.flags.Test<ReportFlag::Transmitted>(), record->info.type);
        }

        /* Delete any attachments. */
        if (force_delete_attachments || record->info.flags.Test<ReportFlag::HasAttachment>()) {
            JournalForAttachments::DeleteAttachments(record->info.id);
        }

        /* Delete the object, if we should. */
        if (record->RemoveReference()) {
            Stream::DeleteStream(Report::FileName(record->info.id, false).name);
            delete record;
        }
    }

    Result JournalForReports::DeleteReport(ReportId report_id) {
        for (auto it = s_record_list.begin(); it != s_record_list.end(); it++) {
            auto *record = std::addressof(*it);
            if (record->info.id == report_id) {
                EraseReportImpl(record, false, false);
                return ResultSuccess();
            }
        }
        return erpt::ResultInvalidArgument();
    }

    Result JournalForReports::DeleteReportWithAttachments() {
        for (auto it = s_record_list.rbegin(); it != s_record_list.rend(); it++) {
            auto *record = std::addressof(*it);
            if (record->info.flags.Test<ReportFlag::HasAttachment>()) {
                EraseReportImpl(record, true, true);
                return ResultSuccess();
            }
        }
        return erpt::ResultNotFound();
    }

    s64 JournalForReports::GetMaxReportSize() {
        s64 max_size = 0;
        for (auto it = s_record_list.begin(); it != s_record_list.end(); it++) {
            max_size = std::max(max_size, it->info.report_size);
        }
        return max_size;
    }

    Result JournalForReports::GetReportList(ReportList *out, ReportType type_filter) {
        u32 count = 0;
        for (auto it = s_record_list.cbegin(); it != s_record_list.cend() && count < util::size(out->reports); it++) {
            if (type_filter == ReportType_Any || type_filter == it->info.type) {
                out->reports[count++] = it->info;
            }
        }
        out->report_count = count;
        return ResultSuccess();
    }

    u32 JournalForReports::GetStoredReportCount(ReportType type) {
        if (ReportType_Start <= type && type < ReportType_End) {
            return s_record_count_by_type[type];
        } else {
            return 0;
        }
    }

    u32 JournalForReports::GetUsedStorage() {
        return s_used_storage;
    }

    Result JournalForReports::RestoreJournal(Stream *stream) {
        /* Clear the used storage. */
        s_used_storage = 0;

        /* Read the count from storage. */
        u32 read_size;
        u32 count;
        R_TRY(stream->ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(count)), sizeof(count)));

        R_UNLESS(read_size == sizeof(count), erpt::ResultCorruptJournal());
        R_UNLESS(count <= ReportCountMax,    erpt::ResultCorruptJournal());

        /* If we fail in the middle of reading reports, we want to do cleanup. */
        auto cleanup_guard = SCOPE_GUARD { CleanupReports(); };

        ReportInfo info;
        for (u32 i = 0; i < count; i++) {
            R_TRY(stream->ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(info)), sizeof(info)));

            R_UNLESS(read_size == sizeof(info),     erpt::ResultCorruptJournal());
            R_UNLESS(ReportType_Start <= info.type, erpt::ResultCorruptJournal());
            R_UNLESS(info.type < ReportType_End,    erpt::ResultCorruptJournal());

            auto *record = new JournalRecord<ReportInfo>(info);
            R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

            /* NOTE: Nintendo does not ensure that the newly allocated record does not leak in the failure case. */
            /* We will ensure it is freed if we early error. */
            auto record_guard = SCOPE_GUARD { delete record; };

            if (record->info.report_size == 0) {
                R_UNLESS(R_SUCCEEDED(Stream::GetStreamSize(std::addressof(record->info.report_size), Report::FileName(record->info.id, false).name)), erpt::ResultCorruptJournal());
            }

            record_guard.Cancel();

            /* NOTE: Nintendo does not check the result of storing the new record... */
            StoreRecord(record);
        }

        cleanup_guard.Cancel();
        return ResultSuccess();
    }

    JournalRecord<ReportInfo> *JournalForReports::RetrieveRecord(ReportId report_id) {
        for (auto it = s_record_list.begin(); it != s_record_list.end(); it++) {
            return std::addressof(*it);
        }
        return nullptr;
    }

    Result JournalForReports::StoreRecord(JournalRecord<ReportInfo> *record) {
        /* Check if the record already exists. */
        for (auto it = s_record_list.begin(); it != s_record_list.end(); it++) {
            R_UNLESS(it->info.id != record->info.id, erpt::ResultAlreadyExists());
        }

        /* Delete an older report if we need to. */
        if (s_record_count >= ReportCountMax) {
            /* Nintendo deletes the oldest report from the type with the most reports. */
            /* This is an approximation of FIFO. */
            ReportType most_used_type  = record->info.type;
            u32 most_used_count        = s_record_count_by_type[most_used_type];

            for (int i = ReportType_Start; i < ReportType_End; i++) {
                if (s_record_count_by_type[i] > most_used_count) {
                    most_used_type  = static_cast<ReportType>(i);
                    most_used_count = s_record_count_by_type[i];
                }
            }

            for (auto it = s_record_list.rbegin(); it != s_record_list.rend(); it++) {
                if (it->info.type != most_used_type) {
                    continue;
                }

                EraseReportImpl(std::addressof(*it), true, false);
                break;
            }
        }
        AMS_ASSERT(s_record_count < ReportCountMax);

        /* Add a reference to the new record. */
        record->AddReference();

        /* Push the record into the list. */
        s_record_list.push_front(*record);
        s_record_count++;
        s_record_count_by_type[record->info.type]++;
        s_used_storage += static_cast<u32>(record->info.report_size);

        return ResultSuccess();
    }

}
