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

    ClientClass &GetClient() { return client; }
    ServerClass &GetServer() { return server; }

    protected:

    void SetClientServerParent()
    {
        ParentClass *par = (ParentClass *)this;
        client.parent = par;
        server.parent = par;
    }

    ClientClass client{};
    ServerClass server{};
};

}
