#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/threading/KThread.hpp>

#include <mutex>

namespace mesosphere
{

KSynchronizationObject::~KSynchronizationObject()
{
}

void KSynchronizationObject::NotifyWaiters()
{
    KScopedCriticalSection criticalSection;

    if (IsSignaled()) {
        for (auto &&waiter : waiters) {
            waiter->HandleSyncObjectSignaled(this);
        }
    }
}

KLinkedList<KThread *>::const_iterator KSynchronizationObject::AddWaiter(KThread &thread)
{
    return waiters.insert(waiters.end(), &thread);
}

void KSynchronizationObject::RemoveWaiter(KLinkedList<KThread *>::const_iterator it)
{
    waiters.erase(it);
}

}
