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

    ParentClass *GetParent() const { return parent.get(); }

    protected:
    friend class IClientServerParent<ParentClass, ClientClass, ServerClass>;
    SharedPtr<ParentClass> parent{};
};

}
