#include "lm_custom_sink_buffer.hpp"

namespace ams::lm::impl {

    namespace {

        u8 g_sink_buffer_log_buffer[0x8000];
        void *g_sink_buffer_log_data = nullptr;
        size_t g_sink_buffer_log_size = 0;
        u64 g_sink_buffer_log_packet_drop_count = 0;

        u64 g_sink_buffer_current_process_id = 0;
        u64 g_sink_buffer_current_thread_id = 0;
        size_t g_sink_buffer_remaining_size = 0;

        bool CustomSinkBuffer_PrepareLogData(u8 *log_data, size_t log_data_size) {
            /* Simply update the global values with the buffer and size. */
            g_sink_buffer_log_data = log_data;
            g_sink_buffer_log_size = log_data_size;
            return true;
        }

        bool WriteToSinkBufferImpl(const void *log_data, size_t size, u64 unk_param) {
            const auto ok = GetCustomSinkBuffer()->Log(log_data, size);
            if (!ok) {
                /* Logging failed, thus increase packet drop count. */
                g_sink_buffer_log_packet_drop_count++;
            }

            return ok;
        }

    }

    bool CustomSinkBuffer::HasLogData() {
        if (this->cur_offset == 0) {
            /* There is no log data. */
            return false;
        }

        /* Update log data with our values. */
        const auto ok = this->flush_func(this->log_data_buffer, this->cur_offset);
        if (!ok) {
            return false;
        }

        /* Reset the offset, ready to receive new log data. */
        this->cur_offset = 0;
        return true;
    }

    bool CustomSinkBuffer::Log(const void *log_data, size_t size) {
        if (size > 0) {
            if ((this->size - this->cur_offset) < size) {
                return false;
            }

            std::memcpy(this->log_data_buffer + this->cur_offset, log_data, size);
            this->cur_offset += size;
        }
        return true;
    }

    CustomSinkBuffer *GetCustomSinkBuffer() {
        static CustomSinkBuffer custom_sink_buffer(g_sink_buffer_log_buffer, sizeof(g_sink_buffer_log_buffer), CustomSinkBuffer_PrepareLogData);
        return std::addressof(custom_sink_buffer);
    }

    void WriteLogToCustomSink(const impl::LogPacketHeader *log_packet_header, size_t log_packet_size, u64 unk_param) {
        /* Process ID value is set earlier, so it's guaranteed to be valid. */
        if (log_packet_header->GetProcessId() != g_sink_buffer_current_process_id) {
            /* We're starting to log from a different process, so we need first a head packet. */
            if (!log_packet_header->IsHead()) {
                return;
            }
        }
        const auto logging_in_same_thread = log_packet_header->GetThreadId() == g_sink_buffer_current_thread_id;
        if (!logging_in_same_thread) {
            /* We're starting to log from a different thread, so we need first a head packet. */
            if (!log_packet_header->IsHead()) {
                return;
            }
        }

        auto log_payload_start = reinterpret_cast<const u8*>(log_packet_header) + sizeof(impl::LogPacketHeader);
        auto log_payload_cur = log_payload_start;

        /* No need to check packet header's payload size, since it has already been validated. */
        auto log_payload_size = log_packet_size - sizeof(impl::LogPacketHeader);
        if (logging_in_same_thread && (g_sink_buffer_remaining_size > 0)) {
            auto log_remaining_size = log_payload_size;
            if (log_payload_size >= g_sink_buffer_remaining_size) {
                log_remaining_size = g_sink_buffer_remaining_size;
            }
            if (GetCustomSinkBuffer()->ExpectsMorePackets()) {
                WriteToSinkBufferImpl(log_payload_cur, log_remaining_size, unk_param);
            }
            log_payload_cur += log_remaining_size;
            log_payload_size -= log_remaining_size;
            g_sink_buffer_remaining_size -= log_remaining_size;
        }

        size_t remaining_chunk_size = 0;
        auto contains_log_data = false;
        if (log_payload_size) {
            auto log_payload_end = log_payload_start + log_payload_size;
            while (true) {
                const auto chunk_key = static_cast<impl::LogDataChunkKey>(PopUleb128(std::addressof(log_payload_cur), log_payload_end));
                const auto chunk_is_text_log = chunk_key == impl::LogDataChunkKey_TextLog;
                const auto chunk_size = PopUleb128(std::addressof(log_payload_cur), log_payload_end);
                auto left_payload_size = static_cast<size_t>(log_payload_end - log_payload_cur);
                if (chunk_size >= left_payload_size) {
                    contains_log_data = chunk_is_text_log;
                    remaining_chunk_size = chunk_size - left_payload_size;
                }
                if (chunk_is_text_log) {
                    /* Log getter only retains TextLog data. */
                    auto actual_size = left_payload_size;
                    if (chunk_size < actual_size) {
                        actual_size = chunk_size;
                    }
                    WriteToSinkBufferImpl(log_payload_cur, actual_size, unk_param);
                }
                log_payload_cur += chunk_size;
                if (log_payload_cur >= log_payload_end) {
                    break;
                }
            }
        }

        if (!log_packet_header->IsTail()) {
            /* If more packets will come after this one, set current values. */
            g_sink_buffer_current_process_id = log_packet_header->GetProcessId();
            g_sink_buffer_current_thread_id = log_packet_header->GetThreadId();
            g_sink_buffer_remaining_size = remaining_chunk_size;
            GetCustomSinkBuffer()->SetExpectsMorePackets(contains_log_data);
        }
    }

    size_t ReadLogFromCustomSink(void *out_log_data, size_t size, u64 *out_packet_drop_count) {
        /* Copy log data from the global pointer and size values. */
        auto read_size = g_sink_buffer_log_size;
        if (g_sink_buffer_log_size > size) {
            /* Buffer can't hold all the log data, thus increase the drop count. */
            read_size = size;
            g_sink_buffer_log_packet_drop_count++;
        }

        /* Copy data. */
        std::memcpy(out_log_data, g_sink_buffer_log_data, read_size);

        /* Get the packet drop count value, and reset it. */
        *out_packet_drop_count = g_sink_buffer_log_packet_drop_count;
        g_sink_buffer_log_packet_drop_count = 0;
        return read_size;
    }

}