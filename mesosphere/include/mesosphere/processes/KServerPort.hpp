#pragma once

#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/processes/KLightServerSession.hpp>

namespace mesosphere
{

class KPort;
class KClientPort;

class KServerPort final :
    public KSynchronizationObject,
    public IServer<KPort, KClientPort, KServerPort> {

    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(SynchronizationObject, ServerPort);

    virtual ~KServerPort();

    virtual bool IsSignaled() const override;

    private:
    friend class KPort;
    Result AddLightServerSession(KLightServerSession &lightServerSession);

    KLightServerSession::List lightServerSessions{};
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(ServerPort);

}
