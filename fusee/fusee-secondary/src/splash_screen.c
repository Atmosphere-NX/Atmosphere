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
#include "timers.h"
#include "splash_screen.h"
#include "fs_utils.h"
#include "display/video_fb.h"

void display_splash_screen_bmp(const char *custom_splash_path) {
    uint8_t *splash_screen = g_default_splash_screen;
    if ((custom_splash_path != NULL) && (custom_splash_path[0] != '\x00')) {
        if (!read_from_file(splash_screen, sizeof(g_default_splash_screen), custom_splash_path)) {
            fatal_error("Failed to read custom splash screen from %s!\n", custom_splash_path);
        }
    }

    /* TODO: Display the splash screen. It should be a pointer to a BMP, at this point. */

    /* Display the splash screen for three seconds. */
    /* udelay(3000000); */
}
