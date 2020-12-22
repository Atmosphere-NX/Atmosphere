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
#include "erpt_srv_attachment_impl.hpp"
#include "erpt_srv_attachment.hpp"

namespace ams::erpt::srv {

    AttachmentFileName Attachment::FileName(AttachmentId attachment_id) {
        char uuid_str[AttachmentFileNameLength];
        attachment_id.uuid.ToString(uuid_str, sizeof(uuid_str));

        AttachmentFileName attachment_name;
        std::snprintf(attachment_name.name, sizeof(attachment_name.name), "%s:/%s.att", ReportStoragePath, uuid_str);
        return attachment_name;
    }

    Attachment::Attachment(JournalRecord<AttachmentInfo> *r) : record(r) {
        this->record->AddReference();
    }

    Attachment::~Attachment() {
        this->CloseStream();
        if (this->record->RemoveReference()) {
            this->DeleteStream(this->FileName().name);
            delete this->record;
        }
    }

    AttachmentFileName Attachment::FileName() const {
        return FileName(this->record->info.attachment_id);
    }

    Result Attachment::Open(AttachmentOpenType type) {
        switch (type) {
            case AttachmentOpenType_Create: return this->OpenStream(this->FileName().name, StreamMode_Write, AttachmentStreamBufferSize);
            case AttachmentOpenType_Read:   return this->OpenStream(this->FileName().name, StreamMode_Read,  AttachmentStreamBufferSize);
            default:                        return erpt::ResultInvalidArgument();
        }
    }

    Result Attachment::Read(u32 *out_read_count, u8 *dst, u32 dst_size) {
        return this->ReadStream(out_read_count, dst, dst_size);
    }

    Result Attachment::Delete() {
        return this->DeleteStream(this->FileName().name);
    }

    void Attachment::Close() {
        return this->CloseStream();
    }

    Result Attachment::GetFlags(AttachmentFlagSet *out) const {
        *out = this->record->info.flags;
        return ResultSuccess();
    }

    Result Attachment::SetFlags(AttachmentFlagSet flags) {
        if (((~this->record->info.flags) & flags).IsAnySet()) {
            this->record->info.flags |= flags;
            return Journal::Commit();
        }
        return ResultSuccess();
    }

    Result Attachment::GetSize(s64 *out) const {
        return this->GetStreamSize(out);
    }

}
