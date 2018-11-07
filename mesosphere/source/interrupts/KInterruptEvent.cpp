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
    return ResultSuccess();
}

}
