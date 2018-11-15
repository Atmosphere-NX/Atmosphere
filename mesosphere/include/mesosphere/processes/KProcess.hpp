#pragma once

class KThread;
class KResourceLimit;

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/processes/KHandleTable.hpp>
#include <mesosphere/threading/KThread.hpp>

namespace mesosphere
{

class KProcess final : public KSynchronizationObject /* FIXME */ {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(SynchronizationObject, Process);

    enum class State : uint {
        Created = 0,
        CreatedAttached,
        Started,
        Crashed,
        StartedAttached,
        Exiting,
        Exited,
        DebugSuspended,
    };

    virtual bool IsSignaled() const override;

    constexpr long GetSchedulerOperationCount() const { return schedulerOperationCount; }

    void IncrementSchedulerOperationCount() { ++schedulerOperationCount; }
    void SetLastThreadAndIdleSelectionCount(KThread *thread, ulong idleSelectionCount);

    const SharedPtr<KResourceLimit> &GetResourceLimit() const { return reslimit; }

    KHandleTable &GetHandleTable() { return handleTable; }

    constexpr State GetState() const { return state; }

    void SetDebugPauseState(bool pause);

    KDebug *GetDebug() const { return debug; }
    void SetDebug(KDebug *debug);
    void ClearDebug(State attachState);

    private:
    KThread *lastThreads[MAX_CORES]{nullptr};
    ulong lastIdleSelectionCount[MAX_CORES]{0};
    long schedulerOperationCount = -1;

    State state = State::Created;
    bool stateChanged = false;

    KDebug *debug = nullptr;
    SharedPtr<KResourceLimit> reslimit{};
    KHandleTable handleTable{};
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(Process);

}
