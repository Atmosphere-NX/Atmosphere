#include <algorithm>
#include <atomic>

#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

namespace {
    struct MlqTraitsFactory {
        constexpr KThread::SchedulerValueTraits operator()(size_t i) const
        {
            return KThread::SchedulerValueTraits{(uint)i};
        }
    };
}

using MlqT = KScheduler::Global::MlqType;

bool KScheduler::Global::reselectionRequired = false;

std::array<MlqT, MAX_CORES> KScheduler::Global::scheduledMlqs =
    detail::MakeArrayWithFactorySequenceOf<MlqT, MlqTraitsFactory, MAX_CORES>(
        &KThread::GetPriorityOf
    );

std::array<MlqT, MAX_CORES> KScheduler::Global::suggestedMlqs =
    detail::MakeArrayWithFactorySequenceOf<MlqT, MlqTraitsFactory, MAX_CORES>(
        &KThread::GetPriorityOf
    );


void KScheduler::Global::SetThreadRunning(KThread &thread)
{
    ApplyReschedulingOperation([](MlqT &mlq, KThread &t){ mlq.add(t); }, thread);
}

void KScheduler::Global::SetThreadPaused(KThread &thread)
{
    ApplyReschedulingOperation([](MlqT &mlq, KThread &t){ mlq.remove(t); }, thread);
}

void KScheduler::Global::AdjustThreadPriorityChanged(KThread &thread, uint oldPrio, bool isCurrentThread)
{
    ApplyReschedulingOperation(
    [oldPrio, isCurrentThread](MlqT &mlq, KThread &t){
        mlq.adjust(t, oldPrio, isCurrentThread);
    }, thread);
}

void KScheduler::Global::AdjustThreadAffinityChanged(KThread &thread, int oldCoreId, u64 oldAffinityMask)
{
    int newCoreId = thread.GetCurrentCoreId();
    u64 newAffinityMask = thread.GetAffinityMask();

    ApplyReschedulingOperationImpl([](MlqT &mlq, KThread &t){ mlq.remove(t); }, thread, oldCoreId, oldAffinityMask);
    ApplyReschedulingOperationImpl([](MlqT &mlq, KThread &t){ mlq.add(t); }, thread, newCoreId, newAffinityMask);

    thread.IncrementSchedulerOperationCount();
    reselectionRequired = true;
}

void KScheduler::Global::TransferThreadToCore(KThread &thread, int coreId)
{
    int currentCoreId = thread.GetCurrentCoreId();

    if (currentCoreId != coreId) {
        if (currentCoreId != -1) {
            scheduledMlqs[currentCoreId].transferToBack(thread, suggestedMlqs[currentCoreId]);
        }

        if (coreId != -1) {
            suggestedMlqs[coreId].transferToFront(thread, scheduledMlqs[coreId]);
        }
    }

    thread.SetCurrentCoreId(coreId);
}

void KScheduler::Global::AskForReselectionOrMarkRedundant(KThread *currentThread, KThread *winner)
{
    if (currentThread == winner) {
        // Nintendo (not us) has a nullderef bug on currentThread->owner, but which is never triggered.
        currentThread->SetRedundantSchedulerOperation();
    } else {
        reselectionRequired = true;
    }
}

KThread *KScheduler::Global::PickOneSuggestedThread(const std::array<KThread *, MAX_CORES> &curThreads,
uint coreId, bool compareTime, bool allowSecondPass, uint maxPrio, uint minPrio) {
    if (minPrio < maxPrio) {
        return nullptr;
    }

    auto hasWorseTime = [coreId, minPrio, compareTime](const KThread &t) {
        if (!compareTime || scheduledMlqs[coreId].size(minPrio) <= 1 || t.GetPriority() < minPrio) {
            return false;
        } else {
            // Condition means the thread *it would have been scheduled again after the thread
            return t.GetLastScheduledTime() > scheduledMlqs[coreId].front(minPrio).GetLastScheduledTime();
        }
    };

    std::array<uint, MAX_CORES> secondPassCores;
    size_t numSecondPassCores = 0;

    auto it = std::find_if(
        suggestedMlqs[coreId].begin(maxPrio),
        suggestedMlqs[coreId].end(minPrio),
        [&hasWorseTime, &secondPassCores, &numSecondPassCores, &curThreads](const KThread &t) {
            int srcCoreId = t.GetCurrentCoreId();
            //bool worseTime = compareTime && hasWorseTime(t);
            // break if hasWorse time too
            if (srcCoreId >= 0) {
                bool srcHasEphemeralKernThread = scheduledMlqs[srcCoreId].highestPrioritySet() < minRegularPriority;
                bool isSrcCurT = &t == curThreads[srcCoreId];
                if (isSrcCurT) {
                    secondPassCores[numSecondPassCores++] = (uint)srcCoreId;
                }

                // Note, if checkTime official kernel breaks if srcHasEphemeralKernThread
                // I believe this is a bug
                if(srcHasEphemeralKernThread || isSrcCurT) {
                    return false;
                }
            }

            return true;
        }
    );

    if (it != suggestedMlqs[coreId].end(minPrio) && (!compareTime || !hasWorseTime(*it))) {
        return &*it;
    } else if (allowSecondPass) {
        // Allow to re-pick a selected thread about to be current, if it doesn't make the core idle
        auto srcCoreIdPtr = std::find_if(
            secondPassCores.cbegin(),
            secondPassCores.cbegin() + numSecondPassCores,
            [](uint id) {
                return scheduledMlqs[id].highestPrioritySet() >= minRegularPriority && scheduledMlqs[id].size() > 1;
            }
        );

        return srcCoreIdPtr == secondPassCores.cbegin() + numSecondPassCores ? nullptr : &scheduledMlqs[*srcCoreIdPtr].front();
    } else {
        return nullptr;
    }
}

void KScheduler::Global::YieldThread(KThread &currentThread)
{
    // Note: caller should use critical section, etc.
    kassert(currentThread.GetCurrentCoreId() >= 0);
    uint coreId = (uint)currentThread.GetCurrentCoreId();
    uint priority = currentThread.GetPriority();

    // Yield the thread
    scheduledMlqs[coreId].yield(currentThread);
    currentThread.IncrementSchedulerOperationCount();

    KThread *winner = &scheduledMlqs[coreId].front(priority);
    AskForReselectionOrMarkRedundant(&currentThread, winner);
}

void KScheduler::Global::YieldThreadAndBalanceLoad(KThread &currentThread)
{
    // Note: caller should check if !currentThread.IsSchedulerOperationRedundant and use critical section, etc.
    kassert(currentThread.GetCurrentCoreId() >= 0);
    uint coreId = (uint)currentThread.GetCurrentCoreId();
    uint priority = currentThread.GetPriority();

    std::array<KThread *, MAX_CORES> curThreads;
    for (uint i = 0; i < MAX_CORES; i++) {
        curThreads[i] = scheduledMlqs[i].empty() ? nullptr : &scheduledMlqs[i].front();
    }

    // Yield the thread
    scheduledMlqs[coreId].yield(currentThread);
    currentThread.IncrementSchedulerOperationCount();

    KThread *winner = PickOneSuggestedThread(curThreads, coreId, true, false, 0, priority);

    if (winner != nullptr) {
        TransferThreadToCore(*winner, coreId);
        winner->IncrementSchedulerOperationCount();
        currentThread.SetRedundantSchedulerOperation();
    } else {
        winner = &scheduledMlqs[coreId].front(priority);
    }

    AskForReselectionOrMarkRedundant(&currentThread, winner);
}

void KScheduler::Global::YieldThreadAndWaitForLoadBalancing(KThread &currentThread)
{
    // Note: caller should check if !currentThread.IsSchedulerOperationRedundant and use critical section, etc.
    KThread *winner = nullptr;
    kassert(currentThread.GetCurrentCoreId() >= 0);
    uint coreId = (uint)currentThread.GetCurrentCoreId();

    // Remove the thread from its scheduled mlq, put it on the corresponding "suggested" one instead
    TransferThreadToCore(currentThread, -1);
    currentThread.IncrementSchedulerOperationCount();

    // If the core is idle, perform load balancing, excluding the threads that have just used this function...
    if (scheduledMlqs[coreId].empty()) {
        // Here, "curThreads" is calculated after the ""yield"", unlike yield -1
        std::array<KThread *, MAX_CORES> curThreads;
        for (uint i = 0; i < MAX_CORES; i++) {
            curThreads[i] = scheduledMlqs[i].empty() ? nullptr : &scheduledMlqs[i].front();
        }

        KThread *winner = PickOneSuggestedThread(curThreads, coreId, false);

        if (winner != nullptr) {
            TransferThreadToCore(*winner, coreId);
            winner->IncrementSchedulerOperationCount();
        } else {
            winner = &currentThread;
        }
    }

    AskForReselectionOrMarkRedundant(&currentThread, winner);
}

void KScheduler::Global::YieldPreemptThread(KThread &currentKernelHandlerThread, uint coreId, uint maxPrio)
{
    if (!scheduledMlqs[coreId].empty(maxPrio)) {
        // Yield the first thread in the level queue
        scheduledMlqs[coreId].front(maxPrio).IncrementSchedulerOperationCount();
        scheduledMlqs[coreId].yield(maxPrio);
        if (scheduledMlqs[coreId].size() > 1) {
            scheduledMlqs[coreId].front(maxPrio).IncrementSchedulerOperationCount();
        }
    }

    // Here, "curThreads" is calculated after the forced yield, unlike yield -1
    std::array<KThread *, MAX_CORES> curThreads;
    for (uint i = 0; i < MAX_CORES; i++) {
        curThreads[i] = scheduledMlqs[i].empty() ? nullptr : &scheduledMlqs[i].front();
    }

    KThread *winner = PickOneSuggestedThread(curThreads, coreId, true, false, maxPrio, maxPrio);
    if (winner != nullptr) {
        TransferThreadToCore(*winner, coreId);
        winner->IncrementSchedulerOperationCount();
    }

    for (uint i = 0; i < MAX_CORES; i++) {
        curThreads[i] = scheduledMlqs[i].empty() ? nullptr : &scheduledMlqs[i].front();
    }

    // Find first thread which is not the kernel handler thread.
    auto itFirst = std::find_if(
        scheduledMlqs[coreId].begin(),
        scheduledMlqs[coreId].end(),
        [&currentKernelHandlerThread, coreId](const KThread &t) {
            return &t != &currentKernelHandlerThread;
        }
    );

    if (itFirst != scheduledMlqs[coreId].end()) {
        // If under the threshold, do load balancing again
        winner = PickOneSuggestedThread(curThreads, coreId, true, false, maxPrio, itFirst->GetPriority() - 1);
        if (winner != nullptr) {
            TransferThreadToCore(*winner, coreId);
            winner->IncrementSchedulerOperationCount();
        }
    }

    reselectionRequired = true;
}

void KScheduler::Global::SelectThreads()
{
    auto updateThread = [](KThread *thread, KScheduler &sched) {
        if (thread != sched.selectedThread) {
            if (thread != nullptr) {
                thread->IncrementSchedulerOperationCount();
                thread->UpdateLastScheduledTime();
                thread->SetProcessLastThreadAndIdleSelectionCount(sched.idleSelectionCount);
            } else {
                ++sched.idleSelectionCount;
            }
            sched.selectedThread = thread;
            sched.isContextSwitchNeeded = true;
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    };

    // This maintain the "current thread is on front of queue" invariant
    std::array<KThread *, MAX_CORES> curThreads;
    for (uint i = 0; i < MAX_CORES; i++) {
        KScheduler &sched = *KCoreContext::GetInstance(i).GetScheduler();
        curThreads[i] = scheduledMlqs[i].empty() ? nullptr : &scheduledMlqs[i].front();
        updateThread(curThreads[i], sched);
    }

    // Do some load-balancing. Allow second pass.
    std::array<KThread *, MAX_CORES> curThreads2 = curThreads;
    for (uint i = 0; i < MAX_CORES; i++) {
        if (scheduledMlqs[i].empty()) {
            KThread *winner = PickOneSuggestedThread(curThreads2, i, false, true);
            if (winner != nullptr) {
                curThreads2[i] = winner;
                TransferThreadToCore(*winner, i);
                winner->IncrementSchedulerOperationCount();
            }
        }
    }

    // See which to-be-current threads have changed & update accordingly
    for (uint i = 0; i < MAX_CORES; i++) {
        KScheduler &sched = *KCoreContext::GetInstance(i).GetScheduler();
        if (curThreads2[i] != curThreads[i]) {
            updateThread(curThreads2[i], sched);
        }
    }
    reselectionRequired = false;
}

KCriticalSection KScheduler::criticalSection{};

void KScheduler::YieldCurrentThread()
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    cctx.GetScheduler()->DoYieldOperation(Global::YieldThread, *cctx.GetCurrentThread());
}

void KScheduler::YieldCurrentThreadAndBalanceLoad()
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    cctx.GetScheduler()->DoYieldOperation(Global::YieldThreadAndBalanceLoad, *cctx.GetCurrentThread());
}

void KScheduler::YieldCurrentThreadAndWaitForLoadBalancing()
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    cctx.GetScheduler()->DoYieldOperation(Global::YieldThreadAndWaitForLoadBalancing, *cctx.GetCurrentThread());
}

}
