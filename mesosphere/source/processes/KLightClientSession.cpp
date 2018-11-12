#include <mesosphere/processes/KLightClientSession.hpp>
#include <mesosphere/processes/KLightSession.hpp>
#include <mesosphere/threading/KScopedCriticalSection.hpp>

namespace mesosphere
{

KLightClientSession::~KLightClientSession()
{
    parent->Terminate(false);
}


}
