#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/arch/arm64/arch.hpp>

namespace mesosphere
{
inline namespace arch
{
inline namespace arm64
{

// Dummy. Should be platform-independent:

class KInterruptMaskGuard final {
    public:

    using FlagsType = u64;

    KInterruptMaskGuard()
    {
        flags = MaskInterrupts();
    }

    ~KInterruptMaskGuard()
    {
        RestoreInterrupts(flags);
    }

    KInterruptMaskGuard(const KInterruptMaskGuard &) = delete;
    KInterruptMaskGuard(KInterruptMaskGuard &&) = delete;
    KInterruptMaskGuard &operator=(const KInterruptMaskGuard &) = delete;
    KInterruptMaskGuard &operator=(KInterruptMaskGuard &&) = delete;

    private:
    u64 flags = 0;
};

}
}
}
