#include <mesosphere/processes/KBaseSession.hpp>

namespace mesosphere
{

KBaseSession::~KBaseSession()
{
}

Result KBaseSession::Initialize()
{
    SetClientServerParent();

    return ResultSuccess();
}

}
