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

#include <stdio.h>
#include "pmc.h"
#include "se.h"

#define AL16 __attribute__((aligned(16)))

#define DERIVATION_ID_MAX 2

static const uint8_t AL16 keyblob_seed_00[0x10] = {
    0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3
};

static const uint8_t AL16 masterkey_seed[0x10] = {
    0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
};

static const uint8_t AL16 devicekey_seed[0x10] = {
    0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78
};

static const uint8_t AL16 devicekey_4x_seed[0x10] = {
    0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
};

static const uint8_t AL16 masterkey_4x_seed[0x10] = {
    0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66
};

static const uint8_t AL16 master_kek_seeds[DERIVATION_ID_MAX][0x10] = {
    {0x9A, 0x3E, 0xA9, 0xAB, 0xFD, 0x56, 0x46, 0x1C, 0x9B, 0xF6, 0x48, 0x7F, 0x5C, 0xFA, 0x09, 0x5C},
    {0xDE, 0xDC, 0xE3, 0x39, 0x30, 0x88, 0x16, 0xF8, 0xAE, 0x97, 0xAD, 0xEC, 0x64, 0x2D, 0x41, 0x41},
};

static const uint8_t AL16 master_devkey_seeds[DERIVATION_ID_MAX][0x10] = {
    {0x8F, 0x77, 0x5A, 0x96, 0xB0, 0x94, 0xFD, 0x8D, 0x28, 0xE4, 0x19, 0xC8, 0x16, 0x1C, 0xDB, 0x3D},
    {0x67, 0x62, 0xD4, 0x8E, 0x55, 0xCF, 0xFF, 0x41, 0x31, 0x15, 0x3B, 0x24, 0x0C, 0x7C, 0x07, 0xAE},
};

void derive_keys(void) {
    /* Set mailbox. */
    volatile uint32_t *mailbox = (volatile uint32_t *)0x4003FF00;
    const uint32_t derivation_id = *((volatile uint32_t *)0x4003E800);

    if (derivation_id < DERIVATION_ID_MAX) {
        uint8_t *partial_se_state = (uint8_t *)0x4000FFC0;
        uint8_t *enc_se_state = (uint8_t *)0x4003E000;

        volatile tegra_pmc_t *pmc = pmc_get_regs();
        uint32_t AL16 work_buffer[4];

        /* Save a partial context, only the keyslots we want. */
        /* We can't avoid touching memory, but save to a location that the bootrom will overwrite during init. */
        se_set_in_context_save_mode(true);
        se_save_partial_context(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY, partial_se_state);
        se_set_in_context_save_mode(false);

        /* Clear the copy of the root key still inside the SE. */
        clear_aes_keyslot(0xD);

        /* Copy SRK into keyslot 0xE, clear it. */
        {
            work_buffer[0] = pmc->secure_scratch4;
            pmc->secure_scratch4 = 0xCCCCCCCC;
            work_buffer[1] = pmc->secure_scratch5;
            pmc->secure_scratch5 = 0xCCCCCCCC;
            work_buffer[2] = pmc->secure_scratch6;
            pmc->secure_scratch6 = 0xCCCCCCCC;
            work_buffer[3] = pmc->secure_scratch7;
            pmc->secure_scratch7 = 0xCCCCCCCC;
            set_aes_keyslot(0xE, work_buffer, 0x10);
            for (size_t i = 0; i < 4; i++) {
                work_buffer[i] = 0xCCCCCCCC;
            }
        }

        /* Decrypt SE state. */
        se_aes_128_cbc_decrypt(0xE, partial_se_state, 0x40, partial_se_state, 0x40);

        /* Clear keyslots to wipe IVs. */
        clear_aes_keyslot(0xE);
        clear_aes_keyslot(0xF);

        /* Mov root key into keyslot 0xE. Clear first to wipe from IV. */
        set_aes_keyslot(0xE, partial_se_state + 0x30, 0x10);
        for (size_t i = 0; i < 4; i++) {
            *((volatile uint32_t *)(partial_se_state + 0x30)) = 0xCCCCCCCC;
        }

        /* Derive master kek. */
        decrypt_data_into_keyslot(0xE, 0xE, master_kek_seeds[derivation_id], 0x10);

        /* Derive master key, device master key. */
        decrypt_data_into_keyslot(0xC, 0xE, masterkey_seed, 0x10);
        decrypt_data_into_keyslot(0xE, 0xE, masterkey_4x_seed, 0x10);

        /* Derive Keyblob Key 00. */
        set_aes_keyslot(0xF, partial_se_state + 0x20, 0x10);
        se_aes_ecb_decrypt_block(0xF, work_buffer, 0x10, keyblob_seed_00, 0x10);
        set_aes_keyslot(0xF, partial_se_state + 0x10, 0x10);
        decrypt_data_into_keyslot(0xF, 0xF, work_buffer, 0x10);

        /* Clear TSEC key + SBK. */
        for (size_t i = 0; i < 8; i++) {
            *((volatile uint32_t *)(partial_se_state + 0x10)) = 0xCCCCCCCC;
        }

        /* Derive device keys. */
        decrypt_data_into_keyslot(0xA, 0xF, devicekey_4x_seed, 0x10);
        decrypt_data_into_keyslot(0xF, 0xF, devicekey_seed, 0x10);

        /* Derive firmware specific device key. */
        se_aes_ecb_decrypt_block(0xA, work_buffer, 0x10, master_devkey_seeds[derivation_id], 0x10);
        decrypt_data_into_keyslot(0xE, 0xE, work_buffer, 0x10);

        /* Clear work buffer. */
        for (size_t i = 0; i < 4; i++) {
            work_buffer[i] = 0xCCCCCCCC;
        }

        /* Save context for real. */
        se_set_in_context_save_mode(true);
        se_save_context(KEYSLOT_SWITCH_SRKGENKEY, KEYSLOT_SWITCH_RNGKEY, enc_se_state);
        se_set_in_context_save_mode(false);
    }

    /* Clear all keyslots. */
    for (size_t i = 0; i < 0x10; i++) {
        clear_aes_keyslot(i);
    }

    *(volatile uint32_t *)(0x4003FFC0) = 0xCACACACA;

    *mailbox = 7;
    while (1) { /* Wait for sept to handle the rest. */ }
}
