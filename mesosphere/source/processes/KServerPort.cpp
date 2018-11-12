#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KServerPort::~KServerPort()
{
    KScopedCriticalSection critsec{};
    parent->isServerAlive = false;
}

bool KServerPort::IsSignaled() const
{
    if (!parent->isLight) {
        return false; // TODO
    } else {
        return !lightServerSessions.empty();
    }
}

Result KServerPort::AddServerSession(KLightServerSession &lightServerSession)
{
    KScopedCriticalSection critsec{};
    bool wasEmpty = lightServerSessions.empty();
    lightServerSessions.push_back(lightServerSession);
    if (wasEmpty && !lightServerSessions.empty()) {
        NotifyWaiters();
    }

    return ResultSuccess();
}

}
