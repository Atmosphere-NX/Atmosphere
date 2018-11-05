#pragma once

#include <mesosphere/core/types.hpp>

#define MESOSPHERE_CLIENT_SERVER_PARENT_TRAITS(ClientId, ServerId)\
using ClientClass = K##ClientId;\
using ServerClass = K##ServerId;

namespace mesosphere
{

struct IClientServerParentTag {};

template<typename Parent, typename Client, typename Server>
class IClientServerParent : public IClientServerParentTag {
    public:
    using ParentClass = Parent;
    using ClientClass = Client;
    using ServerClass = Server;

    void SetClientServerParent()
    {
        Parent *par = (Parent *)this;
        client.SetParent(par);
        server.SetParentAndClient(par, &client);
    }

    protected:

    ClientClass client{};
    ServerClass server{};
};

}
