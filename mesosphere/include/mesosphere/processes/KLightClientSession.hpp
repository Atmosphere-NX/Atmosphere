#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/IClient.hpp>

namespace mesosphere
{

class KLightSession;
class KClientPort;

class KLightClientSession final : public KAutoObject, public IClient<KLightSession, KLightClientSession, KLightServerSession> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, LightClientSession);

    virtual ~KLightClientSession();

    private:
    friend class KLightSession;

    KClientPort *parentPort = nullptr;
    bool isRemoteActive = false;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(LightClientSession);

}
