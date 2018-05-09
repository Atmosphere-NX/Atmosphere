#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/iosupport.h>
#include "console.h"
#include "display/video_fb.h"

static ssize_t console_write(struct _reent *r, void *fd, const char *ptr, size_t len);

static const devoptab_t dotab_stdout = {
    .name = "con",
    .write_r = console_write,
};

/* https://github.com/switchbrew/libnx/blob/master/nx/source/runtime/util/utf/decode_utf8.c */
ssize_t decode_utf8(uint32_t *out, const uint8_t *in) {
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

static ssize_t console_write(struct _reent *r, void *fd, const char *ptr, size_t len) {
    size_t i = 0;
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

static bool g_console_created = false;

static int console_create(void) {
    if (g_console_created) {
        errno = EEXIST;
        return -1;
    }

    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;

    setvbuf(stdout, NULL , _IOLBF, 4096);
    setvbuf(stderr, NULL , _IONBF, 0);

    g_console_created = true;
    return 0;
}

int console_init(void *fb) {
    if (video_init(fb) == -1) {
        errno = EIO;
        return -1;
    }

    return console_create();
}

int console_resume(void *fb, int row, int col) {
   video_resume(fb, row, col); if(false){// if (video_resume(fb, row, col) == -1) {
        errno = EIO;
        return -1;
    }
    return console_create();
}
