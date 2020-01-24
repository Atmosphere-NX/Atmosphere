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
 
#include <stdint.h>

#include "utils.h"
#include "lp0.h"
#include "secmon.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"

/* "private" functions. */
static bool secmon_should_clear_aes_keyslot(unsigned int keyslot);
static void secmon_clear_unused_keyslots(void);
static void secmon_decrypt_saved_image(void *dst, const void *src, size_t size);

void secmon_restore_to_tzram(const uint32_t target_firmware) {
    /* Newer warmboot binaries clear the untouched keyslots for safety. */
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_500) {
        secmon_clear_unused_keyslots();
    }

    /* Decrypt Secure Monitor from DRAM into TZRAM. */
    void *tzram_src = (void *)(0x80010000);
    void *tzram_dst = (void *)(target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_500 ? 0x7C012000 : 0x7C010000);
    const size_t tzram_size = 0xE000;
    secmon_decrypt_saved_image(tzram_dst, tzram_src, tzram_size);

    /* Nintendo clears DRAM, but I'm not sure why, given they lock out BPMP access to DRAM. */
    for (size_t i = 0; i < tzram_size/sizeof(uint32_t); i++) {
        ((volatile uint32_t *)tzram_src)[i] = 0;
    }

    /* Make security engine require secure busmaster. */
    se_get_regs()->SE_TZRAM_SECURITY = 0;

    /* TODO: se_verify_keys_unreadable(); */

    /* TODO: pmc_lockout_wb_scratch_registers(); */

    /* Disable fuse programming. */
    fuse_disable_programming();
}

void secmon_decrypt_saved_image(void *dst, const void *src, size_t size) {
    /* First, AES-256-CBC decrypt the image into TZRAM. */
    se_aes_256_cbc_decrypt(KEYSLOT_SWITCH_LP0TZRAMKEY, dst, size, src, size);

    /* Next, calculate CMAC. */
    uint32_t tzram_cmac[4] = {0, 0, 0, 0};
    se_compute_aes_256_cmac(KEYSLOT_SWITCH_LP0TZRAMKEY, tzram_cmac, sizeof(tzram_cmac), dst, size);

    /* Validate the MAC against saved one in PMC scratch. */
    if (tzram_cmac[0] != APBDEV_PMC_SECURE_SCRATCH112_0 ||
        tzram_cmac[1] != APBDEV_PMC_SECURE_SCRATCH113_0 ||
        tzram_cmac[2] != APBDEV_PMC_SECURE_SCRATCH114_0 ||
        tzram_cmac[3] != APBDEV_PMC_SECURE_SCRATCH115_0) {
        reboot();
    }

    /* Clear the PMC scratch registers that hold the CMAC. */
    APBDEV_PMC_SECURE_SCRATCH112_0 = 0;
    APBDEV_PMC_SECURE_SCRATCH113_0 = 0;
    APBDEV_PMC_SECURE_SCRATCH114_0 = 0;
    APBDEV_PMC_SECURE_SCRATCH115_0 = 0;

    /* Clear keyslot now that we're done with it. */
    clear_aes_keyslot(KEYSLOT_SWITCH_LP0TZRAMKEY);
}

bool secmon_should_clear_aes_keyslot(unsigned int keyslot) {
    /* We'll just compare keyslot against a hardcoded list of keys. */
    static const uint8_t saved_keyslots[6] = {
        KEYSLOT_SWITCH_LP0TZRAMKEY,
        KEYSLOT_SWITCH_SESSIONKEY,
        KEYSLOT_SWITCH_RNGKEY,
        KEYSLOT_SWITCH_MASTERKEY,
        KEYSLOT_SWITCH_DEVICEKEY,
        KEYSLOT_SWITCH_4XOLDDEVICEKEY
    };

    for (unsigned int i = 0; i < sizeof(saved_keyslots)/sizeof(saved_keyslots[0]); i++) {
        if (keyslot == saved_keyslots[i]) {
            return false;
        }
    }
    return true;
}

void secmon_clear_unused_keyslots(void) {
    /* Clear unused keyslots. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        if (secmon_should_clear_aes_keyslot(i)) {
            clear_aes_keyslot(i);
        }
        clear_aes_keyslot_iv(i);
    }
    for (unsigned int i = 0; i < KEYSLOT_RSA_MAX; i++) {
        clear_rsa_keyslot(i);
    }
}
