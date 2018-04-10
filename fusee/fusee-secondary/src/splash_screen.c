#include "utils.h"
#include "timers.h"
#include "splash_screen.h"
#include "sd_utils.h"
#include "lib/printk.h"
#include "display/video_fb.h"

void display_splash_screen_bmp(const char *custom_splash_path) {
    unsigned char *splash_screen = g_default_splash_screen;
    if (custom_splash_path != NULL && custom_splash_path[0] != '\x00') {
        if (!read_sd_file(splash_screen, sizeof(g_default_splash_screen), custom_splash_path)) {
            printk("Error: Failed to read custom splash screen from %s!\n", custom_splash_path);
            generic_panic();
        }
    }
    
    /* TODO: Display the splash screen. It should be a pointer to a BMP, at this point. */
    
    /* Display the splash screen for three seconds. */
    wait(3000000);
}