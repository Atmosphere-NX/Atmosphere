#pragma once

#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/interfaces/IClient.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <tuple>

namespace mesosphere
{

class KPort;
class KServerPort;

class KClientPort final :
    public KSynchronizationObject,
    public IClient<KPort, KClientPort, KServerPort> {

    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(SynchronizationObject, ClientPort);

    virtual ~KClientPort();

    virtual bool IsSignaled() const override;

    std::tuple<Result, SharedPtr<KLightClientSession>> ConnectLight();

    private:
    friend class KPort;

    std::atomic<int> numSessions{0};
    std::atomic<int> peakNumNormalSessions{0};
    int maxSessions = 0;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(ClientPort);

}
