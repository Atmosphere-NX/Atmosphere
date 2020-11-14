/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <exosphere.hpp>

namespace ams::log {

    namespace {

        constexpr inline uart::Port UartLogPort = uart::Port_ReservedDebug;
        constexpr inline int UartBaudRate = 115200;
        constinit bool g_initialized_uart = false;

        constexpr inline u32 UartPortFlags = [] {
            if constexpr (UartLogPort == uart::Port_ReservedDebug) {
                /* Logging to the debug port. */
                /* Don't invert transactions. */
                return uart::Flag_None;
            } else if constexpr (UartLogPort == uart::Port_LeftJoyCon) {
                /* Logging to left joy-con (e.g. with Joyless). */
                /* Invert transactions. */
                return uart::Flag_Inverted;
            } else if constexpr (UartLogPort == uart::Port_RightJoyCon) {
                /* Logging to right joy-con (e.g. with Joyless). */
                /* Invert transactions. */
                return uart::Flag_Inverted;
            } else {
                __builtin_unreachable();
            }
        }();

        ALWAYS_INLINE void SetupUart() {
            if constexpr (UartLogPort == uart::Port_ReservedDebug) {
                /* Logging to the debug port. */
                pinmux::SetupUartA();
                clkrst::EnableUartAClock();
            } else if constexpr (UartLogPort == uart::Port_LeftJoyCon) {
                /* Logging to left joy-con (e.g. with Joyless). */
                static_assert(uart::Port_LeftJoyCon == uart::Port_C);
                pinmux::SetupUartC();
                clkrst::EnableUartCClock();
            } else if constexpr (UartLogPort == uart::Port_RightJoyCon) {
                /* Logging to right joy-con (e.g. with Joyless). */
                static_assert(uart::Port_RightJoyCon == uart::Port_B);
                pinmux::SetupUartB();
                clkrst::EnableUartBClock();
            } else {
                __builtin_unreachable();
            }
        }

    }

    void Initialize() {
        /* Initialize pinmux and clock for the target uart port. */
        SetupUart();

        /* Initialize the target uart port. */
        uart::Initialize(UartLogPort, 115200, UartPortFlags);

        /* Note that we've initialized. */
        g_initialized_uart = true;
    }

    void Finalize() {
        g_initialized_uart = false;
    }

    NOINLINE void VPrintf(const char *fmt, ::std::va_list vl) {
        /* TODO: What's a good size for the log buffer? Nintendo uses 0x100, but this seems big. */
        char log_buf[0x80];
        const auto len = util::TVSNPrintf(log_buf, sizeof(log_buf), fmt, vl);

        if (g_initialized_uart) {
            uart::SendText(UartLogPort, log_buf, len);
        }
    }

    NOINLINE void Printf(const char *fmt, ...)  {
        ::std::va_list vl;
        va_start(vl, fmt);
        VPrintf(fmt, vl);
        va_end(vl);
    }

    NOINLINE void Dump(const void *src, size_t size) {
        const u8 *src_u8 = static_cast<const u8 *>(src);

        for (size_t i = 0; i < size; ++i) {
            if ((i % 0x20) == 0x00) {
                Printf("%03zx| ", i);
            }
            Printf("%02x ", src_u8[i]);
            if ((i % 0x20) == 0x1F) {
                Printf("\n");
            }
        }
        if ((size % 0x20) != 0) {
            Printf("\n");
        }
     }


    void SendText(const void *text, size_t size) {
        if (g_initialized_uart) {
            uart::SendText(UartLogPort, text, size);
        }
    }

    void Flush() {
        if (g_initialized_uart) {
            uart::WaitFlush(UartLogPort);
        }
    }

}
