#include <mesosphere/processes/KWritableEvent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KEvent.hpp>
#include <mesosphere/threading/KScheduler.hpp>

namespace mesosphere
{
    
bool KReadableEvent::IsSignaled() const {
    return this->is_signaled;
}

Result KReadableEvent::Signal() {
    KScopedCriticalSection critical_section;
    
    if (!this->is_signaled) {
        this->is_signaled = true;
        this->NotifyWaiters();
    }
    
    return ResultSuccess();
}

Result KReadableEvent::Clear() {
    this->Reset();
    
    return ResultSuccess();
}

Result KReadableEvent::Reset() {
    KScopedCriticalSection critical_section;
    
    if (this->is_signaled) {
        this->is_signaled = false;
        return ResultSuccess();
    }
    return ResultKernelInvalidState();
}

}
