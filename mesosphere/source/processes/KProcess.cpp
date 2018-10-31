#include <mesosphere/processes/KProcess.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/kresources/KResourceLimit.hpp>

namespace mesosphere
{

void KProcess::SetLastThreadAndIdleSelectionCount(KThread *thread, ulong idleSelectionCount)
{
    lastThreads[thread->GetCurrentCoreId()] = thread;
    lastIdleSelectionCount[thread->GetCurrentCoreId()] = idleSelectionCount;
}

}
