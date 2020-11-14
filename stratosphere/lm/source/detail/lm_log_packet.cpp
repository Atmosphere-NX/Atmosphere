#include "lm_log_packet.hpp"

namespace ams::lm::detail {

    LogPacketTransmitterBase::LogPacketTransmitterBase(u8 *log_buffer, size_t log_buffer_size, LogPacketFlushFunction flush_fn, LogSeverity severity, bool verbosity, u64 process_id, bool head, bool tail) {
        AMS_ABORT_UNLESS(log_buffer != nullptr);
        // TODO: check log_buffer align == alignof(LogPacketHeader)
        AMS_ABORT_UNLESS(log_buffer_size > sizeof(LogPacketHeader));
        AMS_ABORT_UNLESS(flush_fn != nullptr);
        
        this->header = reinterpret_cast<LogPacketHeader*>(log_buffer);
        this->log_buffer_start = log_buffer;
        this->log_buffer_end = log_buffer + log_buffer_size;
        this->log_buffer_payload_start = log_buffer + sizeof(LogPacketHeader);
        this->log_buffer_payload_current = log_buffer + sizeof(LogPacketHeader);
        this->is_tail = tail;
        this->flush_function = flush_fn;
        this->header->SetProcessId(process_id);

        auto current_thread = os::GetCurrentThread();
        auto thread_id = os::GetThreadId(current_thread);
        this->header->SetThreadId(thread_id);

        this->header->SetHead(head);
        this->header->SetLittleEndian(true);
        this->header->SetSeverity(severity);
        this->header->SetVerbosity(verbosity);
    }

    bool LogPacketTransmitterBase::Flush(bool tail) {
        if (this->log_buffer_payload_current == this->log_buffer_payload_start) {
            /* Nothing to log. */
            return true;
        }
        
        this->header->SetTail(tail);
        this->header->SetPayloadSize(static_cast<size_t>(this->log_buffer_payload_current - this->log_buffer_payload_start));

        auto ret = (this->flush_function)(this->log_buffer_start, static_cast<size_t>(this->log_buffer_payload_current - this->log_buffer_start));

        this->header->SetHead(false);
        this->log_buffer_payload_current = this->log_buffer_payload_start;
        return ret;
    }

    void LogPacketTransmitterBase::PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t data_size, bool is_string) {
        auto start_position = this->log_buffer_payload_current;
        auto data_start = reinterpret_cast<const u8*>(data);
        auto data_end = data_start + size;
        this->PushUleb128(key);
        auto unk = this->log_buffer_payload_current + 2; // ?
        do {
            auto left_data = this->log_buffer_end - this->log_buffer_payload_current;
            if (left_data >= unk) {
                // A
            }

            if (this->log_buffer_payload_current != this->log_buffer_payload_start) {
                this->Flush(false);
            }

            if (left_data >= unk) {
                // A
            }
            // ...
            if (data_end - data_start) >= x) {
                z = x;
            }
            else {
                z = static_cast<size_t>(data_end - data_start);
            }
            if (is_string) {
                auto some_size = z;
                auto utf8_size = 0; // nn::diag::detail::GetValidSizeAsUtf8String(...);
                if (utf8_size >= 0) {
                    some_size = utf8_size;
                }
                d = some_size;
            }
            else {
                d = z;
            }
        } while(a < data_end);
    }

    void LogPacketTransmitterBase::PushData(const void *data, size_t data_size) {
        AMS_ABORT_UNLESS(data != nullptr);
        AMS_ABORT_UNLESS(data_size > this->GetRemainSize());
        if (data_size == 0) {
            return;
        }

        std::memcpy(this->log_buffer_payload_current, data, data_size);
        this->log_buffer_payload_current += data_size;
    }

}