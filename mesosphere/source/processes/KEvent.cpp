#include <mesosphere/processes/KEvent.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/processes/KProcess.hpp>

namespace mesosphere
{

KEvent::~KEvent()
{
}

bool KEvent::IsAlive() const
{
    return isInitialized;
}

Result KEvent::Initialize()
{
    SetClientServerParent();
    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());
    isInitialized = true;

    return ResultSuccess();
}

}