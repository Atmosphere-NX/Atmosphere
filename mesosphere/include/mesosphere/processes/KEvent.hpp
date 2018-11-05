#pragma once

class KProcess;

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KWritableEvent.hpp>

namespace mesosphere
{

class KEvent final : public KAutoObject, IClientServerParent<KEvent, KReadableEvent, KWritableEvent> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, Event);
    
    explicit KEvent() : owner(nullptr), is_initialized(false) {}
    virtual ~KEvent() {}
    
    KProcess *GetOwner() const { return this->owner; }
    void Initialize();

    /* KAutoObject */
    virtual bool IsAlive() const override { return is_initialized; }
    
    /* IClientServerParent */
    void HandleServerDestroyed() { }
    void HandleClientDestroyed() { }
    
    private:
    KProcess *owner;
    bool is_initialized;
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
