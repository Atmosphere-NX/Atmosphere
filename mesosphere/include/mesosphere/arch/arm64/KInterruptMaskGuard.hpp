#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/arch/arm64/arm64.hpp>

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

    static FlagsType MaskInterrupts()
    {
        FlagsType flags = MESOSPHERE_READ_SYSREG(daif);
        MESOSPHERE_WRITE_SYSREG(flags | PSR_I_BIT, daif);
        return flags;
    }

    static void RestoreInterrupts(FlagsType flags)
    {
        MESOSPHERE_WRITE_SYSREG(MESOSPHERE_READ_SYSREG(daif) | (flags & PSR_I_BIT), daif);
    }

    KInterruptMaskGuard(const KInterruptMaskGuard &) = delete;
    KInterruptMaskGuard(KInterruptMaskGuard &&) = delete;
    KInterruptMaskGuard &operator=(const KInterruptMaskGuard &) = delete;
    KInterruptMaskGuard &operator=(KInterruptMaskGuard &&) = delete;

    private:
    FlagsType flags;
};

}
}
}
