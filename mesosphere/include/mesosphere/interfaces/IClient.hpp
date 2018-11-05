#pragma once

#include <utility>
#include <mesosphere/core/types.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>

namespace mesosphere
{

template<typename Parent, typename Client, typename Server>
class IClient {
    public:
    using ParentType = Parent;
    using ClientType = Client;
    using ServerType = Server;

    ~IClient()
    {
        parent->HandleClientDestroyed();
    }

    ParentType *GetParent() const { return parent; }
    
    protected:
    void SetParent(SharedPtr<Parent> parent)
    {
        this->parent = std::move(parent);
    }

    SharedPtr<Parent> parent{};
};

}