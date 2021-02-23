#include "lm_log_getter.hpp"
#include "impl/lm_custom_sink_buffer.hpp"

namespace ams::lm {

    /* Note: this global flag isn't accessed by lm:get, it'll be read by the Log() command from lm service. */
    bool g_is_logging_to_custom_sink = false;

    void LogGetter::StartLogging() {
        /* Set global flag. */
        g_is_logging_to_custom_sink = true;
    }

    void LogGetter::StopLogging() {
        /* Set global flag. */
        g_is_logging_to_custom_sink = false;
    }

    void LogGetter::GetLog(const sf::OutAutoSelectBuffer &out_log_buffer, sf::Out<size_t> out_size, sf::Out<u64> out_log_packet_drop_count) {
        u64 log_packet_drop_count = 0;
        size_t read_size = 0;

        /* Check if we have any log data in the custom sink. */
        if (impl::GetCustomSinkBuffer()->HasLogData()) {
            /* Read log data. */
            read_size = impl::ReadLogFromCustomSink(out_log_buffer.GetPointer(), out_log_buffer.GetSize(), std::addressof(log_packet_drop_count));
        }
        
        /* This value will contain the amount of packets missed before reading this one */
        out_log_packet_drop_count.SetValue(log_packet_drop_count);
        out_size.SetValue(read_size);
    }

}