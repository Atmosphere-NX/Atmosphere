#include <mesosphere/interrupts/KWorkQueue.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/arch/KInterruptMaskGuard.hpp>

namespace mesosphere
{

void KWorkQueue::AddWork(IWork &work)
{
    workQueue.push_back(work);
    KCoreContext::GetCurrentInstance().GetScheduler()->SetContextSwitchNeededForWorkQueue();
}

void KWorkQueue::Initialize()
{
    //handlerThread.reset(new KThread); //TODO!
    kassert(handlerThread == nullptr);
}

void KWorkQueue::HandleWorkQueue()
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    while (true) {
        IWork *work = nullptr;
        do {
            KInterruptMaskGuard imguard{};
            auto it = workQueue.begin();
            if (it != workQueue.end()) {
                work = &*it;
                workQueue.erase(it);
            } else {
                {
                    //TODO: thread usercontext scheduler/bottom hard guard
                    cctx.GetCurrentThread()->Reschedule(KThread::SchedulingStatus::Paused);
                }
                cctx.GetScheduler()->ForceContextSwitch();
            } 
        } while (work == nullptr);

        work->DoWork();
    }
}

}
