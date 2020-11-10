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
 
#include <stdbool.h>
#include <stdarg.h>
#include "utils.h"
#include "../../../fusee/common/display/video_fb.h"
#include "../../../fusee/common/log.h"

__attribute__ ((noreturn)) void generic_panic(void) {
    while (true) {
        /* Lock. */
    }
}

__attribute__((noreturn)) void fatal_error(const char *fmt, ...) {
    /* Forcefully initialize the screen if logging is disabled. */
    if (log_get_log_level() == SCREEN_LOG_LEVEL_NONE) {
        /* Zero-fill the framebuffer and register it as printk provider. */
        video_init((void *)0xC0000000);
        
        /* Override the global logging level. */
        log_set_log_level(SCREEN_LOG_LEVEL_ERROR);
    }
    
    /* Display fatal error. */
    va_list args;
    print(SCREEN_LOG_LEVEL_ERROR, "Fatal error: ");
    va_start(args, fmt);
    vprint(SCREEN_LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
    
    while (true) {
        /* Lock. */
    }
}