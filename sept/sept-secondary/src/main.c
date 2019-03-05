/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "utils.h"
#include "exception_handlers.h"
#include "panic.h"
#include "hwinit.h"
#include "di.h"
#include "se.h"
#include "pmc.h"
#include "emc.h"
#include "key_derivation.h"
#include "timers.h"
#include "fs_utils.h"
#include "stage2.h"
#include "splash.h"
#include "chainloader.h"
#include "sdmmc/sdmmc.h"
#include "lib/fatfs/ff.h"
#include "lib/log.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"
#include "display/video_fb.h"

extern void (*__program_exit_callback)(int rc);

static void *g_framebuffer;

static uint32_t g_tsec_root_key[0x4] = {0};
static uint32_t g_tsec_key[0x4] = {0};

static bool has_rebooted(void) {
    return MAKE_REG32(0x4003FFFC) == 0xFAFAFAFA;
}

static void set_has_rebooted(bool rebooted) {
    MAKE_REG32(0x4003FFFC) = rebooted ? 0xFAFAFAFA : 0x00000000;
}


static void exfiltrate_keys_and_reboot_if_needed(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    uint8_t *enc_se_state = (uint8_t *)0x4003E000;
    uint8_t *dec_se_state = (uint8_t *)0x4003F000;
    
    if (!has_rebooted()) {
        /* Prepare for a reboot before doing anything else. */
        prepare_for_reboot_to_self();
        set_has_rebooted(true);
        
        /* Save the security engine context. */
        se_get_regs()->_0x4 = 0x0;
        se_set_in_context_save_mode(true);
        se_save_context(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY, enc_se_state);
        se_set_in_context_save_mode(false);
                
        /* Clear all keyslots. */
        for (size_t k = 0; k < 0x10; k++) {
            clear_aes_keyslot(k);
        }
        
        reboot_to_self();
    } else {
        /* Decrypt the security engine state. */
        uint32_t ALIGN(16) context_key[4];
        context_key[0] = pmc->secure_scratch4;
        context_key[1] = pmc->secure_scratch5;
        context_key[2] = pmc->secure_scratch6;
        context_key[3] = pmc->secure_scratch7;
        set_aes_keyslot(0xC, context_key, sizeof(context_key));
        se_aes_128_cbc_decrypt(0xC, dec_se_state, 0x840, enc_se_state, 0x840);
        
        /* Copy out tsec key + tsec root key. */
        for (size_t i = 0; i < 0x10; i += 4) {
            g_tsec_key[i/4] = MAKE_REG32((uintptr_t)(dec_se_state) + 0x1B0 + i);
            g_tsec_root_key[i/4] = MAKE_REG32((uintptr_t)(dec_se_state) + 0x1D0 + i);
        }
        
        /* Clear the security engine state. */
        for (size_t i = 0; i < 0x840; i += 4) {
            MAKE_REG32((uintptr_t)(enc_se_state) + i) = 0xCCCCCCCC;
            MAKE_REG32((uintptr_t)(dec_se_state) + i) = 0xCCCCCCCC;
        }
        for (size_t i = 0; i < 4; i++) {
            context_key[i] = 0xCCCCCCCC;
        }
        pmc->secure_scratch4 = 0xCCCCCCCC;
        pmc->secure_scratch5 = 0xCCCCCCCC;
        pmc->secure_scratch6 = 0xCCCCCCCC;
        pmc->secure_scratch7 = 0xCCCCCCCC;
        
        /* Clear all keyslots except for SBK/SSK. */
        for (size_t k = 0; k < 0xE; k++) {
            clear_aes_keyslot(k);
        }
    }
}

static void setup_env(void) {
    g_framebuffer = (void *)0xC0000000;

    /* Initialize hardware. */
    nx_hwinit();

    /* Check for panics. */
    check_and_display_panic();

    /* Zero-fill the framebuffer and register it as printk provider. */
    video_init(g_framebuffer);

    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);
    
    /* Draw splash. */
    draw_splash((volatile uint32_t *)g_framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_backlight(true);

    /* Set up the exception handlers. */
    setup_exception_handlers();
        
    /* Mount the SD card. */
    mount_sd();
}

static void cleanup_env(void) {
    /* Unmount the SD card. */
    unmount_sd();

    display_backlight(false);
    display_end();
}

static void exit_callback(int rc) {
    (void)rc;
    relocate_and_chainload();
}

int main(void) {
    const char *stage2_path;
    stage2_args_t *stage2_args;
    uint32_t stage2_version = 0;
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_NONE;
    
    /* Extract keys from the security engine, which TSEC FW locked down. */
    exfiltrate_keys_and_reboot_if_needed();
    
    /* Override the global logging level. */
    log_set_log_level(log_level);
    
    /* Initialize the display, console, etc. */
    setup_env();
        
    /* Derive keys. */
    derive_7x_keys(g_tsec_key, g_tsec_root_key);
    
    /* Cleanup keys in memory. */
    for (size_t i = 0; i < 0x10; i += 4) {
        g_tsec_root_key[i/4] = 0xCCCCCCCC;
        g_tsec_key[i/4] = 0xCCCCCCCC;
    }
    
    /* Mark EMC scratch to say that sept has run. */
    MAKE_EMC_REG(EMC_SCRATCH0) |= 0x80000000;
    
    /* Load the loader payload into DRAM. */
    load_stage2();

    /* Setup argument data. */
    log_level = SCREEN_LOG_LEVEL_MANDATORY;
    stage2_path = stage2_get_program_path();
    strcpy(g_chainloader_arg_data, stage2_path);
    stage2_args = (stage2_args_t *)(g_chainloader_arg_data + strlen(stage2_path) + 1); /* May be unaligned. */
    memcpy(&stage2_args->version, &stage2_version, 4);
    memcpy(&stage2_args->log_level, &log_level, sizeof(log_level));
    stage2_args->display_initialized = false;
    strcpy(stage2_args->bct0, "");
    g_chainloader_argc = 2;
    
    /* Wait a while. */
    mdelay(1500);
    
    /* Deinitialize the display, console, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}
