#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KSynchronizationObject.hpp>
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

    virtual ~KReadableEvent();

    Result Signal();
    Result Clear();
    Result Reset();

    virtual bool IsSignaled() const override;

    private:
    bool isSignaled = false;
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(ReadableEvent);

}
