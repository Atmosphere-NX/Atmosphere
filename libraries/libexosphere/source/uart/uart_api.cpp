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
#include "uart_registers.hpp"

namespace ams::uart {

    namespace {

        constexpr inline const u16 UartRegisterOffsets[Port_Count] = {
            secmon::MemoryRegionPhysicalDeviceUartA.GetAddress() - secmon::MemoryRegionPhysicalDeviceUart.GetAddress(),
            secmon::MemoryRegionPhysicalDeviceUartB.GetAddress() - secmon::MemoryRegionPhysicalDeviceUart.GetAddress(),
            secmon::MemoryRegionPhysicalDeviceUartC.GetAddress() - secmon::MemoryRegionPhysicalDeviceUart.GetAddress(),
        };

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceUart.GetAddress();

        volatile UartRegisters *GetRegisters(Port port) {
            return reinterpret_cast<volatile UartRegisters *>(g_register_address + UartRegisterOffsets[port]);
        }

        void WaitSymbols(int baud, u32 num) {
            util::WaitMicroSeconds(util::DivideUp(num * 1'000'000, baud));
        }

        void WaitCycles(int baud, u32 num) {
            util::WaitMicroSeconds(util::DivideUp(num * 1'000'000, 16 * baud));
        }

        ALWAYS_INLINE void WaitFifoNotFull(volatile UartRegisters *uart) {
            while ((uart->lsr & UART_LSR_TX_FIFO_FULL) != 0) { /* ... */ }
        }

        ALWAYS_INLINE void WaitFifoNotEmpty(volatile UartRegisters *uart) {
            while ((uart->lsr & UART_LSR_RX_FIFO_EMPTY) != 0) { /* ... */ }
        }

        void WaitIdle(volatile UartRegisters *uart, u32 vendor_state) {
            if (vendor_state & UART_VENDOR_STATE_TX_IDLE) {
                while ((uart->lsr & UART_LSR_TMTY) == 0) { /* ... */ }
            }

            if (vendor_state & UART_VENDOR_STATE_RX_IDLE) {
                while ((uart->lsr & UART_LSR_RDR) != 0) { /* ... */ }
            }
        }

        constexpr inline u32 LockBit = (1 << 6);

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void Initialize(Port port, int baud_rate, u32 flags)  {
        /* Get the registers. */
        auto *uart = GetRegisters(port);

        /* Parse flags. */
        const bool inverted = (flags & Flag_Inverted) != 0;

        /* Calculate the baud rate divisor. */
        constexpr u32 UartClock = 408000000;
        const     u32 divisor = (UartClock + (baud_rate * 16) / 2) / (baud_rate * 16);

        /* Wait for idle state. */
        WaitIdle(uart, UART_VENDOR_STATE_TX_IDLE);

        /* Wait 100 us. */
        util::WaitMicroSeconds(100);

        /* Disable interrupts. */
        uart->lcr = uart->lcr & ~UART_LCR_DLAB;
        uart->ier = 0;
        uart->mcr = 0;

        /* Setup the uart in FIFO mode. */
        uart->lcr = UART_LCR_DLAB | UART_LCR_WD_LENGTH_8;
        uart->dll = static_cast<u8>(divisor);
        uart->dlh = static_cast<u8>(divisor >> 8);
        uart->lcr = uart->lcr & ~UART_LCR_DLAB;
        reg::Read(std::addressof(uart->spr));

        /* Wait three symbols. */
        WaitSymbols(baud_rate, 3);

        /* Enable FIFO with default settings. */
        uart->fcr = UART_FCR_FCR_EN_FIFO;
        uart->irda_csr = inverted ? UART_IRDA_CSR_INVERT_TXD : 0;
        reg::Read(std::addressof(uart->spr));

        /* Wait three cycles. */
        WaitCycles(baud_rate, 3);

        /* Flush the FIFO. */
        WaitIdle(uart, UART_VENDOR_STATE_TX_IDLE);
        uart->fcr = uart->fcr | UART_FCR_RX_CLR | UART_FCR_TX_CLR;
        WaitCycles(baud_rate, 32);

        /* Wait for idle state. */
        WaitIdle(uart, UART_VENDOR_STATE_TX_IDLE | UART_VENDOR_STATE_RX_IDLE);

        /* Wait 100 us. */
        util::WaitMicroSeconds(100);
    }

    void SendText(Port port, const void *data, size_t size) {
        /* Get the registers. */
        auto *uart = GetRegisters(port);

        /* Get pointer to data. */
        const u8 *p = static_cast<const u8 *>(data);

        /* Send each byte. */
        for (size_t i = 0; i < size; ++i) {
            WaitFifoNotFull(uart);

            if (p[i] == '\n') {
                *reinterpret_cast<volatile u8 *>(std::addressof(uart->thr)) = '\r';
                WaitFifoNotFull(uart);
            }

            *reinterpret_cast<volatile u8 *>(std::addressof(uart->thr)) = p[i];
        }
    }

    void WaitFlush(Port port) {
        /* Get the registers. */
        auto *uart = GetRegisters(port);

        /* Wait for idle. */
        WaitIdle(uart, UART_VENDOR_STATE_TX_IDLE);
    }

}
