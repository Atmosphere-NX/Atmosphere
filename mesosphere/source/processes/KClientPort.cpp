#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KClientPort::~KClientPort()
{
    KScopedCriticalSection critsec{};
    parent->isClientAlive = false;
}

bool KClientPort::IsSignaled() const
{
    return false; // TODO
}

}
