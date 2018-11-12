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
    return false; // TODO
}

}
