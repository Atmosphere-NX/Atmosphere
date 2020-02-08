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

    GDB_DEFINE_HANDLER(VerboseCommand)
    {
        // Extract name
        char delim = ':';

        size_t delimPos = m_commandData.find_first_of(";:");
        std::string_view cmdName = m_commandData;
        if (delimPos != std::string_view::npos) {
            delim = m_commandData[delimPos];
            cmdName.remove_suffix(cmdName.size() - delimPos);
            m_commandData.remove_prefix(delimPos + 1);
        }

        if (cmdName == "Cont?") {
            GDB_VERBOSE_HANDLER(ContinueSupported)();
        } else if (cmdName == "Cont") {
            GDB_VERBOSE_HANDLER(Continue)();
        } else if (cmdName == "CtrlC") {
            GDB_VERBOSE_HANDLER(CtrlC)();
        } else if (cmdName == "MustReplyEmpty") {
            return HandleUnsupported();
        } else if (cmdName == "Stopped") {
            return GDB_VERBOSE_HANDLER(Stopped)();
        } else {
            return HandleUnsupported(); // No handler found!
        }
    }

    GDB_DEFINE_VERBOSE_HANDLER(ContinueSupported)
    {
        return SendPacket("vCont;c;C;s;S;t;r");
    }

}
