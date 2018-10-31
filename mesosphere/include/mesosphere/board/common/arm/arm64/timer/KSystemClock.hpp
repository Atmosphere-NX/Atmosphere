#pragma once

#include <chrono>
#include <mesosphere/core/util.hpp>
#include <mesosphere/arch/arm64/arm64.hpp>

#ifndef MESOSPHERE_SYSTEM_CLOCK_RATE // NEEDS to be defined; depends on cntfreq
#define MESOSPHERE_SYSTEM_CLOCK_RATE 192000000ull
#endif

// Architectural aarch64 armv8 timer

namespace mesosphere
{
inline namespace board
{
inline namespace common
{
inline namespace arm
{
inline namespace arm64
{

// Dummy implementation
// Needs to be changed for platform stuff

using namespace std::chrono_literals;

/// Fulfills Clock named requirements
class KSystemClock {
    public:

    using rep           = s64;
    using period        = std::ratio<1, MESOSPHERE_SYSTEM_CLOCK_RATE>;
    using duration      = std::chrono::duration<rep, period>;
    using time_point    = std::chrono::time_point<KSystemClock>;

    static constexpr bool is_steady = true;

    static time_point now()
    {
        return time_point{duration::zero()};
    }

    static constexpr bool isCorePrivate = true;
    static constexpr duration forever = duration{-1};
    static constexpr time_point never = time_point{forever};

    static constexpr uint GetIrqId() { return 30; }

    static void Disable()
    {
        // Note: still continues counting.
        MESOSPHERE_WRITE_SYSREG(0, cntp_ctl_el0);
    }

    static void SetInterruptMasked(bool maskInterrupts)
    {
        u64 val = maskInterrupts ? 3 : 1; // Note: also enables the timer.
        MESOSPHERE_WRITE_SYSREG(val, cntp_ctl_el0);
    }

    static void SetAlarm(const time_point &when)
    {
        u64 val = (u64)when.time_since_epoch().count();
        MESOSPHERE_WRITE_SYSREG(val, cntp_cval_el0);
        SetInterruptMasked(false);
    }

    static void Initialize()
    {
        MESOSPHERE_WRITE_SYSREG(1, cntkctl_el1); // Trap register accesses from el0.
        Disable();
        MESOSPHERE_WRITE_SYSREG(UINT64_MAX, cntp_cval_el0);
        SetInterruptMasked(true);
    }
};

}
}
}
}
}
