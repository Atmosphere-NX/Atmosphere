#include "utils.h"
#include "hwinit.h"
#include "fuse.h"
#include "se.h"
#include "sd_utils.h"

#define LOADER_ADDRESS (void (*)(void))(0xFFF00000) 

void load_stage2(void *dst_address) {
    if (!read_sd_file(dst_address, "fusee-loader.bin")) {
        /* TODO: This is the failure condition. How should we panic? */
    }
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
    void (*loader)(void) = LOADER_ADDRESS;
    
    /* Initialize DRAM. */
    /* TODO: What can be stripped out to make this minimal? */
    nx_hwinit();
    
    /* Try to load the SBK into the security engine, if possible. */
    /* TODO: Should this be done later? */
    load_sbk();
    
    /* Load the loader payload into DRAM. */
    load_stage2(LOADER_ADDRESS);
    
    /* TODO: Should the loader take argc/argv? */
    loader();
    return 0;
}

