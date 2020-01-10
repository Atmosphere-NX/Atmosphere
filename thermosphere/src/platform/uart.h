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

#if PLATFORM_TEGRA
// TODO

/*#include "tegra/uart.h"

#define DEFAULT_UART            UART_C
#define DEFAULT_UARTINV_STATUS  true

static inline void uartInit(u32 baudRate)
{
    uart_reset(DEFAULT_UART);
    uart_init(DEFAULT_UART, baudRate, DEFAULT_UARTINV_STATUS);
}

static inline void uartWriteData(const void *buffer, size_t size)
{
    uart_send(DEFAULT_UART, buffer, size);
}

static inline void uartReadData(void *buffer, size_t size)
{
    uart_recv(DEFAULT_UART, buffer, size);
}

static inline size_t uartReadDataMax(void *buffer, size_t maxSize)
{
    return uart_recv_max(DEFAULT_UART, buffer, maxSize);
}
*/

#elif defined(PLATFORM_QEMU)

#define DEFAULT_UART            UART_A

#include "qemu/uart.h"

#else

#error "Error: platform not defined"

#endif