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
#include <mesosphere.hpp>
#include "kern_debug_log_impl.hpp"

namespace ams::kern {

    namespace {

        enum UartRegister {
            UartRegister_THR = 0,
            UartRegister_IER = 1,
            UartRegister_FCR = 2,
            UartRegister_LCR = 3,

            UartRegister_LSR = 5,

            UartRegister_IRSA_CSR = 8,

            UartRegister_DLL = 0,
            UartRegister_DLH = 1,
        };

        KVirtualAddress g_uart_address = 0;

        NOINLINE u32 ReadUartRegister(UartRegister which) {
            return GetPointer<volatile u32>(g_uart_address)[which];
        }

        NOINLINE void WriteUartRegister(UartRegister which, u32 value) {
            GetPointer<volatile u32>(g_uart_address)[which] = value;
        }

    }

    bool KDebugLogImpl::Initialize() {
        /* Set the uart register base address. */
        g_uart_address = KMemoryLayout::GetUartAddress();

        /* Parameters for uart. */
        constexpr u32 BaudRate = 115200;
        constexpr u32 Pllp = 408000000;
        constexpr u32 Rate = 16 * BaudRate;
        constexpr u32 Divisor = (Pllp + Rate / 2) / Rate;

        /* Initialize the UART registers. */
        /* Set Divisor Latch Access bit, to allow access to DLL/DLH */
        WriteUartRegister(UartRegister_LCR, 0x80);
        ReadUartRegister(UartRegister_LCR);

        /* Program the divisor into DLL/DLH. */
        WriteUartRegister(UartRegister_DLL, Divisor & 0xFF);
        WriteUartRegister(UartRegister_DLH, (Divisor >> 8) & 0xFF);
        ReadUartRegister(UartRegister_DLH);

        /* Set word length to 3, clear Divisor Latch Access. */
        WriteUartRegister(UartRegister_LCR, 0x03);
        ReadUartRegister(UartRegister_LCR);

        /* Disable UART interrupts. */
        WriteUartRegister(UartRegister_IER, 0x00);

        /* Configure the FIFO to be enabled and clear receive. */
        WriteUartRegister(UartRegister_FCR, 0x03);
        WriteUartRegister(UartRegister_IRSA_CSR, 0x02);
        ReadUartRegister(UartRegister_FCR);

        return true;
    }

    void KDebugLogImpl::PutChar(char c) {
        while (ReadUartRegister(UartRegister_LSR) & 0x100) {
            /* While the FIFO is full, yield. */
            __asm__ __volatile__("yield" ::: "memory");
        }
        WriteUartRegister(UartRegister_THR, c);
        cpu::DataSynchronizationBarrier();
    }

    void KDebugLogImpl::Flush() {
        while ((ReadUartRegister(UartRegister_LSR) & 0x40) == 0) {
            /* Wait for the TMTY bit to be one (transmit empty). */
        }
    }

}
