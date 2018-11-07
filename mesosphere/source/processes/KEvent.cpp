#include <mesosphere/processes/KEvent.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/processes/KProcess.hpp>

namespace mesosphere
{

KEvent::~KEvent()
{
}

Result KEvent::Initialize()
{
    SetClientServerParent();
    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());

    return ResultSuccess();
}

}
