#pragma once

#include <utility>
#include <mesosphere/core/types.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>

#define MESOSPHERE_SERVER_TRAITS(ParentId) using ParentClass = K##ParentId;

namespace mesosphere
{

struct IServerTag {};

template<typename Parent, typename Client, typename Server>
class IServer : public IServerTag {
    public:
    using ParentClass = Parent;
    using ClientClass = Client;
    using ServerClass = Server;

    void *operator new(size_t sz) noexcept = delete;
    void operator delete(void *ptr) noexcept {};

    ~IServer()
    {
        parent->HandleServerDestroyed();
    }

    const SharedPtr<Parent> &GetParent() const { return parent; }

    void SetParentAndClient(SharedPtr<Parent> parent, SharedPtr<Client> client)
    {
        this->parent = std::move(parent);
        this->client = std::move(client);
    }

    protected:

    SharedPtr<Parent> parent{};
    SharedPtr<Client> client{};
};

}
