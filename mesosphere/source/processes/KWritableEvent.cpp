#include <mesosphere/processes/KWritableEvent.hpp>
#include <mesosphere/processes/KReadableEvent.hpp>
#include <mesosphere/processes/KEvent.hpp>

namespace mesosphere
{

Result KWritableEvent::Signal() {
    return this->client->Signal();
}

Result KWritableEvent::Clear() {
    return this->client->Clear();
}

}
