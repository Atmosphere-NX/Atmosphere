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

#include "../../defines.hpp"

// AMBA PL011 driver
// Originally from
/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

namespace ams::hvisor::drivers::arm {

    class PL011 final {
        private:
            struct Registers {
                u32 dr;
                union {
                    u32 sr;
                    u32 ecr;
                };
                u32 _0x08, _0x0C, _0x10, _0x14;
                u32 fr;
                u32 _0x1C;
                u32 ilpr;
                u32 ibrd;
                u32 fbrd;
                u32 lcr_h;
                u32 cr;
                u32 ifls;
                u32 imsc;
                u32 ris;
                u32 mis;
                u32 icr;
                u32 dmacr;
            };

            enum Mask : u32 {
                DATA_ERROR_MASK     = 0x0F00,           // Data status bits
                PL011_STATUS_ERROR_MASK = 0x0F,     // Status reg bits
            };

            enum Error : u32 {
                OE                  = BIT(3),  // Overrun error
                BE                  = BIT(2),  // Break error
                PE                  = BIT(1),  // Parity error
                FE                  = BIT(0),  // Framing error
            };

            enum Interrupt : u32 {
                OEI                 = BIT(10), // Overrun error interrupt
                BEI                 = BIT(9),  // Break error interrupt
                PEI                 = BIT(8),  // Parity error interrupt
                FEI                 = BIT(7),  // Framing error interrupt
                RTI                 = BIT(6),  // Receive timeout interrupt
                TXI                 = BIT(5),  // Transmit interrupt
                RXI                 = BIT(4),  // Receive interrupt
                DSRMI               = BIT(3),  // DSR modem interrupt
                DCDMI               = BIT(2),  // DCD modem interrupt
                CTSMI               = BIT(1),  // CTS modem interrupt
                RIMI                = BIT(0),  // RI modem interrupt
                ALL_INTERRUPTS      = MASK(11),
            };

            // Flag reg bits
            enum FrFlags : u32 {
                UARTFR_RI           = BIT(8),  // Ring indicator
                UARTFR_TXFE         = BIT(7),  // Transmit FIFO empty
                UARTFR_RXFF         = BIT(6),  // Receive  FIFO full
                UARTFR_TXFF         = BIT(5),  // Transmit FIFO full
                UARTFR_RXFE         = BIT(4),  // Receive  FIFO empty
                UARTFR_BUSY         = BIT(3),  // UART busy
                UARTFR_DCD          = BIT(2),  // Data carrier detect
                UARTFR_DSR          = BIT(1),  // Data set ready
                UARTFR_CTS          = BIT(0),  // Clear to send
            };

            // Control reg bits
            enum CrFlags : u32 {
                UARTCR_CTSEN        = BIT(15), // CTS hardware flow control enable
                UARTCR_RTSEN        = BIT(14), // RTS hardware flow control enable
                UARTCR_RTS          = BIT(11), // Request to send
                UARTCR_DTR          = BIT(10), // Data transmit ready.
                UARTCR_RXE          = BIT(9),  // Receive enable
                UARTCR_TXE          = BIT(8),  // Transmit enable
                UARTCR_LBE          = BIT(7),  // Loopback enable
                UARTCR_UARTEN       = BIT(0),  // UART Enable
            };

            // Line Control Register Bits
            enum LcrFlags : u32 {
                UARTLCR_H_SPS       = BIT(7),  // Stick parity select
                UARTLCR_H_WLEN_8    = (3 << 5),
                UARTLCR_H_WLEN_7    = (2 << 5),
                UARTLCR_H_WLEN_6    = BIT(5),
                UARTLCR_H_WLEN_5    = (0 << 5),
                UARTLCR_H_FEN       = BIT(4),  // FIFOs Enable
                UARTLCR_H_STP2      = BIT(3),  // Two stop bits select
                UARTLCR_H_EPS       = BIT(2),  // Even parity select
                UARTLCR_H_PEN       = BIT(1),  // Parity Enable
                UARTLCR_H_BRK       = BIT(0),  // Send break
            };

            // FIFO level select register
            enum IflsLevels : u32 {
                IFLS_RX1_8          = (0 << 3),
                IFLS_RX2_8          = (1 << 3),
                IFLS_RX4_8          = (2 << 3),
                IFLS_RX6_8          = (3 << 3),
                IFLS_RX7_8          = (4 << 3),
                IFLS_TX1_8          = (0 << 0),
                IFLS_TX2_8          = (1 << 0),
                IFLS_TX4_8          = (2 << 0),
                IFLS_TX6_8          = (3 << 0),
                IFLS_TX7_8          = (4 << 0),
            };

        private:
            volatile Registers *m_regs = nullptr;
            void Initialize(u32 baudRate, u32 clkRate = 1) const;

            // TODO friend
        public:

            void WriteData(const void *buffer, size_t size) const;
            void ReadData(void *buffer, size_t size) const;
            size_t ReadDataMax(void *buffer, size_t maxSize) const;
            size_t ReadDataUntil(char *buffer, size_t maxSize, char delimiter) const;

            void SetRxInterruptEnabled(bool enabled) const;
    };
}
