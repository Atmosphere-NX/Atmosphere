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

    std::string_view GenerateTargetXml(char *buf)
    {
        int pos;
        const char *hdr = "<?xml version=\"1.0\"?><!DOCTYPE feature SYSTEM \"gdb-target.dtd\"><target>";
        const char *cpuDescBegin = "<feature name=\"org.gnu.gdb.aarch64.core\">";
        const char *cpuDescEnd =
        "<reg name=\"sp\" bitsize=\"64\" type=\"data_ptr\"/>"
        "<reg name=\"pc\" bitsize=\"64\" type=\"code_ptr\"/>"
        "<reg name=\"cpsr\" bitsize=\"32\"/></feature>";

        const char *fpuDescBegin =
        "<feature name=\"org.gnu.gdb.aarch64.fpu\"><vector id=\"v2d\" type=\"ieee_double\" count=\"2\"/>"
        "<vector id=\"v2u\" type=\"uint64\" count=\"2\"/><vector id=\"v2i\" type=\"int64\" count=\"2\"/>"
        "<vector id=\"v4f\" type=\"ieee_single\" count=\"4\"/><vector id=\"v4u\" type=\"uint32\" count=\"4\"/>"
        "<vector id=\"v4i\" type=\"int32\" count=\"4\"/><vector id=\"v8u\" type=\"uint16\" count=\"8\"/>"
        "<vector id=\"v8i\" type=\"int16\" count=\"8\"/><vector id=\"v16u\" type=\"uint8\" count=\"16\"/>"
        "<vector id=\"v16i\" type=\"int8\" count=\"16\"/><vector id=\"v1u\" type=\"uint128\" count=\"1\"/>"
        "<vector id=\"v1i\" type=\"int128\" count=\"1\"/><union id=\"vnd\"><field name=\"f\" type=\"v2d\"/>"
        "<field name=\"u\" type=\"v2u\"/><field name=\"s\" type=\"v2i\"/></union><union id=\"vns\">"
        "<field name=\"f\" type=\"v4f\"/><field name=\"u\" type=\"v4u\"/><field name=\"s\" type=\"v4i\"/></union>"
        "<union id=\"vnh\"><field name=\"u\" type=\"v8u\"/><field name=\"s\" type=\"v8i\"/></union><union id=\"vnb\">"
        "<field name=\"u\" type=\"v16u\"/><field name=\"s\" type=\"v16i\"/></union><union id=\"vnq\">"
        "<field name=\"u\" type=\"v1u\"/><field name=\"s\" type=\"v1i\"/></union><union id=\"aarch64v\">"
        "<field name=\"d\" type=\"vnd\"/><field name=\"s\" type=\"vns\"/><field name=\"h\" type=\"vnh\"/>"
        "<field name=\"b\" type=\"vnb\"/><field name=\"q\" type=\"vnq\"/></union>";

        const char *fpuDescEnd = "<reg name=\"fpsr\" bitsize=\"32\"/>\r\n<reg name=\"fpcr\" bitsize=\"32\"/>\r\n</feature>";
        const char *footer = "</target>";

        std::strcpy(buf, hdr);

        // CPU registers
        std::strcat(buf, cpuDescBegin);
        pos = static_cast<int>(std::strlen(buf));
        for (u32 i = 0; i < 31; i++) {
            pos += std::sprintf(buf + pos, "<reg name=\"x%u\" bitsize=\"64\"/>", i);
        }
        std::strcat(buf, cpuDescEnd);

        std::strcat(buf, fpuDescBegin);
        pos = static_cast<int>(std::strlen(buf));
        for (u32 i = 0; i < 32; i++) {
            pos += std::sprintf(buf + pos, "<reg name=\"v%u\" bitsize=\"128\" type=\"aarch64v\"/>", i);
        }
        std::strcat(buf, fpuDescEnd);

        std::strcat(buf, footer);
        return std::string_view{buf};
    }

}

namespace ams::hvisor::gdb {

    GDB_DEFINE_XFER_HANDLER(Features)
    {
        if (write || annex != "target.xml") {
            return ReplyEmpty();
        }

        // Generate the target xml on-demand
        // This is a bit whack, we rightfully assume that GDB won't sent any other command during the stream transfer
        if (m_targetXml.empty()) {
            m_targetXml = GenerateTargetXml(m_workBuffer);
        }

        int n = SendStreamData(m_targetXml, offset, length, false);

        // Transfer ended
        if(offset + length >= m_targetXml.size()) {
            m_targetXml = {};
        }

        return n;
    }

    GDB_DEFINE_QUERY_HANDLER(Xfer)
    {
        // e.g. qXfer:features:read:annex:offset,length

        // Split
        auto [cmd, directionStr, annex, offsetlen] = SplitString<4>(m_commandData, ':');
        if (offsetlen.empty()) {
            return ReplyErrno(EILSEQ);
        }

        // Check direction
        bool isWrite;
        if (directionStr == "read") {
            isWrite = false;
        } else if (directionStr == "write") {
            isWrite = true;
        } else {
            return ReplyErrno(EILSEQ);
        }

        // Get offset and length
        auto [nread, off, len] = ParseHexIntegerList<2>(offsetlen, isWrite ? ':' : '\0');
        if (nread == 0) {
            return ReplyErrno(EILSEQ);
        }

        // Get data/nothing
        m_commandData = offsetlen;
        m_commandData.remove_prefix(nread);

        // Run command
        if (cmd == "features") {
            return HandleXferFeatures(isWrite, annex, off, len);
        } else {
            return HandleUnsupported();
        }
    }

}