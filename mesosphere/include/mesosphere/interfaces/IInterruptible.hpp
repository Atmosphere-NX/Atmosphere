#pragma once

#include <mesosphere/core/util.hpp>

namespace mesosphere
{

class IWork;

class IInterruptible {
    public:

    /// Top half in Linux jargon
    virtual IWork *HandleInterrupt(uint interruptId) = 0;
};
}
