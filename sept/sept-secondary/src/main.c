/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "car.h"
#include "di.h"
#include "se.h"
#include "pmc.h"
#include "emc.h"
#include "sysreg.h"
#include "key_derivation.h"
#include "timers.h"
#include "fs_utils.h"
#include "stage2.h"
#include "splash.h"
#include "chainloader.h"
#include "../../../fusee/common/sdmmc/sdmmc.h"
#include "../../../fusee/common/fatfs/ff.h"
#include "../../../fusee/common/log.h"
#include "../../../fusee/common/vsprintf.h"
#include "../../../fusee/common/ini.h"
#include "../../../fusee/common/display/video_fb.h"

extern void (*__program_exit_callback)(int rc);

static void *g_framebuffer;

static bool has_rebooted(void) {
    return MAKE_REG32(0x4003FFFC) == 0xFAFAFAFA;
}

static void set_has_rebooted(bool rebooted) {
    MAKE_REG32(0x4003FFFC) = rebooted ? 0xFAFAFAFA : 0x00000000;
}

static void exfiltrate_keys_and_reboot_if_needed(uint32_t version) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    uint8_t *enc_se_state = (uint8_t *)0x4003E000;
    uint8_t *dec_se_state = (uint8_t *)0x4003F000;

    if (!has_rebooted()) {
        /* Prepare for a reboot before doing anything else. */
        prepare_for_reboot_to_self();
        set_has_rebooted(true);

        /* Derive keys. */
        derive_keys(version);

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

        /* Load keys in from decrypted state. */
        load_keys(dec_se_state);

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
    }
}

static void display_splash_screen(void) {
    /* Draw splash. */
    draw_splash((volatile uint32_t *)g_framebuffer);

    /* Turn on the backlight. */
    display_backlight(true);

    /* Ensure the splash screen is displayed for at least one second. */
    mdelay(1000);

    /* Turn off the backlight. */
    display_backlight(false);
}

static void setup_env(void) {
    g_framebuffer = (void *)0xC0000000;

    /* Initialize hardware. */
    nx_hwinit(false);

    /* Zero-fill the framebuffer and register it as printk provider. */
    video_init(g_framebuffer);

    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);

    /* Set up the exception handlers. */
    setup_exception_handlers();

    /* Mount the SD card. */
    mount_sd();
}

static void cleanup_env(void) {
    /* Unmount the SD card. */
    unmount_sd();

    /* Terminate the display. */
    display_end();
}

static void exit_callback(int rc) {
    (void)rc;
    relocate_and_chainload();
}

int sept_main(uint32_t version) {
    const char *stage2_path;
    stage2_args_t *stage2_args;
    uint32_t stage2_version = 0;
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_NONE;

    /* Validate that we can safely boot the CCPLEX. */
    if (SB_CSR_0 & 2) {
        generic_panic();
    }

    /* Extract keys from the security engine, which TSEC FW locked down. */
    exfiltrate_keys_and_reboot_if_needed(version);

    /* Override the global logging level. */
    log_set_log_level(log_level);

    /* Initialize the boot environment. */
    setup_env();

    /* Mark EMC scratch to say that sept has run. */
    MAKE_EMC_REG(EMC_SCRATCH0) |= 0x80000000;

    /* Load the loader payload into DRAM. */
    load_stage2();

    /* Display the splash screen. */
    display_splash_screen();

    /* Setup argument data. */
    stage2_path = stage2_get_program_path();
    strcpy(g_chainloader_arg_data, stage2_path);
    stage2_args = (stage2_args_t *)(g_chainloader_arg_data + strlen(stage2_path) + 1); /* May be unaligned. */
    memcpy(&stage2_args->version, &stage2_version, 4);
    memcpy(&stage2_args->log_level, &log_level, sizeof(log_level));
    strcpy(stage2_args->bct0, "");
    g_chainloader_argc = 2;

    /* Terminate the boot environment. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}
