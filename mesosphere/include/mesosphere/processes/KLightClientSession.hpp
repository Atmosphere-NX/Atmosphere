#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/IClient.hpp>

namespace mesosphere
{

class KLightSession;
class KClientPort;

struct LightSessionRequest;

class KLightClientSession final : public KAutoObject, public IClient<KLightSession, KLightClientSession, KLightServerSession> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, LightClientSession);

    virtual ~KLightClientSession();

    Result SendSyncRequest(LightSessionRequest *request);

    private:
    friend class KLightSession;

    KClientPort *parentPort = nullptr;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(LightClientSession);

}
