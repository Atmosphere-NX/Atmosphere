#pragma once

#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/interfaces/IClient.hpp>
#include <mesosphere/threading/KThread.hpp>

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

    private:
    friend class KPort;

};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(ClientPort);

}