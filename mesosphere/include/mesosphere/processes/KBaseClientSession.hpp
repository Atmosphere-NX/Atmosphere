#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/IClient.hpp>

namespace mesosphere
{

class KBaseServerSession;
class KBaseSession;
class KClientPort;
class KBaseClientSession : public KAutoObject, public IClient<KBaseSession, KBaseClientSession, KBaseServerSession> {
    // Note: hidden from the KAutoObject hierarchy

    public:

    virtual ~KBaseClientSession();

    // For covariant types
    virtual KBaseSession *GetParentSession() const { return parent.get(); }

    protected:

    friend class KBaseSession;
    friend class KBaseServerSession;

    KBaseClientSession() = default;

    bool isRemoteActive = false;
    KClientPort *parentPort = nullptr;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(BaseClientSession);

}
