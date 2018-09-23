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

#include <stdio.h>

#include "log.h"
#include "display/video_fb.h"

/* default log level for screen output */
ScreenLogLevel g_screen_log_level = SCREEN_LOG_LEVEL_MANDATORY;

void log_set_log_level(ScreenLogLevel log_level) {
    g_screen_log_level = log_level;
}

void log_to_uart(const char *message) {
    /* TODO: add UART logging */
}

void print_to_screen(ScreenLogLevel screenLogLevel, char *message) {
    /* don't print to screen if below log level */
    if(g_screen_log_level == SCREEN_LOG_LEVEL_NONE || screenLogLevel > g_screen_log_level) return;

    //video_puts(buf);
    printf(message);
}

/**
 * vprintk - logs a message to the console
 *
 * This text will not be colored or prefixed but logged to UART
 * UART is TODO
 */
void vprint(ScreenLogLevel screenLogLevel, const char *fmt, va_list args)
{
    char buf[PRINT_MESSAGE_MAX_LENGTH];
    vsnprintf(buf, PRINT_MESSAGE_MAX_LENGTH, fmt, args);

    /* log to UART */
    log_to_uart(buf);

    print_to_screen(screenLogLevel, buf);
}

/**
 * print - logs a message and prints it to screen based on its screenLogLevel
 * 
 * If the level is below g_screen_log_level it will not be shown but logged to UART
 * UART is TODO
 */
void print(ScreenLogLevel screenLogLevel, const char * fmt, ...)
{
    char typebuf[] = "[%s] %s";
    char buf[PRINT_MESSAGE_MAX_LENGTH] = {};
    char message[PRINT_MESSAGE_MAX_LENGTH] = {};

    /* apply prefix and append message format */
    /* TODO: add coloring to the output */
    /* TODO: make splash disappear if level > MANDATORY */
    switch(screenLogLevel)
    {
        case SCREEN_LOG_LEVEL_ERROR:
            snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "ERROR", fmt);
            break;
        case SCREEN_LOG_LEVEL_WARNING:
            snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "WARNING", fmt);
            break;
        case SCREEN_LOG_LEVEL_MANDATORY:
            snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, "%s", fmt);
            break;
        case SCREEN_LOG_LEVEL_INFO:
            snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "INFO", fmt);
            break;
        case SCREEN_LOG_LEVEL_DEBUG:
            snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "DEBUG", fmt);
            break;
        default:
            break;
    }

    /* input arguments */
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, PRINT_MESSAGE_MAX_LENGTH, buf, args);
    va_end(args);

    /* log to UART */
    log_to_uart(message);

    print_to_screen(screenLogLevel, message);
}