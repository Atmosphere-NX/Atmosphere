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

#include "print.h"

#include "display/video_fb.h"

PrintLogLevel g_print_log_level = PRINT_LOG_DEBUG;

void vprintk(const char *fmt, va_list args)
{
    char buf[PRINT_MESSAGE_MAX_LENGTH];
    vsnprintf(buf, PRINT_MESSAGE_MAX_LENGTH, fmt, args);
    video_puts(buf);
}

void printk(const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    vprintk(fmt, list);
    va_end(list);
}

/**
 * print - logs a message based on its message_level
 * 
 * If the level is below g_message_level it will not be shown
 * but logged to UART (UART is TO DO) 
 */
void print(PrintLogLevel printLogLevel, const char * fmt, ...)
{
    char typebuf[] = "[%s] %s";
    char buf[PRINT_MESSAGE_MAX_LENGTH] = {};

    /* apply prefix and append message format */
    /* TODO: add coloring to the output */
    switch(printLogLevel)
    {
    case PRINT_LOG_DEBUG:
        snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "DEBUG", fmt);
        break;
    case PRINT_LOG_INFO:
        snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "INFO", fmt);
        break;
    case PRINT_LOG_MANDATORY:
         snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, "%s", fmt);
         break;
    case PRINT_LOG_WARNING:
        snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "WARNING", fmt);
        break;
    case PRINT_LOG_ERROR:
        snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, typebuf, "ERROR", fmt);
        break;
    default:
        break;
    }

    /* input arguments for UART logging */
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, PRINT_MESSAGE_MAX_LENGTH, buf, args);
    va_end(args);

    /* TODO: implement SD and/or UART logging */

    /* don't print to screen if below log level */
    if (printLogLevel < g_print_log_level) return;

    printk(buf);
}