#pragma once

#include <boost/preprocessor.hpp>
#include <mesosphere/core/types.hpp>

#define MESOSPHERE_READ_SYSREG(r) ({\
    u64 __val;                                                      \
    asm volatile("mrs %0, " BOOST_PP_STRINGIZE(r) : "=r" (__val));  \
    __val;                                                          \
})

#define MESOSPHERE_WRITE_SYSREG(v, r) do {                          \
    u64 __val = (u64)v;                                             \
    asm volatile("msr " BOOST_PP_STRINGIZE(r) ", %0"                \
    :: "r" (__val));                                                \
} while (false)

#define MESOSPHERE_DAIF_BIT(v)  (((u64)(v)) >> 6)

namespace mesosphere
{
inline namespace arch
{
inline namespace arm64
{

enum PsrMode {
    PSR_MODE_EL0t = 0x0u,
    PSR_MODE_EL1t = 0x4u,
    PSR_MODE_EL1h = 0x5u,
    PSR_MODE_EL2t = 0x8u,
    PSR_MODE_EL2h = 0x9u,
    PSR_MODE_EL3t = 0xCu,
    PSR_MODE_EL3h = 0xDu,
    PSR_MODE_MASK = 0xFu,
    PSR_MODE32_BIT = 0x10u,
};

enum PsrInterruptBit {
    PSR_F_BIT = 1u << 6,
    PSR_I_BIT = 1u << 7,
    PSR_A_BIT = 1u << 8,
    PSR_D_BIT = 1u << 9,
};

enum PsrStatusBit {
    PSR_PAN_BIT = 1u << 22,
    PSR_UAO_BIT = 1u << 23,
};

enum PsrFlagBit {
    PSR_V_BIT = 1u << 28,
    PSR_C_BIT = 1u << 29,
    PSR_Z_BIT = 1u << 30,
    PSR_N_BIT = 1u << 31,
};

enum PsrBitGroup {
    PSR_c = 0x000000FFu,
    PSR_x = 0x0000FF00u,
    PSR_s = 0x00FF0000u,
    PSR_f = 0xFF000000u,
};

}
}
}
