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
#include "uart_mitm_logger.hpp"
#include "../amsmitm_fs_utils.hpp"

namespace ams::mitm::uart {

    alignas(os::ThreadStackAlignment) u8 g_logger_stack[0x1000];

    std::shared_ptr<UartLogger> g_logger;

    UartLogger::UartLogger() : m_request_event(os::EventClearMode_ManualClear), m_finish_event(os::EventClearMode_ManualClear), m_client_queue(m_client_queue_list, this->QueueSize), m_thread_queue(m_thread_queue_list, this->QueueSize) {
        for (size_t i=0; i<this->QueueSize; i++) {
            UartLogMessage *msg = &this->m_queue_list_msgs[i];
            std::memset(msg, 0, sizeof(UartLogMessage));

            msg->data = static_cast<u8 *>(std::malloc(this->QueueBufferSize));
            AMS_ABORT_UNLESS(msg->data != nullptr);
            std::memset(msg->data, 0, this->QueueBufferSize);

            this->m_client_queue.Send(reinterpret_cast<uintptr_t>(msg));
        }

        /* Create and start the logger thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(this->m_thread), this->ThreadEntry, this, g_logger_stack, sizeof(g_logger_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(uart, IpcServer) - 2));
        os::StartThread(std::addressof(this->m_thread));
    }

    UartLogger::~UartLogger() {
        /* Tell the logger thread to exit. */
        UartLogMessage *msg=nullptr;
        this->m_client_queue.Receive(reinterpret_cast<uintptr_t *>(&msg));

        msg->type = 0;

        this->m_finish_event.Clear();
        this->m_thread_queue.Send(reinterpret_cast<uintptr_t>(msg));
        this->m_request_event.Signal();

        /* Wait on the logger thread, then destroy it. */
        os::WaitThread(std::addressof(this->m_thread));
        os::DestroyThread(std::addressof(this->m_thread));

        for (size_t i=0; i<this->QueueSize; i++) {
            UartLogMessage *msg = &this->m_queue_list_msgs[i];
            std::free(msg->data);
            msg->data = nullptr;
        }
    }

    void UartLogger::ThreadFunction() {
        bool exit_flag=false;

        this->m_cache_count = 0;
        this->m_cache_pos = 0;
        std::memset(this->m_cache_list, 0, sizeof(this->m_cache_list));
        std::memset(this->m_cache_buffer, 0, sizeof(this->m_cache_buffer));

        while (!exit_flag) {
            this->m_request_event.Wait();

            /* Receive messages, process them, then Send them. */
            UartLogMessage *msg=nullptr;
            while (this->m_thread_queue.TryReceive(reinterpret_cast<uintptr_t *>(&msg))) {
                if (msg->type==0) {
                    exit_flag = true;
                }
                else if (msg->type==1) {
                    this->WriteCache(msg);
                }
                else if (msg->type==2) {
                    this->WriteCmdLog(reinterpret_cast<const char*>(msg->data), reinterpret_cast<const char*>(&msg->data[std::strlen((const char*)msg->data)+1]), msg->file_pos);
                }
                else if (msg->type==3) {
                    this->FlushCache();
                }
                this->m_client_queue.Send(reinterpret_cast<uintptr_t>(msg));
            }

            this->m_request_event.Clear();
            this->m_finish_event.Signal();
        }
    }

    /* Wait for the thread to finish processing messages. */
    void UartLogger::WaitFinished() {
        /* Tell the thread to flush the cache. */
        UartLogMessage *msg=nullptr;
        this->m_client_queue.Receive(reinterpret_cast<uintptr_t *>(&msg));

        msg->type = 3;

        this->m_finish_event.Clear();
        this->m_thread_queue.Send(reinterpret_cast<uintptr_t>(msg));
        this->m_request_event.Signal();

        /* Wait for processing to finish. */
        m_finish_event.Wait();
    }

    /* Initialize the specified btsnoop log file. */
    void UartLogger::InitializeDataLog(FsFile *f, size_t *datalog_pos) {
        *datalog_pos = 0;

        /* Setup the btsnoop header. */

        struct {
            char id[8];
            u32 version;
            u32 datalink_type;
        } btsnoop_header = { .id = "btsnoop" };

        u32 version = 1;
        u32 datalink_type = 1002; /* HCI UART (H4) */
        ams::util::StoreBigEndian(&btsnoop_header.version, version);
        ams::util::StoreBigEndian(&btsnoop_header.datalink_type, datalink_type);

        /* Write the btsnoop header to the datalog. */
        this->WriteLog(f, datalog_pos, &btsnoop_header, sizeof(btsnoop_header));
    }

    /* Flush the cache into the file. */
    void UartLogger::FlushCache() {
        for (size_t i=0; i<this->m_cache_count; i++) {
            UartLogMessage *cache_msg=&this->m_cache_list[i];
            this->WriteLogPacket(cache_msg->datalog_file, cache_msg->file_pos, cache_msg->timestamp, cache_msg->dir, cache_msg->data, cache_msg->size);
        }

        this->m_cache_count = 0;
        this->m_cache_pos = 0;
    }

    /* Write the specified message into the cache. */
    /* dir: false = Send (host->controller), true = Receive (controller->host). */
    void UartLogger::WriteCache(UartLogMessage *msg) {
        if (this->m_cache_count >= this->CacheListSize || this->m_cache_pos + msg->size >= this->CacheBufferSize) {
            this->FlushCache();
        }

        UartLogMessage *cache_msg=&this->m_cache_list[this->m_cache_count];
        *cache_msg = *msg;
        cache_msg->data = &this->m_cache_buffer[this->m_cache_pos];
        std::memcpy(cache_msg->data, msg->data, msg->size);
        this->m_cache_count++;
        this->m_cache_pos+= msg->size;
    }

    /* Append the specified string to the text file. */
    void UartLogger::WriteCmdLog(const char *path, const char *str, size_t *file_pos) {
        Result rc=0;
        FsFile file={};
        size_t len = std::strlen(str);
        rc = ams::mitm::fs::OpenAtmosphereSdFile(&file, path, FsOpenMode_Read | FsOpenMode_Write | FsOpenMode_Append);
        if (R_SUCCEEDED(rc)) {
            rc = fsFileWrite(&file, *file_pos, str, len, FsWriteOption_None);
        }
        if (R_SUCCEEDED(rc)) {
            *file_pos += len;
        }
        fsFileClose(&file);
    }

    /* Append the specified data to the datalog file. */
    void UartLogger::WriteLog(FsFile *f, size_t *datalog_pos, const void* buffer, size_t size) {
        if (R_SUCCEEDED(fsFileWrite(f, *datalog_pos, buffer, size, FsWriteOption_None))) {
            *datalog_pos += size;
        }
    }

    /* Append the specified packet to the datalog via WriteLog. */
    /* dir: false = Send (host->controller), true = Receive (controller->host). */
    void UartLogger::WriteLogPacket(FsFile *f, size_t *datalog_pos, s64 timestamp, bool dir, const void* buffer, size_t size) {
        struct {
            u32 original_length;
            u32 included_length;
            u32 packet_flags;
            u32 cumulative_drops;
            s64 timestamp_microseconds;
        } pkt_hdr = {};

        u32 flags = 0;
        if (dir) {
            flags |= BIT(0);
        }
        ams::util::StoreBigEndian(&pkt_hdr.original_length, static_cast<u32>(size));
        ams::util::StoreBigEndian(&pkt_hdr.included_length, static_cast<u32>(size));
        ams::util::StoreBigEndian(&pkt_hdr.packet_flags, flags);
        ams::util::StoreBigEndian(&pkt_hdr.timestamp_microseconds, timestamp);

        this->WriteLog(f, datalog_pos, &pkt_hdr, sizeof(pkt_hdr));
        this->WriteLog(f, datalog_pos, buffer, size);
    }

    /* Send the specified data to the Logger thread. */
    /* dir: false = Send (host->controller), true = Receive (controller->host). */
    bool UartLogger::SendLogData(FsFile *f, size_t *file_pos, s64 timestamp_base, s64 tick_base, bool dir, const void* buffer, size_t size) {
        /* Ignore log data which is too large. */
        if (size > this->QueueBufferSize) return false;

        UartLogMessage *msg=nullptr;
        this->m_client_queue.Receive(reinterpret_cast<uintptr_t *>(&msg));
        AMS_ABORT_UNLESS(msg->data != nullptr);

        /* Setup the msg and send it. */
        msg->type = 1;
        msg->dir = dir;
        if (timestamp_base) {
            msg->timestamp = (armTicksToNs(armGetSystemTick() - tick_base) / 1000) + timestamp_base;
        }
        else {
            msg->timestamp = 0;
        }
        msg->datalog_file = f;
        msg->file_pos = file_pos;
        msg->size = size;
        std::memcpy(msg->data, buffer, size);

        this->m_finish_event.Clear();
        this->m_thread_queue.Send(reinterpret_cast<uintptr_t>(msg));
        this->m_request_event.Signal();
        return true;
    }

    /* Send the specified text log to the Logger thread. */
    void UartLogger::SendTextLogData(const char *path, size_t *file_pos, const char *str) {
        /* Ignore log data which is too large. */
        if (std::strlen(path)+1 + std::strlen(str)+1 > this->QueueBufferSize) return;

        UartLogMessage *msg=nullptr;
        this->m_client_queue.Receive(reinterpret_cast<uintptr_t *>(&msg));
        AMS_ABORT_UNLESS(msg->data != nullptr);

        /* Setup the msg and send it. */
        msg->type = 2;
        msg->file_pos = file_pos;
        std::memcpy(msg->data, path, std::strlen(path)+1);
        std::memcpy(&msg->data[std::strlen(path)+1], str, std::strlen(str)+1);

        this->m_finish_event.Clear();
        this->m_thread_queue.Send(reinterpret_cast<uintptr_t>(msg));
        this->m_request_event.Signal();
    }
}
