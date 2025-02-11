/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "erpt_srv_report_impl.hpp"
#include "erpt_srv_report.hpp"

namespace ams::erpt::srv {

    ReportFileName Report::FileName(ReportId report_id, bool redirect_to_sd) {
        AMS_UNUSED(report_id);       // Marks the parameter as unused
        AMS_UNUSED(redirect_to_sd);  // Marks the parameter as unused

        ReportFileName report_name;
        util::SNPrintf(report_name.name, sizeof(report_name.name), "erpt_disabled");
        return report_name;
    }

    Report::Report(JournalRecord<ReportInfo> *r, bool redirect_to_sd) 
        : m_record(r), m_redirect_to_sd_card(redirect_to_sd) {
        m_record->AddReference();
    }

    Report::~Report() {
        this->CloseStream();
        if (m_record->RemoveReference()) {
            delete m_record;
        }
    }

    ReportFileName Report::FileName() const {
        return FileName(m_record->m_info.id, m_redirect_to_sd_card);
    }

    Result Report::Open(ReportOpenType type) {
        AMS_UNUSED(type); // Marks the parameter as unused
        R_SUCCEED();
    }

    Result Report::Read(u32 *out_read_count, u8 *dst, u32 dst_size) {
        AMS_UNUSED(out_read_count);
        AMS_UNUSED(dst);
        AMS_UNUSED(dst_size);
        R_SUCCEED();
    }

    Result Report::Delete() {
        R_SUCCEED();
    }

    void Report::Close() {
    }

    Result Report::GetFlags(ReportFlagSet *out) const {
        *out = m_record->m_info.flags;
        R_SUCCEED();
    }

    Result Report::SetFlags(ReportFlagSet flags) {
        AMS_UNUSED(flags);
        R_SUCCEED();
    }

    Result Report::GetSize(s64 *out) const {
        *out = 0; 
        R_SUCCEED();
    }

    void Report::CloseStream() {
        // Implement the logic to close the stream, if necessary
    }

}