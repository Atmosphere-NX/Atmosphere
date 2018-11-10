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

    void *operator new(size_t sz) noexcept = delete;
    void operator delete(void *ptr) noexcept {}

    const SharedPtr<ParentClass>& GetParent() const { return parent; }

    protected:
    friend class IClientServerParent<ParentClass, ClientClass, ServerClass>;
    SharedPtr<ParentClass> parent{};
};

}
