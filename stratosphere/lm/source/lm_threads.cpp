#include "lm_threads.hpp"
#include "impl/lm_log_buffer.hpp"
#include "impl/lm_event_log_transmitter.hpp"
#include "impl/lm_log_server_proxy.hpp"

std::atomic_bool g_disabled_flush = false;

namespace ams::lm {

    namespace {

        /* TODO: htcs support. */
        /*
        constexpr u32 HtcsSessionCount = 2;
        constexpr const char HtcsPeerName[] = "iywys@$LogManager";
        u8 g_htcs_work_memory[0x800];
        */

        void HtcsMain(void *log_server_proxy_v) {
            auto log_server_proxy = reinterpret_cast<impl::LogServerProxy*>(log_server_proxy_v);

            /* TODO: htcs support. */
            /*
            auto work_mem_size = htcs::GetWorkingMemorySize(HtcsSessionCount);
            AMS_ABORT_UNLESS(work_mem_size <= sizeof(g_htcs_work_memory));
            R_ABORT_UNLESS(htcs::Initialize(g_htcs_work_memory, work_mem_size, HtcsSessionCount));
            
            htcs::SockAddrHtcs addr = {};
            addr.family = HTCS_AF_HTCS;
            strncpy(addr.host_name, sizeof(addr.host_name), htcs::GetDefaultHostName());
            strncpy(addr.peer_name, sizeof(addr.peer_name), HtcsPeerName);

            // Handle htcs until the finalize event signals
            do {
                auto server_fd = htcs::Socket();
                log_server_proxy->SetHtcsServerFd(server_fd);
                if (R_SUCCEEDED(htcs::Bind(server_fd, std::addressof(addr))) && R_SUCCEEDED(htcs::Listen(server_fd, 0))) {
                    while (!os::TryWaitEvent(log_server_proxy->GetFinalizeEvent())) {
                        auto client_fd = htcs::Accept(server_fd, 0);
                        log_server_proxy->SetHtcsClientFd(client_fd);
                        log_server_proxy->SetEnabled(true);

                        {
                            std::scoped_lock lk(log_server_proxy->GetSomeCondvarLock());
                            os::BroadcastConditionVariable(log_server_proxy->GetSomeCondvar());
                        }

                        u8 dummy_val;
                        while(htcs::Recv(client_fd, std::addressof(dummy_val), sizeof(dummy_val)) == sizeof(dummy_val));

                        htcs::Close(client_fd);
                        log_server_proxy->SetEnabled(false);
                    }
                    htcs::Close(server_fd);
                }
            } while (!os::TimedWaitEvent(log_server_proxy->GetFinalizeEvent(), 1'000'000'000ul));
            <sub_710002B470>();
            */

            /* Placeholder loop while htcs is not implemented. */
            while (!log_server_proxy->GetFinalizeEvent().TryWait()) {
                log_server_proxy->SetEnabled(true);

                log_server_proxy->DoHtcsLoopThing();
                while (true) {
                    /* Htcs implementation would continously read byte by byte, and end this loop when read failed, re-opening the client socket and starting again. */
                    os::SleepThread(TimeSpan::FromMilliSeconds(100));
                }

                log_server_proxy->SetEnabled(false);
            }
        }

        os::ThreadType g_flush_thread;
        alignas(os::ThreadStackAlignment) u8 g_flush_thread_stack[8_KB];

        os::WaitableManagerType g_waitable_manager;
        os::SdkMutex g_waitable_manager_lock;
        bool g_flush_thread_active = false;

        os::WaitableHolderType g_finalize_waitable_holder;
        os::Event g_finalize_event(os::EventClearMode_ManualClear);
        os::WaitableHolderType g_sd_card_logging_enabled_waitable_holder;
        os::Event g_sd_card_logging_enabled_event(os::EventClearMode_ManualClear);
        
        os::WaitableHolderType g_sd_card_detection_waitable_holder;
        std::unique_ptr<fs::IEventNotifier> g_sd_card_detection_event_notifier;
        os::SystemEvent g_sd_card_detection_event;
        
        os::WaitableHolderType g_log_server_proxy_enabled_waitable_holder;
        os::Event g_log_server_proxy_enabled_event(os::EventClearMode_ManualClear);
        
        u8 g_fs_heap_memory[0x8000];
        lmem::HeapHandle g_fs_heap_handle;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
        }

        void Deallocate(void *ptr, size_t size) {
            lmem::FreeToExpHeap(g_fs_heap_handle, ptr);
        }

        void SdCardLogging_UpdateEnabledFunction(bool enabled) {
            if (enabled) {
                /* Tell the flush thread we're enabled, so it can continue flushing. */
                g_sd_card_logging_enabled_event.Signal();
            }
            else {
                /* ? */
                g_sd_card_logging_enabled_event.Clear();
                if (!g_log_server_proxy_enabled_event.TryWait()) {
                    impl::GetLogBuffer()->SomeInlinedFn();
                }
            }
        }

        void LogServerProxy_UpdateEnabledFunction(bool enabled) {
            if (enabled) {
                /* Tell the flush thread we're enabled, so it can continue flushing. */
                g_log_server_proxy_enabled_event.Signal();
            }
            else {
                /* ? */
                g_log_server_proxy_enabled_event.Clear();
                if (!g_sd_card_logging_enabled_event.TryWait()) {
                    impl::GetLogBuffer()->SomeInlinedFn();
                }
            }
        }

        void FlushMain(void *arg) {
            /* Initialize everything. */
            fs::SetEnabledAutoAbort(false);
            g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_None);
            AMS_ABORT_UNLESS(g_fs_heap_handle != nullptr);
            fs::SetAllocator(Allocate, Deallocate);

            {
                std::scoped_lock lk(g_waitable_manager_lock);

                os::InitializeWaitableManager(std::addressof(g_waitable_manager));

                os::InitializeWaitableHolder(std::addressof(g_finalize_waitable_holder), g_finalize_event.GetBase());
                os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_finalize_waitable_holder));
                
                impl::GetSdCardLogging()->SetUpdateEnabledFunction(SdCardLogging_UpdateEnabledFunction);
                os::InitializeWaitableHolder(std::addressof(g_sd_card_logging_enabled_waitable_holder), g_sd_card_logging_enabled_event.GetBase());
                os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_sd_card_logging_enabled_waitable_holder));
                
                R_ABORT_UNLESS(fs::OpenSdCardDetectionEventNotifier(std::addressof(g_sd_card_detection_event_notifier)));
                R_ABORT_UNLESS(g_sd_card_detection_event_notifier->BindEvent(g_sd_card_detection_event.GetBase(), os::EventClearMode_ManualClear));
                os::InitializeWaitableHolder(std::addressof(g_sd_card_detection_waitable_holder), g_sd_card_detection_event.GetBase());
                os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_sd_card_detection_waitable_holder));

                impl::GetLogServerProxy()->SetUpdateEnabledFunction(LogServerProxy_UpdateEnabledFunction);
                os::InitializeWaitableHolder(std::addressof(g_log_server_proxy_enabled_waitable_holder), g_log_server_proxy_enabled_event.GetBase());
                os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_log_server_proxy_enabled_waitable_holder));
                g_flush_thread_active = true;
            }

            /* Main loop. */
            auto is_finalizing = false;
            while (!is_finalizing) {
                /* Flush, and send LogPacketDropCount special packet if success. */
                if (impl::GetLogBuffer()->Flush()) {
                    impl::GetEventLogTransmitter()->SendLogPacketDropCountPacket();
                }
                
                g_waitable_manager_lock.Lock();
                auto *signaled_holder = os::WaitAny(std::addressof(g_waitable_manager));
                g_waitable_manager_lock.Unlock();

                if (signaled_holder == std::addressof(g_finalize_waitable_holder)) {
                    /* We're finalizing, thus break loop. */
                    break;
                }

                while (true) {
                    /* Wait until SdCardLogging or LogServerProxy signal, then return to the main loop to flush again. */
                    if ((signaled_holder == std::addressof(g_sd_card_logging_enabled_waitable_holder)) || (signaled_holder == std::addressof(g_log_server_proxy_enabled_waitable_holder))) {
                        break;
                    }

                    if (signaled_holder == std::addressof(g_sd_card_detection_waitable_holder)) {
                        g_sd_card_detection_event.Clear();
                        if (fs::IsSdCardInserted()) {
                            break;
                        }
                    }

                    g_waitable_manager_lock.Lock();
                    signaled_holder = os::WaitAny(std::addressof(g_waitable_manager));
                    g_waitable_manager_lock.Unlock();

                    if (signaled_holder == std::addressof(g_finalize_waitable_holder)) {
                        /* We're finalizing, thus break loop. */
                        is_finalizing = true;
                        break;
                    }
                }
            }

            /* Finalize everything. */
            {
                std::scoped_lock lk(g_waitable_manager_lock);

                g_flush_thread_active = false;

                /* What functions are these? */
                /* sub_7100022F30(std::addressof(g_waitable_manager)); */
                /* unknown_libname_142(std::addressof(g_waitable_manager)); */

                os::UnlinkWaitableHolder(std::addressof(g_finalize_waitable_holder));

                os::UnlinkWaitableHolder(std::addressof(g_log_server_proxy_enabled_waitable_holder));
                impl::GetLogServerProxy()->SetUpdateEnabledFunction(nullptr);

                os::UnlinkWaitableHolder(std::addressof(g_sd_card_detection_waitable_holder));

                os::UnlinkWaitableHolder(std::addressof(g_sd_card_logging_enabled_waitable_holder));
                impl::GetSdCardLogging()->Dispose();
                impl::GetSdCardLogging()->SetUpdateEnabledFunction(nullptr);

                time::Finalize();
                fsExit();
                lmem::DestroyExpHeap(g_fs_heap_handle);
            }
        }

    }

    void StartHtcsThread() {
        impl::GetLogServerProxy()->StartHtcsThread(HtcsMain);
    }

    void DisposeHtcsThread() {
        impl::GetLogServerProxy()->DisposeHtcsThread();
    }

    void StartFlushThread() {
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_flush_thread), FlushMain, nullptr, g_flush_thread_stack, sizeof(g_flush_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, Flush)));
        os::SetThreadNamePointer(std::addressof(g_flush_thread), AMS_GET_SYSTEM_THREAD_NAME(lm, Flush));
        os::StartThread(std::addressof(g_flush_thread));
    }

    void DisposeFlushThread() {
        g_finalize_event.Signal();
        os::WaitThread(std::addressof(g_flush_thread));
        os::DestroyThread(std::addressof(g_flush_thread));
    }

    bool ShouldLogWithFlush() {
        if (g_disabled_flush) {
            /* Flushing is disabled. (system might be starting sleep, etc.) */
            return false;
        }

        if (!g_waitable_manager_lock.TryLock()) {
            /* The flushing thread is currently locked. */
            return false;
        }

        auto ret_value = false;
        if (g_flush_thread_active) {
            /* If anything but the finalize holder signals, we should be able to flush correctly. */
            auto *signaled_holder = os::WaitAny(std::addressof(g_waitable_manager));
            ret_value = (signaled_holder != nullptr) && (signaled_holder != std::addressof(g_finalize_waitable_holder));
        }
        g_waitable_manager_lock.Unlock();
        return ret_value;
    }

}