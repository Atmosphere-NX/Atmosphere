#include <mesosphere/processes/KBaseServerSession.hpp>

namespace mesosphere
{

KBaseServerSession::~KBaseServerSession()
{
    // Children classes will lock critical section + set client "isRemoteAlive"
}

}
