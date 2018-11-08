#include <mesosphere/processes/KWritableEvent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KEvent.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>

namespace mesosphere
{

bool KReadableEvent::IsSignaled() const
{
    return this->isSignaled;
}

KReadableEvent::~KReadableEvent()
{
}

Result KReadableEvent::Signal()
{
    KScopedCriticalSection criticalSection{};

    if (!this->isSignaled) {
        this->isSignaled = true;
        NotifyWaiters();
    }

    return ResultSuccess();
}

Result KReadableEvent::Clear()
{
    Reset();

    return ResultSuccess();
}

Result KReadableEvent::Reset()
{
    KScopedCriticalSection criticalSection{};

    if (this->isSignaled) {
        this->isSignaled = false;
        return ResultSuccess();
    }
    return ResultKernelInvalidState();
}

}
