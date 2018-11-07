#pragma once

#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/arch/KInterruptMaskGuard.hpp>
#include <mesosphere/arch/KSpinLock.hpp>

namespace mesosphere
{

class KThread;
inline void IncrementThreadInterruptBottomHalfLockCount(KThread &thread);
inline void DecrementThreadInterruptBottomHalfLockCount(KThread &thread);

template<bool disableInterrupts = false>
class KInterruptSpinLock final : public KSpinLock {
    public:

    bool try_lock()
    {
        KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        IncrementThreadInterruptBottomHalfLockCount(*curThread);
        if (!KSpinLock::try_lock()) {
            DecrementThreadInterruptBottomHalfLockCount(*curThread);
            return false;
        }
        return true;
    }

    void lock()
    {
        KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        IncrementThreadInterruptBottomHalfLockCount(*curThread);
        KSpinLock::lock();
    }

    void unlock()
    {
        KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        KSpinLock::unlock();
        DecrementThreadInterruptBottomHalfLockCount(*curThread);
    }

    KInterruptSpinLock() = default;
    KInterruptSpinLock(const KInterruptSpinLock &) = delete;
    KInterruptSpinLock(KInterruptSpinLock &&) = delete;
    KInterruptSpinLock &operator=(const KInterruptSpinLock &) = delete;
    KInterruptSpinLock &operator=(KInterruptSpinLock &&) = delete;
};

template<>
class KInterruptSpinLock<true> final : public KSpinLock {
    public:

    bool try_lock()
    {
        flags = KInterruptMaskGuard::MaskInterrupts();
        if (!KSpinLock::try_lock()) {
            KInterruptMaskGuard::RestoreInterrupts(flags);
            return false;
        }
        return true;
    }

    void lock()
    {
        flags = KInterruptMaskGuard::MaskInterrupts();
        KSpinLock::lock();
    }

    void unlock()
    {
        KSpinLock::unlock();
        KInterruptMaskGuard::RestoreInterrupts(flags);
    }

    KInterruptSpinLock() = default;
    KInterruptSpinLock(const KInterruptSpinLock &) = delete;
    KInterruptSpinLock(KInterruptSpinLock &&) = delete;
    KInterruptSpinLock &operator=(const KInterruptSpinLock &) = delete;
    KInterruptSpinLock &operator=(KInterruptSpinLock &&) = delete;

    private:
    typename KInterruptMaskGuard::FlagsType flags;
};

}
