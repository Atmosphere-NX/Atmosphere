#include <mesosphere/threading/KConditionVariable.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

void KConditionVariable::wait_until_impl(const KSystemClock::time_point &timeoutPoint) noexcept
{
    // Official kernel counts number of waiters, but that isn't necessary
    bool hasWaited = false;
    KThread *currentThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
    {
        KScopedCriticalSection criticalSection{};
        mutex_.unlock();
        if (currentThread->WaitForKernelSync(waiterList) && timeoutPoint > KSystemClock::time_point{}) {
            hasWaited = true;
            currentThread->SetAlarmTime(timeoutPoint);
        }
    }
    if (hasWaited) {
        currentThread->ClearAlarm();
    }
    mutex_.lock();
}

void KConditionVariable::notify_one() noexcept
{
    KScopedCriticalSection criticalSection{};
    auto t = waiterList.begin();
    if (t != waiterList.end()) {
        t->ResumeFromKernelSync();
    }
}

void KConditionVariable::notify_all() noexcept
{
    KScopedCriticalSection criticalSection{};
    KThread::ResumeAllFromKernelSync(waiterList);
}

}
