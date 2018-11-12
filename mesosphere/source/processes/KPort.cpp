#include <mesosphere/processes/KPort.hpp>
#include <mesosphere/core/KCoreContext.hpp>

namespace mesosphere
{

KPort::~KPort()
{
}

Result KPort::Initialize()
{
    SetClientServerParent();
    isClientAlive = true;
    isServerAlive = true;

    return ResultSuccess();
}

}
