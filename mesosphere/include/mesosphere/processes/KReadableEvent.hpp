#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/core/KSynchronizationObject.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>
#include <mesosphere/interfaces/IClient.hpp>

namespace mesosphere
{

class KWritableEvent;
class KEvent;

// Inherited by KInterruptEvent
class KReadableEvent : public KSynchronizationObject, public IClient<KEvent, KReadableEvent, KWritableEvent> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(SynchronizationObject, ReadableEvent);
    MESOSPHERE_CLIENT_TRAITS(Event);

    virtual bool IsAlive() const override { return true; }

    virtual ~KReadableEvent();

    Result Signal();
    Result Clear();
    Result Reset();

    virtual bool IsSignaled() const override;

    private:
    bool isSignaled = false;
};

inline void intrusive_ptr_add_ref(KReadableEvent *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KReadableEvent *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
