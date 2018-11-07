#pragma once

#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

class KThread;
inline void IncrementThreadInterruptBottomHalfLockCount(KThread &thread);
inline void DecrementThreadInterruptBottomHalfLockCount(KThread &thread);

class KInterruptBottomHalfGuard final {
    public:
    KInterruptBottomHalfGuard()
    {
        KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        IncrementInterruptBottomHalfLockCount(*curThread);
    }

    ~KInterruptBottomHalfGuard()
    {
        KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        DecrementInterruptBottomHalfLockCount(*curThread);
    }

    KInterruptBottomHalfGuard(const KInterruptBottomHalfGuard &) = delete;
    KInterruptBottomHalfGuard(KInterruptBottomHalfGuard &&) = delete;
    KInterruptBottomHalfGuard &operator=(const KInterruptBottomHalfGuard &) = delete;
    KInterruptBottomHalfGuard &operator=(KInterruptBottomHalfGuard &&) = delete;
};

}