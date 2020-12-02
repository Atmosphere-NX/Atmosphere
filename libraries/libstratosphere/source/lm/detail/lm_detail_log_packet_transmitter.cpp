#include <stratosphere.hpp>

namespace ams::lm::detail {

    LogPacketTransmitterBase::LogPacketTransmitterBase(u8 *log_buffer, size_t log_buffer_size, LogPacketTransmitterBase::FlushFunction flush_fn, diag::LogSeverity severity, bool verbosity, u64 process_id, bool head, bool tail) {
        AMS_ABORT_UNLESS(log_buffer != nullptr);
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

        auto ret = (this->flush_function)(const_cast<const u8*>(this->log_buffer_start), static_cast<size_t>(this->log_buffer_payload_current - this->log_buffer_start));

        this->header->SetHead(false);
        this->log_buffer_payload_current = this->log_buffer_payload_start;
        return ret;
    }

    void LogPacketTransmitterBase::PushDataChunkImpl(LogDataChunkKey key, const void *data, size_t data_size, bool is_string) {
        AMS_ABORT_UNLESS(data != nullptr);

        auto data_start = reinterpret_cast<const u8*>(data);
        auto data_cur = data_start;
        auto data_end = data_start + data_size;

        /* Get the size we will need to push the chunk key. */
        auto key_size = GetRequiredSizeToPushUleb128(key);
        size_t required_size = is_string ? 4 : 1;
        size_t pushable_size = 0;
        do {
            /* Try to get the pushable size. */
            if (this->GetRemainSize() >= (key_size + 2)) {
                pushable_size = this->GetPushableDataSize(required_size, key_size);
            }

            /* Flush if we have to. */
            if (this->log_buffer_payload_current != this->log_buffer_payload_start) {
                this->Flush(false);
            }

            /* Try again. */
            if (this->GetRemainSize() >= (key_size + 2)) {
                pushable_size = this->GetPushableDataSize(required_size, key_size);
            }

            AMS_ABORT_UNLESS(pushable_size != 0);

            /* Determine the actual size of the data we're about to push. */
            size_t cur_pushable_size = 0;
            auto cur_data_size = static_cast<size_t>(data_end - data_cur);
            if (cur_data_size >= pushable_size) {
                cur_pushable_size = pushable_size;
            }
            else {
                cur_pushable_size = cur_data_size;
            }
            size_t data_chunk_size = 0;
            if (is_string) {
                auto tmp_chunk_size = cur_pushable_size;
                auto utf8_size = diag::detail::GetValidSizeAsUtf8String(reinterpret_cast<const char*>(data_cur), tmp_chunk_size);
                if (utf8_size >= 0) {
                    tmp_chunk_size = static_cast<size_t>(utf8_size);
                }
                data_chunk_size = tmp_chunk_size;
            }
            else {
                data_chunk_size = cur_pushable_size;
            }

            /* Push chunk key and size as ULEB128. */
            this->PushUleb128(key);
            this->PushUleb128(data_chunk_size);
            
            /* Push the rest of the chunk data. */
            this->PushData(data_cur, data_chunk_size);
            
            data_cur += data_chunk_size;
        } while(data_cur < data_end);
        AMS_ABORT_UNLESS(data_cur == data_end);
    }

    size_t LogPacketTransmitterBase::PushUleb128(u64 value) {
        size_t count = 0;
        do {
            AMS_ASSERT(this->log_buffer_payload_current != this->log_buffer_end);
            u8 byte = value & 0x7F;
            value >>= 7;

            if (value != 0) {
                byte |= 0x80;
            }

            *this->log_buffer_payload_current = byte;
            this->log_buffer_payload_current += sizeof(byte);
            count++;
        } while(value);
        return count;
    }

    size_t LogPacketTransmitterBase::GetPushableDataSize(size_t sz, size_t required_size) {
        auto data_size = (this->log_buffer_end - this->log_buffer_payload_current) - sz;
        size_t a = 1;
        if (data_size >= 0x81) {
            size_t b = 0x7F;
            do {
                b |= b << 7;
                ++a;
            } while(data_size > (a + b));
        }
        auto pushable_size = data_size - a;
        AMS_ABORT_UNLESS(pushable_size >= required_size);
        return pushable_size;
    }

    void LogPacketTransmitterBase::PushData(const void *data, size_t data_size) {
        AMS_ABORT_UNLESS(data != nullptr);
        AMS_ABORT_UNLESS(data_size <= this->GetRemainSize());
        if (data_size == 0) {
            return;
        }

        /* Simply copy data. */
        std::memcpy(this->log_buffer_payload_current, data, data_size);
        this->log_buffer_payload_current += data_size;
    }

}