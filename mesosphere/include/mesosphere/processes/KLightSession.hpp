#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/interfaces/ILimitedResource.hpp>
#include <mesosphere/processes/KLightClientSession.hpp>
#include <mesosphere/processes/KLightServerSession.hpp>

namespace mesosphere
{

class KPort;

struct LightSessionRequest {
    s32 cmdId;
    u32 data[6];
};

class KLightSession final :
    public KAutoObject,
    public ISetAllocated<KLightSession>,
    public ILimitedResource<KLightSession>,
    public IClientServerParent<KLightSession, KLightClientSession, KLightServerSession> {

    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, LightSession);
    MESOSPHERE_LIMITED_RESOURCE_TRAITS(10s);

    virtual ~KLightSession();

    Result Initialize(KPort *parentPort = nullptr);

    private:
    friend class KLightClientSession;
    friend class KLightServerSession;

    void Terminate(bool fromServer);
    bool isClientAlive = false;
    bool isServerAlive = false;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(LightSession);

}
