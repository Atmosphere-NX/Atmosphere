#pragma once

#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/core/KLinkedList.hpp>

namespace mesosphere
{

class KSynchronizationObject : public KAutoObject {
    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, SynchronizationObject);

    virtual ~KSynchronizationObject();
    virtual bool IsSignaled() const = 0;

    void NotifyWaiters(); // Note: NotifyWaiters() with !IsSignaled() is no-op

    KLinkedList<KThread *>::const_iterator AddWaiter(KThread &thread);
    void RemoveWaiter(KLinkedList<KThread *>::const_iterator it);

    private:
    KLinkedList<KThread *> waiters{};
};

MESOSPHERE_AUTO_OBJECT_DEFINE_INCREF(SynchronizationObject);

}
