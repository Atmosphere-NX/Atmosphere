/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <string.h>
#include <stdarg.h>
#include "log.h"
#include "log_device_null.h"
#include "log_device_uart.h"
#include "fmt.h"
#include "../synchronization.h"

#ifndef NDEBUG
static atomic_flag g_log_lock = ATOMIC_FLAG_INIT;
static debug_log_device_t *dev;
#endif

void dbg_log_initialize(DebugLogDevice device) {
#ifndef NDEBUG
    static debug_log_device_t *const devs[] = {&g_debug_log_device_null.super, &g_debug_log_device_uart.super};
    dev = device >= DEBUGLOGDEVICE_MAX ? &g_debug_log_device_null.super : devs[device];
#else
    (void)device;
#endif
}

/* NOTE: no bound checks are done */
void dbg_log_write(const char *fmt, ...) {
#ifndef NDEBUG
    char buf[DBG_LOG_BUF_SIZE];
    int len;
    va_list args;
    lock_acquire(&g_log_lock);

    va_start(args, fmt);
    len = visprintf(buf, fmt, args);
    va_end(args);

    dev->write_string(dev, buf, len);
    lock_release(&g_log_lock);
#else
    (void)fmt;
#endif
}

void dbg_log_finalize(void) {
#ifndef NDEBUG
    dev->finalize(dev);
    dev = NULL;
#endif
}
