#pragma once

#include <mesosphere/interfaces/IWork.hpp>
#include <mesosphere/interfaces/IInterruptible.hpp>

namespace mesosphere
{

class IInterruptibleWork : public IInterruptible, public IWork {
    public:

    virtual IWork *HandleInterrupt(uint interruptId) override;
};

}
