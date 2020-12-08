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

#if defined(MESOSPHERE_DEBUG_LOG_USE_UART)

    namespace {

        constexpr bool DoSaveAndRestore = false;

        enum UartRegister {
            UartRegister_THR = 0,
            UartRegister_IER = 1,
            UartRegister_FCR = 2,
            UartRegister_LCR = 3,

            UartRegister_LSR = 5,

            UartRegister_IRDA_CSR = 8,

            UartRegister_DLL = 0,
            UartRegister_DLH = 1,
        };

        KVirtualAddress g_uart_address = 0;

        [[maybe_unused]] constinit u32 g_saved_registers[5];

        ALWAYS_INLINE u32 ReadUartRegister(UartRegister which) {
            return GetPointer<volatile u32>(g_uart_address)[which];
        }

        ALWAYS_INLINE void WriteUartRegister(UartRegister which, u32 value) {
            GetPointer<volatile u32>(g_uart_address)[which] = value;
        }

    }

    bool KDebugLogImpl::Initialize() {
        /* Get the uart memory region. */
        const KMemoryRegion *uart_region = KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerived(KMemoryRegionType_Uart);
        if (uart_region == nullptr) {
            return false;
        }

        /* Set the uart register base address. */
        g_uart_address = uart_region->GetPairAddress();
        if (g_uart_address == Null<KVirtualAddress>) {
            return false;
        }

        /* NOTE: We assume here that UART init/config has been done by the Secure Monitor. */
        /* As such, we only need to disable interrupts. */
        WriteUartRegister(UartRegister_IER, 0x00);

        return true;
    }

    void KDebugLogImpl::PutChar(char c) {
        while (ReadUartRegister(UartRegister_LSR) & 0x100) {
            /* While the FIFO is full, yield. */
            cpu::Yield();
        }
        WriteUartRegister(UartRegister_THR, c);
        cpu::DataSynchronizationBarrier();
    }

    void KDebugLogImpl::Flush() {
        while ((ReadUartRegister(UartRegister_LSR) & 0x40) == 0) {
            /* Wait for the TMTY bit to be one (transmit empty). */
        }
    }

    void KDebugLogImpl::Save() {
        if constexpr (DoSaveAndRestore) {
            /* Save LCR, IER, FCR. */
            g_saved_registers[0] = ReadUartRegister(UartRegister_LCR);
            g_saved_registers[1] = ReadUartRegister(UartRegister_IER);
            g_saved_registers[2] = ReadUartRegister(UartRegister_FCR);

            /* Set Divisor Latch Access bit, to allow access to DLL/DLH */
            WriteUartRegister(UartRegister_LCR, 0x80);
            ReadUartRegister(UartRegister_LCR);

            /* Save DLL/DLH. */
            g_saved_registers[3] = ReadUartRegister(UartRegister_DLL);
            g_saved_registers[4] = ReadUartRegister(UartRegister_DLH);

            /* Restore Divisor Latch Access bit. */
            WriteUartRegister(UartRegister_LCR, g_saved_registers[0]);
            ReadUartRegister(UartRegister_LCR);
        }
    }

    void KDebugLogImpl::Restore() {
        if constexpr (DoSaveAndRestore) {
            /* Set Divisor Latch Access bit, to allow access to DLL/DLH */
            WriteUartRegister(UartRegister_LCR, 0x80);
            ReadUartRegister(UartRegister_LCR);

            /* Restore DLL/DLH. */
            WriteUartRegister(UartRegister_DLL, g_saved_registers[3]);
            WriteUartRegister(UartRegister_DLH, g_saved_registers[4]);
            ReadUartRegister(UartRegister_DLH);

            /* Restore Divisor Latch Access bit. */
            WriteUartRegister(UartRegister_LCR, g_saved_registers[0]);
            ReadUartRegister(UartRegister_LCR);

            /* Restore IER and FCR. */
            WriteUartRegister(UartRegister_IER, g_saved_registers[1]);
            WriteUartRegister(UartRegister_FCR, g_saved_registers[2] | 2);
            WriteUartRegister(UartRegister_IRDA_CSR, 0x02);
            ReadUartRegister(UartRegister_FCR);
        }
    }

#elif defined(MESOSPHERE_DEBUG_LOG_USE_IRAM_RINGBUFFER)

    namespace {

        constinit KVirtualAddress g_debug_iram_address = 0;

        constexpr size_t RingBufferSize = 0x5000;
        constinit uintptr_t g_offset = 0;

        constinit u8 g_saved_buffer[RingBufferSize];

    }

    bool KDebugLogImpl::Initialize() {
        /* Set the base address. */
        g_debug_iram_address = KMemoryLayout::GetDeviceVirtualAddress(KMemoryRegionType_LegacyLpsIram) + 0x38000;

        std::memset(GetVoidPointer(g_debug_iram_address), 0xFF, RingBufferSize);

        return true;
    }

    void KDebugLogImpl::PutChar(char c) {
        GetPointer<char>(g_debug_iram_address)[g_offset++] = c;

        if (g_offset == RingBufferSize) {
            g_offset = 0;
        }
    }

    void KDebugLogImpl::Flush() {
        /* ... */
    }

    void KDebugLogImpl::Save() {
        std::memcpy(g_saved_buffer, GetVoidPointer(g_debug_iram_address), RingBufferSize);
    }

    void KDebugLogImpl::Restore() {
        std::memcpy(GetVoidPointer(g_debug_iram_address), g_saved_buffer, RingBufferSize);
    }

#else

    #error "Unknown Debug UART device!"

#endif

}
