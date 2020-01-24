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
 
#ifndef FUSEE_SPLASH_SCREEN_H
#define FUSEE_SPLASH_SCREEN_H

#include <stdint.h>

#define SPLASH_SCREEN_WIDTH_MAX     720
#define SPLASH_SCREEN_HEIGHT_MAX    1280
#define SPLASH_SCREEN_BPP           32
#define SPLASH_SCREEN_STRIDE        768
#define SPLASH_SCREEN_SIZE_MAX      (SPLASH_SCREEN_HEIGHT_MAX * SPLASH_SCREEN_STRIDE * 4)

void display_splash_screen_bmp(const char *custom_splash_path, void *fb_address);
void splash_screen_wait_delay(void);

#endif
