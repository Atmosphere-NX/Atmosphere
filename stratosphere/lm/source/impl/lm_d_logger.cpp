#include "lm_d_logger.hpp"

namespace ams::lm::impl {

    u8 g_log_getter_log_buffer[0x8000];
    void *g_log_getter_log_data = nullptr;
    size_t g_log_getter_log_size = 0;
    u64 g_log_getter_packet_drop_count = 0;

    namespace {

        bool OnLog(void *data, size_t size) {
            /*  */
            g_log_getter_log_data = data;
            g_log_getter_log_size = size;
            return true;
        }

    }

    bool DLogger::HasLogData() {
        if (this->cur_offset == 0) {
            /* If we have log data to be collected, offset cannot be zero. */
            return false;
        }

        /* Update log pointer and size with our values. */
        const auto ok = (this->some_fn)(this->log_data_buffer, this->cur_offset);
        if (!ok) {
            /* Failed to update them. */
            return false;
        }

        /* Reset the offset, ready to receive new data later. */
        this->cur_offset = 0;
        return true;
    }

    bool DLogger::Log(void *log_data, size_t size) {
        if (size > 0) {
            if ((this->size - this->cur_offset) < size) {
                return false;
            }

            std::memcpy(reinterpret_cast<u8*>(this->log_data_buffer) + this->cur_offset, log_data, size);
            this->cur_offset += size;
        }
        return true;
    }

    DLogger *GetDLogger() {
        static DLogger d_logger(g_log_getter_log_buffer, sizeof(g_log_getter_log_buffer), OnLog);
        return &d_logger;
    }

    bool DLoggerLogGetterWriteFunction(void *log_data, size_t size, u64 unk_param) {
        const auto ok = GetDLogger()->Log(log_data, size);
        if (!ok) {
            /* Logging failed, thus increase packet drop count. */
            g_log_getter_packet_drop_count++;
        }

        return true;
    }

    void WriteLogToLogGetter(const void *log_data, size_t size, LogGetterWriteFunction write_fn, u64 unk_param) {
        // TODO
    }

    size_t ReadLogFromLogGetter(void *out_log_data, size_t size, u64 *out_packet_drop_count) {
        /* Copy log data from the global pointer and size values. */
        auto read_size = g_log_getter_log_size;
        if (g_log_getter_log_size > size) {
            /* Buffer can't hold all the packet, thus increase the drop count. */
            read_size = size;
            g_log_getter_packet_drop_count++;
        }

        /* Copy data. */
        std::memcpy(out_log_data, g_log_getter_log_data, read_size);

        /* Get the packet drop count value, and reset it. */
        *out_packet_drop_count = g_log_getter_packet_drop_count;
        g_log_getter_packet_drop_count = 0;
        return read_size;
    }

}