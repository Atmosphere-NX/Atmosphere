#pragma once

class KProcess;

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/interfaces/ILimitedResource.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KWritableEvent.hpp>

namespace mesosphere
{

class KEvent final :
    public KAutoObject,
    public IClientServerParent<KEvent, KReadableEvent, KWritableEvent>,
    public ISetAllocated<KEvent>,
    public ILimitedResource<KEvent> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, Event);

    virtual ~KEvent();

    Result Initialize();

    /* KAutoObject */
    virtual bool IsAlive() const override;
    
    /* IClientServerParent */
    void HandleServerDestroyed() { }
    void HandleClientDestroyed() { }
    
    private:
    bool isInitialized = false;
};

inline void intrusive_ptr_add_ref(KEvent *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KEvent *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
