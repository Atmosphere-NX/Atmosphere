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
 
#include <stdint.h>

#include "utils.h"
#include "lp0.h"
#include "secmon.h"
#include "se.h"
#include "pmc.h"

/* "private" functions. */
static bool secmon_should_clear_aes_keyslot(unsigned int keyslot);
static void secmon_clear_unused_keyslots(void);

void secmon_restore_to_tzram(const uint32_t target_firmware) {
    /* Newer warmboot binaries clear the untouched keyslots for safety. */
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_500) {
        secmon_clear_unused_keyslots();
    }

    /* TODO: stuff */
}

bool secmon_should_clear_aes_keyslot(unsigned int keyslot) {
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