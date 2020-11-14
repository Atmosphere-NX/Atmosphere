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
#include <stratosphere.hpp>

namespace ams::lm::detail {

    enum LogPacketHeaderFlag {
        LogPacketHeaderFlag_Head = (1 << 0),
        LogPacketHeaderFlag_Tail = (1 << 1),
        LogPacketHeaderFlag_LittleEndian = (1 << 2),
    };

    enum LogSeverity : u8 {
        LogSeverity_Trace = 0,
        LogSeverity_Info = 1,
        LogSeverity_Warn = 2,
        LogSeverity_Error = 3,
        LogSeverity_Fatal = 4
    };

    struct LogBinaryHeader {
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
        LogSeverity severity;
        bool verbosity;
        u32 payload_size;

        void SetProcessId(u64 process_id) {
            this->process_id = process_id;
        }

        void SetThreadId(u64 thread_id) {
            this->thread_id = thread_id;
        }

        void SetLittleEndian(bool le) {
            if (le) {
                this->flags |= LogPacketHeaderFlag_LittleEndian;
            }
            else {
                this->flags &= ~LogPacketHeaderFlag_LittleEndian;
            }
        }

        void SetSeverity(LogSeverity severity) {
            this->severity = severity;
        }

        void SetVerbosity(bool verbosity) {
            this->verbosity = verbosity;
        }

        void SetHead(bool head) {
            if (head) {
                this->flags |= LogPacketHeaderFlag_Head;
            }
            else {
                this->flags &= ~LogPacketHeaderFlag_Head;
            }
        }

        void SetTail(bool tail) {
            if (tail) {
                this->flags |= LogPacketHeaderFlag_Tail;
            }
            else {
                this->flags &= ~LogPacketHeaderFlag_Tail;
            }
        }

        void SetPayloadSize(u32 size) {
            this->payload_size = size;
        }

        u32 GetPayloadSize() {
            return this->payload_size;
        }

    };
    static_assert(sizeof(LogPacketHeader) == 0x18);

    enum LogDataChunkKey : u8 {
        LogDataChunkKey_LogSessionBegin    = 0,  ///< Log session begin (unknown)
        LogDataChunkKey_LogSessionEnd      = 1,  ///< Log session end (unknown)
        LogDataChunkKey_TextLog            = 2,  ///< Text to be logged.
        LogDataChunkKey_LineNumber         = 3,  ///< Source line number.
        LogDataChunkKey_FileName           = 4,  ///< Source file name.
        LogDataChunkKey_FunctionName       = 5,  ///< Source function name.
        LogDataChunkKey_ModuleName         = 6,  ///< Process module name.
        LogDataChunkKey_ThreadName         = 7,  ///< Process thread name.
        LogDataChunkKey_LogPacketDropCount = 8,  ///< Log packet drop count (unknown)
        LogDataChunkKey_UserSystemClock    = 9,  ///< User system clock (unknown)
        LogDataChunkKey_ProcessName        = 10, ///< Process name.
    };

    struct LogDataChunkHeader {
        LogDataChunkKey chunk_key;
        u8 chunk_len;
    };

    using LogPacketFlushFunction = bool(*)(void*, size_t);

    class LogPacketTransmitterBase {
        private:
            LogPacketHeader *header;
            u8 *log_buffer_start;
            u8 *log_buffer_end;
            u8 *log_buffer_payload_start;
            u8 *log_buffer_payload_current;
            bool is_tail;
            LogPacketFlushFunction flush_function;
        public:
            LogPacketTransmitterBase(u8 *log_buffer, size_t log_buffer_size, LogPacketFlushFunction flush_fn, LogSeverity severity, bool verbosity, u64 process_id, bool head, bool tail);

            ~LogPacketTransmitterBase() {
                this->Flush(this->is_tail);
            }

            void PushData(const void *data, size_t data_size);
            bool Flush(bool tail);
            void PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t data_size, bool is_string);
            
            void PushUleb128(u64 value) {
                AMS_ASSERT(this->log_buffer_payload_current != this->log_buffer_end);
                do {
                    u8 byte = value & 0x7F;
                    value >>= 7;

                    if(value != 0) {
                        byte |= 0x80;
                    }

                    *this->log_buffer_payload_current = byte;
                    this->log_buffer_payload_current += sizeof(byte);
                } while(value != 0);
            }

            void PushDataChunk(LogDataChunkKey key, const void *data, size_t data_size) {
                return this->PushDataChunkImpl(key, data, data_size, false);
            }

            void PushDataChunk(LogDataChunkKey key, const char *str, size_t str_len) {
                return this->PushDataChunkImpl(key, str, str_len, true);
            }

            size_t GetRemainSize() {
                return static_cast<size_t>(this->log_buffer_end - this->log_buffer_payload_current);
            }

            size_t GetRequiredSizeToPushUleb128() {
                // TODO
                return 0;
            }
    };

    class LogPacketTransmitter : public LogPacketTransmitterBase {
        public:
            using LogPacketTransmitterBase::LogPacketTransmitterBase;

            void PushLogSessionBegin() {
                u8 dummy_value = 1;
                this->PushDataChunk(LogDataChunkKey_LogSessionBegin, &dummy_value, sizeof(dummy_value));
            }

            void PushLogSessionEnd() {
                u8 dummy_value = 1;
                this->PushDataChunk(LogDataChunkKey_LogSessionEnd, &dummy_value, sizeof(dummy_value));
            }

            void PushTextLog(const char *log, size_t log_len) {
                this->PushDataChunk(LogDataChunkKey_TextLog, log, log_len);
            }

            void PushLineNumber(u32 line_no) {
                this->PushDataChunk(LogDataChunkKey_LineNumber, &line_no, sizeof(line_no));
            }

            void PushFileName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_FileName, name, name_len);
            }

            void PushFunctionName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_FunctionName, name, name_len);
            }

            void PushModuleName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_ModuleName, name, name_len);
            }

            void PushThreadName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_ThreadName, name, name_len);
            }

            void PushLogPacketDropCount(u64 count) {
                this->PushDataChunk(LogDataChunkKey_LogPacketDropCount, &count, sizeof(count));
            }

            void PushUserSystemClock(u64 usc) {
                this->PushDataChunk(LogDataChunkKey_UserSystemClock, &usc, sizeof(usc));
            }

            void PushProcessName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_ProcessName, name, name_len);
            }
    };

}