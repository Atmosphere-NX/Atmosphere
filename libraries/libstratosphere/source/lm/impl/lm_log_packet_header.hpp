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
#pragma once
#include <stratosphere.hpp>

namespace ams::lm::impl {

    constexpr inline size_t LogPacketHeaderSize = 0x18;

    class LogPacketHeader {
        private:
            u64 m_process_id;
            u64 m_thread_id;
            u8 m_flags;
            u8 m_padding;
            u8 m_severity;
            u8 m_verbosity;
            u32 m_payload_size;
        public:
            constexpr u64 GetProcessId() const { return m_process_id; }
            constexpr void SetProcessId(u64 v) { m_process_id = v; }

            constexpr u64 GetThreadId() const { return m_thread_id; }
            constexpr void SetThreadId(u64 v) { m_thread_id = v; }

            constexpr bool IsHead() const { return (m_flags & (1 << 0)) != 0; }
            constexpr void SetHead(bool v) { m_flags = (m_flags & ~(1 << 0)) | ((v ? 1 : 0) << 0); }

            constexpr bool IsTail() const { return (m_flags & (1 << 1)) != 0; }
            constexpr void SetTail(bool v) { m_flags = (m_flags & ~(1 << 1)) | ((v ? 1 : 0) << 1); }

            constexpr bool IsLittleEndian() const { return (m_flags & (1 << 2)) != 0; }
            constexpr void SetLittleEndian(bool v) { m_flags = (m_flags & ~(1 << 2)) | ((v ? 1 : 0) << 2); }

            constexpr u8 GetSeverity() const { return m_severity; }
            constexpr void SetSeverity(u8 v) { m_severity = v; }

            constexpr u8 GetVerbosity() const { return m_verbosity; }
            constexpr void SetVerbosity(u8 v) { m_verbosity = v; }

            constexpr u32 GetPayloadSize() const { return m_payload_size; }
            constexpr void SetPayloadSize(u32 v) { m_payload_size = v; }
    };
    static_assert(sizeof(LogPacketHeader) == LogPacketHeaderSize);

}
