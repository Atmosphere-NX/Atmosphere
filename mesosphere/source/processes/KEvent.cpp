#include <mesosphere/processes/KEvent.hpp>
#include <mesosphere/processes/KProcess.hpp>

namespace mesosphere
{

KEvent::~KEvent()
{
}

Result KEvent::Initialize()
{
    SetClientServerParent();
    server.SetClient(&client);
    SetResourceOwner(KCoreContext::GetCurrentInstance().GetCurrentProcess());

    return ResultSuccess();
}

}
