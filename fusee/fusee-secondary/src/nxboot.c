#include "utils.h"
#include "nxboot.h"
#include "loader.h"
#include "splash_screen.h"

/* This is the main function responsible for booting Horizon. */
void nxboot_main(void) {
    loader_ctx_t *loader_ctx = get_loader_ctx();
      
    /* TODO: Implement this function. */
    
    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path);
}