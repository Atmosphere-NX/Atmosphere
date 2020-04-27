
#pragma once
#include <stratosphere.hpp>

namespace ams::lm {

    enum class LogDestination : u32 {
        TMA = 1,
        UART = 2,
        UARTSleeping = 4,
        All = 0xFFFF,
    };

}