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
#include <stddef.h>
#include "panic.h"
#include "di.h"
#include "pmc.h"
#include "se.h"
#include "fuse.h"
#include "utils.h"
#include "uart.h"

static uint32_t g_panic_code = 0;

__attribute__ ((noreturn)) void panic(uint32_t code) {
    /* Set panic code. */
    if (g_panic_code == 0) {
        g_panic_code = code;
        APBDEV_PMC_SCRATCH200_0 = code;
    }

    /* Clear all keyslots. */
    for (size_t i = 0; i < 0x10; i++) {
        clear_aes_keyslot(i);
    }

    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */

    while(true);
}
