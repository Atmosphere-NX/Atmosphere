#pragma once

#include <mesosphere/interrupts/KInterruptSpinLock.hpp>

namespace mesosphere
{

class KCriticalSection final : public KInterruptSpinLock<false> {
    public:

    bool try_lock();
    void lock();
    void unlock();

    KCriticalSection() = default;
    KCriticalSection(const KCriticalSection &) = delete;
    KCriticalSection(KCriticalSection &&) = delete;
    KCriticalSection &operator=(const KCriticalSection &) = delete;
    KCriticalSection &operator=(KCriticalSection &&) = delete;

    private:
    KThread *lockingThread = nullptr;
    ulong lockCount = 0;
};

}
