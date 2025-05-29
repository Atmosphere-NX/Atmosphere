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
#include "erpt_srv_attachment.hpp"

namespace ams::erpt::srv {

    constinit util::IntrusiveListBaseTraits<JournalRecord<AttachmentInfo>>::ListType JournalForAttachments::s_attachment_list;
    constinit u32 JournalForAttachments::s_attachment_count = 0;
    constinit u32 JournalForAttachments::s_used_storage = 0;

    namespace {

        constexpr inline u32 AttachmentUsedStorageMax = 4_MB;

    }

    void JournalForAttachments::CleanupAttachments() {
        for (auto it = s_attachment_list.begin(); it != s_attachment_list.end(); /* ... */) {
            auto *record = std::addressof(*it);
            it = s_attachment_list.erase(s_attachment_list.iterator_to(*record));
            if (record->RemoveReference()) {
                Stream::DeleteStream(Attachment::FileName(record->m_info.attachment_id).name);
                delete record;
            }
        }
        AMS_ASSERT(s_attachment_list.empty());

        s_attachment_count = 0;
        s_used_storage     = 0;

    }

    Result JournalForAttachments::CommitJournal(Stream *stream) {
        R_TRY(stream->WriteStream(reinterpret_cast<const u8 *>(std::addressof(s_attachment_count)), sizeof(s_attachment_count)));
        for (auto it = s_attachment_list.crbegin(); it != s_attachment_list.crend(); it++) {
            R_TRY(stream->WriteStream(reinterpret_cast<const u8 *>(std::addressof(it->m_info)), sizeof(it->m_info)));
        }
        R_SUCCEED();
    }

    Result JournalForAttachments::DeleteAttachments(ReportId report_id) {
        for (auto it = s_attachment_list.begin(); it != s_attachment_list.end(); /* ... */) {
            auto *record = std::addressof(*it);
            if (record->m_info.owner_report_id == report_id) {
                /* Erase from the list. */
                it = s_attachment_list.erase(s_attachment_list.iterator_to(*record));

                /* Update storage tracking counts. */
                --s_attachment_count;
                s_used_storage -= static_cast<u32>(record->m_info.attachment_size);

                /* Delete the object, if we should. */
                if (record->RemoveReference()) {
                    Stream::DeleteStream(Attachment::FileName(record->m_info.attachment_id).name);
                    delete record;
                }
            } else {
                /* Not attached, just advance. */
                it++;
            }
        }
        R_SUCCEED();
    }

    Result JournalForAttachments::GetAttachmentList(u32 *out_count, AttachmentInfo *out_infos, size_t max_out_infos, ReportId report_id) {
        if (hos::GetVersion() >= hos::Version_20_0_0) {
            /* TODO: What define gives a minimum of 10? */
            R_UNLESS(max_out_infos >= 10, erpt::ResultInvalidArgument());
        }

        u32 count = 0;
        for (auto it = s_attachment_list.cbegin(); it != s_attachment_list.cend() && count < max_out_infos; it++) {
            if (report_id == it->m_info.owner_report_id) {
                out_infos[count++] = it->m_info;
            }
        }
        *out_count = count;
        R_SUCCEED();
    }

    u32 JournalForAttachments::GetUsedStorage() {
        return s_used_storage;
    }

    Result JournalForAttachments::RestoreJournal(Stream *stream) {
        /* Clear the used storage. */
        s_used_storage = 0;

        /* Read the count from storage. */
        u32 read_size;
        u32 count;
        R_TRY(stream->ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(count)), sizeof(count)));

        R_UNLESS(read_size == sizeof(count),  erpt::ResultCorruptJournal());
        R_UNLESS(count <= AttachmentCountMax, erpt::ResultCorruptJournal());

        /* If we fail in the middle of reading reports, we want to do cleanup. */
        auto cleanup_guard = SCOPE_GUARD { CleanupAttachments(); };

        AttachmentInfo info;
        for (u32 i = 0; i < count; i++) {
            R_TRY(stream->ReadStream(std::addressof(read_size), reinterpret_cast<u8 *>(std::addressof(info)), sizeof(info)));

            R_UNLESS(read_size == sizeof(info),     erpt::ResultCorruptJournal());

            auto *record = new JournalRecord<AttachmentInfo>(info);
            R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

            auto record_guard = SCOPE_GUARD { delete record; };

            if (R_FAILED(Stream::GetStreamSize(std::addressof(record->m_info.attachment_size), Attachment::FileName(record->m_info.attachment_id).name))) {
                continue;
            }

            if (record->m_info.flags.Test<AttachmentFlag::HasOwner>() && JournalForReports::RetrieveRecord(record->m_info.owner_report_id) != nullptr) {
                /* NOTE: Nintendo does not check the result of storing the new record... */
                record_guard.Cancel();
                StoreRecord(record);
            } else {
                /* If the attachment has no owner (or we deleted the report), delete the file associated with it. */
                Stream::DeleteStream(Attachment::FileName(record->m_info.attachment_id).name);
            }
        }

        cleanup_guard.Cancel();
        R_SUCCEED();
    }

    JournalRecord<AttachmentInfo> *JournalForAttachments::RetrieveRecord(AttachmentId attachment_id) {
        for (auto it = s_attachment_list.begin(); it != s_attachment_list.end(); it++) {
            if (auto *record = std::addressof(*it); record->m_info.attachment_id == attachment_id) {
                return record;
            }
        }
        return nullptr;
    }

    Result JournalForAttachments::SetOwner(AttachmentId attachment_id, ReportId report_id) {
        for (auto it = s_attachment_list.begin(); it != s_attachment_list.end(); it++) {
            auto *record = std::addressof(*it);
            if (record->m_info.attachment_id == attachment_id) {
                R_UNLESS(!record->m_info.flags.Test<AttachmentFlag::HasOwner>(), erpt::ResultAlreadyOwned());

                record->m_info.owner_report_id = report_id;
                record->m_info.flags.Set<AttachmentFlag::HasOwner>();
                R_SUCCEED();
            }
        }
        R_THROW(erpt::ResultInvalidArgument());
    }

    Result JournalForAttachments::StoreRecord(JournalRecord<AttachmentInfo> *record) {
        /* Check if the record already exists. */
        for (auto it = s_attachment_list.begin(); it != s_attachment_list.end(); it++) {
            R_UNLESS(it->m_info.attachment_id != record->m_info.attachment_id, erpt::ResultAlreadyExists());
        }

        /* Add a reference to the new record. */
        record->AddReference();

        /* Push the record into the list. */
        s_attachment_list.push_front(*record);
        s_attachment_count++;
        s_used_storage += static_cast<u32>(record->m_info.attachment_size);

        R_SUCCEED();
    }

    Result JournalForAttachments::SubmitAttachment(AttachmentId *out, char *name, const u8 *data, u32 data_size) {
        R_UNLESS(data_size > 0,                 erpt::ResultInvalidArgument());
        R_UNLESS(data_size < AttachmentSizeMax, erpt::ResultInvalidArgument());

        const auto name_len = std::strlen(name);
        R_UNLESS(name_len < AttachmentNameSizeMax, erpt::ResultInvalidArgument());

        /* Ensure that we have free space. */
        while (s_used_storage > AttachmentUsedStorageMax) {
            R_TRY(JournalForReports::DeleteReportWithAttachments());
        }

        AttachmentInfo info;
        info.attachment_id.uuid = util::GenerateUuid();
        info.flags              = erpt::srv::MakeNoAttachmentFlags();
        info.attachment_size    = data_size;
        util::Strlcpy(info.attachment_name, name, sizeof(info.attachment_name));

        auto *record = new JournalRecord<AttachmentInfo>(info);
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        record->AddReference();
        ON_SCOPE_EXIT {
            if (record->RemoveReference()) {
                delete record;
            }
        };

        {
            auto attachment = std::make_unique<Attachment>(record);
            R_UNLESS(attachment != nullptr, erpt::ResultOutOfMemory());

            R_TRY(attachment->Open(AttachmentOpenType_Create));
            ON_SCOPE_EXIT { attachment->Close(); };

            R_TRY(attachment->Write(data, data_size));
            R_TRY(StoreRecord(record));
        }

        *out = info.attachment_id;

        R_SUCCEED();
    }

}
