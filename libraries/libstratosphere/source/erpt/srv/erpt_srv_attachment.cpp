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
#include "erpt_srv_attachment_impl.hpp"
#include "erpt_srv_attachment.hpp"

namespace ams::erpt::srv {

    AttachmentFileName Attachment::FileName(AttachmentId attachment_id) {
        char uuid_str[AttachmentFileNameLength];
        attachment_id.uuid.ToString(uuid_str, sizeof(uuid_str));

        AttachmentFileName attachment_name;
        util::SNPrintf(attachment_name.name, sizeof(attachment_name.name), "%s:/%s.att", ReportStoragePath, uuid_str);
        return attachment_name;
    }

    Attachment::Attachment(JournalRecord<AttachmentInfo> *r) : m_record(r) {
        m_record->AddReference();
    }

    Attachment::~Attachment() {
        this->CloseStream();
        if (m_record->RemoveReference()) {
            if (R_FAILED(this->DeleteStream(this->FileName().name))) {
                /* TODO: Log failure? */
            }
            delete m_record;
        }
    }

    AttachmentFileName Attachment::FileName() const {
        return FileName(m_record->m_info.attachment_id);
    }

    Result Attachment::Open(AttachmentOpenType type) {
        switch (type) {
            case AttachmentOpenType_Create: R_RETURN(this->OpenStream(this->FileName().name, StreamMode_Write, AttachmentStreamBufferSize));
            case AttachmentOpenType_Read:   R_RETURN(this->OpenStream(this->FileName().name, StreamMode_Read,  AttachmentStreamBufferSize));
            default:                        R_THROW(erpt::ResultInvalidArgument());
        }
    }

    Result Attachment::Read(u32 *out_read_count, u8 *dst, u32 dst_size) {
        R_RETURN(this->ReadStream(out_read_count, dst, dst_size));
    }

    Result Attachment::Delete() {
        R_RETURN(this->DeleteStream(this->FileName().name));
    }

    void Attachment::Close() {
        return this->CloseStream();
    }

    Result Attachment::GetFlags(AttachmentFlagSet *out) const {
        *out = m_record->m_info.flags;
        R_SUCCEED();
    }

    Result Attachment::SetFlags(AttachmentFlagSet flags) {
        if (((~m_record->m_info.flags) & flags).IsAnySet()) {
            m_record->m_info.flags |= flags;
            R_RETURN(Journal::Commit());
        }
        R_SUCCEED();
    }

    Result Attachment::GetSize(s64 *out) const {
        R_RETURN(this->GetStreamSize(out));
    }

}
