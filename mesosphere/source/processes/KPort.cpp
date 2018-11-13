#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>

namespace mesosphere
{

KPort::~KPort()
{
}

Result KPort::Initialize(int maxSessions, bool isLight)
{
    SetClientServerParent();
    isClientAlive = true;
    isServerAlive = true;

    client.maxSessions = maxSessions;
    this->isLight = isLight;

    return ResultSuccess();
}

Result KPort::AddLightServerSession(KLightServerSession &lightServerSession)
{
    KScopedCriticalSection critsec{};
    if (isClientAlive || isServerAlive) {
        return ResultKernelConnectionRefused();
    } else {
        return server.AddLightServerSession(lightServerSession);
    }
}


}
