#include <mutex>
#include <atomic>
#include <algorithm>

#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

bool KThread::IsAlive() const
{
    return true;
}

void KThread::OnAlarm()
{
    CancelKernelSync();
}

void KThread::AdjustScheduling(ushort oldMaskFull)
{
    if (currentSchedMaskFull == oldMaskFull) {
        return;
    } else if (CompareSchedulingStatusFull(oldMaskFull, SchedulingStatus::Running)) {
        KScheduler::Global::SetThreadPaused(*this);
    } else if (CompareSchedulingStatusFull(SchedulingStatus::Running)) {
        KScheduler::Global::SetThreadRunning(*this);
    }
}

void KThread::Reschedule(KThread::SchedulingStatus newStatus)
{
    //std::lock_guard criticalSection{KScheduler::GetCriticalSection()};
    // TODO check the above ^
    AdjustScheduling(SetSchedulingStatusField(newStatus));
}

void KThread::RescheduleIfStatusEquals(SchedulingStatus expectedStatus, SchedulingStatus newStatus)
{
    if(GetSchedulingStatus() == expectedStatus) {
        Reschedule(newStatus);
    }
}

void KThread::AddForcePauseReason(KThread::ForcePauseReason reason)
{
    KScopedCriticalSection criticalSection;

    if (!IsDying()) {
        AddForcePauseReasonToField(reason);
        if (numKernelMutexWaiters == 0) {
            AdjustScheduling(CommitForcePauseToField());
        }
    }
}

void KThread::RemoveForcePauseReason(KThread::ForcePauseReason reason)
{
    KScopedCriticalSection criticalSection;

    if (!IsDying()) {
        RemoveForcePauseReasonToField(reason);
        if (!IsForcePaused() && numKernelMutexWaiters == 0) {
            AdjustScheduling(CommitForcePauseToField());
        }
    }
}

bool KThread::WaitForKernelSync(KThread::WaitList &waitList)
{
    // Has to be called from critical section
    currentWaitList = &waitList;
    Reschedule(SchedulingStatus::Paused);
    waitList.push_back(*this);
    if (IsDying()) {
        // Whoops
        ResumeFromKernelSync();
        return false;
    }

    return true;
}

void KThread::ResumeFromKernelSync()
{
    // Has to be called from critical section
    currentWaitList->erase(currentWaitList->iterator_to(*this));
    currentWaitList = nullptr;
    Reschedule(SchedulingStatus::Running);
}

void KThread::ResumeAllFromKernelSync(KThread::WaitList &waitList)
{
    // Has to be called from critical section
    waitList.clear_and_dispose(
        [](KThread *t) {
            t->currentWaitList = nullptr;
            t->Reschedule(SchedulingStatus::Running);
        }
    );
}

void KThread::CancelKernelSync()
{
    KScopedCriticalSection criticalSection;
    if (GetSchedulingStatus() == SchedulingStatus::Paused) {
        // Note: transparent to force-pause
        if (currentWaitList != nullptr) {
            ResumeFromKernelSync();
        } else {
            Reschedule(SchedulingStatus::Running);
        }
    }
}

void KThread::CancelKernelSync(Result res)
{
    syncResult = res;
    CancelKernelSync();
}

void KThread::HandleSyncObjectSignaled(KSynchronizationObject *syncObj)
{
    if (GetSchedulingStatus() == SchedulingStatus::Paused) {
        signaledSyncObject = syncObj;
        syncResult = ResultSuccess{};
        Reschedule(SchedulingStatus::Running);
    }
}

Result KThread::WaitSynchronizationImpl(int &outId, KSynchronizationObject **syncObjs, int numSyncObjs, const KSystemClock::time_point &timeoutTime)
{
    KLinkedList<KThread *>::const_iterator nodes[numSyncObjs];

    outId = -1;
    {
        KScopedCriticalSection criticalSection;

        // Try to find an already signaled object.
        if (numSyncObjs >= 1) {
            KSynchronizationObject **readyFound = std::find_if(
                syncObjs,
                syncObjs + numSyncObjs,
                [](KSynchronizationObject *obj) {
                    return obj->IsSignaled();
                }
            );

            outId = readyFound - syncObjs >= numSyncObjs ? -1 : readyFound - syncObjs;
        }

        if (timeoutTime == KSystemClock::time_point{} && outId == -1) {
            return ResultKernelTimedOut{};
        }
        if (IsDying()) {
            return ResultKernelThreadTerminating{};
        }
        if (cancelled) {
            return ResultKernelCancelled{};
        }

        for (int i = 0; i < numSyncObjs; i++) {
            nodes[i] = syncObjs[i]->AddWaiter(*this);
        }

        isWaitingSync = true;
        signaledSyncObject = nullptr;
        syncResult = ResultKernelTimedOut{};

        Reschedule(SchedulingStatus::Paused);
        if (timeoutTime > KSystemClock::time_point{}) {
            SetAlarmTime(timeoutTime);
        }
    }

    // Now waiting...

    {
        KScopedCriticalSection criticalSection;

        isWaitingSync = false;
        if (timeoutTime > KSystemClock::time_point{}) {
            ClearAlarm();
        }

        for (int i = 0; i < numSyncObjs; i++) {
            syncObjs[i]->RemoveWaiter(nodes[i]);
            if (syncObjs[i] == signaledSyncObject) {
                outId = i;
            }
        }
    }

    return syncResult;
}

void KThread::AddToMutexWaitList(KThread &thread)
{
    // TODO: check&increment numKernelMutexWaiters
    // Ordered list insertion
    auto it = std::find_if(
        mutexWaitList.begin(),
        mutexWaitList.end(),
        [&thread](const KThread &t) {
            return t.GetPriority() > thread.GetPriority();
        }
    );

    if (it != mutexWaitList.end()) {
        mutexWaitList.insert(it, thread);
    } else {
        mutexWaitList.push_back(thread);
    }
}

KThread::MutexWaitList::iterator KThread::RemoveFromMutexWaitList(KThread::MutexWaitList::const_iterator it)
{
    // TODO: check&decrement numKernelMutexWaiters
    return mutexWaitList.erase(it);
}

void KThread::RemoveFromMutexWaitList(const KThread &t)
{
    RemoveFromMutexWaitList(mutexWaitList.iterator_to(t));
}

void KThread::InheritDynamicPriority()
{
    /*
        Do priority inheritance
        Since we're maybe changing the priority of the thread,
        we must go through the entire mutex owner chain.
        The invariant must be preserved:
            A thread holding a mutex must have a higher-or-same priority than
            all threads waiting for it to release the mutex.
    */

    for (KThread *t = this; t != nullptr; t = t->wantedMutexOwner) {
        uint newPrio, oldPrio = priority;
        if (!mutexWaitList.empty() && mutexWaitList.front().priority < basePriority) {
            newPrio =  mutexWaitList.front().priority;
        } else {
            newPrio = basePriority;
        }

        if (newPrio == oldPrio) {
            break;
        } else {
            // Update everything that depends on dynamic priority:

            // TODO update condvar
            // TODO update ctr arbiter
            priority = newPrio;
            // TODO update condvar
            // TODO update ctr arbiter
            if (CompareSchedulingStatusFull(SchedulingStatus::Running)) {
                KScheduler::Global::AdjustThreadPriorityChanged(*this, oldPrio, this == KCoreContext::GetCurrentInstance().GetCurrentThread());
            }

            if (wantedMutexOwner != nullptr) {
                wantedMutexOwner->RemoveFromMutexWaitList(*this);
                wantedMutexOwner->AddToMutexWaitList(*this);
            }
        }
    }
}

void KThread::AddMutexWaiter(KThread &waiter)
{
    AddToMutexWaitList(waiter);
    InheritDynamicPriority();
}

void KThread::RemoveMutexWaiter(KThread &waiter)
{
    RemoveFromMutexWaitList(waiter);
    InheritDynamicPriority();
}

KThread *KThread::RelinquishMutex(size_t *count, uiptr mutexAddr)
{
    KThread *newOwner = nullptr;
    *count = 0;

    // First in list wanting mutexAddr becomes owner, the rest is transferred
    for (auto it = mutexWaitList.begin(); it != mutexWaitList.end(); ) {
        if (it->wantedMutex != mutexAddr) {
            ++it;
            continue;
        } else {
            KThread &t = *it;
            ++(*count);
            it = RemoveFromMutexWaitList(it);
            if (newOwner == nullptr) {
                newOwner = &t;
            } else {
                newOwner->AddToMutexWaitList(t);
            }
        }
    }

    // Mutex waiters list have changed
    InheritDynamicPriority();
    if (newOwner != nullptr) {
        newOwner->InheritDynamicPriority();
    }

    return newOwner;
}

}
