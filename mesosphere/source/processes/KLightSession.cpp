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
    client.isRemoteActive = true;
    server.isRemoteActive = true;

    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());
    return ResultSuccess();
}

}
