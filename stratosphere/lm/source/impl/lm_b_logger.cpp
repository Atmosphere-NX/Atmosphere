#include "lm_b_logger.hpp"

namespace ams::lm::impl {

    namespace {

        bool BLoggerFlushFunction(void *log_data, size_t size) {
            /* GetCLogger()->Log(log_data, size); */
            return true;
        }

    }

    bool BLogger::SendLogSessionBeginPacket(u64 process_id) {
        /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + LogSessionBegin dummy value (0x1) = 0x1B */
        constexpr auto log_buffer_size = sizeof(detail::LogPacketHeader) + sizeof(detail::LogDataChunkHeader) + sizeof(u8);
        u8 log_buffer[log_buffer_size] = {};
        detail::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_fn, detail::LogSeverity_Info, false, process_id, true, true);

        log_packet_transmitter.PushLogSessionBegin();
        const auto ok = log_packet_transmitter.Flush(true);
        if (!ok) {
            this->IncrementPacketDropCount();
        }

        return ok;
    }

    bool BLogger::SendLogSessionEndPacket(u64 process_id) {
        /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + LogSessionEnd dummy value (0x1) = 0x1B */
        constexpr auto log_buffer_size = sizeof(detail::LogPacketHeader) + sizeof(detail::LogDataChunkHeader) + sizeof(u8);
        u8 log_buffer[log_buffer_size] = {};
        detail::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_fn, detail::LogSeverity_Info, false, process_id, true, true);

        log_packet_transmitter.PushLogSessionEnd();
        const auto ok = log_packet_transmitter.Flush(true);
        if (!ok) {
            this->IncrementPacketDropCount();
        }

        return ok;
    }

    bool BLogger::SendLogPacketDropCountPacket() {
        std::scoped_lock lk(this->packet_drop_count_lock);

        /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + log packet drop count (0x8) = 0x22 */
        constexpr auto log_buffer_size = sizeof(detail::LogPacketHeader) + sizeof(detail::LogDataChunkHeader) + sizeof(this->packet_drop_count);
        u8 log_buffer[log_buffer_size] = {};
        detail::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_fn, detail::LogSeverity_Info, false, 0, true, true);

        log_packet_transmitter.PushLogPacketDropCount(this->packet_drop_count);
        const auto ok = log_packet_transmitter.Flush(true);
        if (!ok) {
            this->IncrementPacketDropCount();
        }

        return ok;
    }

    BLogger *GetBLogger() {
        static BLogger b_logger(BLoggerFlushFunction);
        return &b_logger;
    }

}