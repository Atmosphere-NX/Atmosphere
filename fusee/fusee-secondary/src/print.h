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

#ifndef FUSEE_PRINT_H
#define FUSEE_PRINT_H

#define PRINT_MESSAGE_MAX_LENGTH      512

typedef enum {
    PRINT_LOG_DEBUG = 0,
    PRINT_LOG_INFO,
    PRINT_LOG_MANDATORY,
    PRINT_LOG_WARNING,
    PRINT_LOG_ERROR
} PrintLogLevel;

#include <stdint.h>
#include "../../fusee-primary/src/lib/vsprintf.h"

extern PrintLogLevel g_print_log_level;

//void vprintk(const char *fmt, va_list args);
void print(PrintLogLevel printLogLevel, const char* fmt, ...);

#endif