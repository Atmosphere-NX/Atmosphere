#include "lm_log_service.hpp"
#include "lm_threads.hpp"
#include "impl/lm_event_log_transmitter.hpp"
#include "impl/lm_log_buffer.hpp"
#include "impl/lm_custom_sink_buffer.hpp"

namespace ams::lm {

    /* Defined in CustomSinkBuffer's source code. */
    extern bool g_is_logging_to_custom_sink;

    namespace {

        LogDestination g_log_destination = LogDestination_TargetManager;

        u8 g_logger_heap_memory[0x4000];
        os::SdkMutex g_logger_heap_lock;

        sf::ExpHeapAllocator GetLoggerAllocator() {
            static auto logger_heap_handle = lmem::CreateExpHeap(g_logger_heap_memory, sizeof(g_logger_heap_memory), lmem::CreateOption_None);
            sf::ExpHeapAllocator logger_allocator;
            logger_allocator.Attach(logger_heap_handle);
            return logger_allocator;
        }

        inline sf::SharedPointer<impl::ILogger> CreateLogger(os::ProcessId process_id) {
            static auto logger_allocator = GetLoggerAllocator();
            return sf::ObjectFactory<sf::ExpHeapAllocator::Policy>::CreateSharedEmplaced<impl::ILogger, Logger>(std::addressof(logger_allocator), process_id);
        }

    }

    Logger::Logger(os::ProcessId process_id) : process_id(process_id) {
        impl::GetEventLogTransmitter()->SendLogSessionBeginPacket(static_cast<u64>(process_id));
    }

    Logger::~Logger() {
        impl::GetEventLogTransmitter()->SendLogSessionEndPacket(static_cast<u64>(this->process_id));
    }

    void Logger::Log(const sf::InAutoSelectBuffer &log_buffer) {
        auto log_packet_header = (impl::LogPacketHeader*)log_buffer.GetPointer();

        /* Don't log anything if payload size isn't correct. */
        if ((log_packet_header->GetPayloadSize() + sizeof(impl::LogPacketHeader)) != log_buffer.GetSize()) {
            return;
        }
        
        /* Set process ID. */
        log_packet_header->SetProcessId(static_cast<u64>(this->process_id));

        if (g_log_destination & LogDestination_TargetManager) {
            /* Log through normal means, which also implies logging to SD card if enabled. */
            if (!impl::GetLogBuffer()->Log(log_buffer.GetPointer(), log_buffer.GetSize(), ShouldLogWithFlush())) {
                /* If logging through LogBuffer failed, increment packet drop count. */
                impl::GetEventLogTransmitter()->IncrementLogPacketDropCount();
            }
        }

        if (!(g_log_destination & LogDestination_Uart)) {
            /* TODO: what's done here? */
        }

        if (g_is_logging_to_custom_sink) {
            /* Log to custom sink if enabled. */
            impl::WriteLogToCustomSink(const_cast<const impl::LogPacketHeader*>(log_packet_header), log_buffer.GetSize(), 0);
        }
    }

    void Logger::SetDestination(LogDestination log_destination) {
        /* Set global destination value. */
        g_log_destination = log_destination;
    }

    void LogService::OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<sf::SharedPointer<impl::ILogger>> out_logger) {
        /* Simply create a logger object, which will be allocated/freed with the exp heap created above. */
        out_logger.SetValue(CreateLogger(client_pid.process_id));
    }

}