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

#include "console.h"
#include "di.h"
#include "timers.h"
#include "splash_screen.h"
#include "fs_utils.h"
#include "../../../fusee/common/display/video_fb.h"

#define u8 uint8_t
#define u32 uint32_t
#include "splash_screen_bin.h"
#undef u8
#undef u32

static uint32_t g_splash_start_time = 0;

static void render_bmp(const uint32_t *bmp_data, uint32_t *framebuffer, uint32_t bmp_width, uint32_t bmp_height, uint32_t bmp_pos_x, uint32_t bmp_pos_y) {
    /* Render the BMP. */
    for (uint32_t y = bmp_pos_y; y < (bmp_pos_y + bmp_height); y++) {
        for (uint32_t x = bmp_pos_x; x < (bmp_pos_x + bmp_width); x++) {
            framebuffer[x + (y * SPLASH_SCREEN_STRIDE)] = bmp_data[(bmp_height + bmp_pos_y - 1 - y) * bmp_width + x - bmp_pos_x];
        }
    }

    /* Re-initialize the frame buffer. */
    console_display(framebuffer);
}

void splash_screen_wait_delay(void) {
    /* Ensure the splash screen is displayed for at least three seconds. */
    udelay_absolute(g_splash_start_time, 3000000);
}

void display_splash_screen_bmp(const char *custom_splash_path, void *fb_address) {
    uint8_t *splash_screen = (uint8_t *)splash_screen_bin;

    /* Try to load an external custom splash screen. */
    if ((custom_splash_path != NULL) && (custom_splash_path[0] != '\x00')) {
        if (!read_from_file(splash_screen, splash_screen_bin_size, custom_splash_path)) {
            fatal_error("Failed to read custom splash screen from %s!\n", custom_splash_path);
        }

        /* Check for 'BM' magic. */
        if ((splash_screen[0] == 'B') && (splash_screen[1] == 'M')) {
            /* Extract BMP parameters. */
            uint32_t bmp_size = (splash_screen[0x02] | (splash_screen[0x03] << 8) | (splash_screen[0x04] << 16) | (splash_screen[0x05] << 24));
            uint32_t bmp_offset = (splash_screen[0x0A] | (splash_screen[0x0B] << 8) | (splash_screen[0x0C] << 16) | (splash_screen[0x0D] << 24));
            uint32_t bmp_width = (splash_screen[0x12] | (splash_screen[0x13] << 8) | (splash_screen[0x14] << 16) | (splash_screen[0x15] << 24));
            uint32_t bmp_height = (splash_screen[0x16] | (splash_screen[0x17] << 8) | (splash_screen[0x18] << 16) | (splash_screen[0x19] << 24));
            uint16_t bmp_bpp = (splash_screen[0x1C] | (splash_screen[0x1D] << 8));
            uint32_t bmp_data_size = (splash_screen[0x22] | (splash_screen[0x23] << 8) | (splash_screen[0x24] << 16) | (splash_screen[0x25] << 24));

            /* Data size can be wrong or set to 0. In that case, we calculate it instead. */
            if (!bmp_data_size || (bmp_data_size >= bmp_size))
                bmp_data_size = (bmp_size - bmp_offset);

            /* Only accept images up to 720x1280 resolution and with 32 BPP. */
            if ((bmp_width > SPLASH_SCREEN_WIDTH_MAX) || (bmp_height > SPLASH_SCREEN_HEIGHT_MAX)) {
                fatal_error("Invalid splash screen dimensions!\n");
            } else if (bmp_bpp != SPLASH_SCREEN_BPP) {
                fatal_error("Invalid splash screen color depth!\n");
            } else if (bmp_data_size > SPLASH_SCREEN_SIZE_MAX) {
                fatal_error("Splash screen data size is too big!\n");
            }

            /* Calculate screen positions. */
            uint32_t bmp_pos_x = ((SPLASH_SCREEN_WIDTH_MAX - bmp_width) / 2);
            uint32_t bmp_pos_y = ((SPLASH_SCREEN_HEIGHT_MAX - bmp_height) / 2);

            /* Advance to data. */
            splash_screen += bmp_offset;

            /* Render the BMP. */
            render_bmp((uint32_t *)splash_screen, (uint32_t *)fb_address, bmp_width, bmp_height, bmp_pos_x, bmp_pos_y);
        } else {
            fatal_error("Invalid splash screen format!\n");
        }
    } else {
        /* Copy the pre-rendered framebuffer. */
        memcpy(fb_address, splash_screen, splash_screen_bin_size);
        console_display(fb_address);
    }

    /* Note the time we started displaying the splash. */
    g_splash_start_time = get_time_us();
}
