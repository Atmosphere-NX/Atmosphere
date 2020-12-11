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

        constexpr inline uart::Port DefaultLogPort = uart::Port_ReservedDebug;
        constexpr inline u32 DefaultLogFlags = static_cast<u32>(uart::Flag_None);
        constexpr inline int DefaultBaudRate = 115200;
        constinit uart::Port g_log_port   = DefaultLogPort;
        constinit bool g_initialized_uart = false;

        ALWAYS_INLINE void SetupUartClock(uart::Port port) {
            /* The debug port must always be set up, for compatibility with official hos. */
            pinmux::SetupUartA();
            clkrst::EnableUartAClock();

            /* If logging to a joy-con port, configure appropriately. */
            if (port == uart::Port_LeftJoyCon) {
                /* Logging to left joy-con (e.g. with Joyless). */
                static_assert(uart::Port_LeftJoyCon == uart::Port_C);
                pinmux::SetupUartC();
                clkrst::EnableUartCClock();
            } else if (port == uart::Port_RightJoyCon) {
                /* Logging to right joy-con (e.g. with Joyless). */
                static_assert(uart::Port_RightJoyCon == uart::Port_B);
                pinmux::SetupUartB();
                clkrst::EnableUartBClock();
            }
        }

    }

    void Initialize() {
        return Initialize(DefaultLogPort, DefaultBaudRate, DefaultLogFlags);
    }

    void Initialize(uart::Port port, u32 baud_rate, u32 flags) {
        /* Initialize pinmux and clock for the target uart port. */
        SetupUartClock(port);

        /* Initialize the target uart port. */
        uart::Initialize(port, baud_rate, flags);

        /* Note that we've initialized. */
        g_log_port        = port;
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
            uart::SendText(g_log_port, log_buf, len);
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
            uart::SendText(g_log_port, text, size);
        }
    }

    void Flush() {
        if (g_initialized_uart) {
            uart::WaitFlush(g_log_port);
        }
    }

}
