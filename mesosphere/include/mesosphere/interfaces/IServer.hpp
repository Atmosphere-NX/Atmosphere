#pragma once

#include <utility>
#include <mesosphere/core/types.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>

namespace mesosphere
{

template<typename Parent, typename Client, typename Server>
class IServer {
    public:
    using ParentType = Parent;
    using ClientType = Client;
    using ServerType = Server;

    ~IServer()
    {
        parent->HandleServerDestroyed();
    }
    
    ParentType *GetParent() const { return parent; }

    protected:
    void SetParent(SharedPtr<Parent> parent)
    {
        this->parent = std::move(parent);
        this->client = &this->parent->client;
    }

    SharedPtr<Parent> parent{};
    SharedPtr<Client> client{};
};

}