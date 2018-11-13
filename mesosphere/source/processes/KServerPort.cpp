#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KServerPort::~KServerPort()
{
    KCriticalSection &critsec = KScheduler::GetCriticalSection();
    critsec.lock();
    parent->isServerAlive = false;
    // TODO: normal sessions
    lightServerSessions.clear_and_dispose(
        [&critsec](KLightServerSession *s) {
            critsec.unlock();
            intrusive_ptr_release(s);
            critsec.lock();
        }
    );
    critsec.unlock();
}

bool KServerPort::IsSignaled() const
{
    if (!parent->isLight) {
        return false; // TODO
    } else {
        return !lightServerSessions.empty();
    }
}

Result KServerPort::AddLightServerSession(KLightServerSession &lightServerSession)
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
