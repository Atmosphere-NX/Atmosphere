#pragma once

#include <mesosphere/core/types.hpp>

namespace mesosphere
{

template<typename Parent, typename Client, typename Server>
class IClientServerParent {
    public:
    using ParentType = Parent;
    using ClientType = Client;
    using ServerType = Server;

    protected:
    void SetClientServerParent()
    {
        Parent par = (Parent *)this;
        client.SetParent(par);
        server.SetParent(par);
    }

    ClientType client{};
    ServerType server{};
};

}
