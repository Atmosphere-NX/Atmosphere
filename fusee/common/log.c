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

#include "log.h"

#ifdef FUSEE_STAGE2_SRC
#include "../../../fusee/fusee-secondary/src/console.h"
#include <stdio.h>
#else
#include "display/video_fb.h"
#include "vsprintf.h"
#endif

/* Default log level for screen output. */
ScreenLogLevel g_screen_log_level = SCREEN_LOG_LEVEL_NONE;

void log_set_log_level(ScreenLogLevel log_level) {
    g_screen_log_level = log_level;
}

ScreenLogLevel log_get_log_level() {
    return g_screen_log_level;
}

void log_to_uart(const char *message) {
    /* TODO: Add UART logging. */
}

static void print_to_screen(ScreenLogLevel screen_log_level, char *message) {
    /* Don't print to screen if below log level */
    if(screen_log_level > g_screen_log_level) return;
    
    #ifdef FUSEE_STAGE2_SRC
    printf(message);
    #else
    video_puts(message);
    #endif
}

/**
 * vprintk - logs a message and prints it to screen based on its screen_log_level
 *
 * If the level is below g_screen_log_level it will not be shown but logged to UART
 * This text will not be colored or prefixed
 * UART is TODO
 */
void vprint(ScreenLogLevel screen_log_level, const char *fmt, va_list args)
{
    char buf[PRINT_MESSAGE_MAX_LENGTH];
    vsnprintf(buf, PRINT_MESSAGE_MAX_LENGTH, fmt, args);

    /* We don't need that flag here, but if it gets used, strip it so we print correctly. */
    screen_log_level &= ~SCREEN_LOG_LEVEL_NO_PREFIX;

    /* Log to UART. */
    log_to_uart(buf);

    print_to_screen(screen_log_level, buf);
}

static void add_prefix(ScreenLogLevel screen_log_level, const char *fmt, char *buf) {
    char typebuf[] = "[%s] %s";

    /* Apply prefix and append message format. */
    /* TODO: Add coloring to the output. */
    switch(screen_log_level)
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
}

/**
 * print - logs a message and prints it to screen based on its screen_log_level
 * 
 * If the level is below g_screen_log_level it will not be shown but logged to UART
 * Use SCREEN_LOG_LEVEL_NO_PREFIX if you don't want a prefix to be added
 * UART is TODO
 */
void print(ScreenLogLevel screen_log_level, const char * fmt, ...)
{
    char buf[PRINT_MESSAGE_MAX_LENGTH] = {};
    char message[PRINT_MESSAGE_MAX_LENGTH] = {};

    /* Make splash disappear if level is ERROR or WARNING. */
    #ifdef FUSEE_STAGE2_SRC
    if (screen_log_level < SCREEN_LOG_LEVEL_MANDATORY) {
        console_resume();
    }
    #endif

    /* Make prefix free messages with log_level possible. */
    if(screen_log_level & SCREEN_LOG_LEVEL_NO_PREFIX) {
        /* Remove the NO_PREFIX flag so the enum can be recognized later on. */
        screen_log_level &= ~SCREEN_LOG_LEVEL_NO_PREFIX;

        snprintf(buf, PRINT_MESSAGE_MAX_LENGTH, "%s", fmt);
    }
    else {
        add_prefix(screen_log_level, fmt, buf);
    }

    /* Input arguments. */
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, PRINT_MESSAGE_MAX_LENGTH, buf, args);
    va_end(args);

    /* Log to UART. */
    log_to_uart(message);

    print_to_screen(screen_log_level, message);
}