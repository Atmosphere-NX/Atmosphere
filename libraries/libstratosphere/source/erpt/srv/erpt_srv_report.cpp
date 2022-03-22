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
#include "erpt_srv_report_impl.hpp"
#include "erpt_srv_report.hpp"

namespace ams::erpt::srv {

    ReportFileName Report::FileName(ReportId report_id, bool redirect_to_sd) {
        ReportFileName report_name;
        util::SNPrintf(report_name.name, sizeof(report_name.name),
                      "%s:/%08x-%04x-%04x-%02x%02x-%04x%08x",
                      (redirect_to_sd ? ReportOnSdStoragePath : ReportStoragePath),
                      report_id.uuid_data.time_low,
                      report_id.uuid_data.time_mid,
                      report_id.uuid_data.time_high_and_version,
                      report_id.uuid_data.clock_high,
                      report_id.uuid_data.clock_low,
                      static_cast<u32>((report_id.uuid_data.node >> BITSIZEOF(u32)) & 0x0000FFFF),
                      static_cast<u32>((report_id.uuid_data.node >> 0)              & 0xFFFFFFFF));
        return report_name;
    }

    Report::Report(JournalRecord<ReportInfo> *r, bool redirect_to_sd) : m_record(r), m_redirect_to_sd_card(redirect_to_sd) {
        m_record->AddReference();
    }

    Report::~Report() {
        this->CloseStream();
        if (m_record->RemoveReference()) {
            this->DeleteStream(this->FileName().name);
            delete m_record;
        }
    }

    ReportFileName Report::FileName() const {
        return FileName(m_record->m_info.id, m_redirect_to_sd_card);
    }

    Result Report::Open(ReportOpenType type) {
        switch (type) {
            case ReportOpenType_Create: return this->OpenStream(this->FileName().name, StreamMode_Write, ReportStreamBufferSize);
            case ReportOpenType_Read:   return this->OpenStream(this->FileName().name, StreamMode_Read,  ReportStreamBufferSize);
            default:                    return erpt::ResultInvalidArgument();
        }
    }

    Result Report::Read(u32 *out_read_count, u8 *dst, u32 dst_size) {
        return this->ReadStream(out_read_count, dst, dst_size);
    }

    Result Report::Delete() {
        return this->DeleteStream(this->FileName().name);
    }

    void Report::Close() {
        return this->CloseStream();
    }

    Result Report::GetFlags(ReportFlagSet *out) const {
        *out = m_record->m_info.flags;
        return ResultSuccess();
    }

    Result Report::SetFlags(ReportFlagSet flags) {
        if (((~m_record->m_info.flags) & flags).IsAnySet()) {
            m_record->m_info.flags |= flags;
            return Journal::Commit();
        }
        return ResultSuccess();
    }

    Result Report::GetSize(s64 *out) const {
        return this->GetStreamSize(out);
    }

}
