#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KLightSession::~KLightSession()
{
}

Result KLightSession::Initialize()
{
    SetClientServerParent();
    isClientAlive = true;
    isServerAlive = true;

    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());
    return ResultSuccess();
}

void KLightSession::Terminate(bool fromServer)
{
    server.Terminate(fromServer);
}

}
