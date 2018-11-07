#include <mesosphere/interrupts/KInterruptEvent.hpp>

namespace mesosphere
{

KInterruptEvent::~KInterruptEvent()
{
    // delete receiver? 
}

Result KInterruptEvent::Initialize(int irqId, u32 flags)
{
    // TODO implement
    (void)flags;
    this->irqId = irqId;
    isInitialized = true;
    return ResultSuccess();
}

bool KInterruptEvent::IsAlive() const
{
    return isInitialized;
}

}
