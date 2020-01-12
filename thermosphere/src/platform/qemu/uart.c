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

#include "uart.h"
#include "../../irq.h"

// Adapted from TF asm code
// AMBA PL101 driver

/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//115200

static inline volatile PL011UartRegisters *uartGetRegisters(UartDevice dev)
{
    switch (dev) {
        case UART_A:
            return (volatile PL011UartRegisters *)0x09000000;
        default:
            return NULL;
    }
}

void uartInit(UartDevice dev, u32 baudRate, u32 flags)
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
    (void)flags;
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    // First, disable the UART. Flush the receive FIFO, wait for tx to complete, and disable both FIFOs.
    uart->cr &= ~PL011_UARTCR_UARTEN;
    while (!(uart->fr & PL011_UARTFR_RXFE)) {
        uart->dr;
    }
    while (uart->fr & PL011_UARTFR_BUSY);
    // This flushes the transmit FIFO:
    uart->lcr_h &= ~PL011_UARTLCR_H_FEN;

    // Set baudrate, Divisor =  (Uart clock * 4) / baudrate; stored in IBRD|FBRD
    u32 divisor = (4 * UART_CLK_IN_HZ) / baudRate;
    uart->ibrd = divisor >> 6;
    uart->fbrd = divisor & 0x3F;

    // Select FIFO fill levels for interrupts
    uart->ifls = PL011_IFLS_RX4_8 | PL011_IFLS_TX4_8;

    // FIFO Enabled / No Parity / 8 Data bit / One Stop Bit
    uart->lcr_h = PL011_UARTLCR_H_FEN | PL011_UARTLCR_H_WLEN_8;

    // Select the interrupts we want to have
    // RX timeout and TX/RX fill interrupts
    uart->imsc = PL011_RTI | PL011_RXI | PL011_RXI;

    // Clear any pending errors
    uart->ecr = 0;

    // Clear all interrupts
    uart->icr = PL011_ALL_INTERRUPTS;

    // Register the interrupt ID
    configureInterrupt(uartGetIrqId(dev), IRQ_PRIORITY_HOST, true);

    // Enable tx, rx, and uart overall
    uart->cr = PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN;
}

void uartWriteData(UartDevice dev, const void *buffer, size_t size)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    const u8 *buf8 = (const u8 *)buffer;
    for (size_t i = 0; i < size; i++) {
        while (uart->fr & PL011_UARTFR_TXFF); // while TX FIFO full
        uart->dr = buf8[i]; 
    }
}

void uartReadData(UartDevice dev, void *buffer, size_t size)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    u8 *buf8 = (u8 *)buffer;
    size_t i;

    for (i = 0; i < size; i++) {
        while (uart->fr & PL011_UARTFR_RXFE);
        buf8[i] = uart->dr;
    }
}

size_t uartReadDataMax(UartDevice dev, void *buffer, size_t maxSize)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    u8 *buf8 = (u8 *)buffer;
    size_t count = 0;

    for (size_t i = 0; i < maxSize && !(uart->fr & PL011_UARTFR_RXFE); i++) {
        buf8[i] = uart->dr;
        ++count;
    }

    return count;
}

size_t uartReadDataUntil(UartDevice dev, char *buffer, size_t maxSize, char delimiter)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    size_t count = 0;

    for (size_t i = 0; i < maxSize; i++) {
        while (uart->fr & PL011_UARTFR_RXFE);
        buffer[i] = uart->dr;
        ++count;
        if (buffer[i] == delimiter) {
            break;
        }
    }

    return count;
}

ReadWriteDirection uartGetInterruptDirection(UartDevice dev)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);
    u32 ret = 0;

    u32 istatus = uart->mis;
    if (istatus & (PL011_RTI | PL011_RXI)) {
        ret |= DIRECTION_READ;
    }
    if (istatus & PL011_TXI) {
        ret |= DIRECTION_WRITE;
    }

    return (ReadWriteDirection)ret;
}

void uartSetInterruptStatus(UartDevice dev, ReadWriteDirection direction, bool enable)
{
    volatile PL011UartRegisters *uart = uartGetRegisters(dev);

    u32 mask = 0;
    if (direction & DIRECTION_READ) {
        mask |= PL011_RTI | PL011_RXI;
    }
    if (direction & DIRECTION_WRITE) {
        mask |= PL011_TXI;
    }

    if (enable) {
        uart->imsc |= mask;
    } else {
        uart->icr = mask;
        uart->imsc &= ~mask;
    }
}
