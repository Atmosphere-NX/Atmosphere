#include <stdio.h>
#include "utils.h"
#include "timers.h"
#include "splash_screen.h"
#include "fs_utils.h"
#include "display/video_fb.h"

void display_splash_screen_bmp(const char *custom_splash_path) {
    uint8_t *splash_screen = g_default_splash_screen;
    if (custom_splash_path != NULL && custom_splash_path[0] != '\x00') {
        if (!read_from_file(splash_screen, sizeof(g_default_splash_screen), custom_splash_path)) {
            printf("Error: Failed to read custom splash screen from %s!\n", custom_splash_path);
            generic_panic();
        }
    }

    /* TODO: Display the splash screen. It should be a pointer to a BMP, at this point. */

    /* Display the splash screen for three seconds. */
    wait(3000000);
}
