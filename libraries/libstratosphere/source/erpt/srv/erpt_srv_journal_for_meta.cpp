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

    constinit JournalMeta JournalForMeta::s_journal_meta;

    void JournalForMeta::InitializeJournal() {
        std::memset(std::addressof(s_journal_meta), 0, sizeof(s_journal_meta));
        s_journal_meta.journal_id = util::GenerateUuid();
        s_journal_meta.version    = JournalVersion;
    }

    Result JournalForMeta::CommitJournal(Stream *stream) {
        return stream->WriteStream(reinterpret_cast<const u8 *>(std::addressof(s_journal_meta)), sizeof(s_journal_meta));
    }

    Result JournalForMeta::RestoreJournal(Stream *stream) {
        u32 size;
        if (R_FAILED(stream->ReadStream(std::addressof(size), reinterpret_cast<u8 *>(std::addressof(s_journal_meta)), sizeof(s_journal_meta))) || size != sizeof(s_journal_meta)) {
            InitializeJournal();
        }
        return ResultSuccess();
    }

    u32 JournalForMeta::GetTransmittedCount(ReportType type) {
        if (ReportType_Start <= type && type < ReportType_End) {
            return s_journal_meta.transmitted_count[type];
        } else {
            return 0;
        }
    }

    u32 JournalForMeta::GetUntransmittedCount(ReportType type) {
        if (ReportType_Start <= type && type < ReportType_End) {
            return s_journal_meta.untransmitted_count[type];
        } else {
            return 0;
        }
    }

    void JournalForMeta::IncrementCount(bool transmitted, ReportType type) {
        if (ReportType_Start <= type && type < ReportType_End) {
            if (transmitted) {
                s_journal_meta.transmitted_count[type]++;
            } else {
                s_journal_meta.untransmitted_count[type]++;
            }
        }
    }

    util::Uuid JournalForMeta::GetJournalId() {
        return s_journal_meta.journal_id;
    }

}
