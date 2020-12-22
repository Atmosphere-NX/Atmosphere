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
#pragma once
#include <stratosphere.hpp>
#include "erpt_srv_allocator.hpp"
#include "erpt_srv_stream.hpp"
#include "erpt_srv_journal.hpp"

namespace ams::erpt::srv {

    enum ReportOpenType {
        ReportOpenType_Create = 0,
        ReportOpenType_Read   = 1,
    };

    constexpr inline u32 ReportStreamBufferSize = 1_KB;

    class Report : public Allocator, public Stream {
        private:
            JournalRecord<ReportInfo> *record;
            bool redirect_to_sd_card;
        private:
            ReportFileName FileName() const;
        public:
            static ReportFileName FileName(ReportId report_id, bool redirect_to_sd);
        public:
            explicit Report(JournalRecord<ReportInfo> *r, bool redirect_to_sd);
            ~Report();

            Result Open(ReportOpenType type);
            Result Read(u32 *out_read_count, u8 *dst, u32 dst_size);
            Result Delete();
            void Close();

            Result GetFlags(ReportFlagSet *out) const;
            Result SetFlags(ReportFlagSet flags);
            Result GetSize(s64 *out) const;

            template<typename T>
            Result Write(T val) {
                return this->WriteStream(reinterpret_cast<const u8 *>(std::addressof(val)), sizeof(val));
            }

            template<typename T>
            Result Write(const T *buf, u32 buffer_size) {
                return this->WriteStream(reinterpret_cast<const u8 *>(buf), buffer_size);
            }
    };

}
