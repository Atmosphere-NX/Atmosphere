#include "utils.h"
#include "hwinit.h"
#include "fuse.h"
#include "se.h"
#include "sd_utils.h"
#include "stage2.h"
#include "chainloader.h"
#include "sdmmc.h"
#include "lib/fatfs/ff.h"
#include "lib/printk.h"
#include "display/video_fb.h"

static char g_bct0_buffer[BCTO_MAX_SIZE];

#define DEFAULT_BCT0_FOR_DEBUG \
"BCT0\n"\
"[stage1]\n"\
"stage2_path = fusee-secondary.bin\n"\
"stage2_addr = 0xFFF00000\n"\
"stage2_entrypoint = 0xFFF00000\n"

const char *load_config(void) {
    if (!read_sd_file(g_bct0_buffer, BCTO_MAX_SIZE, "BCT.ini")) {
        printk("Failed to read BCT0 from SD!\n");
        printk("[DEBUG] Using default BCT0!\n");
        memcpy(g_bct0_buffer, DEFAULT_BCT0_FOR_DEBUG, sizeof(DEFAULT_BCT0_FOR_DEBUG));
        /* TODO: Stop using default. */
        /* printk("Error: Failed to load BCT.ini!\n");
         * generic_panic(); */
    }

    if (memcmp(g_bct0_buffer, "BCT0", 4) != 0) {
        printk("Error: Unexpected magic in BCT.ini!\n");
        generic_panic();
    }
    /* Return pointer to first line of the ini. */
    const char *bct0 = g_bct0_buffer;
    while (*bct0 && *bct0 != '\n') {
        bct0++;
    }
    if (!bct0) {
        printk("Error: BCT.ini has no newline!\n");
        generic_panic();
    }
    return bct0;
}

void load_sbk(void) {
    uint32_t sbk[0x4];
    /* Load the SBK into the security engine, if relevant. */
    memcpy(sbk, (void *)get_fuse_chip_regs()->FUSE_PRIVATE_KEY, 0x10);
    for (unsigned int i = 0; i < 4; i++) {
        if (sbk[i] != 0xFFFFFFFF) {
            set_aes_keyslot(0xE, sbk, 0x10);
            break;
        }
    }
}

int main(void) {
    const char *bct0;
    u32 *lfb_base;
    const char *stage2_path;
    stage2_args_t stage2_args = {0};

    /* Initialize DRAM. */
    /* TODO: What can be stripped out to make this minimal? */
    nx_hwinit();

    /* Initialize the display. */
    display_init();

    /* Register the display as a printk provider. */
    lfb_base = display_init_framebuffer((void *)0xC0000000);
    video_init(lfb_base);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_enable_backlight(true);

    /* Say hello. */
    printk("Welcome to Atmosph\xe8re Fus\xe9" "e!\n");
    printk("Using color linear framebuffer at 0x%p!\n", lfb_base);

#ifndef I_KNOW_WHAT_I_AM_DOING
#error "Fusee is a work-in-progress bootloader, and is not ready for usage yet. If you want to play with it anyway, please #define I_KNOW_WHAT_I_AM_DOING -- and recognize that we will be unable to provide support until it is ready for general usage :)"

    printk("Warning: Fus\e9e is not yet completed, and not ready for general testing!\n");
    printk("Please do not seek support for it until it is done.\n");
    generic_panic();
#endif

    /* Try to load the SBK into the security engine, if possible. */
    /* TODO: Should this be done later? */
    load_sbk();

    /* Load the BCT0 configuration ini off of the SD. */
    bct0 = load_config();

    /* Load the loader payload into DRAM. */
    load_stage2(bct0);

    f_unmount("");

    display_enable_backlight(false);
    display_end();

    /* Setup argument data. */
    stage2_path = stage2_get_program_path();
    stage2_args.version = 0;
    strcpy(stage2_args.bct0, bct0);
    strcpy(g_chainloader_arg_data, stage2_path);
    memcpy(g_chainloader_arg_data + strlen(stage2_path) + 1, &stage2_args, sizeof(stage2_args_t));

    /* Jump to Stage 2. */
    relocate_and_chainload(STAGE2_ARGC);
    return 0;
}
