/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "../../utils.h"
#include "interrupt_config.h"

// AMBA PL011 driver
// Originally from
/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

typedef enum UartDevice {
    UART_A = 0,

    UART_MAX,
} UartDevice;

typedef struct PL011UartRegisters {
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
} PL011UartRegisters;

// Data status bits
#define UART_DATA_ERROR_MASK        0x0F00

// Status reg bits
#define UART_STATUS_ERROR_MASK      0x0F

// Errors
#define PL011_OE                    BIT(3)  // Overrun error
#define PL011_BE                    BIT(2)  // Break error
#define PL011_PE                    BIT(1)  // Parity error
#define PL011_FE                    BIT(0)  // Framing error

// Interrupts
#define PL011_OEI                   BIT(10) // Overrun error interrupt
#define PL011_BEI                   BIT(9)  // Break error interrupt
#define PL011_PEI                   BIT(8)  // Parity error interrupt
#define PL011_FEI                   BIT(7)  // Framing error interrupt
#define PL011_RTI                   BIT(6)  // Receive timeout interrupt
#define PL011_TXI                   BIT(5)  // Transmit interrupt
#define PL011_RXI                   BIT(4)  // Receive interrupt
#define PL011_DSRMI                 BIT(3)  // DSR modem interrupt
#define PL011_DCDMI                 BIT(2)  // DCD modem interrupt
#define PL011_CTSMI                 BIT(1)  // CTS modem interrupt
#define PL011_RIMI                  BIT(0)  // RI modem interrupt
#define PL011_ALL_INTERRUPTS        MASK(11)

// Flag reg bits
#define PL011_UARTFR_RI             BIT(8)  // Ring indicator
#define PL011_UARTFR_TXFE           BIT(7)  // Transmit FIFO empty
#define PL011_UARTFR_RXFF           BIT(6)  // Receive  FIFO full
#define PL011_UARTFR_TXFF           BIT(5)  // Transmit FIFO full
#define PL011_UARTFR_RXFE           BIT(4)  // Receive  FIFO empty
#define PL011_UARTFR_BUSY           BIT(3)  // UART busy
#define PL011_UARTFR_DCD            BIT(2)  // Data carrier detect
#define PL011_UARTFR_DSR            BIT(1)  // Data set ready
#define PL011_UARTFR_CTS            BIT(0)  // Clear to send

// Control reg bits
#define PL011_UARTCR_CTSEN          BIT(15) // CTS hardware flow control enable
#define PL011_UARTCR_RTSEN          BIT(14) // RTS hardware flow control enable
#define PL011_UARTCR_RTS            BIT(11) // Request to send
#define PL011_UARTCR_DTR            BIT(10) // Data transmit ready.
#define PL011_UARTCR_RXE            BIT(9)  // Receive enable
#define PL011_UARTCR_TXE            BIT(8)  // Transmit enable
#define PL011_UARTCR_LBE            BIT(7)  // Loopback enable
#define PL011_UARTCR_UARTEN         BIT(0)  // UART Enable

// Line Control Register Bits
#define PL011_UARTLCR_H_SPS         BIT(7)  // Stick parity select
#define PL011_UARTLCR_H_WLEN_8      (3 << 5)
#define PL011_UARTLCR_H_WLEN_7      (2 << 5)
#define PL011_UARTLCR_H_WLEN_6      BIT(5)
#define PL011_UARTLCR_H_WLEN_5      (0 << 5)
#define PL011_UARTLCR_H_FEN         BIT(4)  // FIFOs Enable
#define PL011_UARTLCR_H_STP2        BIT(3)  // Two stop bits select
#define PL011_UARTLCR_H_EPS         BIT(2)  // Even parity select
#define PL011_UARTLCR_H_PEN         BIT(1)  // Parity Enable
#define PL011_UARTLCR_H_BRK         BIT(0)  // Send break

// FIFO level select register
#define PL011_IFLS_RX1_8            (0 << 3)
#define PL011_IFLS_RX2_8            (1 << 3)
#define PL011_IFLS_RX4_8            (2 << 3)
#define PL011_IFLS_RX6_8            (3 << 3)
#define PL011_IFLS_RX7_8            (4 << 3)
#define PL011_IFLS_TX1_8            (0 << 0)
#define PL011_IFLS_TX2_8            (1 << 0)
#define PL011_IFLS_TX4_8            (2 << 0)
#define PL011_IFLS_TX6_8            (3 << 0)
#define PL011_IFLS_TX7_8            (4 << 0)

#define UART_CLK_IN_HZ              1

void uartInit(UartDevice dev, u32 baudRate, u32 flags);
void uartWriteData(UartDevice dev, const void *buffer, size_t size);
void uartReadData(UartDevice dev, void *buffer, size_t size);
size_t uartReadDataMax(UartDevice dev, void *buffer, size_t maxSize);
size_t uartReadDataUntil(UartDevice dev, char *buffer, size_t maxSize, char delimiter);

ReadWriteDirection uartGetInterruptDirection(UartDevice dev);
void uartSetInterruptStatus(UartDevice dev, ReadWriteDirection direction, bool enable);

static inline u16 uartGetIrqId(UartDevice dev)
{
    switch (dev) {
        case UART_A:
            return GIC_IRQID_UART;
        default:
            return GIC_IRQID_SPURIOUS;
    }
}
