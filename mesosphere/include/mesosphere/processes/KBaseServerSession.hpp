#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/interfaces/IServer.hpp>

#include <boost/intrusive/list.hpp>

namespace mesosphere
{

class KBaseClientSession;
class KBaseSession;
class KClientPort;

struct ServerSessionListTag;
using  ServerSessionListBaseHook = boost::intrusive::list_base_hook<boost::intrusive::tag<ServerSessionListTag> >;

class KBaseServerSession :
    public KSynchronizationObject,
    public IServer<KBaseSession, KBaseClientSession, KBaseServerSession>,
    public ServerSessionListBaseHook {
    // Note: hidden from the KAutoObject hierarchy

    public:

    using List = typename boost::intrusive::make_list<
        KBaseServerSession,
        boost::intrusive::base_hook<ServerSessionListBaseHook>,
        boost::intrusive::constant_time_size<false>
    >::type;

    virtual ~KBaseServerSession();

    // For covariant types
    virtual KBaseSession *GetParentSession() const { return parent.get(); }

    virtual bool IsSignaled() const override { return false; } // hacky; to make it non-abstract

    protected:

    friend class KBaseSession;

    KBaseServerSession() = default;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(BaseServerSession);

}
