#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/core/make_object.hpp>

namespace mesosphere
{

KClientPort::~KClientPort()
{
    KScopedCriticalSection critsec{};
    parent->isClientAlive = false;
}

bool KClientPort::IsSignaled() const
{
    return numSessions.load() < maxSessions;
}

std::tuple<Result, SharedPtr<KLightClientSession>> KClientPort::ConnectLight()
{
    using RetType = std::tuple<Result, SharedPtr<KLightClientSession>>;
    // Official kernel first checks reslimit then session max count. We will do the opposite.
    int curCount = numSessions.load();
    while (curCount < maxSessions || !numSessions.compare_exchange_weak(curCount, curCount + 1));

    if (curCount >= maxSessions) {
        return RetType{ResultKernelOutOfSessions(), nullptr};
    }

    auto [res, serverSession, clientSession] = MakeObject<KLightSession>(parent.get());
    if (res.IsSuccess()) {
        serverSession.detach(); // Lifetime is now managed my KServerPort session list
        return RetType{ResultSuccess(), clientSession};
    } else {
        if (numSessions.fetch_sub(1) == maxSessions) {
            NotifyWaiters();
        }
        return RetType{res, nullptr};
    }
}

}
