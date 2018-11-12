#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/processes/KClientPort.hpp>
#include <mesosphere/processes/KServerPort.hpp>

namespace mesosphere
{

class KPort final :
    public KAutoObject,
    public ISetAllocated<KPort>,
    public IClientServerParent<KPort, KClientPort, KServerPort> {

    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, Port);

    virtual ~KPort();

    Result Initialize();

    private:
    friend class KClientPort;
    friend class KServerPort;

    bool isClientAlive = false;
    bool isServerAlive = false;
    bool isLight = false;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(Port);

}