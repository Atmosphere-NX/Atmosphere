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

namespace ams::mitm::uart {

    struct UartLogMessage {
        u8 type;
        bool dir;
        s64 timestamp;
        FsFile *datalog_file;
        size_t *file_pos;
        size_t size;
        u8 *data;
    };

    class UartLogger {
        private:
            ams::os::ThreadType m_thread;
            os::Event m_request_event;
            os::Event m_finish_event;
            os::MessageQueue m_client_queue;
            os::MessageQueue m_thread_queue;

            static constexpr inline size_t QueueSize = 0x20;
            static constexpr inline size_t QueueBufferSize = 0x400;
            static constexpr inline size_t CacheListSize = 0x80;
            static constexpr inline size_t CacheBufferSize = 0x1000;

            uintptr_t m_client_queue_list[QueueSize];
            uintptr_t m_thread_queue_list[QueueSize];
            UartLogMessage m_queue_list_msgs[QueueSize];

            size_t m_cache_count;
            size_t m_cache_pos;
            UartLogMessage m_cache_list[CacheListSize];
            u8 m_cache_buffer[CacheBufferSize];

            static void ThreadEntry(void *arg) { static_cast<UartLogger *>(arg)->ThreadFunction(); }
            void ThreadFunction();

            void FlushCache();
            void WriteCache(UartLogMessage *msg);

            void WriteCmdLog(const char *path, const char *str, size_t *file_pos);
            void WriteLog(FsFile *f, size_t *datalog_pos, const void* buffer, size_t size);
            void WriteLogPacket(FsFile *f, size_t *datalog_pos, s64 timestamp, bool dir, const void* buffer, size_t size);
        public:
            UartLogger();
            ~UartLogger();

            void WaitFinished();

            void InitializeDataLog(FsFile *f, size_t *datalog_pos);

            bool SendLogData(FsFile *f, size_t *file_pos, s64 timestamp_base, s64 tick_base, bool dir, const void* buffer, size_t size);
            void SendTextLogData(const char *path, size_t *file_pos, const char *str);
    };

    extern std::shared_ptr<UartLogger> g_logger;
}
