#include "lm_log_service.hpp"
#include "lm_threads.hpp"
#include "impl/lm_b_logger.hpp"
#include "impl/lm_c_logger.hpp"
#include "impl/lm_d_logger.hpp"

namespace ams::lm {

    /* Defined in DLogger's source code. */
    extern bool g_log_getter_logging_enabled;

    namespace {

        LogDestination g_log_destination = LogDestination_TMA;

        inline void SomeLog(const sf::InAutoSelectBuffer &log_buffer, const bool increment_drop_count) {
            if (increment_drop_count) {
                impl::GetBLogger()->IncrementPacketDropCount();
            }

            if (!(g_log_destination & 2)) {
                /* TODO: what's done here? */
            }

            if (g_log_getter_logging_enabled) {
                /* Log to lm:get. */
                impl::WriteLogToLogGetterDefault(log_buffer.GetPointer(), log_buffer.GetSize());
            }
        }

        u8 g_logger_heap_memory[0x4000];
        os::SdkMutex g_logger_heap_lock;

        lmem::HeapHandle GetLoggerHeapHandle() {
            static lmem::HeapHandle g_logger_heap_handle = lmem::CreateExpHeap(g_logger_heap_memory, sizeof(g_logger_heap_memory), lmem::CreateOption_None);
            return g_logger_heap_handle;
        }

    }

    Logger::Logger(os::ProcessId process_id) : process_id(process_id) {
        impl::GetBLogger()->SendLogSessionBeginPacket(static_cast<u64>(process_id));
    }

    Logger::~Logger() {
        impl::GetBLogger()->SendLogSessionEndPacket(static_cast<u64>(this->process_id));
    }

    void *Logger::operator new(size_t size) {
        std::scoped_lock lk(g_logger_heap_lock);
        return lmem::AllocateFromExpHeap(GetLoggerHeapHandle(), size);
    }

    void Logger::operator delete(void *ptr) {
        std::scoped_lock lk(g_logger_heap_lock);
        lmem::FreeToExpHeap(GetLoggerHeapHandle(), ptr);
    }

    void Logger::Log(const sf::InAutoSelectBuffer &log_buffer) {
        auto log_packet_header = (detail::LogPacketHeader*)log_buffer.GetPointer();

        /* Don't log anything if payload size isn't correct. */
        if ((log_packet_header->GetPayloadSize() + sizeof(detail::LogPacketHeader)) != log_buffer.GetSize()) {
            return;
        }
        
        /* Set process ID. */
        log_packet_header->SetProcessId(static_cast<u64>(this->process_id));

        bool do_increment_drop_count = false;
        if (g_log_destination & 1) {
            if (ShouldLogWithFlush()) {
                do_increment_drop_count = !impl::GetCLogger()->Log(log_buffer.GetPointer(), log_buffer.GetSize());
            }
            else {
                /* TODO: ? */
                do_increment_drop_count = !impl::GetCLogger()->Log(log_buffer.GetPointer(), log_buffer.GetSize());
            }
        }
        
        SomeLog(log_buffer, do_increment_drop_count);
    }

    void Logger::SetDestination(LogDestination log_destination) {
        /* Set global destination value. */
        g_log_destination = log_destination;
    }

    void LogService::OpenLogger(const sf::ClientProcessId &client_pid, sf::Out<std::shared_ptr<impl::ILogger>> out_logger) {
        /* Simply create a logger object, which will be allocated/freed with the exp heap created above. */
        out_logger.SetValue(sf::MakeShared<impl::ILogger, Logger>(client_pid.process_id));
    }

}