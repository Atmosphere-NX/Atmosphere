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
#include "../../../fusee/common/display/video_fb.h"

static void *g_framebuffer = NULL;
static bool g_display_initialized = false;

static ssize_t console_write(struct _reent *r, void *fd, const char *ptr, size_t len);
static void console_init_display(void);

static const devoptab_t dotab_stdout = {
    .name = "con",
    .write_r = console_write,
};

/* https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/util/utf/decode_utf8.c */
static ssize_t decode_utf8(uint32_t *out, const uint8_t *in) {
    uint8_t code1, code2, code3, code4;

    code1 = *in++;
    if(code1 < 0x80) {
        /* 1-byte sequence */
        *out = code1;
        return 1;
    } else if(code1 < 0xC2) {
        return -1;
    } else if(code1 < 0xE0) {
        /* 2-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80) {
            return -1;
        }

        *out = (code1 << 6) + code2 - 0x3080;
        return 2;
    } else if(code1 < 0xF0) {
        /* 3-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80) {
            return -1;
        }
        if(code1 == 0xE0 && code2 < 0xA0) {
            return -1;
        }

        code3 = *in++;
        if((code3 & 0xC0) != 0x80) {
            return -1;
        }

        *out = (code1 << 12) + (code2 << 6) + code3 - 0xE2080;
        return 3;
    } else if(code1 < 0xF5) {
        /* 4-byte sequence */
        code2 = *in++;
        if((code2 & 0xC0) != 0x80) {
            return -1;
        }
        if(code1 == 0xF0 && code2 < 0x90) {
            return -1;
        }
        if(code1 == 0xF4 && code2 >= 0x90) {
            return -1;
        }

        code3 = *in++;
        if((code3 & 0xC0) != 0x80) {
            return -1;
        }

        code4 = *in++;
        if((code4 & 0xC0) != 0x80) {
            return -1;
        }

        *out = (code1 << 18) + (code2 << 12) + (code3 << 6) + code4 - 0x3C82080;
        return 4;
    }

    return -1;
}

static void console_init_display(void) {
    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_backlight(true);

    /* Display is initialized. */
    g_display_initialized = true;
}

static ssize_t console_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    size_t i = 0;
    if (!g_display_initialized && (g_framebuffer != NULL)) {
        console_init_display();
    }
    while (i < len) {
        uint32_t chr;
        ssize_t n = decode_utf8(&chr, (uint8_t *)(ptr + i));
        if (n == -1) {
            break;
        } else {
            i += (size_t)n;
            video_putc(chr > 0xFF ? '?' : (char)chr);
        }
    }

    return i;
}

static int console_create(void) {
    if (g_framebuffer != NULL) {
        errno = EEXIST;
        return -1;
    }
    
    /* Allocate memory for the framebuffer. */
    g_framebuffer = memalign(0x1000, CONFIG_VIDEO_VISIBLE_ROWS * CONFIG_VIDEO_COLS * CONFIG_VIDEO_PIXEL_SIZE);

    if (g_framebuffer == NULL) {
        errno = ENOMEM;
        return -1;
    }

    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;

    setvbuf(stdout, NULL , _IOLBF, 4096);
    setvbuf(stderr, NULL , _IONBF, 0);

    return 0;
}

int console_init(void) {
    if (console_create() == -1) {
        return -1;
    }

    /* Zero-fill the framebuffer, etc. */
    if (video_init(g_framebuffer) == -1) {
        errno = EIO;
        console_end();
        return -1;
    }

    return 0;
}

void *console_get_framebuffer(void) {
    if (!g_display_initialized && (g_framebuffer != NULL)) {
        console_init_display();
    }
    return g_framebuffer;
}

int console_display(const void *framebuffer) {
    if (!g_display_initialized && (g_framebuffer != NULL)) {
        console_init_display();
    }
    display_init_framebuffer((void *)framebuffer);
    return 0;
}

int console_resume(void) {
    if (!g_display_initialized && (g_framebuffer != NULL)) {
        console_init_display();
    } else {
        display_init_framebuffer(g_framebuffer);
    }
    return 0;
}

int console_end(void) {
    if (g_display_initialized) {    
        /* Turn off the backlight. */
        display_backlight(false);
        
        /* Terminate the display. */
        display_end();
        
        /* Display is terminated. */
        g_display_initialized = false;
    }
    free(g_framebuffer);
    g_framebuffer = NULL;
    return 0;
}
