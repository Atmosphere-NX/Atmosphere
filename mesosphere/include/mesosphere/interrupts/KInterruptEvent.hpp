#pragma once

#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/interfaces/ISetAllocated.hpp>

namespace mesosphere
{

class KInterruptEvent final : public KReadableEvent, public ISetAllocated<KInterruptEvent> {
    public:
    MESOSPHERE_AUTO_OBJECT_TRAITS(ReadableEvent, InterruptEvent);

    void *operator new(size_t sz) noexcept { return ISetAllocated<KInterruptEvent>::operator new(sz); }
    void operator delete(void *ptr) noexcept { ISetAllocated<KInterruptEvent>::operator delete(ptr); }

    virtual ~KInterruptEvent();
    Result Initialize(int irqId, u32 flags);

    // KAutoObject
    virtual bool IsAlive() const override;

    private:
    // TODO: receiver
    int irqId = -1;
    bool isInitialized = false;
};

inline void intrusive_ptr_add_ref(KInterruptEvent *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KInterruptEvent *obj)
{
    intrusive_ptr_release((KAutoObject *)obj);
}

}
