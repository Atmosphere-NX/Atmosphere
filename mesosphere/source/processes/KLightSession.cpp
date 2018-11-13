#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KLightSession::~KLightSession()
{
}

Result KLightSession::Initialize(KPort *parentPort)
{
    SetClientServerParent();
    isClientAlive = true;
    isServerAlive = true;

    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());

    if (parentPort == nullptr) {
        return ResultSuccess();
    } else {
        // Another difference with official kernel: if adding the session fails, the session isn't added to allocator set (since it'll be deleted right away).
        Result res = parentPort->AddLightServerSession(server);
        if (res.IsSuccess()) {
            client.parentClientPort = &parentPort->GetClient(); 
        }
        return res;
    }
}

void KLightSession::Terminate(bool fromServer)
{
    server.Terminate(fromServer);
}

}
