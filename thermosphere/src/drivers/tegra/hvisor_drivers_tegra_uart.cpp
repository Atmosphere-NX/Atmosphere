/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#pragma once

#include "hvisor_drivers_tegra_uart.hpp"
#include "../../hvisor_generic_timer.hpp"

#include <chrono>

using namespace ams::hvisor;

namespace {

    inline void WaitCycles(u32 baudRate, u32 num)
    {
        u32 t = (num * 1000000 + 16 * baudRate - 1) / (16 * baudRate);
        GenericTimer::GetInstance().Wait(std::chrono::microseconds{t});
    }

    inline void WaitSyms(u32 baudRate, u32 num)
    {
        u32 t = (num * 1000000 + baudRate - 1) / baudRate;
        GenericTimer::GetInstance().Wait(std::chrono::microseconds{t});
    }

}

namespace ams::hvisor::drivers::tegra {

    void Uart::Initialize(u32 baudRate, u32 clkRate, bool invertTx) const
    {
        // Calculate baud rate, round to nearest (clkRate / (16 * baudRate))
        u32 divisor = (8 * baudRate + clkRate) / (16 * baudRate);

        m_regs->lcr &= ~LCR_DLAB;                               // Disable DLAB.
        m_regs->ier = 0;                                        // Disable all interrupts.
        m_regs->mcr = 0;

        // Setup UART in FIFO mode
        m_regs->lcr = LCR_DLAB | LCR_WD_LENGTH_8;               // Enable DLAB and set word length 8.
        m_regs->dll = divisor & 0xFF;                           // Divisor latch LSB.
        m_regs->dlh = (divisor >> 8) & 0xFF;                    // Divisor latch MSB.
        m_regs->lcr &= ~LCR_DLAB;                               // Disable DLAB.
        m_regs->spr;                                            // Dummy read.
        WaitSyms(baudRate, 3);                                  // Wait for 3 symbols at the new baudrate.

        // Enable FIFO with default settings.
        m_regs->fcr = FCR_FCR_EN_FIFO;
        m_regs->irda_csr = invertTx ? IRDA_CSR_INVERT_TXD : 0;  // Invert TX if needed
        m_regs->spr;                                            // Dummy read as mandated by TRM.
        WaitCycles(baudRate, 3);                                // Wait for 3 baud cycles, as mandated by TRM (erratum).

        // Flush FIFO.
        WaitIdle(STATUS_TX_IDLE);                               // Make sure there's no data being written in TX FIFO (TRM).
        m_regs->fcr |= FCR_RX_CLR | FCR_TX_CLR;                 // Clear TX and RX FIFOs.
        WaitCycles(baudRate, 32);                               // Wait for 32 baud cycles (TRM, erratum).

        // Wait for idle state (TRM).
        WaitIdle(STATUS_TX_IDLE | STATUS_RX_IDLE);
    }

    void Uart::WriteData(const void *buffer, size_t size) const
    {
        const u8 *buf8 = reinterpret_cast<const u8 *>(buffer);
        for (size_t i = 0; i < size; i++) {
            while (!(m_regs->lsr & LSR_THRE)); // Wait until it's possible to send data.
            m_regs->thr = buf8[i]; 
        }
    }

    void Uart::ReadData(void *buffer, size_t size) const
    {
        u8 *buf8 = reinterpret_cast<u8 *>(buffer);
        for (size_t i = 0; i < size; i++) {
            while (!(m_regs->lsr & LSR_RDR)) // Wait until it's possible to receive data.
            buf8[i] = m_regs->rbr;
        }
    }

    size_t Uart::ReadDataMax(void *buffer, size_t maxSize) const
    {
        u8 *buf8 = reinterpret_cast<u8 *>(buffer);
        size_t count = 0;

        for (size_t i = 0; i < maxSize && (m_regs->lsr & LSR_RDR); i++) {
            buf8[i] = m_regs->rbr;
            ++count;
        }

        return count;
    }

    size_t Uart::ReadDataUntil(char *buffer, size_t maxSize, char delimiter) const
    {
        size_t count = 0;

        for (size_t i = 0; i < maxSize && (m_regs->lsr & LSR_RDR); i++) {
            while (!(m_regs->lsr & LSR_RDR)) // Wait until it's possible to receive data.

            buffer[i] = m_regs->rbr;
            ++count;
            if (buffer[i] == delimiter) {
                break;
            }
        }

        return count;
    }

    void Uart::SetRxInterruptEnabled(bool enabled) const
    {
        constexpr u32 mask = IER_IE_RX_TIMEOUT | IER_IE_RHR;

        // We don't support any other interrupt here.
        m_regs->ier = enabled ? mask : 0;
    }
}