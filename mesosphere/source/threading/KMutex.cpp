#include <mesosphere/threading/KMutex.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/threading/KScheduler.hpp>

namespace mesosphere
{

void KMutex::lock_slow_path(KThread &owner, KThread &requester)
{
    // Requester is currentThread most of (all ?) the time
    KCriticalSection &critsec = KScheduler::GetCriticalSection();
    std::lock_guard criticalSection{critsec};
    if (KCoreContext::GetCurrentInstance().GetScheduler()->IsActive()) {
        requester.SetWantedMutex((uiptr)this);
        owner.AddMutexWaiter(requester);

        // If the requester is/was running, pause it (sets status even if force-paused).
        requester.RescheduleIfStatusEquals(KThread::SchedulingStatus::Running, KThread::SchedulingStatus::Paused);

        // If the owner is force-paused, temporarily wake it.
        if (owner.IsForcePaused()) {
            owner.AdjustScheduling(owner.RevertForcePauseToField());
        }

        // Commit scheduler changes NOW.
        critsec.unlock();
        critsec.lock();

        /*
            At this point, mutex ownership has been transferred to requester or another thread (false wake).
            Make sure the requester, now resumed, isn't in any mutex wait list.
        */
       owner.RemoveMutexWaiter(requester);
    }
}

void KMutex::unlock_slow_path(KThread &owner)
{
    KScopedCriticalSection criticalSection;
    size_t count;
    KThread *newOwner = owner.RelinquishMutex(&count, (uiptr)this);
    native_handle_type newTag;

    if (newOwner != nullptr) {
        // Wake up new owner
        newTag = (native_handle_type)newOwner | (count > 1 ? 1 : 0);
        // Sets status even if force-paused.
        newOwner->RescheduleIfStatusEquals(KThread::SchedulingStatus::Paused, KThread::SchedulingStatus::Running);
    } else {
        // Free the mutex.
        newTag = 0;
    }

    // Allow previous owner to get back to forced-sleep, if no other thread wants the kmutexes it is holding.
    if (!owner.IsDying() && owner.GetNumberOfKMutexWaiters() == 0) {
        owner.AdjustScheduling(owner.CommitForcePauseToField());
    }

    tag = newTag;
}

}
