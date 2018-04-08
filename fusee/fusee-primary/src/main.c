#include "utils.h"
#include "hwinit.h"
#include "fuse.h"
#include "se.h"
#include "sd_utils.h"
#include "stage2.h"
#include "lib/printk.h"
#include "display/video_fb.h"

/* TODO: Should we allow more than 32K for the BCT0? */
#define BCT0_LOAD_ADDRESS (uintptr_t)(0x40038000)
#define BCT0_LOAD_END_ADDRESS (uintptr_t)(0x4003F000)
#define MAGIC_BCT0 0x30544342

#define DEFAULT_BCT0_FOR_DEBUG "BCT0\n[stage1]\nstage2_path = stage2.bin\nstage2_addr = 0xFFF00000\nstage2_entrypoint = 0xCAFEBABE\n"

const char *load_config(void) {
    if (!read_sd_file((void *)BCT0_LOAD_ADDRESS, BCT0_LOAD_END_ADDRESS - BCT0_LOAD_ADDRESS, "BCT.ini")) {
        printk("Failed to read BCT0 from SD!\n");
        printk("[DEBUG] Using default BCT0!\n");
        memcpy((void *)BCT0_LOAD_ADDRESS, DEFAULT_BCT0_FOR_DEBUG, sizeof(DEFAULT_BCT0_FOR_DEBUG));
        /* TODO: Stop using default. */
        /* printk("Error: Failed to load BCT.ini!\n");
         * generic_panic(); */
    }
    
    if ((*((u32 *)(BCT0_LOAD_ADDRESS))) != MAGIC_BCT0) {
        printk("Error: Unexpected magic in BCT.ini!\n");
        generic_panic();
    }
    /* Return pointer to first line of the ini. */
    const char *bct0 = (const char *)BCT0_LOAD_ADDRESS;
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
    stage2_entrypoint_t stage2_entrypoint;
    void **stage2_argv = (void **)(BCT0_LOAD_END_ADDRESS);
    const char *bct0;
    u32 *lfb_base;
    
    /* Initialize DRAM. */
    /* TODO: What can be stripped out to make this minimal? */
    nx_hwinit();
    
    /* Initialize the display. */
    display_init();
    
    /* Register the display as a printk provider. */
    lfb_base = display_init_framebuffer();
    video_init(lfb_base);
    
    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_enable_backlight(true);
    
    /* Say hello. */
    printk("Welcome to Atmosph\xe8re Fus\xe9" "e!\n");
    printk("Using color linear framebuffer at 0x%p!\n", lfb_base);
    
    /* Try to load the SBK into the security engine, if possible. */
    /* TODO: Should this be done later? */
    load_sbk();
    
    /* Load the BCT0 configuration ini off of the SD. */
    bct0 = load_config();
    
    /* Load the loader payload into DRAM. */
    stage2_entrypoint = load_stage2(bct0);
    
    /* Setup argv. */
    memset(stage2_argv, 0, STAGE2_ARGC * sizeof(*stage2_argv));
    stage2_argv[STAGE2_ARGV_PROGRAM_PATH] = (void *)stage2_get_program_path();
    stage2_argv[STAGE2_ARGV_ARGUMENT_STRUCT] = &stage2_argv[STAGE2_ARGC];
    stage2_args_t *args = (stage2_args_t *)stage2_argv[STAGE2_ARGV_ARGUMENT_STRUCT];
    
    /* Setup arguments struct. */
    args->version = 0;
    args->bct0 = bct0;
    args->lfb = (uint32_t *)lfb_base;
    args->console_col = video_get_col();
    args->console_row = video_get_row();
    
    
    /* Jump to Stage 2. */
    stage2_entrypoint(STAGE2_ARGC, stage2_argv);
    return 0;
}

