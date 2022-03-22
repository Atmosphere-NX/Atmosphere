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

    ReportImpl::ReportImpl() : m_report(nullptr) {
        /* ... */
    }

    ReportImpl::~ReportImpl() {
        R_ABORT_UNLESS(this->Close());
    }

    Result ReportImpl::Open(const ReportId &report_id) {
        R_UNLESS(m_report == nullptr, erpt::ResultAlreadyInitialized());

        JournalRecord<ReportInfo> *record = Journal::Retrieve(report_id);
        R_UNLESS(record != nullptr, erpt::ResultNotFound());

        m_report = new Report(record, false);
        R_UNLESS(m_report != nullptr, erpt::ResultOutOfMemory());
        auto report_guard = SCOPE_GUARD { delete m_report; m_report = nullptr; };

        R_TRY(m_report->Open(ReportOpenType_Read));
        report_guard.Cancel();

        return ResultSuccess();
    }

    Result ReportImpl::Read(ams::sf::Out<u32> out_count, const ams::sf::OutBuffer &out_buffer)  {
        R_UNLESS(m_report != nullptr, erpt::ResultNotInitialized());

        return m_report->Read(out_count.GetPointer(), static_cast<u8 *>(out_buffer.GetPointer()), static_cast<u32>(out_buffer.GetSize()));
    }

    Result ReportImpl::SetFlags(ReportFlagSet flags)  {
        R_UNLESS(m_report != nullptr, erpt::ResultNotInitialized());

        return m_report->SetFlags(flags);
    }

    Result ReportImpl::GetFlags(ams::sf::Out<ReportFlagSet> out)  {
        R_UNLESS(m_report != nullptr, erpt::ResultNotInitialized());

        return m_report->GetFlags(out.GetPointer());
    }

    Result ReportImpl::Close()  {
        if (m_report != nullptr) {
            m_report->Close();
            delete m_report;
            m_report = nullptr;
        }
        return ResultSuccess();
    }

    Result ReportImpl::GetSize(ams::sf::Out<s64> out)  {
        R_UNLESS(m_report != nullptr, erpt::ResultNotInitialized());

        return m_report->GetSize(out.GetPointer());
    }

}
