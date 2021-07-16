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
#include <stratosphere.hpp>
#include "dmnt2_debug_log.hpp"
#include "dmnt2_gdb_packet_parser.hpp"

namespace ams::dmnt {

    namespace {

        constexpr const char TargetXmlAarch64[] =
            "l<?xml version=\"1.0\"?>"
            "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
            "<target>"
            "<architecture>aarch64</architecture>"
            "<osabi>GNU/Linux</osabi>"
            "<xi:include href=\"aarch64-core.xml\"/>"
            "<xi:include href=\"aarch64-fpu.xml\"/>"
            "</target>";

        constexpr const char Aarch64CoreXml[] =
            "l<?xml version=\"1.0\"?>\n"
            "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
            "<feature name=\"org.gnu.gdb.aarch64.core\">\n"
            "\t<reg name=\"x0\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x1\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x2\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x3\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x4\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x5\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x6\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x7\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x8\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x9\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x10\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x11\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x12\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x13\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x14\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x15\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x16\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x17\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x18\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x19\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x20\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x21\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x22\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x23\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x24\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x25\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x26\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x27\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x28\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x29\" bitsize=\"64\"/>\n"
            "\t<reg name=\"x30\" bitsize=\"64\"/>\n"
            "\t<reg name=\"sp\" bitsize=\"64\" type=\"data_ptr\"/>\n"
            "\t<reg name=\"pc\" bitsize=\"64\" type=\"code_ptr\"/>\n"
            "\t<flags id=\"cpsr_flags\" size=\"4\">\n"
            "\t\t<field name=\"SP\"   start=\"0\"  end=\"0\" /><!-- Stack Pointer selection -->\n"
            "\t\t<field name=\"EL\"   start=\"2\"  end=\"3\" /><!-- Exception level -->\n"
            "\t\t<field name=\"nRW\"  start=\"4\"  end=\"4\" /><!-- Execution state (not Register Width) 0=64-bit 1-32-bit -->\n"
            "\n"
            "\t\t<field name=\"F\"    start=\"6\"  end=\"6\" /><!-- FIQ mask -->\n"
            "\t\t<field name=\"I\"    start=\"7\"  end=\"7\" /><!-- IRQ mask -->\n"
            "\t\t<field name=\"A\"    start=\"8\"  end=\"8\" /><!-- SError interrupt mask -->\n"
            "\t\t<field name=\"D\"    start=\"9\"  end=\"9\" /><!-- Debug mask bit -->\n"
            "\n"
            "\t\t<field name=\"SSBS\" start=\"12\" end=\"12\"/><!-- Speculative Store Bypass Safe -->\n"
            "\n"
            "\t\t<field name=\"IL\"   start=\"20\" end=\"20\"/><!-- Illegal execution state -->\n"
            "\t\t<field name=\"SS\"   start=\"21\" end=\"21\"/><!-- Software step -->\n"
            "\t\t<field name=\"PAN\"  start=\"22\" end=\"22\"/><!-- v8.1 Privileged Access Never -->\n"
            "\t\t<field name=\"UAO\"  start=\"23\" end=\"23\"/><!-- v8.2 User Access Override -->\n"
            "\t\t<field name=\"DIT\"  start=\"24\" end=\"24\"/><!-- v8.4 Data Independent Timing -->\n"
            "\t\t<field name=\"TCO\"  start=\"25\" end=\"25\"/><!-- v8.5 Tag Check Override -->\n"
            "\n"
            "\t\t<field name=\"V\"    start=\"28\" end=\"28\"/><!-- oVerflow condition flag -->\n"
            "\t\t<field name=\"C\"    start=\"29\" end=\"29\"/><!-- Carry condition flag -->\n"
            "\t\t<field name=\"Z\"    start=\"30\" end=\"30\"/><!-- Zero condition flag -->\n"
            "\t\t<field name=\"N\"    start=\"31\" end=\"31\"/><!-- Negative condition flag -->\n"
            "\t</flags>\n"
            "\t<reg name=\"cpsr\" bitsize=\"32\" type=\"cpsr_flags\"/>\n"
            "</feature>";

        constexpr const char Aarch64FpuXml[] =
            "l<?xml version=\"1.0\"?>\n"
            "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">\n"
            "<feature name=\"org.gnu.gdb.aarch64.fpu\">\n"
            "\t<vector id=\"v2d\" type=\"ieee_double\" count=\"2\"/>\n"
            "\t<vector id=\"v2u\" type=\"uint64\" count=\"2\"/>\n"
            "\t<vector id=\"v2i\" type=\"int64\" count=\"2\"/>\n"
            "\t<vector id=\"v4f\" type=\"ieee_single\" count=\"4\"/>\n"
            "\t<vector id=\"v4u\" type=\"uint32\" count=\"4\"/>\n"
            "\t<vector id=\"v4i\" type=\"int32\" count=\"4\"/>\n"
            "\t<!-- <vector id=\"v8f\" type=\"ieee_half\" count=\"8\"/> -->\n"
            "\t<vector id=\"v8u\" type=\"uint16\" count=\"8\"/>\n"
            "\t<vector id=\"v8i\" type=\"int16\" count=\"8\"/>\n"
            "\t<vector id=\"v16u\" type=\"uint8\" count=\"16\"/>\n"
            "\t<vector id=\"v16i\" type=\"int8\" count=\"16\"/>\n"
            "\t<vector id=\"v1u\" type=\"uint128\" count=\"1\"/>\n"
            "\t<vector id=\"v1i\" type=\"int128\" count=\"1\"/>\n"
            "\t<union id=\"vnd\">\n"
            "\t\t<field name=\"f\" type=\"v2d\"/>\n"
            "\t\t<field name=\"u\" type=\"v2u\"/>\n"
            "\t\t<field name=\"s\" type=\"v2i\"/>\n"
            "\t</union>\n"
            "\t<union id=\"vns\">\n"
            "\t\t<field name=\"f\" type=\"v4f\"/>\n"
            "\t\t<field name=\"u\" type=\"v4u\"/>\n"
            "\t\t<field name=\"s\" type=\"v4i\"/>\n"
            "\t</union>\n"
            "\t<union id=\"vnh\">\n"
            "\t\t<!-- <field name=\"f\" type=\"v8f\"/> -->\n"
            "\t\t<field name=\"u\" type=\"v8u\"/>\n"
            "\t\t<field name=\"s\" type=\"v8i\"/>\n"
            "\t</union>\n"
            "\t<union id=\"vnb\">\n"
            "\t\t<field name=\"u\" type=\"v16u\"/>\n"
            "\t\t<field name=\"s\" type=\"v16i\"/>\n"
            "\t</union>\n"
            "\t<union id=\"vnq\">\n"
            "\t\t<field name=\"u\" type=\"v1u\"/>\n"
            "\t\t<field name=\"s\" type=\"v1i\"/>\n"
            "\t</union>\n"
            "\t<union id=\"aarch64v\">\n"
            "\t\t<field name=\"d\" type=\"vnd\"/>\n"
            "\t\t<field name=\"s\" type=\"vns\"/>\n"
            "\t\t<field name=\"h\" type=\"vnh\"/>\n"
            "\t\t<field name=\"b\" type=\"vnb\"/>\n"
            "\t\t<field name=\"q\" type=\"vnq\"/>\n"
            "\t</union>\n"
            "\t<reg name=\"v0\" bitsize=\"128\" type=\"aarch64v\" regnum=\"34\"/>\n"
            "\t<reg name=\"v1\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v2\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v3\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v4\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v5\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v6\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v7\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v8\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v9\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v10\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v11\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v12\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v13\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v14\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v15\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v16\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v17\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v18\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v19\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v20\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v21\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v22\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v23\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v24\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v25\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v26\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v27\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v28\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v29\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v30\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"v31\" bitsize=\"128\" type=\"aarch64v\"/>\n"
            "\t<reg name=\"fpsr\" bitsize=\"32\"/>\n"
            "\t<reg name=\"fpcr\" bitsize=\"32\"/>\n"
            "</feature>";

        constexpr const char TargetXmlAarch32[] =
            "l<?xml version=\"1.0\"?>"
            "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
            "<target>"
            "<xi:include href=\"arm-core.xml\"/>"
            "<xi:include href=\"arm-vfp.xml\"/>"
            "</target>";

        constexpr const char ArmCoreXml[] =
            "l<?xml version=\"1.0\"?>\n"
            "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n"
            "<feature name=\"org.gnu.gdb.arm.core\">\n"
            "\t<reg name=\"r0\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r1\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r2\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r3\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r4\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r5\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r6\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r7\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r8\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r9\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"r10\" bitsize=\"32\"/>\n"
            "\t<reg name=\"r11\" bitsize=\"32\"/>\n"
            "\t<reg name=\"r12\" bitsize=\"32\"/>\n"
            "\t<reg name=\"sp\"  bitsize=\"32\" type=\"data_ptr\"/>\n"
            "\t<reg name=\"lr\"  bitsize=\"32\"/>\n"
            "\t<reg name=\"pc\"  bitsize=\"32\" type=\"code_ptr\"/>\n"
            "\t<reg name=\"cpsr\" bitsize=\"32\" regnum=\"25\"/>\n"
            "</feature>\n";

        constexpr const char ArmVfpXml[] =
            "l<?xml version=\"1.0\"?>\n"
            "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n"
            "<feature name=\"org.gnu.gdb.arm.vfp\">\n"
            "\t<reg name=\"d0\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d1\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d2\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d3\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d4\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d5\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d6\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d7\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d8\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d9\"  bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d10\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d11\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d12\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d13\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d14\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d15\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d16\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d17\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d18\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d19\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d20\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d21\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d22\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d23\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d24\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d25\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d26\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d27\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d28\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d29\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d30\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"d31\" bitsize=\"64\" type=\"ieee_double\"/>\n"
            "\t<reg name=\"fpscr\" bitsize=\"32\" type=\"int\" group=\"float\"/>\n"
            "</feature>\n";

        bool ParsePrefix(char *&packet, const char *prefix) {
            const auto len = std::strlen(prefix);
            if (std::strncmp(packet, prefix, len) == 0) {
                packet += len;
                return true;
            } else {
                return false;
            }
        }

        void SetReplyOk(char *reply) {
            std::strcpy(reply, "OK");
        }

        void SetReplyError(char *reply, const char *err) {
            AMS_DMNT2_GDB_LOG_ERROR("Reply Error: %s\n", err);
            std::strcpy(reply, err);
        }

        void SetReply(char *reply, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

        void SetReply(char *reply, const char *fmt, ...) {
            std::va_list vl;
            va_start(vl, fmt);
            util::VSNPrintf(reply, GdbPacketBufferSize, fmt, vl);
            va_end(vl);
        }

        void AppendReply(char *reply, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

        void AppendReply(char *reply, const char *fmt, ...) {
            const auto len = std::strlen(reply);

            std::va_list vl;
            va_start(vl, fmt);
            util::VSNPrintf(reply + len, GdbPacketBufferSize - len, fmt, vl);
            va_end(vl);
        }

        constexpr int DecodeHex(char c) {
            if ('a' <= c && c <= 'f') {
                return 10 + (c - 'a');
            } else if ('A' <= c && c <= 'F') {
                return 10 + (c - 'A');
            } else if ('0' <= c && c <= '9') {
                return 0 + (c - '0');
            } else {
                return -1;
            }
        }

        void ParseOffsetLength(const char *packet, u32 &offset, u32 &length) {
            /* Default to zero. */
            offset = 0;
            length = 0;

            bool parsed_offset = false;
            while (*packet) {
                const char c = *(packet++);
                if (c == ',') {
                    parsed_offset = true;
                } else if (auto hex = DecodeHex(c); hex >= 0) {
                    if (parsed_offset) {
                        length <<= 4;
                        length |= hex;
                    } else {
                        offset <<= 4;
                        offset |= hex;
                    }
                }
            }

            AMS_DMNT2_GDB_LOG_DEBUG("Offset/Length %x/%x\n", offset, length);
        }

    }

    void GdbPacketParser::Process() {
        /* Log the packet we're processing. */
        AMS_DMNT2_GDB_LOG_DEBUG("Receive: %s\n", m_receive_packet);

        /* Clear our reply packet. */
        m_reply_packet[0] = 0;

        /* Handle the received packet. */
        switch (m_receive_packet[0]) {
            case 'q':
                this->q();
                break;
            case '!':
                SetReplyOk(m_reply_packet);
                break;
            default:
                AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented: %s\n", m_receive_packet);
                break;
        }
    }

    void GdbPacketParser::q() {
        if (ParsePrefix(m_receive_packet, "qSupported:")) {
            this->qSupported();
        } else if (ParsePrefix(m_receive_packet, "qXfer:features:read:")) {
            this->qXferFeaturesRead();
        } else {
            AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented q: %s\n", m_receive_packet);
        }
    }

    void GdbPacketParser::qSupported() {
        /* Current string from devkita64-none-elf-gdb: */
        /* qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+ */

        SetReply(m_reply_packet, "PacketSize=%lx", GdbPacketBufferSize - 1);
        AppendReply(m_reply_packet, ";multiprocess+");
        AppendReply(m_reply_packet, ";qXfer:osdata:read+");
        AppendReply(m_reply_packet, ";qXfer:features:read+");
        AppendReply(m_reply_packet, ";qXfer:libraries:read+");
        AppendReply(m_reply_packet, ";qXfer:threads:read+");
        AppendReply(m_reply_packet, ";swbreak+");
        AppendReply(m_reply_packet, ";hwbreak+");
    }

    void GdbPacketParser::qXferFeaturesRead() {
        u32 offset, length;

        if (ParsePrefix(m_receive_packet, "target.xml:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Send the desired xml. */
            std::strncpy(m_reply_packet, (this->Is64Bit() ? TargetXmlAarch64 : TargetXmlAarch32) + offset, length);
            m_reply_packet[length] = 0;
            m_reply_packet += std::strlen(m_reply_packet);
        } else if (ParsePrefix(m_receive_packet, "aarch64-core.xml:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Send the desired xml. */
            std::strncpy(m_reply_packet, Aarch64CoreXml + offset, length);
            m_reply_packet[length] = 0;
            m_reply_packet += std::strlen(m_reply_packet);
        } else if (ParsePrefix(m_receive_packet, "aarch64-fpu.xml:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Send the desired xml. */
            std::strncpy(m_reply_packet, Aarch64FpuXml + offset, length);
            m_reply_packet[length] = 0;
            m_reply_packet += std::strlen(m_reply_packet);
        }  else if (ParsePrefix(m_receive_packet, "arm-core.xml:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Send the desired xml. */
            std::strncpy(m_reply_packet, ArmCoreXml + offset, length);
            m_reply_packet[length] = 0;
            m_reply_packet += std::strlen(m_reply_packet);
        } else if (ParsePrefix(m_receive_packet, "arm-vfp.xml:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Send the desired xml. */
            std::strncpy(m_reply_packet, ArmVfpXml + offset, length);
            m_reply_packet[length] = 0;
            m_reply_packet += std::strlen(m_reply_packet);
        } else {
            AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented qxfer:features:read: %s\n", m_receive_packet);
        }
    }

}