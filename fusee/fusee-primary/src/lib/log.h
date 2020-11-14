/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

#ifndef FUSEE_LOG_H
#define FUSEE_LOG_H

#define PRINT_MESSAGE_MAX_LENGTH 1024

#include <stdarg.h>

typedef enum {
    SCREEN_LOG_LEVEL_NONE       = 0,
    SCREEN_LOG_LEVEL_ERROR      = 1,
    SCREEN_LOG_LEVEL_WARNING    = 2,
    SCREEN_LOG_LEVEL_MANDATORY  = 3, /* no log prefix */
    SCREEN_LOG_LEVEL_INFO       = 4,
    SCREEN_LOG_LEVEL_DEBUG      = 5,

    SCREEN_LOG_LEVEL_NO_PREFIX  = 0x100 /* OR this to your LOG_LEVEL to prevent prefix creation */
} ScreenLogLevel;

extern ScreenLogLevel g_screen_log_level;

void log_set_log_level(ScreenLogLevel screen_log_level);
ScreenLogLevel log_get_log_level();
void log_to_uart(const char *message);
void vprint(ScreenLogLevel screen_log_level, const char *fmt, va_list args);
void print(ScreenLogLevel screen_log_level, const char* fmt, ...);

#endif