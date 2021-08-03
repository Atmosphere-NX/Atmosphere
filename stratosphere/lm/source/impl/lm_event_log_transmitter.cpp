#include "lm_event_log_transmitter.hpp"
#include "lm_log_buffer.hpp"

namespace ams::lm::impl {

    namespace {

        bool EventLogTransmitter_FlushFunction(const u8 *log_data, size_t log_size) {
            /* Simply log with flush through LogBuffer. */
            return GetLogBuffer()->Log(log_data, log_size, true);
        }

    }

    bool EventLogTransmitter::SendLogSessionBeginPacket(u64 process_id) {
        /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + LogSessionBegin dummy value (0x1) = 0x1B */
        constexpr auto log_buffer_size = sizeof(impl::LogPacketHeader) + 2                                  + sizeof(u8);
        u8 log_buffer[log_buffer_size] = {};
        impl::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_func, diag::LogSeverity_Info, false, process_id, true, true);
        log_packet_transmitter.PushLogSessionBegin();

        const auto ok = log_packet_transmitter.Flush(true);
        if (!ok) {
            this->IncrementLogPacketDropCount();
        }
        return ok;
    }

    bool EventLogTransmitter::SendLogSessionEndPacket(u64 process_id) {
        /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + LogSessionEnd dummy value (0x1) = 0x1B */
        constexpr auto log_buffer_size = sizeof(impl::LogPacketHeader) + 2                                  + sizeof(u8);
        u8 log_buffer[log_buffer_size] = {};
        impl::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_func, diag::LogSeverity_Info, false, process_id, true, true);
        log_packet_transmitter.PushLogSessionEnd();

        const auto ok = log_packet_transmitter.Flush(true);
        if (!ok) {
            this->IncrementLogPacketDropCount();
        }
        return ok;
    }

    bool EventLogTransmitter::SendLogPacketDropCountPacket() {
        std::scoped_lock lk(this->log_packet_drop_count_lock);

        /* Only send the log packet if there were any dropped packets. */
        if (this->log_packet_drop_count > 0) {
            /* ------------ Log buffer size: packet header (0x18)            + chunk header (0x2)                 + log packet drop count (0x8) = 0x22 */
            constexpr auto log_buffer_size = sizeof(impl::LogPacketHeader) + 2                                  + sizeof(this->log_packet_drop_count);
            u8 log_buffer[log_buffer_size] = {};
            impl::LogPacketTransmitter log_packet_transmitter(log_buffer, log_buffer_size, this->flush_func, diag::LogSeverity_Info, false, 0, true, true);
            log_packet_transmitter.PushLogPacketDropCount(this->log_packet_drop_count);

            const auto ok = log_packet_transmitter.Flush(true);
            if (ok) {
                this->log_packet_drop_count = 0;
            }
            else {
                this->log_packet_drop_count++;
            }
            return ok;
        }

        return true;
    }

    EventLogTransmitter *GetEventLogTransmitter() {
        static EventLogTransmitter event_log_transmitter(EventLogTransmitter_FlushFunction);
        return std::addressof(event_log_transmitter);
    }

}