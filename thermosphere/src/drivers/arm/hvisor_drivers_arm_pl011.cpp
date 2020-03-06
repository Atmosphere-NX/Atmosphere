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

#include "hvisor_drivers_arm_pl011.hpp"

namespace ams::hvisor::drivers::arm {

    void PL011::Initialize(u32 baudRate, u32 clkRate) const
    {
        /* The TRM (DDI0183) reads:
            Program the control registers as follows:
            1. Disable the UART.
            2. Wait for the end of transmission or reception of the current character.
            3. Flush the transmit FIFO by disabling bit 4 (FEN) in the line control register
            (UARTCLR_H).
            4. Reprogram the control register.
            5. Enable the UART.
         */
        // First, disable the UART. Flush the receive FIFO, wait for tx to complete, and disable both FIFOs.
        m_regs->cr &= ~UARTCR_UARTEN;
        while (!(m_regs->fr & UARTFR_RXFE)) {
            m_regs->dr;
        }
        while (m_regs->fr & UARTFR_BUSY);
        // This flushes the transmit FIFO:
        m_regs->lcr_h &= ~UARTLCR_H_FEN;

        // Set baudrate, Divisor =  (Uart clock * 4) / baudrate; stored in IBRD|FBRD
        u32 divisor = (4 * clkRate) / baudRate;
        m_regs->ibrd = divisor >> 6;
        m_regs->fbrd = divisor & 0x3F;

        // Select FIFO fill levels for interrupts
        m_regs->ifls = IFLS_RX4_8 | IFLS_TX4_8;

        // FIFO Enabled / No Parity / 8 Data bit / One Stop Bit
        m_regs->lcr_h = UARTLCR_H_FEN | UARTLCR_H_WLEN_8;

        // Select the interrupts we want to have
        // RX timeout and TX/RX fill interrupts
        m_regs->imsc = RTI | RXI | RXI;

        // Clear any pending errors
        m_regs->ecr = 0;

        // Clear all interrupts
        m_regs->icr = ALL_INTERRUPTS;

        // Enable tx, rx, and uart overall
        m_regs->cr = UARTCR_RXE | UARTCR_TXE | UARTCR_UARTEN;
    }

    void PL011::WriteData(const void *buffer, size_t size) const
    {
        const u8 *buf8 = reinterpret_cast<const u8 *>(buffer);
        for (size_t i = 0; i < size; i++) {
            while (m_regs->fr & UARTFR_TXFF); // while TX FIFO full
            m_regs->dr = buf8[i]; 
        }

    }

    void PL011::ReadData(void *buffer, size_t size) const
    {
        u8 *buf8 = reinterpret_cast<u8 *>(buffer);
        size_t i;

        for (i = 0; i < size; i++) {
            while (m_regs->fr & UARTFR_RXFE);
            buf8[i] = m_regs->dr;
        }

    }

    size_t PL011::ReadDataMax(void *buffer, size_t maxSize) const
    {
        u8 *buf8 = reinterpret_cast<u8 *>(buffer);
        size_t count = 0;

        for (size_t i = 0; i < maxSize && !(m_regs->fr & UARTFR_RXFE); i++) {
            buf8[i] = m_regs->dr;
            ++count;
        }

        return count;

    }

    size_t PL011::ReadDataUntil(char *buffer, size_t maxSize, char delimiter) const
    {
        size_t count = 0;

        for (size_t i = 0; i < maxSize; i++) {
            while (m_regs->fr & UARTFR_RXFE);
            buffer[i] = m_regs->dr;
            ++count;
            if (buffer[i] == delimiter) {
                break;
            }
        }

        return count;
    }

    void PL011::SetRxInterruptEnabled(bool enabled) const
    {
        constexpr u32 mask = RTI | RXI;

        if (enabled) {
            m_regs->imsc |= mask;
        } else {
            m_regs->imsc &= ~mask;
        }
    }

}
