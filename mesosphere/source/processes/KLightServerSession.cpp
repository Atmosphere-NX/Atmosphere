#include <mesosphere/processes/KLightServerSession.hpp>
#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>

namespace mesosphere
{

KLightServerSession::~KLightServerSession()
{
    Terminate(true);
}

bool KLightServerSession::IsSignaled() const
{
    return false; // TODO
}

void KLightServerSession::Terminate(bool fromServer)
{
    SharedPtr<KThread> curSender{std::move(currentSender)};
    {
        KScopedCriticalSection critsec{};
        if (fromServer) {
            parent->isServerAlive = false; // buggy in official kernel -- where it sets it outside of critical section
        } else {
            parent->isClientAlive = false;
        }
        if (curSender != nullptr) {
            kassert(curSender->GetSchedulingStatus() == KThread::SchedulingStatus::Paused && curSender->IsInKernelSync());
            curSender->CancelKernelSync(ResultKernelConnectionClosed()); //TODO check
            currentSender = nullptr;
            currentReceiver = nullptr;
        }

        for (auto &&sender : senderThreads) {
            kassert(sender.GetSchedulingStatus() == KThread::SchedulingStatus::Paused && sender.IsInKernelSync());
            sender.CancelKernelSync(ResultKernelConnectionClosed()); //TODO check
        }

        KThread::ResumeAllFromKernelSync(receiverThreads);
    }
}

}
