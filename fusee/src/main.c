#include "utils.h"
#include "hwinit.h"
#include "lib/printk.h"
#include "display/video_fb.h"


int main(void) {
    u32 *lfb_base;

    nx_hwinit();
    display_init();

    // Set up the display, and register it as a printk provider.
    lfb_base = display_init_framebuffer();
    video_init(lfb_base);

    // Say hello.
    printk("Welcome to Atmosphere Fusee!\n");
    printk("Using color linear framebuffer at 0x%p!\n", lfb_base);

    /* Do nothing for now */
    return 0;
}
