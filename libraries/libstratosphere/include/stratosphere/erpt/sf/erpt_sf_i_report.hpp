/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <vapours.hpp>
#include <stratosphere/erpt/erpt_types.hpp>

namespace ams::erpt::sf {

    class IReport : public ams::sf::IServiceObject {
        protected:
            enum class CommandId {
                Open     = 0,
                Read     = 1,
                SetFlags = 2,
                GetFlags = 3,
                Close    = 4,
                GetSize  = 5,
            };
        public:
            /* Actual commands. */
            virtual Result Open(const ReportId &report_id) = 0;
            virtual Result Read(ams::sf::Out<u32> out_count, const ams::sf::OutBuffer &out_buffer) = 0;
            virtual Result SetFlags(ReportFlagSet flags) = 0;
            virtual Result GetFlags(ams::sf::Out<ReportFlagSet> out) = 0;
            virtual Result Close() = 0;
            virtual Result GetSize(ams::sf::Out<s64> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Open),
                MAKE_SERVICE_COMMAND_META(Read),
                MAKE_SERVICE_COMMAND_META(SetFlags),
                MAKE_SERVICE_COMMAND_META(GetFlags),
                MAKE_SERVICE_COMMAND_META(Close),
                MAKE_SERVICE_COMMAND_META(GetSize),
            };
    };

}