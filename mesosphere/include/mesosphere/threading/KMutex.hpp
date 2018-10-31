#pragma once

#include <chrono>
#include <atomic>
#include <mutex>

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

/// Fulfills Mutex requirements
class KMutex final {
    public:

    using native_handle_type = uiptr;

    KMutex() = default;
    KMutex(const KMutex &) = delete;
    KMutex(KMutex &&) = delete;
    KMutex &operator=(const KMutex &) = delete;
    KMutex &operator=(KMutex &&) = delete;

    native_handle_type native_handle() { return tag; }

    bool try_lock()
    {
        KThread *currentThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        return try_lock_impl_get_owner(currentThread) == nullptr;
    }

    void lock()
    {
        KThread *currentThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        KThread *owner;

        while ((owner = try_lock_impl_get_owner(currentThread)) != nullptr) {
            // Our thread may be resumed even if we weren't given the mutex
            lock_slow_path(*owner, *currentThread);
        }
    }

    void unlock()
    {
        // Ensure sequencial ordering, to happen-after mutex load
        std::atomic_thread_fence(std::memory_order_seq_cst);

        KThread *currentThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        native_handle_type thisThread = (native_handle_type)currentThread;

        /*
            If we don't have any waiter, just store 0 (free the mutex).
            Otherwise, or if a race condition happens and a new waiter appears,
            take the slow path.
        */
        if (tag.load() != thisThread || !tag.compare_exchange_strong(thisThread, 0)) {
            unlock_slow_path(*currentThread);
        }
    }

    private:

    KThread *try_lock_impl_get_owner(KThread *currentThread)
    {
        native_handle_type oldTag, newTag;
        native_handle_type thisThread = (native_handle_type)currentThread;

        oldTag = tag.load();
        do {
            // Add "has listener" if the mutex was not free
            newTag = oldTag == 0 ? thisThread : (oldTag | 1);
        } while (!tag.compare_exchange_weak(oldTag, newTag, std::memory_order_seq_cst));

        // The mutex was not free or was not ours => return false
        if(oldTag != 0 && (oldTag & ~1) != thisThread) {
            return (KThread *)(oldTag &~ 1);
        } else {
            /*
                Ensure sequencial ordering if mutex was acquired
                and mutex lock happens-before mutex unlock.
            */
            std::atomic_thread_fence(std::memory_order_seq_cst);
            return nullptr;
        }
    }

    void lock_slow_path(KThread &owner, KThread &requester);
    void unlock_slow_path(KThread &owner);
    std::atomic<native_handle_type> tag{};
};

}
