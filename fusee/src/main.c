#include "utils.h"
#include "hwinit.h"

int main(void) {
    nx_hwinit();
    display_init();
    display_color_screen(0xFFFFFFFF);

    /* Do nothing for now */
    return 0;
}
