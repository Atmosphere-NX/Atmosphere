#pragma once

#include <array>
#include <mutex>
#include <mesosphere/core/util.hpp>
#include <mesosphere/threading/KMultiLevelQueue.hpp>
#include <mesosphere/threading/KCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>

namespace mesosphere
{

class KScheduler {
    public:
    class Global {
        public:
            using MlqType = KMultiLevelQueue<64, KThread::SchedulerList, __decltype(&KThread::GetPriorityOf)>;
            Global() = delete;
            Global(const Global &) = delete;
            Global(Global &&) = delete;
            Global &operator=(const Global &) = delete;
            Global &operator=(Global &&) = delete;

            static MlqType &GetScheduledMlq(uint coreId) { return scheduledMlqs[coreId]; }
            static MlqType &GetSuggestedMlq(uint coreId) { return suggestedMlqs[coreId]; }

            static void SetThreadRunning(KThread &thread);
            static void SetThreadPaused(KThread &thread);
            static void AdjustThreadPriorityChanged(KThread &thread, uint oldPrio, bool isCurrentThread = false);
            static void AdjustThreadAffinityChanged(KThread &thread, int oldCoreId, u64 oldAffinityMask);
            static void YieldThread(KThread &thread);
            static void YieldThreadAndBalanceLoad(KThread &thread);
            static void YieldThreadAndWaitForLoadBalancing(KThread &thread);

            static void YieldPreemptThread(KThread &currentKernelHandlerThread, uint coreId, uint maxPrio = 59);

            static void SelectThreads();

            static constexpr uint minRegularPriority = 2;
        private:
            friend class KScheduler;

            static void TransferThreadToCore(KThread &thread, int coreId);
            static void AskForReselectionOrMarkRedundant(KThread *currentThread, KThread *winner);

            // allowSecondPass = true is only used in SelectThreads
            static KThread *PickOneSuggestedThread(const std::array<KThread *, MAX_CORES> &currentThreads,
                uint coreId, bool compareTime = false, bool allowSecondPass = false,
                uint maxPrio = 0, uint minPrio = MlqType::depth - 1);

            static bool reselectionRequired;
            static std::array<MlqType, MAX_CORES> scheduledMlqs, suggestedMlqs;

            template<typename F, typename ...Args>
            static void ApplyReschedulingOperationImpl(F f, KThread &thread, int coreId, u64 affMask, Args&& ...args)
            {
                if (coreId >= 0) {
                    f(scheduledMlqs[coreId], thread, std::forward<Args>(args)...);
                    affMask &= ~(1 << coreId);
                }

                while (affMask != 0) {
                    coreId = __builtin_ffsll(affMask) - 1;
                    f(suggestedMlqs[coreId], thread, std::forward<Args>(args)...);
                    affMask &= ~(1 << coreId);
                }
            }

            template<typename F, typename ...Args>
            static void ApplyReschedulingOperation(F f, KThread &thread, Args&& ...args)
            {
                u64 aff = thread.GetAffinityMask();
                int coreId = thread.GetCurrentCoreId();

                ApplyReschedulingOperationImpl(f, thread, coreId, aff, std::forward<Args>(args)...);

                thread.IncrementSchedulerOperationCount();
                reselectionRequired = true;
            }
    };

    KScheduler() = default;
    KScheduler(const KScheduler &) = delete;
    KScheduler(KScheduler &&) = delete;
    KScheduler &operator=(const KScheduler &) = delete;
    KScheduler &operator=(KScheduler &&) = delete;

    static KCriticalSection &GetCriticalSection() { return criticalSection; }

    static void YieldCurrentThread();
    static void YieldCurrentThreadAndBalanceLoad();
    static void YieldCurrentThreadAndWaitForLoadBalancing();

    static void HandleCriticalSectionLeave();
    friend void SchedulerHandleCriticalSectionLeave()
    {
        HandleCriticalSectionLeave();
    }

    void ForceContextSwitch() {}
    void ForceContextSwitchAfterIrq() {}

    void SetContextSwitchNeededForWorkQueue() { isContextSwitchNeededForWorkQueue = true; }

    constexpr ulong GetIdleSelectionCount() const { return idleSelectionCount; }
    constexpr bool IsActive() const { return /*isActive */ true; } // TODO

    private:
    bool hasContextSwitchStartedAfterIrq;
    bool isActive;
    bool isContextSwitchNeeded;
    bool isContextSwitchNeededForWorkQueue;
    uint coreId;
    u64 lastContextSwitchTime;
    KThread *selectedThread;
    KThread *previousThread;
    ulong idleSelectionCount;

    void *tmpStack;
    KThread *idleThread;

    static KCriticalSection criticalSection;

    template<typename F>
    void DoYieldOperation(F f, KThread &currentThread)
    {
        if (!currentThread.IsSchedulerOperationRedundant())
        {
            criticalSection.lock();
            f(currentThread);
            criticalSection.unlock();
            ForceContextSwitch();
        }
    }
};

}
