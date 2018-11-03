#pragma once

#include <atomic>
#include <boost/intrusive/list.hpp>
#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Handle.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/processes/KProcess.hpp>
#include <mesosphere/interfaces/IAlarmable.hpp>
#include <mesosphere/interfaces/ILimitedResource.hpp>

namespace mesosphere
{

struct ThreadWaitListTag;
struct ThreadMutexWaitListTag;
using  ThreadWaitListBaseHook = boost::intrusive::list_base_hook<boost::intrusive::tag<ThreadWaitListTag> >;
using  ThreadMutexWaitListBaseHook = boost::intrusive::list_base_hook<boost::intrusive::tag<ThreadMutexWaitListTag> >;

class KThread final :
    public KAutoObject,
    public ILimitedResource<KThread>,
    public ISetAllocated<KThread>,
    public IAlarmable,
    public ThreadWaitListBaseHook,
    public ThreadMutexWaitListBaseHook
{
    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, Thread);
    virtual bool IsAlive() const override;

    virtual void OnAlarm() override;

    struct SchedulerValueTraits {
        using node_traits =  boost::intrusive::list_node_traits<KThread *>;
        using node = node_traits::node;
        using node_ptr = node *;
        using const_node_ptr = const node *;
        using value_type = KThread;
        using pointer = KThread *;
        using const_pointer = const KThread *;
        static constexpr boost::intrusive::link_mode_type link_mode = boost::intrusive::normal_link;

        constexpr SchedulerValueTraits(uint coreId) : coreId(coreId) {}
        node_ptr to_node_ptr (value_type &value) const {
            return &value.schedulerNodes[coreId];
        }
        const_node_ptr to_node_ptr (const value_type &value) const {
            return &value.schedulerNodes[coreId];
        }
        pointer to_value_ptr(node_ptr n) const {
            return detail::GetParentFromArrayMember(n, coreId, &KThread::schedulerNodes);
        }
        const_pointer to_value_ptr(const_node_ptr n) const {
            return detail::GetParentFromArrayMember(n, coreId, &KThread::schedulerNodes);
        }

        private:
        uint coreId;
    };

    enum class SchedulingStatus : u16 {
        Paused      = 1,
        Running     = 2,
        Exited      = 3,
    };

    enum class ForcePauseReason : u16 {
        ThreadActivity     = 0,
        ProcessActivity    = 1,
        Debug              = 2,
        Reserved           = 3,
        KernelLoading      = 4,
    };

    using SchedulerList = typename boost::intrusive::make_list<
        KThread,
        boost::intrusive::value_traits<KThread::SchedulerValueTraits>
    >::type;

    using WaitList = typename boost::intrusive::make_list<
        KThread,
        boost::intrusive::base_hook<ThreadWaitListBaseHook>,
        boost::intrusive::constant_time_size<false>
    >::type;

    private:
    using MutexWaitList = typename boost::intrusive::make_list<
        KThread,
        boost::intrusive::base_hook<ThreadMutexWaitListBaseHook>
    >::type;

    public:

    static constexpr uint GetPriorityOf(const KThread &thread)
    {
        return thread.priority;
    }

    constexpr uint GetPriority() const { return priority; }
    constexpr u64 GetId() const { return id; }
    constexpr int GetCurrentCoreId() const { return currentCoreId; }
    constexpr ulong GetAffinityMask() const { return affinityMask; }
    constexpr long GetLastScheduledTime() const { return lastScheduledTime; }

    KProcess *GetOwner() const { return owner; }
    bool IsSchedulerOperationRedundant() const { return owner != nullptr && owner->GetSchedulerOperationCount() == redundantSchedulerOperationCount; }

    void IncrementSchedulerOperationCount() { if (owner != nullptr) owner->IncrementSchedulerOperationCount(); }
    void SetRedundantSchedulerOperation() { redundantSchedulerOperationCount = owner != nullptr ? owner->GetSchedulerOperationCount() : redundantSchedulerOperationCount; }
    void SetCurrentCoreId(int coreId) { currentCoreId = coreId; }

    void SetProcessLastThreadAndIdleSelectionCount(ulong idleSelectionCount)
    {
        if (owner != nullptr) {
            owner->SetLastThreadAndIdleSelectionCount(this, idleSelectionCount);
        }
    }

    void UpdateLastScheduledTime() { ++lastScheduledTime; /* FIXME */}

    constexpr SchedulingStatus GetSchedulingStatus() const
    {
        return (SchedulingStatus)(currentSchedMaskFull & 0xF);
    }
    constexpr bool IsForcePausedFor(ForcePauseReason reason) const
    {
        return (schedMaskForForcePauseFull & (1 << (4 + ((ushort)reason)))) != 0;
    }
    constexpr bool IsForcePaused() const
    {
        return (schedMaskForForcePauseFull & ~0xF) != 0;
    }
    static constexpr bool CompareSchedulingStatusFull(ushort fullMask, SchedulingStatus status)
    {
        return fullMask == (ushort)status;
    }
    constexpr bool CompareSchedulingStatusFull(SchedulingStatus status) const
    {
        return CompareSchedulingStatusFull(schedMaskForForcePauseFull, status);
    }

    /// Returns old full mask
    ushort SetSchedulingStatusField(SchedulingStatus status)
    {
        ushort oldMaskFull = currentSchedMaskFull;
        currentSchedMaskFull = (currentSchedMaskFull & ~0xF) | ((ushort)status & 0xF);
        return oldMaskFull;
    }
    void AddForcePauseReasonToField(ForcePauseReason reason)
    {
        schedMaskForForcePauseFull |= 1 << (4 + ((ushort)reason));
    }
    void RemoveForcePauseReasonToField(ForcePauseReason reason)
    {
        schedMaskForForcePauseFull |= ~(1 << (4 + ((ushort)reason)));
    }

    ushort CommitForcePauseToField()
    {

        ushort oldMaskFull = currentSchedMaskFull;
        currentSchedMaskFull = (schedMaskForForcePauseFull & ~0xF) | (currentSchedMaskFull & 0xF);
        return oldMaskFull;
    }
    ushort RevertForcePauseToField()
    {
        ushort oldMaskFull = currentSchedMaskFull;
        currentSchedMaskFull &= 0xF;
        return oldMaskFull;
    }

    void AdjustScheduling(ushort oldMaskFull);
    void Reschedule(SchedulingStatus newStatus);
    /// Sets status regardless of force-pausing.
    void RescheduleIfStatusEquals(SchedulingStatus expectedStatus, SchedulingStatus newStatus);
    void AddForcePauseReason(ForcePauseReason reason);
    void RemoveForcePauseReason(ForcePauseReason reason);

    bool IsDying() const
    {
        // Or already dead
        /*
            terminationWanted is only set on exit, under scheduler critical section, to true,
            and the readers are either a thread under critical section (most common), or end-of-irq/svc/other exception,
            therefore synchronization outside critsec can be implemented through fences, I think
        */
        return CompareSchedulingStatusFull(SchedulingStatus::Exited) || terminationWanted;
    }

    void SetTerminationWanted()
    {
        terminationWanted = true;
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    /// Takes effect when critical section is left
    bool WaitForKernelSync(WaitList &waitList);
    /// Takes effect when critical section is left
    void ResumeFromKernelSync();
    /// Takes effect when critical section is left -- all threads in waitlist
    static void ResumeAllFromKernelSync(WaitList &waitList);
    /// Takes effect immediately
    void CancelKernelSync();
    /// Takes effect immediately
    void CancelKernelSync(Result res);

    constexpr size_t GetNumberOfKMutexWaiters() const { return numKernelMutexWaiters; }
    constexpr uiptr GetWantedMutex() const { return wantedMutex; }
    void SetWantedMutex(uiptr mtx)  { wantedMutex = mtx; }

    void AddMutexWaiter(KThread &waiter);
    KThread *RelinquishMutex(size_t *count, uiptr mutexAddr);
    void RemoveMutexWaiter(KThread &waiter);
    void InheritDynamicPriority();

    KThread() = default;
    KThread(KProcess *owner, u64 id, uint priority) : KAutoObject(), owner(owner), schedulerNodes(),
    id(id), basePriority(priority), priority(priority),
    currentCoreId(0), affinityMask(15) {};
private:
    void AddToMutexWaitList(KThread &thread);
    MutexWaitList::iterator RemoveFromMutexWaitList(MutexWaitList::const_iterator it);
    void RemoveFromMutexWaitList(const KThread &t);

    KProcess *owner = nullptr;

    boost::intrusive::list_node_traits<KThread *>::node schedulerNodes[4]{};

    WaitList *currentWaitList = nullptr;

    u64 id = 0;
    long redundantSchedulerOperationCount = 0;
    ushort currentSchedMaskFull = (ushort)SchedulingStatus::Paused;
    ushort schedMaskForForcePauseFull = 0;
    bool terminationWanted = false;
    uint basePriority = 64, priority = 64;
    int currentCoreId = -1;
    ulong affinityMask = 0;

    uiptr wantedMutex = 0;
    KThread *wantedMutexOwner = nullptr;
    MutexWaitList mutexWaitList{};
    size_t numKernelMutexWaiters = 0;

    Handle syncResultHandle{};
    Result syncResult = ResultSuccess();

    u64 lastScheduledTime = 0;
};

inline void intrusive_ptr_add_ref(KThread *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KThread *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
