#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/interfaces/IServer.hpp>
#include <mesosphere/threading/KThread.hpp>

#include <boost/intrusive/list.hpp>

namespace mesosphere
{

class KLightClientSession;
class KLightSession;
class KClientPort;

struct LightServerSessionListTag;
using  LightServerSessionListBaseHook = boost::intrusive::list_base_hook<boost::intrusive::tag<LightServerSessionListTag> >;

class KLightServerSession final :
    public KSynchronizationObject,
    public IServer<KLightSession, KLightClientSession, KLightServerSession>,
    public LightServerSessionListBaseHook {

    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(SynchronizationObject, LightServerSession);

    using List = typename boost::intrusive::make_list<
        KLightServerSession,
        boost::intrusive::base_hook<LightServerSessionListBaseHook>,
        boost::intrusive::constant_time_size<false>
    >::type;

    virtual ~KLightServerSession();

    virtual bool IsSignaled() const override;

    private:
    friend class KLightSession;

    void Terminate(bool fromServer);

    KThread::WaitList senderThreads{}, receiverThreads{};
    SharedPtr<KThread> currentSender{};
    KThread *currentReceiver = nullptr;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(LightServerSession);

}
