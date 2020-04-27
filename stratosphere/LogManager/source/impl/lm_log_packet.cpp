#include "lm_log_packet.hpp"
#include "lm_scoped_log_file.hpp"

namespace ams::lm::impl {

    namespace {

        bool can_access_fs = true;
        os::Mutex fs_access_lock(false);

    }

    void SetCanAccessFs(bool can_access) {
        std::scoped_lock lk(fs_access_lock);
        can_access_fs = can_access;
    }

    void WriteLogPackets(const char *log_path, std::vector<LogPacket> &packet_list, u64 program_id, LogDestination destination) {
        std::scoped_lock lk(fs_access_lock);
        if(!can_access_fs) {
            /* Only write log if we can access fs. */
            return;
        }
        if(packet_list.empty()) {
            /* This shouldn't happen... */
            return;
        }

        /* For everything except the text log, use the first/head packet. */
        auto &head_packet = packet_list.front();
        const bool is_single_packet = packet_list.size() == 1;
        ScopedLogFile log_file(log_path);
        log_file.WriteFormat("\n");
        log_file.WriteFormat("/----------------------------------------------------------------------------------------------------\\\n");
        log_file.WriteFormat("|\n");
        log_file.WriteFormat("|    Log - %s (0x%016lX)\n", is_single_packet ? "single packet" : "multiple packets", program_id);

        /* Log severity. */
        log_file.WriteFormat("|    Log severity (Trace, Info, Warn, Error, Fatal): ");
        switch(static_cast<LogSeverity>(head_packet.header.severity)) {
            case LogSeverity::Trace:
                log_file.WriteFormat("Trace");
                break;
            case LogSeverity::Info:
                log_file.WriteFormat("Info");
                break;
            case LogSeverity::Warn:
                log_file.WriteFormat("Warn");
                break;
            case LogSeverity::Error:
                log_file.WriteFormat("Error");
                break;
            case LogSeverity::Fatal:
                log_file.WriteFormat("Fatal");
                break;
            default:
                log_file.WriteFormat("Unknown");
                break;
        }
        log_file.WriteFormat("\n");

        /* Log verbosity. */
        log_file.WriteFormat("|    Log verbosity: %d\n", head_packet.header.verbosity);

        /* Log destination. */
        log_file.WriteFormat("|    Log destination: ");
        switch(destination) {
            case LogDestination::TMA:
                log_file.WriteFormat("TMA");
                break;
            case LogDestination::UART:
                log_file.WriteFormat("UART");
                break;
            case LogDestination::UARTSleeping:
                log_file.WriteFormat("UART (when sleeping)");
                break;
            case LogDestination::All:
                log_file.WriteFormat("All (TMA and UART)");
                break;
            default:
                log_file.WriteFormat("Unknown");
                break;
        }
        log_file.WriteFormat("\n");
        log_file.WriteFormat("|\n");

        /* Process ID and thread ID. */
        log_file.WriteFormat("|    Process ID: 0x%016lX\n", head_packet.header.process_id);
        log_file.WriteFormat("|    Thread ID: 0x%016lX\n", head_packet.header.thread_id);

        /* File name, line number, function name. */
        if(!head_packet.payload.file_name.IsEmpty()) {
            /* At <file-name> */
            log_file.WriteFormat("|    At %s", head_packet.payload.file_name.value);
            if(!head_packet.payload.function_name.IsEmpty()) {
                /* If function name is present, line number is present too. */
                /* At <file-name>:<line-number> (<function-name>) */
                log_file.WriteFormat(":%d (%s)", head_packet.payload.line_number.value, head_packet.payload.function_name.value);
            }
            log_file.WriteFormat("\n");
        }
        if(!head_packet.payload.process_name.IsEmpty()) {
            log_file.WriteFormat("|    Process name: %s\n", head_packet.payload.process_name.value);
        }
        if(!head_packet.payload.module_name.IsEmpty()) {
            log_file.WriteFormat("|    Module name: %s\n", head_packet.payload.module_name.value);
        }
        if(!head_packet.payload.thread_name.IsEmpty()) {
            log_file.WriteFormat("|    Thread name: %s\n", head_packet.payload.thread_name.value);
        }

        if(!head_packet.payload.log_packet_drop_count.IsEmpty()) {
            /* Log packet drop count (what is this used for...?) */
            log_file.WriteFormat("|    Log packet drop count: %ld\n", head_packet.payload.log_packet_drop_count.value);
        }

        if(!head_packet.payload.user_system_clock.IsEmpty()) {
            /* User system clock - seconds since the title has started. */
            log_file.WriteFormat("|    Time since title started: %lds\n", head_packet.payload.user_system_clock.value);
        }

        log_file.WriteFormat("|\n");
        log_file.WriteFormat("\\----------------------------------------------------------------------------------------------------/\n");

        if(!head_packet.payload.text_log.IsEmpty()) {
            /* Concatenate all the packets' messages. */
            for(auto &packet: packet_list) {
                log_file.WriteFormat("%s", packet.payload.text_log.value);
            }
            log_file.WriteFormat("\n");
        }
    }
}