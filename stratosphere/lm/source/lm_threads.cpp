#include "lm_threads.hpp"
#include "impl/lm_log_server_proxy.hpp"

std::atomic_bool g_flag;

namespace ams::lm {

    namespace {

        void HtcsMain(void *log_server_proxy_v) {
            auto log_server_proxy = reinterpret_cast<impl::LogServerProxy*>(log_server_proxy_v);
            /* TODO: htcs support in libstratosphere

            htcs::SockAddrHtcs addr = {};
            addr.family = HTCS_AF_HTCS;
            strncpy(addr.host_name, sizeof(addr.host_name), htcs::GetDefaultHostName());
            strncpy(addr.peer_name, sizeof(addr.peer_name), "iywys@$LogManager");

            // Handle htcs until the finalize event signals
            do {
                auto server_fd = htcs::Socket();
                if(R_SUCCEEDED(htcs::Bind(server_fd, &addr)) && R_SUCCEEDED(htcs::Listen(server_fd, 0))) {
                    while(!os::TryWaitEvent(log_server_proxy->GetFinalizeEvent())) {
                        auto client_fd = htcs::Accept(server_fd, 0);
                        log_server_proxy->CallSomeFunction(true);

                        {
                            std::scoped_lock lk(log_server_proxy->GetSomeCondvarLock());
                            os::BroadcastConditionVariable(log_server_proxy->GetSomeCondvar());
                        }

                        u8 dummy_val;
                        while(htcs::Recv(client_fd, &dummy_val, sizeof(dummy_val)) == sizeof(dummy_val));

                        htcs::Close(client_fd);
                        log_server_proxy->CallSomeFunction(false);
                    }
                    htcs::Close(server_fd);
                }
            } while(!os::TimedWaitEvent(log_server_proxy->GetFinalizeEvent(), 1'000'000'000ul));
            <sub_710002B470>();

            */
            while(true) {}
        }

        os::ThreadType g_flush_thread;
        alignas(os::ThreadStackAlignment) u8 g_flush_thread_stack[8_KB];

        os::WaitableManagerType g_waitable_manager;
        os::SdkMutex g_waitable_manager_lock;
        bool g_flush_active = false;

        os::WaitableHolderType g_finalize_waitable_holder;
        os::Event g_finalize_event(os::EventClearMode_ManualClear);
        os::WaitableHolderType g_waitable_holder_1;
        os::Event g_event_1(os::EventClearMode_ManualClear);
        os::WaitableHolderType g_waitable_holder_2;
        os::SystemEvent g_event_2;
        os::WaitableHolderType g_waitable_holder_3;
        os::Event g_event_3(os::EventClearMode_ManualClear);
        
        u8 g_fs_heap_memory[0x8000];
        lmem::HeapHandle g_fs_heap_handle;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(g_fs_heap_handle, size);
        }

        void Deallocate(void *ptr, size_t size) {
            lmem::FreeToExpHeap(g_fs_heap_handle, ptr);
        }

        void SomeALoggerFunction(bool some_flag) {
            if (some_flag) {
                g_event_1.Signal();
            }
            else {
                g_event_1.Clear();
                if (!g_event_3.TryWait()) {
                    /*
                    impl::GetCLogger()-><some-inlined-fn>();
                    */
                }
            }
        }

        void SomeLogServerProxyFunction(bool some_flag) {
            if (some_flag) {
                g_event_3.Signal();
            }
            else {
                g_event_3.Clear();
                if (!g_event_1.TryWait()) {
                    /*
                    impl::GetCLogger()-><some-inlined-fn>();
                    */
                }
            }
        }

        void FlushMain(void *arg) {
            /* Initialize everything. */
            fs::SetEnabledAutoAbort(false);
            g_fs_heap_handle = lmem::CreateExpHeap(g_fs_heap_memory, sizeof(g_fs_heap_memory), lmem::CreateOption_None);
            AMS_ABORT_UNLESS(g_fs_heap_handle != nullptr);
            fs::SetAllocator(Allocate, Deallocate);
            time::Initialize();
            g_waitable_manager_lock.Lock();
            os::InitializeWaitableManager(std::addressof(g_waitable_manager));
            os::InitializeWaitableHolder(std::addressof(g_finalize_waitable_holder), g_finalize_event.GetBase());
            os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_finalize_waitable_holder));
            /* R_ABORT_UNLESS(nn::fs::OpenSdCardEventNotifier(...)); */
            impl::GetALogger()->SetSomeFunction(SomeALoggerFunction);
            os::InitializeWaitableHolder(std::addressof(g_waitable_holder_1), g_event_1.GetBase());
            os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_waitable_holder_1));
            os::InitializeWaitableHolder(std::addressof(g_waitable_holder_2), g_event_2.GetBase());
            os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_waitable_holder_2));
            impl::GetLogServerProxy()->SetSomeFunction(SomeLogServerProxyFunction);
            os::InitializeWaitableHolder(std::addressof(g_waitable_holder_3), g_event_3.GetBase());
            os::LinkWaitableHolder(std::addressof(g_waitable_manager), std::addressof(g_waitable_holder_3));
            g_flush_active = true;
            g_waitable_manager_lock.Unlock();

            /* Main loop. */
            auto is_finalizing = false;
            while(!is_finalizing) {
                /*
                if (impl::GetCLogger()->SomeMemberFn()) {
                    impl::GetBLogger()->SendLogPacketDropCountPacket();
                }
                */
                g_waitable_manager_lock.Lock();
                auto signaled_holder = os::WaitAny(std::addressof(g_waitable_manager));
                g_waitable_manager_lock.Unlock();

                if (signaled_holder == std::addressof(g_finalize_waitable_holder)) {
                    /* We're finalizing, thus break loop. */
                    is_finalizing = true;
                    break;
                }

                while(true) {
                    /* Wait until the holder we get is holder 1 or 3...? */
                    if ((signaled_holder == std::addressof(g_waitable_holder_1)) || (signaled_holder == std::addressof(g_waitable_holder_3))) {
                        break;
                    }

                    if (signaled_holder == std::addressof(g_waitable_holder_2)) {
                        g_event_2.Clear();
                        /*
                        if (fs::IsSdCardInserted()) {
                            break;
                        }
                        */
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
            g_waitable_manager_lock.Lock();
            g_flush_active = false;
            /* sub_7100022F30(std::addressof(g_waitable_manager)); */
            /* unknown_libname_142(std::addressof(g_waitable_manager)); */
            os::UnlinkWaitableHolder(std::addressof(g_finalize_waitable_holder));
            os::UnlinkWaitableHolder(std::addressof(g_waitable_holder_3));
            impl::GetLogServerProxy()->SetSomeFunction(nullptr);
            os::UnlinkWaitableHolder(std::addressof(g_waitable_holder_2));
            os::UnlinkWaitableHolder(std::addressof(g_waitable_holder_1));
            impl::GetALogger()->Dispose();
            impl::GetALogger()->SetSomeFunction(nullptr);
            time::Finalize();
            lmem::DestroyExpHeap(g_fs_heap_handle);
            g_waitable_manager_lock.Unlock();
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
        if (g_flag) {
            return false;
        }

        if (!g_waitable_manager_lock.TryLock()) {
            return false;
        }

        auto ret_value = false;
        if (g_flush_active) {
            auto signaled_holder = os::WaitAny(std::addressof(g_waitable_manager));
            ret_value = (signaled_holder != nullptr) && (signaled_holder != std::addressof(g_finalize_waitable_holder));
        }
        g_waitable_manager_lock.Unlock();
        return ret_value;
    }

}