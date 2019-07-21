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
#include "../../utils.h"

// Adapted from ARM TF asm code
// AMBA PL101 driver

/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//115200

void uartInit(u32 baudRate)
{
    // First, disable UART
    g_uartRegs->CR &= ~PL011_UARTCR_UARTEN;

    // Set baudrate, Divisor =  (Uart clock * 4) / baudrate; stored in IBRD|FBRD
    u32 divisor = (4 * UART0_CLK_IN_HZ) / baudRate;
    g_uartRegs->IBRD = divisor >> 6;
    g_uartRegs->FBRD = divisor & 0x3F;
    g_uartRegs->LCR_H = PL011_LINE_CONTROL;

    // Clear any pending errors
    g_uartRegs->ECR = 0;

    // Enable tx, rx, and uart overall
    g_uartRegs->CR = PL011_UARTCR_RXE | PL011_UARTCR_TXE | PL011_UARTCR_UARTEN;
}

void uartWriteData(const void *buffer, size_t size)
{
    const u8 *buf8 = (const u8 *)buffer;
    for (size_t i = 0; i < size; i++) {
        while (g_uartRegs->FR & PL011_UARTFR_TXFF); // while TX FIFO full
        g_uartRegs->DR = buf8[i]; 
    }
}

void uartReadData(void *buffer, size_t size)
{
    u8 *buf8 = (u8 *)buffer;
    size_t i;

    for (i = 0; i < size; i++) {
        while (g_uartRegs->FR & PL011_UARTFR_RXFE);
        buf8[i] = g_uartRegs->DR;
    }
}

size_t uartReadDataMax(void *buffer, size_t maxSize)
{
    u8 *buf8 = (u8 *)buffer;
    size_t i;

    for (i = 0; i < maxSize && !(g_uartRegs->FR & PL011_UARTFR_RXFE); i++) {
        buf8[i] = g_uartRegs->DR;
    }

    return 1 + i;
}