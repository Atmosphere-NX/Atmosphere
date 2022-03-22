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
#include "lm_log_packet_transmitter_base.hpp"

namespace ams::lm::impl {

    class LogPacketTransmitter : public LogPacketTransmitterBase {
        public:
            LogPacketTransmitter(void *buffer, size_t buffer_size, FlushFunction flush_func, u8 severity, u8 verbosity, u64 process_id, bool head, bool tail)
                : LogPacketTransmitterBase(buffer, buffer_size, flush_func, severity, verbosity, process_id, head, tail) { /* ... */ }

            void PushLogSessionBegin() {
                bool value = true;
                this->PushDataChunk(LogDataChunkKey_LogSessionBegin, std::addressof(value), sizeof(value));
            }

            void PushLogSessionEnd() {
                bool value = true;
                this->PushDataChunk(LogDataChunkKey_LogSessionEnd, std::addressof(value), sizeof(value));
            }

            void PushTextLog(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_TextLog, str, len);
            }

            void PushLineNumber(u32 line) {
                this->PushDataChunk(LogDataChunkKey_LineNumber, std::addressof(line), sizeof(line));
            }

            void PushFileName(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_FileName, str, len);
            }

            void PushFunctionName(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_FunctionName, str, len);
            }

            void PushModuleName(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_ModuleName, str, len);
            }

            void PushThreadName(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_ThreadName, str, len);
            }

            void PushLogPacketDropCount(u64 count) {
                this->PushDataChunk(LogDataChunkKey_LineNumber, std::addressof(count), sizeof(count));
            }

            void PushUserSystemClock(s64 posix_time) {
                this->PushDataChunk(LogDataChunkKey_LineNumber, std::addressof(posix_time), sizeof(posix_time));
            }

            void PushProcessName(const char *str, size_t len) {
                this->PushDataChunk(LogDataChunkKey_ProcessName, str, len);
            }
    };

}
