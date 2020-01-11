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

#define BAUD_115200 115200

#if PLATFORM_TEGRA

#include "tegra/uart.h"

#define DEFAULT_UART            UART_C
#define DEFAULT_UART_FLAGS      1

#elif defined(PLATFORM_QEMU)

#define DEFAULT_UART            UART_A
#define DEFAULT_UART_FLAGS      0

#include "qemu/uart.h"

#else

#error "Error: platform not defined"

#endif
