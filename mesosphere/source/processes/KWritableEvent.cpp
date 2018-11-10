#include <mesosphere/processes/KWritableEvent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KEvent.hpp>

namespace mesosphere
{

KWritableEvent::~KWritableEvent()
{
}

Result KWritableEvent::Signal()
{
    return client->Signal();
}

Result KWritableEvent::Clear()
{
    return client->Clear();
}

}
