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
        constinit bool g_initialized_uart = false;
        constinit bool g_logging_enabled  = false;

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
                pinmux::SetupUartB();
                clkrst::EnableUartBClock();
            } else if constexpr (UartLogPort == uart::Port_RightJoyCon) {
                /* Logging to right joy-con (e.g. with Joyless). */
                pinmux::SetupUartC();
                clkrst::EnableUartCClock();
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

    void SetDebugLogEnabled(bool en) {
        g_logging_enabled = en;
    }

    void SendText(const void *text, size_t size) {
        if (g_initialized_uart && g_logging_enabled) {
            uart::SendText(UartLogPort, text, size);
        }
    }

    void Flush() {
        if (g_initialized_uart && g_logging_enabled) {
            uart::WaitFlush(UartLogPort);
        }
    }

}