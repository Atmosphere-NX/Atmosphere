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

/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "hvisor_gdb_defines_internal.hpp"
#include "hvisor_gdb_packet_data.hpp"

namespace {

    constexpr std::string_view SkipSpaces(std::string_view str)
    {
        size_t n = str.find_first_not_of("\t\v\n\f\r ");
        if (n == std::string_view::npos) {
            return {};
        } else {
            str.remove_prefix(n);
            return str;
        }
    }

}

namespace ams::hvisor::gdb {

    GDB_DEFINE_QUERY_HANDLER(Rcmd)
    {
        char *buf = GetInPlaceOutputBuffer();
        size_t encodedLen = m_commandData.size();
        if (encodedLen == 0 || encodedLen % 2 != 0) {
            ReplyErrno(EILSEQ);
        }

        // Decode in place
        if (DecodeHex(buf, m_commandData) != encodedLen / 2) {
            ReplyErrno(EILSEQ);
        }

        // Extract command name, data
        m_commandData = std::string_view{buf, encodedLen / 2};
        size_t nameSize = m_commandData.find_first_of("\t\v\n\f\r ");
        std::string_view commandName = m_commandData;
        if (nameSize != std::string_view::npos) {
            commandName.remove_suffix(commandName.size() - nameSize);
            m_commandData.remove_prefix(nameSize);
            m_commandData = SkipSpaces(m_commandData);
        } else {
            m_commandData = std::string_view{};
        }

        // Nothing implemented yet
        (void)commandName;

        return SendHexPacket("Unrecognized command.\n");
    }

}
