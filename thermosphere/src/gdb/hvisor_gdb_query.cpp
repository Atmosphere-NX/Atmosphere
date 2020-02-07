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

namespace ams::hvisor::gdb {

    GDB_DEFINE_QUERY_HANDLER(Supported)
    {
        // Ignore what gdb sent...
        return SendFormattedPacket(
            "PacketSize=%x;"
            "qXfer:features:read+;"
            "QStartNoAckMode+;QThreadEvents+"
            "vContSupported+;swbreak+;hwbreak+",

            GDB_BUF_LEN
        );
    }

    GDB_DEFINE_QUERY_HANDLER(StartNoAckMode)
    {
        GDB_CHECK_NO_CMD_DATA();

        m_noAckSent = true;
        return ReplyOk();
    }

    GDB_DEFINE_QUERY_HANDLER(Attached)
    {
        GDB_CHECK_NO_CMD_DATA();

        return SendPacket("1");
    }

#define QUERY_CMD_CASE2(name, fun)  if (cmdName==name) { return GDB_QUERY_HANDLER(fun)(); } else
#define QUERY_CMD_CASE(fun)         QUERY_CMD_CASE2(STRINGIZE(fun), fun)

    GDB_DEFINE_HANDLER(Query)
    {
        // Extract name
        char delim = ':';

        size_t delimPos = m_commandData.find_first_of(":,");
        std::string_view cmdName = m_commandData;
        if (delimPos != std::string_view::npos) {
            delim = m_commandData[delimPos];
            cmdName.remove_suffix(cmdName.size() - delimPos);
            m_commandData.remove_prefix(delimPos + 1);
        }

        // Only 2 commands are delimited by a comma, all with lowercase 'q' prefix
        // We don't handle qP nor qL
        if (delim != ':') {
            if (m_commandLetter != 'q') {
                return ReplyErrno(EILSEQ);
            } else if (cmdName != "Rcmd" && cmdName != "ThreadExtraInfo") {
                return ReplyErrno(EILSEQ);
            }
        }

        if (m_commandLetter == 'q') {
            QUERY_CMD_CASE(Supported)
            QUERY_CMD_CASE(Xfer)
            QUERY_CMD_CASE(Attached)
            QUERY_CMD_CASE(fThreadInfo)
            QUERY_CMD_CASE(sThreadInfo)
            QUERY_CMD_CASE(ThreadExtraInfo)
            QUERY_CMD_CASE2("C", CurrentThreadId)
            QUERY_CMD_CASE(Rcmd)
            /*default :*/{
                return HandleUnsupported();
            }
        } else {
            QUERY_CMD_CASE(StartNoAckMode)
            QUERY_CMD_CASE(ThreadEvents)
            /*default :*/{
                return HandleUnsupported();
            }
        }
    }

    #undef QUERY_CMD_CASE
    #undef QUERY_CMD_CASE2

}