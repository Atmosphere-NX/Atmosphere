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
#include <stratosphere/lm/detail/lm_detail_log_types.hpp>

namespace ams::lm::detail {

    class LogPacketTransmitterBase {
        NON_COPYABLE(LogPacketTransmitterBase);
        NON_MOVEABLE(LogPacketTransmitterBase);
        public:
            using FlushFunction = bool (*)(const u8*, size_t);
        private:
            LogPacketHeader *header;
            u8 *log_buffer_start;
            u8 *log_buffer_end;
            u8 *log_buffer_payload_start;
            u8 *log_buffer_payload_current;
            bool is_tail;
            FlushFunction flush_function;
        public:
            LogPacketTransmitterBase(void *log_buffer, size_t log_buffer_size, FlushFunction flush_func, u8 severity, u8 verbosity, u64 process_id, bool head, bool tail);

            ~LogPacketTransmitterBase() {
                this->Flush(this->is_tail);
            }

            void PushData(const void *data, size_t data_size);
            bool Flush(bool tail);
            void PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t data_size, bool is_string);
            size_t PushUleb128(u64 value);
            size_t GetPushableDataSize(size_t sz, size_t required_size);

            inline void PushDataChunk(LogDataChunkKey key, const void *data, size_t data_size) {
                return this->PushDataChunkImpl(key, data, data_size, false);
            }

            inline void PushDataChunk(LogDataChunkKey key, const char *str, size_t str_len) {
                return this->PushDataChunkImpl(key, str, str_len, true);
            }

            constexpr size_t GetRemainSize() {
                return static_cast<size_t>(this->log_buffer_end - this->log_buffer_payload_current);
            }

            static constexpr size_t GetRequiredSizeToPushUleb128(u64 value) {
                size_t size = 0;
                do {
                    ++size;
                    value >>= 7;
                } while(value);
                return size;
            }
    };

    class LogPacketTransmitter : public LogPacketTransmitterBase {
        NON_COPYABLE(LogPacketTransmitter);
        NON_MOVEABLE(LogPacketTransmitter);
        public:
            using LogPacketTransmitterBase::LogPacketTransmitterBase;

            void PushLogSessionBegin() {
                bool value = true;
                this->PushDataChunk(LogDataChunkKey_LogSessionBegin, std::addressof(value), sizeof(value));
            }

            void PushLogSessionEnd() {
                bool value = true;
                this->PushDataChunk(LogDataChunkKey_LogSessionEnd, std::addressof(value), sizeof(value));
            }

            void PushTextLog(const char *log, size_t log_len) {
                this->PushDataChunk(LogDataChunkKey_TextLog, log, log_len);
            }

            void PushLineNumber(u32 line_no) {
                this->PushDataChunk(LogDataChunkKey_LineNumber, std::addressof(line_no), sizeof(line_no));
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
                this->PushDataChunk(LogDataChunkKey_LogPacketDropCount, std::addressof(count), sizeof(count));
            }

            void PushUserSystemClock(u64 user_system_clock) {
                this->PushDataChunk(LogDataChunkKey_UserSystemClock, std::addressof(user_system_clock), sizeof(user_system_clock));
            }

            void PushProcessName(const char *name, size_t name_len) {
                this->PushDataChunk(LogDataChunkKey_ProcessName, name, name_len);
            }
    };

}