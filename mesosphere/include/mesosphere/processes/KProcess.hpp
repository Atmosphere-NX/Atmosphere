#pragma once

class KThread;
class KResourceLimit;

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/processes/KHandleTable.hpp>

namespace mesosphere
{

class KProcess : public KAutoObject {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, Process);

    virtual bool IsAlive() const override { return true; }
    constexpr long GetSchedulerOperationCount() const { return schedulerOperationCount; }

    void IncrementSchedulerOperationCount() { ++schedulerOperationCount; }
    void SetLastThreadAndIdleSelectionCount(KThread *thread, ulong idleSelectionCount);

    const SharedPtr<KResourceLimit> &GetResourceLimit() const { return reslimit; }

    private:
    KThread *lastThreads[MAX_CORES]{nullptr};
    ulong lastIdleSelectionCount[MAX_CORES]{0};
    long schedulerOperationCount = -1;

    SharedPtr<KResourceLimit> reslimit{};
    KHandleTable handleTable{};
};

inline void intrusive_ptr_add_ref(KProcess *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KProcess *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
