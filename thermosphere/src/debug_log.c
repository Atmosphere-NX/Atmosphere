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

#include <stdio.h>
#include "debug_log.h"
#include "platform/uart.h"
#include "semihosting.h"
#include "utils.h"
#include "transport_interface.h"
#include "platform/uart.h"

#ifndef DLOG_USE_SEMIHOSTING_WRITE0
#define DLOG_USE_SEMIHOSTING_WRITE0 1
#endif

static TransportInterface *g_debugLogTransportInterface;

void debugLogInit(void)
{
    if (!DLOG_USE_SEMIHOSTING_WRITE0) {
        transportInterfaceCreate(TRANSPORT_INTERFACE_TYPE_UART, DEFAULT_UART, DEFAULT_UART_FLAGS, NULL, NULL, NULL);
    }
}

// NOTE: UNSAFE!
int debugLog(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int res = vsprintf(buf, fmt, args);
    va_end(args);

    // Use semihosting if available (we assume qemu was launched with -semihosting), otherwise UART
    if (DLOG_USE_SEMIHOSTING_WRITE0 && semihosting_connection_supported()) {
        semihosting_write_string(buf);
    } else {
        transportInterfaceWriteData(g_debugLogTransportInterface, buf, res);
    }

    return res;
}
