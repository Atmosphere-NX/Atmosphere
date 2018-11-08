#include <mesosphere/threading/KCriticalSection.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

bool KCriticalSection::try_lock()
{
    KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
    if (curThread == lockingThread) {
        ++lockCount;
        return true;
    } else if (KInterruptSpinLock<false>::try_lock()) {
        lockingThread = curThread;
        lockCount = 1;
        return true;
    } else {
        return false;
    }
}

void KCriticalSection::lock()
{
    KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
    if (curThread == lockingThread) {
        ++lockCount;
    } else {
        KInterruptSpinLock<false>::lock();
        lockingThread = curThread;
        lockCount = 1;
    }
}

void KCriticalSection::unlock()
{
    if (--lockCount == 0) {
        lockingThread = nullptr;
        KScheduler::HandleCriticalSectionLeave();
    } else {
        KInterruptSpinLock<false>::unlock();
    }
}

}
