#pragma once

#include <utility>
#include <mesosphere/core/types.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>

#define MESOSPHERE_CLIENT_TRAITS(ParentId) using ParentClass = K##ParentId;

namespace mesosphere
{

struct IClientTag {};

template<typename Parent, typename Client, typename Server>
class IClient : public IClientTag {
    public:
    using ParentClass = Parent;
    using ClientClass = Client;
    using ServerClass = Server;

    void *operator new(size_t sz) noexcept { for (;;); }
    void operator delete(void *ptr) noexcept {}

    ~IClient()
    {
        parent->HandleClientDestroyed();
    }

    const SharedPtr<Parent>& GetParent() const { return parent; }

    void SetParent(SharedPtr<Parent> parent)
    {
        this->parent = std::move(parent);
    }

    protected:
    SharedPtr<Parent> parent{};
};

}
