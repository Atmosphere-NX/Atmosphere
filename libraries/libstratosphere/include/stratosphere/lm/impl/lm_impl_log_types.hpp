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
#include <stratosphere/diag/diag_log_types.hpp>

namespace ams::lm::impl {

    enum LogPacketFlags {
        LogPacketFlags_Head         = (1 << 0),
        LogPacketFlags_Tail         = (1 << 1),
        LogPacketFlags_LittleEndian = (1 << 2)
    };

    struct BinaryLogHeader {
        static constexpr u32 Magic = util::FourCC<'h','p','h','p'>::Code;

        u32 magic;
        u8 version;
        u8 pad[3];
    };

    struct LogPacketHeader {
        u64 process_id;
        u64 thread_id;
        u8 flags;
        u8 pad;
        u8 severity;
        u8 verbosity;
        u32 payload_size;

        constexpr inline u64 GetProcessId() const {
            return this->process_id;
        }

        constexpr inline void SetProcessId(u64 process_id) {
            this->process_id = process_id;
        }

        constexpr inline u64 GetThreadId() const {
            return this->thread_id;
        }

        constexpr inline void SetThreadId(u64 thread_id) {
            this->thread_id = thread_id;
        }

        constexpr inline void SetLittleEndian(bool le) {
            if (le) {
                this->flags |= LogPacketFlags_LittleEndian;
            } else {
                this->flags &= ~LogPacketFlags_LittleEndian;
            }
        }

        constexpr inline void SetSeverity(u8 severity) {
            this->severity = severity;
        }

        constexpr inline void SetVerbosity(u8 verbosity) {
            this->verbosity = verbosity;
        }

        constexpr inline bool IsHead() const {
            return this->flags & LogPacketFlags_Head;
        }

        constexpr inline void SetHead(bool head) {
            if (head) {
                this->flags |= LogPacketFlags_Head;
            } else {
                this->flags &= ~LogPacketFlags_Head;
            }
        }

        constexpr inline bool IsTail() const {
            return this->flags & LogPacketFlags_Tail;
        }

        constexpr inline void SetTail(bool tail) {
            if (tail) {
                this->flags |= LogPacketFlags_Tail;
            }
            else {
                this->flags &= ~LogPacketFlags_Tail;
            }
        }

        constexpr inline void SetPayloadSize(u32 size) {
            this->payload_size = size;
        }

        constexpr inline u32 GetPayloadSize() const {
            return this->payload_size;
        }

    };
    static_assert(util::is_pod<LogPacketHeader>::value && sizeof(LogPacketHeader) == 0x18);

    enum LogDataChunkKey : u8 {
        LogDataChunkKey_LogSessionBegin    = 0,
        LogDataChunkKey_LogSessionEnd      = 1,
        LogDataChunkKey_TextLog            = 2,
        LogDataChunkKey_LineNumber         = 3,
        LogDataChunkKey_FileName           = 4,
        LogDataChunkKey_FunctionName       = 5,
        LogDataChunkKey_ModuleName         = 6,
        LogDataChunkKey_ThreadName         = 7,
        LogDataChunkKey_LogPacketDropCount = 8,
        LogDataChunkKey_UserSystemClock    = 9,
        LogDataChunkKey_ProcessName        = 10,
    };

}