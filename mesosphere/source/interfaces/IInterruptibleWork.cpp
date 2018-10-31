#include <mesosphere/interfaces/IInterruptibleWork.hpp>

namespace mesosphere
{

IWork *IInterruptibleWork::HandleInterrupt(uint interruptId)
{
    (void)interruptId;
    return (IWork *)this;
}

}
