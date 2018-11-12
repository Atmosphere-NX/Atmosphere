#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KLightClientSession::~KLightClientSession()
{
    parent->Terminate(false);
}

Result KLightClientSession::SendSyncRequest(LightSessionRequest *request)
{
    KScopedCriticalSection critsec{};
    Result res;
    KThread *curThread = KCoreContext::GetCurrentInstance().GetCurrentThread();
    curThread->SetCurrentLightSessionRequest(request);
    curThread->ClearSync();
    res = parent->server.HandleSyncRequest(*curThread);
    return res.IsSuccess() ? curThread->GetSyncResult() : res;
}

}
