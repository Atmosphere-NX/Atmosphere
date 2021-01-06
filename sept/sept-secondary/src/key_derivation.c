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

#include <stdio.h>
#include "key_derivation.h"
#include "se.h"
#include "cluster.h"
#include "timers.h"
#include "fuse.h"
#include "uart.h"
#include "utils.h"

#define u8 uint8_t
#define u32 uint32_t
#include "key_derivation_bin.h"
#undef u8
#undef u32


void derive_keys(uint32_t version) {
    /* Clear mailbox. */
    volatile uint32_t *mailbox = (volatile uint32_t *)0x4003FF00;
    while (*mailbox != 0) {
        *mailbox = 0;
    }

    /* Set derivation id. */
    *((volatile uint32_t *)0x4003E800) = version;

    /* Copy key derivation stub into IRAM high. */
    for (size_t i = 0; i < key_derivation_bin_size; i += sizeof(uint32_t)) {
        write32le((void *)0x4003D000, i, read32le(key_derivation_bin, i));
    }

    cluster_boot_cpu0(0x4003D000);

    while (*mailbox != 7) {
        /* Wait until keys have been derived. */
    }
}

void load_keys(const uint8_t *se_state) {
    /* Clear keyslots up to 0xA. */
    for (size_t i = 0; i < 0xA; i++) {
        clear_aes_keyslot(i);
    }

    /* Copy device keygen key out of state keyslot 0xA into keyslot 0xA. */
    set_aes_keyslot(0xA, se_state + 0x30 + (0xA * 0x20), 0x10);

    /* Clear keyslot 0xB. */
    clear_aes_keyslot(0xB);

    /* Copy firmware device key out of state keyslot 0xE into keyslot 0xC. */
    set_aes_keyslot(0xC, se_state + 0x30 + (0xE * 0x20), 0x10);

    /* Copy master key out of state keyslot 0xC into keyslot 0xD. */
    set_aes_keyslot(0xD, se_state + 0x30 + (0xC * 0x20), 0x10);

    /* Clear keyslot 0xE. */
    clear_aes_keyslot(0xE);

    /* Copy device key out of state keyslot 0xF into keyslot 0xF. */
    set_aes_keyslot(0xF, se_state + 0x30 + (0xF * 0x20), 0x10);

    /* Set keyslot flags properly in preparation for secmon. */
    set_aes_keyslot_flags(0xE, 0x15);
    set_aes_keyslot_flags(0xC, 0x15);
}
