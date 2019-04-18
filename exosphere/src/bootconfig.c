/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stdbool.h>
#include <string.h>

#include "utils.h"
#include "se.h"
#include "configitem.h"
#include "fuse.h"
#include "bootconfig.h"

static boot_reason_t g_boot_reason = {0};
static uint64_t g_package2_hash_for_recovery[4] = {0};

bool bootconfig_matches_hardware_info(void) {
    uint32_t hardware_info[4];
    fuse_get_hardware_info(hardware_info);
    return memcmp(LOADED_BOOTCONFIG->signed_config.hardware_info, hardware_info, sizeof(hardware_info)) == 0;
}

void bootconfig_load_and_verify(const bootconfig_t *bootconfig) {
    static const uint8_t bootconfig_modulus[RSA_2048_BYTES] = {
        0xB5, 0x96, 0x87, 0x31, 0x39, 0xAA, 0xBB, 0x3C, 0x28, 0xF3, 0xF0, 0x65, 0xF1, 0x50, 0x70, 0x64,
        0xE6, 0x6C, 0x97, 0x50, 0xCD, 0xA6, 0xEE, 0xEA, 0xC3, 0x8F, 0xE6, 0xB5, 0x81, 0x54, 0x65, 0x33,
        0x1B, 0x88, 0x4B, 0xCE, 0x9F, 0x53, 0xDF, 0xE4, 0xF6, 0xAD, 0xC3, 0x78, 0xD7, 0x3C, 0xD1, 0xDB,
        0x27, 0x21, 0xA0, 0x24, 0x30, 0x2D, 0x98, 0x41, 0xA8, 0xDF, 0x50, 0x7D, 0xAB, 0xCE, 0x00, 0xD9,
        0xCB, 0xAC, 0x8F, 0x37, 0xF5, 0x53, 0xE4, 0x97, 0x1F, 0x13, 0x3C, 0x19, 0xFF, 0x05, 0xA7, 0x3B,
        0xF6, 0xF4, 0x01, 0xDE, 0xF0, 0xC3, 0x77, 0x7B, 0x83, 0xBA, 0xAF, 0x99, 0x30, 0x94, 0x87, 0x25,
        0x4E, 0x54, 0x42, 0x3F, 0xAC, 0x27, 0xF9, 0xCC, 0x87, 0xDD, 0xAE, 0xF2, 0x54, 0xF3, 0x97, 0x49,
        0xF4, 0xB0, 0xF8, 0x6D, 0xDA, 0x60, 0xE0, 0xFD, 0x57, 0xAE, 0x55, 0xA9, 0x76, 0xEA, 0x80, 0x24,
        0xA0, 0x04, 0x7D, 0xBE, 0xD1, 0x81, 0xD3, 0x0C, 0x95, 0xCF, 0xB7, 0xE0, 0x2D, 0x21, 0x21, 0xFF,
        0x97, 0x1E, 0xB3, 0xD7, 0x9F, 0xBB, 0x33, 0x0C, 0x23, 0xC5, 0x88, 0x4A, 0x33, 0xB9, 0xC9, 0x4E,
        0x1E, 0x65, 0x51, 0x45, 0xDE, 0xF9, 0x64, 0x7C, 0xF0, 0xBF, 0x11, 0xB4, 0x93, 0x8D, 0x5D, 0xC6,
        0xAB, 0x37, 0x9E, 0xE9, 0x39, 0xC1, 0xC8, 0xDB, 0xB9, 0xFE, 0x45, 0xCE, 0x7B, 0xDD, 0x72, 0xD9,
        0x6F, 0x68, 0x13, 0xC0, 0x4B, 0xBA, 0x00, 0xF4, 0x1E, 0x89, 0x71, 0x91, 0x26, 0xA6, 0x46, 0x12,
        0xDF, 0x29, 0x6B, 0xC2, 0x5A, 0x53, 0xAF, 0xB9, 0x5B, 0xFD, 0x13, 0x9F, 0xD1, 0x8A, 0x7C, 0xB5,
        0x04, 0xFD, 0x69, 0xEA, 0x23, 0xB4, 0x6D, 0x16, 0x21, 0x98, 0x54, 0xB4, 0xDF, 0xE6, 0xAB, 0x93,
        0x36, 0xB6, 0xD2, 0x43, 0xCF, 0x2B, 0x98, 0x1D, 0x45, 0xC9, 0xBB, 0x20, 0x42, 0xB1, 0x9D, 0x1D
    };
    memcpy(LOADED_BOOTCONFIG, bootconfig, sizeof(bootconfig_t));
    /* TODO: Should these restrictions be loosened for Exosphere? */
    if (configitem_is_retail()
     || se_rsa2048_pss_verify(LOADED_BOOTCONFIG->signature, RSA_2048_BYTES, bootconfig_modulus, RSA_2048_BYTES, &LOADED_BOOTCONFIG->signed_config, sizeof(LOADED_BOOTCONFIG->signed_config)) != 0
     || !bootconfig_matches_hardware_info()) {
        /* Clear signed config. */
        memset(&LOADED_BOOTCONFIG->signed_config, 0, sizeof(LOADED_BOOTCONFIG->signed_config));
    }
}

void bootconfig_clear(void){
    memset(LOADED_BOOTCONFIG, 0, sizeof(bootconfig_t));
}

/* Actual configuration getters. */
bool bootconfig_is_package2_plaintext(void) {
    return (LOADED_BOOTCONFIG->signed_config.package2_config & 1) != 0;
}

bool bootconfig_is_package2_unsigned(void) {
    return (LOADED_BOOTCONFIG->signed_config.package2_config & 2) != 0;
}

void bootconfig_set_package2_plaintext_and_unsigned(void) {
    LOADED_BOOTCONFIG->signed_config.package2_config |= 3;
}

bool bootconfig_disable_program_verification(void) {
     return LOADED_BOOTCONFIG->signed_config.disable_program_verification != 0;
}

bool bootconfig_is_debug_mode(void) {
    return (LOADED_BOOTCONFIG->unsigned_config.data[0x10] & 2) != 0;
}

bool bootconfig_take_extabt_serror_to_el3(void) {
    return (LOADED_BOOTCONFIG->unsigned_config.data[0x10] & 6) != 6;
}

uint64_t bootconfig_get_value_for_sysctr0(void) {
    if (LOADED_BOOTCONFIG->unsigned_config.data[0x24] & 1) {
        return *(volatile uint64_t *)(&(LOADED_BOOTCONFIG->unsigned_config.data[0x30]));
    } else {
        return 0ULL;
    }
}

uint64_t bootconfig_get_memory_arrangement(void) {
    /* TODO: This function has changed pretty significantly since we implemented it. */
    /* Not relevant for retail, but we'll probably want this to be accurate sooner or later. */
    if (bootconfig_is_debug_mode()) {
        if (fuse_get_dram_id() == 4) {
            if (LOADED_BOOTCONFIG->unsigned_config.data[0x23]) {
                return (uint64_t)(LOADED_BOOTCONFIG->unsigned_config.data[0x23]);
            } else {
                return 0x11ull;
            }
        } else {
            if (LOADED_BOOTCONFIG->unsigned_config.data[0x23]) {
                if ((LOADED_BOOTCONFIG->unsigned_config.data[0x23] & 0x30) == 0) {
                    return (uint64_t)(LOADED_BOOTCONFIG->unsigned_config.data[0x23]);
                } else {
                    return 1ull;
                }
            } else {
                return 1ull;
            }
        }
    } else {
        return 1ull;
    }
}

uint64_t bootconfig_get_kernel_configuration(void) {
    if (bootconfig_is_debug_mode()) {
        uint64_t high_val = 0;
        if (fuse_get_dram_id() == 4) {
            if (LOADED_BOOTCONFIG->unsigned_config.data[0x23]) {
                high_val =  ((uint64_t)(LOADED_BOOTCONFIG->unsigned_config.data[0x23]) >> 4) & 0x3;
            } else {
                high_val = 0x1;
            }
        }
        return (high_val << 16) | (((uint64_t)(LOADED_BOOTCONFIG->unsigned_config.data[0x21])) << 8) | ((uint64_t)(LOADED_BOOTCONFIG->unsigned_config.data[0x11]));
    } else {
        return 0ull;
    }
}

void bootconfig_load_boot_reason(volatile boot_reason_t *boot_reason) {
    g_boot_reason = *boot_reason;
}

void bootconfig_set_package2_hash_for_recovery(const void *package2, size_t package2_size) {
    se_calculate_sha256(g_package2_hash_for_recovery, package2, package2_size);
}

void bootconfig_get_package2_hash_for_recovery(uint64_t *out_hash) {
    for (unsigned int i = 0; i < 4; i++) {
        out_hash[i] = g_package2_hash_for_recovery[i];
    }
}

bool bootconfig_is_recovery_boot(void) {
    return ((g_boot_reason.bootloader_attribute & 0x01) != 0);
}

uint64_t bootconfig_get_boot_reason(void) {
    return ((uint64_t)g_boot_reason.boot_reason_state << 24) | (g_boot_reason.boot_reason_value & 0xFFFFFF);
}
