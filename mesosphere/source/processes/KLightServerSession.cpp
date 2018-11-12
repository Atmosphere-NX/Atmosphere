#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KLightServerSession::~KLightServerSession()
{
    Terminate(true);
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
            if (!curSender->IsDying()) {
                curSender->ResumeFromKernelSync(ResultKernelConnectionClosed());
            }
            currentSender = nullptr;
            currentReceiver = nullptr;
        }

        for (auto &&sender : senderThreads) {
            if (!sender.IsDying()) {
                sender.ResumeFromKernelSync(ResultKernelConnectionClosed()); //TODO check
            }
        }

        KThread::ResumeAllFromKernelSync(receiverThreads);
    }
}

Result KLightServerSession::HandleSyncRequest(KThread &sender)
{
    if (!parent->isClientAlive || !parent->isServerAlive) {
        return ResultKernelConnectionClosed();
    }

    if (!sender.WaitForKernelSync(senderThreads)) {
        return ResultKernelThreadTerminating();
    }

    if (currentSender != nullptr || receiverThreads.empty()) {
        // Nothing more to do if a request is being handled or if there's no receiver yet.
        return ResultSuccess();
    } else {
        // Otherwise, wake once receiver.
        receiverThreads.front().ResumeFromKernelSync();
        return ResultSuccess();
    }
}

Result KLightServerSession::ReplyAndReceive(LightSessionRequest *request)
{
    KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
    curThread->SetCurrentLightSessionRequest(request);

    if (request->cmdId < 0) {
        // Reply
        SharedPtr<KThread> curSender{};
        {
            KScopedCriticalSection critsec{};
            if (!parent->isClientAlive || !parent->isServerAlive) {
                return ResultKernelConnectionClosed();
            }
            if (currentSender == nullptr || currentReceiver != curThread) {
                return ResultKernelInvalidState();
            }

            curSender = std::move(currentSender);
            if (!curSender->IsDying()) {
                *curSender->GetCurrentLightSessionRequest() = *curThread->GetCurrentLightSessionRequest();
                curSender->ResumeFromKernelSync();
            }
            currentSender = nullptr;
            currentReceiver = nullptr;
        }
    }

    {
        // Receive
        KCriticalSection &critsec = KScheduler::GetCriticalSection();
        std::scoped_lock criticalSection{critsec};
        bool waitedForSync = false;

        // If there's already one receiver, return an error
        if (!receiverThreads.empty()) {
            return ResultKernelInvalidState();
        }

        while (currentReceiver == nullptr) {
            if (waitedForSync) {
                curThread->SetWaitingSync(false);
            }

            if (!parent->isClientAlive || !parent->isServerAlive) {
                return ResultKernelConnectionClosed();
            }

            // Try to see if we can do sync immediately, otherwise wait until later...
            if (currentSender == nullptr && !senderThreads.empty()) {
                // Do the sync.
                currentSender = &senderThreads.front();
                currentReceiver = curThread;
                *curThread->GetCurrentLightSessionRequest() = *currentSender->GetCurrentLightSessionRequest();
                return ResultSuccess();
            } else {
                // We didn't get to sync, we need to wait.
                if (!curThread->WaitForKernelSync(receiverThreads)) {
                    return ResultKernelThreadTerminating();
                }

                if (curThread->IsSyncCancelled()) {
                    curThread->ResumeFromKernelSync();
                    curThread->SetSyncCancelled(false);
                    return ResultKernelCancelled();
                }

                // Wait NOW.
                critsec.unlock();
                critsec.lock();
                waitedForSync = true;
                if (receiverThreads.empty()) {
                    return ResultKernelInvalidState();
                }
            }
        }
    }

    return ResultKernelInvalidState();
}

}
