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

#include "../guest_memory.h"

namespace ams::hvisor::gdb {

    int Context::SendMemory(uintptr_t addr, size_t len, std::string_view prefix)
    {
        char *buf = GetInPlaceOutputBuffer();
        char *membuf = GetWorkBuffer();

        size_t prefixLen = prefix.size();

        if(prefixLen + 2 * len > GDB_BUF_LEN) {
            // gdb shouldn't send requests which responses don't fit in a packet
            return prefixLen == 0 ? ReplyErrno(ENOMEM) : -1;
        }

        size_t total = guestReadMemory(addr, len, membuf);

        if (total == 0) {
            return prefixLen == 0 ? ReplyErrno(EFAULT) : -EFAULT;
        } else {
            std::copy(prefix.begin(), prefix.end(), buf);
            EncodeHex(buf + prefixLen, membuf, total);
            return SendPacket(std::string_view{buf, prefixLen + 2 * total});
        }
    }

    int Context::WriteMemoryImpl(size_t (*decoder)(void *, const void *, size_t))
    {
        char *workbuf = GetWorkBuffer();

        auto [nread, addr, len] = ParseHexIntegerList<2>(m_commandData, ':');
        if (nread == 0) {
            return ReplyErrno(EILSEQ);
        }

        m_commandData.remove_prefix(nread);
        if (len > m_commandData.length() / 2) {
            // Data len field doesn't match what we got...
            return ReplyErrno(ENOMEM);
        }

        size_t n = decoder(workbuf, m_commandData.data(), m_commandData.size());

        if(n != len) {
            // Decoding error...
            return ReplyErrno(EILSEQ);
        }

        size_t total = guestWriteMemory(addr, len, workbuf);
        return total == len ? ReplyOk() : ReplyErrno(EFAULT);
    }


    GDB_DEFINE_HANDLER(ReadMemory)
    {
        auto [nparsed, addr, len] = ParseHexIntegerList<2>(m_commandData);
        if (nparsed == 0) {
            return ReplyErrno(EILSEQ);
        }

        return SendMemory(addr, len);
    }

    GDB_DEFINE_HANDLER(WriteMemory)
    {
        return WriteMemoryImpl(DecodeHex);
    }

    GDB_DEFINE_HANDLER(WriteMemoryRaw)
    {
        return WriteMemoryImpl(UnescapeBinaryData);
    }

}
