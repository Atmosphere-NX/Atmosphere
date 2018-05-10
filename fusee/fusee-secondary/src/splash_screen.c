#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "utils.h"
#include "timers.h"
#include "splash_screen.h"
#include "sd_utils.h"
#include "display/video_fb.h"

uint8_t *load_default_splash() {
    printf("Failed to read custom splash. Falling back to default splash");
    uint8_t *buffer = calloc(default_splash_bmp_size, sizeof(uint8_t));
    memcpy(buffer, default_splash_bmp, default_splash_bmp_size);
    return buffer;
}

void display_splash_screen_bmp(const char *custom_splash_path, uint8_t* fb_addr) {
    /* Cast the framebuffer to a 2-dimensional array, for easy addressing */
    uint8_t (*fb)[3072] = (uint8_t(*)[3072])fb_addr;

    uint8_t *splash_screen = NULL;

    /* Stat the splash file to set size information, and see if it's there */
    struct stat info;
    if (!stat(custom_splash_path, &info)) {
        /* Prepare the buffer */
        splash_screen = calloc(info.st_size, sizeof(uint8_t));

        /* Read the BMP from the SD card */
        if (!read_sd_file(splash_screen, info.st_size, custom_splash_path)) {
            splash_screen = load_default_splash();
        }
    } else {
        splash_screen = load_default_splash();
    }

    /* BMP pixel data offset (dword @ 0xA) */
    int data_offset = (splash_screen[0xA] & 0xFF) |
        ((splash_screen[0xB] & 0xFF00) << 8) |
        ((splash_screen[0xC] & 0xFF0000) << 16) |
        ((splash_screen[0xD] & 0xFF000000) << 24);

    int count = 0;
    for (int y = 719; y >= 0; y--) {
        for (int x = 1280; x > 0; x--) {
            /* Fill the framebuffer w/ necessary pixel format translations (framebuffer is RGBA, BMP is BGRA) */
            fb[x][y * 4] = splash_screen[data_offset + count + 2];
            fb[x][y * 4 + 1] = splash_screen[data_offset + count + 1];
            fb[x][y * 4 + 2] = splash_screen[data_offset + count];
            fb[x][y * 4 + 3] = splash_screen[data_offset + count + 3];
            count += 4;
        }
    }

    /* Free the splash buffer; we're done with it. */
    free(splash_screen);

    /* Display the splash screen for three seconds. */
    wait(3000000);
}
