#pragma once
#include <switch.h>
#include <vector>

#include "waitablemanager.hpp"
#include "systemevent.hpp"

class MultiThreadedWaitableManager : public WaitableManager {
    protected:
        u32 num_threads;
        Thread *threads;
        HosMutex get_waitable_lock;
        SystemEvent *new_waitable_event;
    public:
        MultiThreadedWaitableManager(u32 n, u64 t, u32 ss = 0x8000) : WaitableManager(t), num_threads(n-1) { 
            u32 prio;
            u32 cpuid = svcGetCurrentProcessorNumber();
            Result rc;
            threads = new Thread[num_threads];
            if (R_FAILED((rc = svcGetThreadPriority(&prio, CUR_THREAD_HANDLE)))) {
                fatalSimple(rc);
            }
            for (unsigned int i = 0; i < num_threads; i++) {
                threads[i] = {0};
                threadCreate(&threads[i], &MultiThreadedWaitableManager::thread_func, this, ss, prio, cpuid);
            }
            new_waitable_event = new SystemEvent(this, &MultiThreadedWaitableManager::add_waitable_callback);
            this->waitables.push_back(new_waitable_event);
        }
        ~MultiThreadedWaitableManager() override {
            /* TODO: Exit the threads? */
        }
        
        IWaitable *get_waitable();
        
        void add_waitable(IWaitable *waitable) override;
        void process() override;
        void process_until_timeout() override;
        
        static Result add_waitable_callback(void *this_ptr, Handle *handles, size_t num_handles, u64 timeout);
        static void thread_func(void *this_ptr);
};