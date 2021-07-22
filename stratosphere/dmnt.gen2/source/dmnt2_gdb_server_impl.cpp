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
#include "dmnt2_gdb_server_impl.hpp"

namespace ams::dmnt {

    namespace {

        constexpr const u32 SdkBreakPoint     = 0xE7FFFFFF;
        constexpr const u32 SdkBreakPointMask = 0xFFFFFFFF;

        constexpr const u32 ArmBreakPoint     = 0xE7FFDEFE;
        constexpr const u32 ArmBreakPointMask = 0xFFFFFFFF;

        constexpr const u32 A64BreakPoint     = 0xD4200000;
        constexpr const u32 A64BreakPointMask = 0xFFE0001F;

        constexpr const u32 A64Halt           = 0xD4400000;
        constexpr const u32 A64HaltMask       = 0xFFE0001F;

        constexpr const u32 A32BreakPoint     = 0xE1200070;
        constexpr const u32 A32BreakPointMask = 0xFFF000F0;

        constexpr const u32 T16BreakPoint     = 0x0000BE00;
        constexpr const u32 T16BreakPointMask = 0x0000FF00;

        constexpr const char TargetXmlAarch64[] =
            "l<?xml version=\"1.0\"?>"
            "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">"
            "<target>"
            "<architecture>aarch64</architecture>"
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

        constexpr u64 DecodeHex(const char *s) {
            u64 value = 0;

            while (true) {
                const char c = *(s++);

                if (int v = DecodeHex(c); v >= 0) {
                    value <<= 4;
                    value |= v & 0xF;
                } else {
                    break;
                }
            }

            return value;
        }

        void MemoryToHex(char *dst, const void *mem, size_t size) {
            const u8 *mem_u8 = static_cast<const u8 *>(mem);

            while (size-- > 0) {
                const u8 v = *(mem_u8++);
                *(dst++) = "0123456789abcdef"[v >> 4];
                *(dst++) = "0123456789abcdef"[v & 0xF];
            }
            *dst = 0;
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

        s32 FindThreadIdIndex(u64 *thread_ids, s32 num_threads, u64 thread_id) {
            for (auto i = 0; i < num_threads; ++i) {
                if (thread_ids[i] == thread_id) {
                    return i;
                }
            }
            return 0;
        }

        void SetGdbRegister32(char *dst, u32 value) {
            if (value != 0) {
                AppendReply(dst, "%08x", util::ConvertToBigEndian(value));
            } else {
                AppendReply(dst, "0*\"00");
            }
        }

        void SetGdbRegister64(char *dst, u64 value) {
            if (value != 0) {
                AppendReply(dst, "%016lx", util::ConvertToBigEndian(value));
            } else {
                AppendReply(dst, "0*,");
            }
        }

        void SetGdbRegister128(char *dst, u128 value) {
            if (value != 0) {
                AppendReply(dst, "%016lx%016lx", util::ConvertToBigEndian(static_cast<u64>(value >> 0)), util::ConvertToBigEndian(static_cast<u64>(value >> BITSIZEOF(u64))));
            } else {
                AppendReply(dst, "0*<");
            }
        }

        void SetGdbRegisterPacket(char *dst, const svc::ThreadContext &thread_context, bool is_64_bit) {
            /* Clear packet. */
            dst[0] = 0;

            if (is_64_bit) {
                /* Copy general purpose registers. */
                for (size_t i = 0; i < util::size(thread_context.r); ++i) {
                    SetGdbRegister64(dst, thread_context.r[i]);
                }

                /* Copy special registers. */
                SetGdbRegister64(dst, thread_context.fp);
                SetGdbRegister64(dst, thread_context.lr);
                SetGdbRegister64(dst, thread_context.sp);
                SetGdbRegister64(dst, thread_context.pc);

                SetGdbRegister32(dst, thread_context.pstate);

                /* Copy FPU registers. */
                for (size_t i = 0; i < util::size(thread_context.v); ++i) {
                    SetGdbRegister128(dst, thread_context.v[i]);
                }

                SetGdbRegister32(dst, thread_context.fpsr);
                SetGdbRegister32(dst, thread_context.fpcr);
            } else {
                /* Copy general purpose registers. */
                for (size_t i = 0; i < 15; ++i) {
                    SetGdbRegister32(dst, thread_context.r[i]);
                }

                /* Copy special registers. */
                SetGdbRegister32(dst, thread_context.pc);
                SetGdbRegister32(dst, thread_context.pstate);

                /* Copy FPU registers. */
                for (size_t i = 0; i < util::size(thread_context.v); ++i) {
                    SetGdbRegister128(dst, thread_context.v[i]);
                }

                const u32 fpscr = (thread_context.fpsr & 0xF80000FF) | (thread_context.fpcr & 0x07FFFF00);
                SetGdbRegister32(dst, fpscr);
            }
        }

        constinit os::SdkMutex g_annex_buffer_lock;
        constinit char g_annex_buffer[0x8000];

        enum AnnexBufferContents {
            AnnexBufferContents_Invalid,
            AnnexBufferContents_Processes,
            AnnexBufferContents_Threads,
            AnnexBufferContents_Libraries,
        };

        constinit AnnexBufferContents g_annex_buffer_contents = AnnexBufferContents_Invalid;

        void GetAnnexBufferContents(char *dst, u32 offset, u32 length) {
            const u32 annex_len = std::strlen(g_annex_buffer);
            if (offset <= annex_len) {
                if (offset + length < annex_len) {
                    dst[0] = 'm';
                    std::memcpy(dst + 1, g_annex_buffer + offset, length);
                    dst[1 + length] = 0;
                } else {
                    const auto size = annex_len - offset;

                    dst[0] = 'l';
                    std::memcpy(dst + 1, g_annex_buffer + offset, size);
                    dst[1 + size] = 0;
                }
            } else {
                dst[0] = '1';
                dst[1] = 0;
            }
        }

        constinit os::SdkMutex g_event_request_lock;
        constinit os::SdkMutex g_event_lock;
        constinit os::SdkConditionVariable g_event_request_cv;
        constinit os::SdkConditionVariable g_event_done_cv;

    }

    GdbServerImpl::GdbServerImpl(int socket, void *stack, size_t stack_size) : m_socket(socket), m_session(socket), m_packet_io(), m_state(State::Initial), m_debug_process(), m_event(os::EventClearMode_AutoClear) {
        /* Create and start the events thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_events_thread), DebugEventsThreadEntry, this, stack, stack_size, os::HighestThreadPriority - 1));
        os::StartThread(std::addressof(m_events_thread));

        /* Set our state. */
        m_state = State::Running;
    }

    GdbServerImpl::~GdbServerImpl() {
        /* Set ourselves as killed. */
        m_killed = true;

        /* Signal to our events thread. */
        {
            std::scoped_lock lk(g_event_request_lock);
            g_event_request_cv.Signal();
        }

        /* Signal our event. */
        m_event.Signal();

        /* Wait for our thread to finish. */
        os::WaitThread(std::addressof(m_events_thread));
        os::DestroyThread(std::addressof(m_events_thread));

        /* Clear our state. */
        m_state = State::Destroyed;

        /* Detach. */
        if (this->HasDebugProcess()) {
            m_debug_process.Detach();
        }
    }

    void GdbServerImpl::DebugEventsThread() {
        /* Process events. */
        {
            std::scoped_lock lk(g_event_lock);

            /* Loop while we're not killed. */
            while (!m_killed) {
                /* Wait for a request to come in. */
                g_event_request_cv.Wait(g_event_lock);

                /* Check that we're not killed now. */
                if (m_killed) {
                    break;
                }

                /* Detach. */
                m_debug_process.Detach();

                /* If we have a process id, attach. */
                if (R_FAILED(m_debug_process.Attach(m_process_id))) {
                    AMS_DMNT2_GDB_LOG_DEBUG("Failed to attach to %016lx\n", m_process_id.value);
                    g_event_done_cv.Signal();
                    continue;
                }

                /* Set our process id. */
                m_process_id = m_debug_process.GetProcessId();

                /* Signal that we're done attaching. */
                g_event_done_cv.Signal();

                /* Process debug events without the lock held. */
                {
                    g_event_lock.Unlock();
                    this->ProcessDebugEvents();
                    g_event_lock.Lock();
                }

                /* Clear our process id and detach. */
                m_process_id = os::InvalidProcessId;
                m_debug_process.Detach();
            }
        }

        /* Set our state. */
        m_state = State::Exited;
    }

    void GdbServerImpl::ProcessDebugEvents() {
        AMS_DMNT2_GDB_LOG_DEBUG("Processing debug events for %016lx\n", m_process_id.value);

        while (true) {
            /* Wait for an event to come in. */
            const Result wait_result = [&] ALWAYS_INLINE_LAMBDA {
                std::scoped_lock lk(g_event_lock);

                s32 dummy = -1;
                svc::Handle handle = m_debug_process.GetHandle();
                return svc::WaitSynchronization(std::addressof(dummy), std::addressof(handle), 1, TimeSpan::FromMilliSeconds(20).GetNanoSeconds());
            }();

            /* Check if we're killed. */
            if (m_killed || !m_debug_process.IsValid()) {
                break;
            }

            /* If we didn't get an event, try again. */
            if (svc::ResultTimedOut::Includes(wait_result)) {
                continue;
            }

            /* Try to get the event. */
            svc::DebugEventInfo d;
            if (R_FAILED(m_debug_process.GetProcessDebugEvent(std::addressof(d)))) {
                continue;
            }

            /* Process the event. */
            bool reply = false;
            GdbSignal signal;
            char send_buffer[GdbPacketBufferSize];
            u64 thread_id = d.thread_id;
            m_debug_process.ClearStep();

            send_buffer[0] = 0;
            switch (d.type) {
                case svc::DebugEvent_Exception:
                    {
                        switch (d.info.exception.type) {
                            case svc::DebugException_BreakPoint:
                                {
                                    signal = GdbSignal_BreakpointTrap;

                                    const uintptr_t address = d.info.exception.address;
                                    const bool is_instr     = d.info.exception.specific.break_point.type == svc::BreakPointType_HardwareInstruction;
                                    AMS_DMNT2_GDB_LOG_DEBUG("BreakPoint %lx, addr=%lx, type=%s\n", thread_id, address, is_instr ? "Instr" : "Data");

                                    if (is_instr) {
                                        SetReply(send_buffer, "T%02Xthread:p%lx.%lx;hwbreak:;", static_cast<u32>(signal), m_process_id.value, thread_id);
                                    } else {
                                        bool read = false, write = false;
                                        const char *type = "watch";
                                        if (R_SUCCEEDED(m_debug_process.GetWatchPointInfo(address, read, write))) {
                                            if (read && write) {
                                                type = "awatch";
                                            } else if (read) {
                                                type = "rwatch";
                                            }
                                        } else {
                                            AMS_DMNT2_GDB_LOG_DEBUG("GetWatchPointInfo FAIL %lx, addr=%lx, type=%s\n", thread_id, address, is_instr ? "Instr" : "Data");
                                        }

                                        SetReply(send_buffer, "T%02Xthread:p%lx.%lx;%s:%lx;", static_cast<u32>(signal), m_process_id.value, thread_id, type, address);
                                    }

                                    reply = true;
                                }
                                break;
                            case svc::DebugException_UserBreak:
                                {
                                    const uintptr_t address = d.info.exception.address;
                                    const auto &info = d.info.exception.specific.user_break;
                                    AMS_DMNT2_GDB_LOG_DEBUG("UserBreak %lx, addr=%lx, reason=%x, data=0x%lx, size=0x%lx\n", thread_id, address, info.break_reason, info.address, info.size);

                                    /* Check reason. */
                                    /* TODO: libnx/Nintendo provide addresses in different ways, but we could optimize to avoid iterating all memory repeatedly. */
                                    if ((info.break_reason & svc::BreakReason_NotificationOnlyFlag) != 0) {
                                        const auto reason = info.break_reason & ~svc::BreakReason_NotificationOnlyFlag;
                                        if (reason == svc::BreakReason_PostLoadDll || reason == svc::BreakReason_PostUnloadDll) {
                                            /* Re-collect the process's modules. */
                                            m_debug_process.CollectModules();
                                        }
                                    }

                                    /* Check if we should automatically continue. */
                                    svc::ThreadContext ctx;
                                    if (R_SUCCEEDED(m_debug_process.GetThreadContext(std::addressof(ctx), thread_id, svc::ThreadContextFlag_Control))) {
                                        u32 insn = 0;
                                        if (R_SUCCEEDED(m_debug_process.ReadMemory(std::addressof(insn), ctx.pc, sizeof(insn)))) {
                                            constexpr u32 Aarch64SvcBreakValue = 0xD4000001 | (svc::SvcId_Break << 5);
                                            constexpr u32 Aarch32SvcBreakValue = 0xEF000000 | (svc::SvcId_Break);
                                            bool is_svc_break = m_debug_process.Is64Bit() ? (insn == Aarch64SvcBreakValue) : (insn == Aarch32SvcBreakValue);

                                            if (!is_svc_break) {
                                                AMS_DMNT2_GDB_LOG_ERROR("UserBreak from non-SvcBreak (%08x)\n", insn);
                                                m_debug_process.Continue();
                                                continue;
                                            }
                                        }
                                    }

                                    signal = GdbSignal_BreakpointTrap;
                                }
                                break;
                            case svc::DebugException_DebuggerBreak:
                                {
                                    AMS_DMNT2_GDB_LOG_DEBUG("DebuggerBreak %lx, last=%lx\n", thread_id, m_debug_process.GetLastThreadId());
                                    signal = GdbSignal_Interrupt;
                                    thread_id = m_debug_process.GetLastThreadId();
                                    m_debug_process.SetLastThreadId(thread_id);
                                }
                                break;
                            case svc::DebugException_UndefinedInstruction:
                                {
                                    signal            = GdbSignal_IllegalInstruction;

                                    uintptr_t address = d.info.exception.address;
                                    const u32 insn    = d.info.exception.specific.undefined_instruction.insn;
                                    u32 new_insn      = 0;

                                    svc::ThreadContext ctx;
                                    if (R_SUCCEEDED(m_debug_process.GetThreadContext(std::addressof(ctx), thread_id, svc::ThreadContextFlag_Control))) {
                                        bool insn_changed = false;
                                        if (ctx.pstate & 0x20) {
                                            /* Thumb mode. */
                                            address &= ~1;

                                            if (R_SUCCEEDED(m_debug_process.ReadMemory(std::addressof(new_insn), address, 2))) {
                                                switch ((new_insn >> 11) & 0x1F) {
                                                    case 0x1D:
                                                    case 0x1E:
                                                    case 0x1F:
                                                        {
                                                            if (R_SUCCEEDED(m_debug_process.ReadMemory(reinterpret_cast<u8 *>(std::addressof(new_insn)) + 2, address + 2, 2))) {
                                                                insn_changed = (new_insn != insn);
                                                            }
                                                        }
                                                        break;
                                                    default:
                                                        insn_changed = (new_insn != insn);
                                                        break;
                                                }

                                                if ((insn & T16BreakPointMask) == T16BreakPoint) {
                                                    signal = GdbSignal_BreakpointTrap;
                                                }
                                            }
                                        } else {
                                            /* Non-thumb. */
                                            if (R_SUCCEEDED(m_debug_process.ReadMemory(std::addressof(new_insn), address, sizeof(new_insn)))) {
                                                insn_changed = (new_insn != insn);
                                            }

                                            if (((insn & SdkBreakPointMask) == SdkBreakPoint) ||
                                                ((insn & ArmBreakPointMask) == ArmBreakPoint) ||
                                                ((insn & A64BreakPointMask) == A64BreakPoint) ||
                                                ((insn & A32BreakPointMask) == A32BreakPoint) ||
                                                ((insn & A64HaltMask)       == A64Halt))
                                            {
                                                signal = GdbSignal_BreakpointTrap;
                                            }
                                        }

                                        if (insn_changed) {
                                            AMS_DMNT2_GDB_LOG_DEBUG("Instruction Changed %lx, address=%p, insn=%08x, new_insn=%08x\n", thread_id, reinterpret_cast<void *>(address), insn, new_insn);
                                        }
                                    }

                                    if (signal == GdbSignal_IllegalInstruction) {
                                        AMS_DMNT2_GDB_LOG_DEBUG("Undefined Instruction %lx, address=%p, insn=%08x\n", thread_id, reinterpret_cast<void *>(address), insn);
                                    } else if (signal == GdbSignal_BreakpointTrap && ((insn & SdkBreakPointMask) != SdkBreakPoint)) {
                                        AMS_DMNT2_GDB_LOG_DEBUG("Non-SDK BreakPoint %lx, address=%p, insn=%08x\n", thread_id, reinterpret_cast<void *>(address), insn);
                                    }

                                    if (signal == GdbSignal_BreakpointTrap) {
                                        SetReply(send_buffer, "T%02Xthread:p%lx.%lx;swbreak:;", static_cast<u32>(signal), m_process_id.value, thread_id);
                                        reply = true;
                                    }

                                    m_debug_process.ClearStep();
                                }
                                break;
                            default:
                                AMS_DMNT2_GDB_LOG_DEBUG("Unhandled Exception %u %lx\n", static_cast<u32>(d.info.exception.type), thread_id);
                                signal = GdbSignal_SegmentationFault;
                                break;
                        }

                        if (!reply) {
                            SetReply(send_buffer, "T%02Xthread:p%lx.%lx;", static_cast<u32>(signal), m_process_id.value, thread_id);
                            reply = true;
                        }

                        m_debug_process.SetLastThreadId(thread_id);
                        m_debug_process.SetLastSignal(signal);
                    }
                    break;
                case svc::DebugEvent_CreateThread:
                    {
                        AMS_DMNT2_GDB_LOG_DEBUG("CreateThread %lx\n", thread_id);

                        if (m_debug_process.IsValid()) {
                            m_debug_process.Continue();
                        } else {
                            SetReply(send_buffer, "W00");
                            reply = true;
                        }
                    }
                case svc::DebugEvent_ExitThread:
                    {
                        AMS_DMNT2_GDB_LOG_DEBUG("ExitThread %lx\n", thread_id);

                        if (m_debug_process.IsValid()) {
                            m_debug_process.Continue();
                        } else {
                            SetReply(send_buffer, "W00");
                            reply = true;
                        }
                    }
                    break;
                case svc::DebugEvent_ExitProcess:
                    {
                        m_killed = true;
                        AMS_DMNT2_GDB_LOG_DEBUG("ExitProcess\n");

                        if (d.info.exit_process.reason == svc::ProcessExitReason_ExitProcess) {
                            SetReply(send_buffer, "W00");
                        } else {
                            SetReply(send_buffer, "X%02X", GdbSignal_Killed);
                        }

                        m_debug_process.Detach();
                        reply = true;
                    }
                    break;
                default:
                    AMS_DMNT2_GDB_LOG_DEBUG("Unhandled ProcessEvent %u %lx\n", static_cast<u32>(d.type), thread_id);
                    m_debug_process.Continue();
                    break;
            }

            if (reply) {
                bool do_break;
                this->SendPacket(std::addressof(do_break), send_buffer);
                if (do_break) {
                    m_debug_process.Break();
                }
            }
        }
    }

    void GdbServerImpl::SetStopReplyPacket(GdbSignal signal) {
        /* Set the signal. */
        SetReply(m_reply_packet, "T%02X", static_cast<u32>(signal));

        /* Get the last thread id. */
        const u64 thread_id = m_debug_process.GetLastThreadId();

        /* Get the thread context. */
        svc::ThreadContext thread_context = {};
        m_debug_process.GetThreadContext(std::addressof(thread_context), thread_id, svc::ThreadContextFlag_General | svc::ThreadContextFlag_Control);

        /* Add important registers. */
        /* TODO: aarch32 */
        {
            if (thread_context.fp != 0) {
                AppendReply(m_reply_packet, "1d:%016lx", util::ConvertToBigEndian(thread_context.fp));
            } else {
                AppendReply(m_reply_packet, "1d:0*,");
            }

            if (thread_context.sp != 0) {
                AppendReply(m_reply_packet, ";1f:%016lx", util::ConvertToBigEndian(thread_context.sp));
            } else {
                AppendReply(m_reply_packet, ";1f:0*,");
            }

            if (thread_context.pc != 0) {
                AppendReply(m_reply_packet, ";20:%016lx", util::ConvertToBigEndian(thread_context.pc));
            } else {
                AppendReply(m_reply_packet, ";20:0*,");
            }
        }

        /* Add the thread id. */
        AppendReply(m_reply_packet, ";thread:p%lx.%lx", m_process_id.value, thread_id);

        /* Add the thread core. */
        {
            u32 core = 0;
            m_debug_process.GetThreadCurrentCore(std::addressof(core), thread_id);

            AppendReply(m_reply_packet, ";core:%u;", core);
        }
    }

    void GdbServerImpl::LoopProcess() {
        /* Process packets. */
        while (m_session.IsValid()) {
            /* Receive a packet. */
            bool do_break = false;
            char recv_buf[GdbPacketBufferSize];
            char *packet = this->ReceivePacket(std::addressof(do_break), recv_buf, sizeof(recv_buf));

            if (!do_break && packet != nullptr) {
                /* Process the packet. */
                char reply_buffer[GdbPacketBufferSize];
                this->ProcessPacket(packet, reply_buffer);

                /* Send packet. */
                this->SendPacket(std::addressof(do_break), reply_buffer);
            }
        }
    }

    void GdbServerImpl::ProcessPacket(char *receive, char *reply) {
        /* Set our fields. */
        m_receive_packet = receive;
        m_reply_packet   = reply;

        /* Log the packet we're processing. */
        AMS_DMNT2_GDB_LOG_DEBUG("Receive: %s\n", m_receive_packet);

        /* Clear our reply packet. */
        m_reply_packet[0] = 0;

        /* Handle the received packet. */
        switch (m_receive_packet[0]) {
            case 'H':
                this->H();
                break;
            case 'T':
                this->T();
                break;
            case 'Z':
                this->Z();
                break;
            case 'g':
                if (!this->g()) {
                    m_killed = true;
                }
                break;
            case 'm':
                this->m();
                break;
            case 'v':
                this->v();
                break;
            case 'q':
                this->q();
                break;
            case 'z':
                this->z();
                break;
            case '!':
                SetReplyOk(m_reply_packet);
                break;
            case '?':
                this->QuestionMark();
                break;
            default:
                AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented: %s\n", m_receive_packet);
                break;
        }
    }

    void GdbServerImpl::H() {
        if (this->HasDebugProcess()) {
            if (ParsePrefix(m_receive_packet, "Hg") || ParsePrefix(m_receive_packet, "HG")) {
                this->Hg();
            } else {
                SetReplyError(m_reply_packet, "E01");
            }
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::Hg() {
        bool success = false;
        s64 thread_id;
        if (const char *dot = std::strchr(m_receive_packet, '.'); dot != nullptr) {
            thread_id = std::strcmp(dot + 1, "-1") == 0 ? -1 : static_cast<s64>(DecodeHex(dot + 1));
            AMS_DMNT2_GDB_LOG_DEBUG("Set thread id = %lx\n", thread_id);

            u64 thread_ids[DebugProcess::ThreadCountMax];
            s32 num_threads;
            if (R_SUCCEEDED(m_debug_process.GetThreadList(std::addressof(num_threads), thread_ids, util::size(thread_ids)))) {
                if (thread_id == 0) {
                    thread_id = thread_ids[0];
                }
                for (auto i = 0; i < num_threads; ++i) {
                    if (thread_id == -1 || static_cast<u64>(thread_id) == thread_ids[i]) {
                        svc::ThreadContext context;
                        if (R_SUCCEEDED(m_debug_process.GetThreadContext(std::addressof(context), thread_ids[i], svc::ThreadContextFlag_Control))) {
                            success = true;
                            if (thread_id != -1) {
                                m_debug_process.SetThreadIdOverride(thread_ids[i]);
                            }
                        }
                    }
                }
            }
        }

        if (success) {
            SetReplyOk(m_reply_packet);
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::T() {
        if (const char *dot = std::strchr(m_receive_packet, '.'); dot != nullptr) {
            const u64 thread_id = DecodeHex(dot + 1);

            svc::ThreadContext ctx;
            if (R_SUCCEEDED(m_debug_process.GetThreadContext(std::addressof(ctx), thread_id, svc::ThreadContextFlag_Control))) {
                SetReplyOk(m_reply_packet);
            } else {
                SetReply(m_reply_packet, "E01");
            }
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::Z() {
        /* Increment past the 'Z'. */
        ++m_receive_packet;

        /* Decode the type. */
        if (!('0' <= m_receive_packet[0] && m_receive_packet[0] <= '4') || m_receive_packet[1] != ',') {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        const auto type = m_receive_packet[0] - '0';
        m_receive_packet += 2;

        /* Decode the address/length. */
        const char *comma = std::strchr(m_receive_packet, ',');
        if (comma == nullptr) {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Parse address/length. */
        const u64 address = DecodeHex(m_receive_packet);
        const u64 length  = DecodeHex(comma + 1);

        switch (type) {
            case 0: /* SW */
                {
                    if (length == 2 || length == 4) {
                        if (R_SUCCEEDED(m_debug_process.SetBreakPoint(address, length, false))) {
                            SetReplyOk(m_reply_packet);
                        } else {
                            SetReplyError(m_reply_packet, "E01");
                        }
                    }
                }
                break;
            case 1: /* HW */
                {
                    if (length == 2 || length == 4) {
                        if (R_SUCCEEDED(m_debug_process.SetHardwareBreakPoint(address, length, false))) {
                            SetReplyOk(m_reply_packet);
                        } else {
                            SetReplyError(m_reply_packet, "E01");
                        }
                    }
                }
                break;
            case 2: /* Watch-W */
                {
                    if (m_debug_process.IsValidWatchPoint(address, length)) {
                        if (R_SUCCEEDED(m_debug_process.SetWatchPoint(address, length, false, true))) {
                            SetReplyOk(m_reply_packet);
                        } else {
                            SetReplyError(m_reply_packet, "E01");
                        }
                    }
                }
                break;
            case 3: /* Watch-R */
                {
                    if (m_debug_process.IsValidWatchPoint(address, length)) {
                        if (R_SUCCEEDED(m_debug_process.SetWatchPoint(address, length, true, false))) {
                            SetReplyOk(m_reply_packet);
                        } else {
                            SetReplyError(m_reply_packet, "E01");
                        }
                    }
                }
                break;
            case 4: /* Watch-A */
                {
                    if (m_debug_process.IsValidWatchPoint(address, length)) {
                        if (R_SUCCEEDED(m_debug_process.SetWatchPoint(address, length, true, true))) {
                            SetReplyOk(m_reply_packet);
                        } else {
                            SetReplyError(m_reply_packet, "E01");
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    bool GdbServerImpl::g() {
        /* Get thread id. */
        u64 thread_id = m_debug_process.GetThreadIdOverride();
        if (thread_id == 0 || thread_id == static_cast<u64>(-1)) {
            thread_id = m_debug_process.GetLastThreadId();
        }

        /* Get thread context. */
        svc::ThreadContext thread_context;
        if (R_FAILED(m_debug_process.GetThreadContext(std::addressof(thread_context), thread_id, svc::ThreadContextFlag_All))) {
            return false;
        }

        /* Populate reply packet. */
        SetGdbRegisterPacket(m_reply_packet, thread_context, m_debug_process.Is64Bit());

        return true;
    }

    void GdbServerImpl::m() {
        ++m_receive_packet;

        /* Validate format. */
        const char *comma = std::strchr(m_receive_packet, ',');
        if (comma == nullptr) {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Parse address/length. */
        const u64 address = DecodeHex(m_receive_packet);
        const u64 length  = DecodeHex(comma + 1);
        if (length >= sizeof(m_buffer)) {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Read the memory. */
        /* TODO: Detect partial readability? */
        if (R_FAILED(m_debug_process.ReadMemory(m_buffer, address, length))) {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Encode the memory. */
        MemoryToHex(m_reply_packet, m_buffer, length);
    }

    void GdbServerImpl::v() {
        if (ParsePrefix(m_receive_packet, "vAttach;")) {
            this->vAttach();
        } else if (ParsePrefix(m_receive_packet, "vCont")) {
            this->vCont();
        } else {
            AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented v: %s\n", m_receive_packet);
        }
    }

    void GdbServerImpl::vAttach() {
        if (!this->HasDebugProcess()) {
            /* Get the process id. */
            if (const u64 process_id = DecodeHex(m_receive_packet); process_id != 0) {
                /* Set our process id. */
                m_process_id = { process_id };

                /* Wait for us to be attached. */
                {
                    std::scoped_lock lk(g_event_request_lock);
                    g_event_request_cv.Signal();
                    if (!g_event_done_cv.TimedWait(g_event_request_lock, TimeSpan::FromSeconds(2))) {
                        m_event.Signal();
                    }
                }

                /* If we're attached, send a stop reply packet. */
                if (m_debug_process.IsValid()) {
                    /* Set the stop reply packet. */
                    this->SetStopReplyPacket(m_debug_process.GetLastSignal());
                } else {
                    SetReplyError(m_reply_packet, "E01");
                }
            } else {
                SetReplyError(m_reply_packet, "E01");
            }
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::vCont() {
        /* Check if this is a query about what we support. */
        if (ParsePrefix(m_receive_packet, "?")) {
            SetReply(m_reply_packet, "vCont;c;C;s;S;");
            return;
        }

        /* We want to parse semicolon separated fields repeatedly. */
        char *saved;
        char *token = strtok_r(m_receive_packet, ";", std::addressof(saved));

        /* Validate the initial token. */
        if (token == nullptr) {
            return;
        }

        /* Prepare to parse threads. */
        u64 thread_ids[DebugProcess::ThreadCountMax] = {};
        u8 continue_modes[DebugProcess::ThreadCountMax] = {};

        s32 num_threads;
        if (R_FAILED(m_debug_process.GetThreadList(std::addressof(num_threads), thread_ids, util::size(thread_ids)))) {
            AMS_DMNT2_GDB_LOG_ERROR("vCont: Failed to get thread list\n");
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Handle each token. */
        Result result = ResultSuccess();
        DebugProcess::ContinueMode default_continue_mode = DebugProcess::ContinueMode_Stopped;
        while (token != nullptr && R_SUCCEEDED(result)) {
            result = this->ParseVCont(token, thread_ids, continue_modes, num_threads, default_continue_mode);
            token = strtok_r(nullptr, ";", std::addressof(saved));
        }

        AMS_DMNT2_GDB_LOG_DEBUG("vCont: NumThreads=%d, Default Continue Mode=%d\n", num_threads, static_cast<int>(default_continue_mode));

        /* Act on all threads. */
        s64 thread_id = -1;
        for (auto i = 0; i < num_threads; ++i) {
            if (continue_modes[i] == DebugProcess::ContinueMode_Step || (continue_modes[i] == DebugProcess::ContinueMode_Stopped && default_continue_mode == DebugProcess::ContinueMode_Step)) {
                thread_id = thread_ids[i];
                result = m_debug_process.Step(thread_ids[i]);
            }
        }

        /* Continue the last thread. */
        if (static_cast<u64>(thread_id) == m_debug_process.GetLastThreadId() && default_continue_mode != DebugProcess::ContinueMode_Continue) {
            result = m_debug_process.Continue(thread_id);
        } else {
            result = m_debug_process.Continue();
        }

        /* Set reply. */
        if (R_SUCCEEDED(result)) {
            SetReplyOk(m_reply_packet);
        } else {
            AMS_DMNT2_GDB_LOG_ERROR("vCont: Failed %08x\n", result.GetValue());
            SetReplyError(m_reply_packet, "E01");
        }
    }

    Result GdbServerImpl::ParseVCont(char * const token, u64 *thread_ids, u8 *continue_modes, s32 num_threads, DebugProcess::ContinueMode &default_continue_mode) {
        /* Parse the thread id. */
        s64 thread_id = -1;
        s32 signal = -1;
        s32 thread_ix = -1;

        if (token[0] && token[1]) {
            if (char *colon = std::strchr(token, ':'); colon != nullptr) {
                *colon = 0;
                if (char *dot = std::strchr(colon + 1, '.'); dot != nullptr) {
                    *dot = 0;
                    thread_id = std::strcmp(dot + 1, "-1") == 0 ? -1 : static_cast<s64>(DecodeHex(dot + 1));
                    thread_ix = FindThreadIdIndex(thread_ids, num_threads, static_cast<u64>(thread_id));
                }
            }
        }

        /* Check that we don't already have a default mode. */
        if (thread_id == -1 && default_continue_mode != DebugProcess::ContinueMode_Stopped) {
            AMS_DMNT2_GDB_LOG_ERROR("vCont: Too many defaults specified\n");
        }

        /* Handle the action. */
        switch (token[0]) {
            case 'c':
                if (thread_id > 0) {
                    AMS_DMNT2_GDB_LOG_DEBUG("vCont: Continue %lx\n", static_cast<u64>(thread_id));
                    continue_modes[thread_ix] = DebugProcess::ContinueMode_Continue;
                } else {
                    default_continue_mode = DebugProcess::ContinueMode_Continue;
                }
                break;
            case 'C':
                if (token[1]) {
                    signal = std::strcmp(token + 1, "-1") == 0 ? -1 : static_cast<s32>(DecodeHex(token + 1));
                }
                AMS_DMNT2_GDB_LOG_WARN("vCont: Ignoring C, signal=%d\n", signal);
                if (thread_id > 0) {
                    AMS_DMNT2_GDB_LOG_DEBUG("vCont: Continue %lx, signal=%d\n", static_cast<u64>(thread_id), signal);
                    continue_modes[thread_ix] = DebugProcess::ContinueMode_Continue;
                } else {
                    default_continue_mode = DebugProcess::ContinueMode_Continue;
                }
                break;
            case 's':
                if (thread_id > 0) {
                    AMS_DMNT2_GDB_LOG_DEBUG("vCont: Step %lx\n", static_cast<u64>(thread_id));
                    continue_modes[thread_ix] = DebugProcess::ContinueMode_Step;
                } else {
                    default_continue_mode = DebugProcess::ContinueMode_Step;
                }
                break;
            case 'S':
                if (token[1]) {
                    signal = std::strcmp(token + 1, "-1") == 0 ? -1 : static_cast<s32>(DecodeHex(token + 1));
                }
                AMS_DMNT2_GDB_LOG_WARN("vCont: Ignoring S, signal=%d\n", signal);
                if (thread_id > 0) {
                    AMS_DMNT2_GDB_LOG_DEBUG("vCont: Step %lx, signal=%d\n", static_cast<u64>(thread_id), signal);
                    continue_modes[thread_ix] = DebugProcess::ContinueMode_Step;
                } else {
                    default_continue_mode = DebugProcess::ContinueMode_Step;
                }
                break;
            default:
                AMS_DMNT2_GDB_LOG_WARN("vCont: Ignoring %c\n", token[0]);
                break;
        }

        return ResultSuccess();
    }

    void GdbServerImpl::q() {
        if (ParsePrefix(m_receive_packet, "qAttached:")) {
            this->qAttached();
        } else if (ParsePrefix(m_receive_packet, "qC")) {
            this->qC();
        } else if (ParsePrefix(m_receive_packet, "qSupported:")) {
            this->qSupported();
        } else if (ParsePrefix(m_receive_packet, "qXfer:")) {
            this->qXfer();
        } else {
            AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented q: %s\n", m_receive_packet);
        }
    }

    void GdbServerImpl::qAttached() {
        if (this->HasDebugProcess()) {
            SetReply(m_reply_packet, "1");
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::qC() {
        if (this->HasDebugProcess()) {
            /* Send the thread id. */
            SetReply(m_reply_packet, "QCp%lx.%lx", m_process_id.value, m_debug_process.GetLastThreadId());
        } else {
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::qSupported() {
        /* Current string from devkita64-none-elf-gdb: */
        /* qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+ */

        SetReply(m_reply_packet, "PacketSize=%lx", GdbPacketBufferSize - 1);
        AppendReply(m_reply_packet, ";multiprocess+");
        AppendReply(m_reply_packet, ";qXfer:osdata:read+");
        AppendReply(m_reply_packet, ";qXfer:features:read+");
        AppendReply(m_reply_packet, ";qXfer:libraries:read+");
        AppendReply(m_reply_packet, ";qXfer:libraries-svr4:read+");
        AppendReply(m_reply_packet, ";augmented-libraries-svr4-read+");
        AppendReply(m_reply_packet, ";qXfer:threads:read+");
        AppendReply(m_reply_packet, ";qXfer:exec-file:read+");
        AppendReply(m_reply_packet, ";swbreak+");
        AppendReply(m_reply_packet, ";hwbreak+");
        AppendReply(m_reply_packet, ";vContSupported+");
    }

    void GdbServerImpl::qXfer() {
        /* Check for osdata. */
        if (ParsePrefix(m_receive_packet, "osdata:read:")) {
            this->qXferOsdataRead();
        } else {
            /* All other qXfer require debug process. */
            if (!this->HasDebugProcess()) {
                SetReplyError(m_reply_packet, "E01");
                return;
            }

            /* Process. */
            if (ParsePrefix(m_receive_packet, "features:read:")) {
                this->qXferFeaturesRead();
            } else if (ParsePrefix(m_receive_packet, "threads:read::")) {
                if (!this->qXferThreadsRead()) {
                    m_killed = true;
                    SetReplyError(m_reply_packet, "E01");
                }
            } else if (ParsePrefix(m_receive_packet, "libraries:read::")) {
                this->qXferLibrariesRead();
            } else if (ParsePrefix(m_receive_packet, "exec-file:read:")) {
                SetReply(m_reply_packet, "l%s", m_debug_process.GetProcessName());
            } else {
                AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented qxfer: %s\n", m_receive_packet);
                SetReplyError(m_reply_packet, "E01");
            }
        }
    }

    void GdbServerImpl::qXferFeaturesRead() {
        /* Handle the qXfer. */
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
            SetReplyError(m_reply_packet, "E01");
        }
    }

    void GdbServerImpl::qXferLibrariesRead() {
        /* Handle the qXfer. */
        u32 offset, length;

        /* Parse offset/length. */
        ParseOffsetLength(m_receive_packet, offset, length);

        /* Acquire access to the annex buffer. */
        std::scoped_lock lk(g_annex_buffer_lock);

        /* If doing a fresh read, generate the module list. */
        if (offset == 0 || g_annex_buffer_contents != AnnexBufferContents_Libraries) {
            /* Set header. */
            SetReply(g_annex_buffer, "<library-list>");

            /* Get the module list. */
            for (size_t i = 0; i < m_debug_process.GetModuleCount(); ++i) {
                AMS_DMNT2_GDB_LOG_DEBUG("Module[%zu]: %p, %s\n", i, reinterpret_cast<void *>(m_debug_process.GetBaseAddress(i)), m_debug_process.GetModuleName(i));

                AppendReply(g_annex_buffer, "<library name=\"%s\"><segment address=\"0x%lx\" /></library>", m_debug_process.GetModuleName(i), m_debug_process.GetBaseAddress(i));
            }

            AppendReply(g_annex_buffer, "</library-list>");

            g_annex_buffer_contents = AnnexBufferContents_Libraries;
        }

        /* Copy out the module list. */
        GetAnnexBufferContents(m_reply_packet, offset, length);
    }

    void GdbServerImpl::qXferOsdataRead() {
        /* Handle the qXfer. */
        u32 offset, length;

        if (ParsePrefix(m_receive_packet, "processes:")) {
            /* Parse offset/length. */
            ParseOffsetLength(m_receive_packet, offset, length);

            /* Acquire access to the annex buffer. */
            std::scoped_lock lk(g_annex_buffer_lock);

            /* If doing a fresh read, generate the process list. */
            if (offset == 0 || g_annex_buffer_contents != AnnexBufferContents_Processes) {
                /* Clear the process list buffer. */
                g_annex_buffer[0] = 0;

                /* Set header. */
                SetReply(g_annex_buffer, "<?xml version=\"1.0\"?>\n<!DOCTYPE target SYSTEM \"osdata.dtd\">\n<osdata type=\"processes\">\n");

                /* Get all processes. */
                {
                    /* Get all process ids. */
                    u64 process_ids[0x50];
                    s32 num_process_ids;
                    R_ABORT_UNLESS(svc::GetProcessList(std::addressof(num_process_ids), process_ids, util::size(process_ids)));

                    /* Send all processes. */
                    for (s32 i = 0; i < num_process_ids; ++i) {
                        svc::Handle handle;
                        if (R_SUCCEEDED(svc::DebugActiveProcess(std::addressof(handle), process_ids[i]))) {
                            ON_SCOPE_EXIT { R_ABORT_UNLESS(svc::CloseHandle(handle)); };

                            /* Get the create process event. */
                            svc::DebugEventInfo d;
                            R_ABORT_UNLESS(svc::GetDebugEvent(std::addressof(d), handle));
                            AMS_ABORT_UNLESS(d.type == svc::DebugEvent_CreateProcess);

                            AppendReply(g_annex_buffer, "<item>\n<column name=\"pid\">%lu</column>\n<column name=\"command\">%s</column>\n</item>\n", d.info.create_process.process_id, d.info.create_process.name);
                        }
                    }
                }

                /* Set footer. */
                AppendReply(g_annex_buffer, "</osdata>");

                g_annex_buffer_contents = AnnexBufferContents_Processes;
            }

            /* Copy out the process list. */
            GetAnnexBufferContents(m_reply_packet, offset, length);
        } else {
            AMS_DMNT2_GDB_LOG_DEBUG("Not Implemented qxfer:osdata:read: %s\n", m_receive_packet);
            SetReplyError(m_reply_packet, "E01");
        }
    }

    bool GdbServerImpl::qXferThreadsRead() {
        /* Handle the qXfer. */
        u32 offset, length;

        /* Parse offset/length. */
        ParseOffsetLength(m_receive_packet, offset, length);

        /* Acquire access to the annex buffer. */
        std::scoped_lock lk(g_annex_buffer_lock);

        /* If doing a fresh read, generate the thread list. */
        if (offset == 0 || g_annex_buffer_contents != AnnexBufferContents_Threads) {
            /* Set header. */
            SetReply(g_annex_buffer, "<threads>");

            /* Get the thread list. */
            u64 thread_ids[DebugProcess::ThreadCountMax];
            s32 num_threads;
            if (R_SUCCEEDED(m_debug_process.GetThreadList(std::addressof(num_threads), thread_ids, util::size(thread_ids)))) {
                for (auto i = 0; i < num_threads; ++i) {
                    /* Check that we can get the thread context. */
                    {
                        svc::ThreadContext dummy_context;
                        if (R_FAILED(m_debug_process.GetThreadContext(std::addressof(dummy_context), thread_ids[i], svc::ThreadContextFlag_All))) {
                            continue;
                        }
                    }

                    /* Get the thread core. */
                    u32 core = 0;
                    m_debug_process.GetThreadCurrentCore(std::addressof(core), thread_ids[i]);

                    /* TODO: `name=\"%s\"`? */
                    AppendReply(g_annex_buffer, "<thread id=\"p%lx.%lx\" core=\"%u\"/>", m_process_id.value, thread_ids[i], core);
                }
            }

            AppendReply(g_annex_buffer, "</threads>");

            g_annex_buffer_contents = AnnexBufferContents_Threads;
        }

        /* Copy out the threads list. */
        GetAnnexBufferContents(m_reply_packet, offset, length);

        return true;
    }

    void GdbServerImpl::z() {
        /* Increment past the 'z'. */
        ++m_receive_packet;

        /* Decode the type. */
        if (!('0' <= m_receive_packet[0] && m_receive_packet[0] <= '4') || m_receive_packet[1] != ',') {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        const auto type = m_receive_packet[0] - '0';
        m_receive_packet += 2;

        /* Decode the address/length. */
        const char *comma = std::strchr(m_receive_packet, ',');
        if (comma == nullptr) {
            SetReplyError(m_reply_packet, "E01");
            return;
        }

        /* Parse address/length. */
        const u64 address = DecodeHex(m_receive_packet);
        const u64 length  = DecodeHex(comma + 1);

        switch (type) {
            case 0: /* SW */
                {
                    if (R_SUCCEEDED(m_debug_process.ClearBreakPoint(address, length))) {
                        SetReplyOk(m_reply_packet);
                    } else {
                        SetReplyError(m_reply_packet, "E01");
                    }
                }
                break;
            case 1: /* HW */
                {
                    if (R_SUCCEEDED(m_debug_process.ClearHardwareBreakPoint(address, length))) {
                        SetReplyOk(m_reply_packet);
                    } else {
                        SetReplyError(m_reply_packet, "E01");
                    }
                }
                break;
            case 2: /* Watch-W */
            case 3: /* Watch-R */
            case 4: /* Watch-A */
                {
                    if (R_SUCCEEDED(m_debug_process.ClearWatchPoint(address, length))) {
                        SetReplyOk(m_reply_packet);
                    } else {
                        SetReplyError(m_reply_packet, "E01");
                    }
                }
                break;
            default:
                break;
        }
    }

    void GdbServerImpl::QuestionMark() {
        if (m_debug_process.IsValid()) {
            if (m_debug_process.GetLastThreadId() == 0) {
                SetReply(m_reply_packet, "X01");
            } else {
                this->SetStopReplyPacket(m_debug_process.GetLastSignal());
            }
        } else {
            SetReply(m_reply_packet, "W00");
        }
    }

}