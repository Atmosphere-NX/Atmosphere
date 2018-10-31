#include <mesosphere/threading/KConditionVariable.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

void KConditionVariable::wait_until_impl(const KSystemClock::time_point &timeoutPoint) noexcept
{
    // Official kernel counts number of waiters, but that isn't necessary
    {
        KThread *currentThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
        std::lock_guard guard{KScheduler::GetCriticalSection()};
        mutex_.unlock();
        if (currentThread->WaitForKernelSync(waiterList)) {
            (void)timeoutPoint; //TODO!
        } else {
            // Termination
        }
    }
    mutex_.lock();
}

void KConditionVariable::notify_one() noexcept
{
    std::lock_guard guard{KScheduler::GetCriticalSection()};
    auto t = waiterList.begin();
    if (t != waiterList.end()) {
        t->ResumeFromKernelSync();
    }
}

void KConditionVariable::notify_all() noexcept
{
    std::lock_guard guard{KScheduler::GetCriticalSection()};
    KThread::ResumeAllFromKernelSync(waiterList);
}

}
