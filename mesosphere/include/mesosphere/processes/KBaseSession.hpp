#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/processes/KBaseClientSession.hpp>
#include <mesosphere/processes/KBaseServerSession.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>

namespace mesosphere
{

class KBaseSession : public KAutoObject, public IClientServerParent<KBaseSession, KBaseClientSession, KBaseServerSession> {
    // Note: hidden from the KAutoObject hierarchy

    public:

    virtual ~KBaseSession();

    // For covariant types:
    virtual KBaseClientSession *GetClientSession() { return &client; }
    virtual KBaseServerSession *GetServerSession() { return &server; }

    protected:

    Result Initialize();
    KBaseSession() = default;
};

inline void intrusive_ptr_add_ref(KBaseSession *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KBaseSession *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
