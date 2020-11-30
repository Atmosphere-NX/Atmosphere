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
#include <stdbool.h>
#include "lz4.h"
#include "utils.h"

#define PRIMARY_START    (0x40008000)
#define PRIMARY_SIZE_MAX (0x28000)
#define PRIMARY_END      (PRIMARY_START + PRIMARY_SIZE_MAX)

void load_fusee_primary_main(void *compressed_main, size_t main_size) {
    /* Relocate the compressed binary to a place where we can safely decompress it. */
    void *relocated_main = (void *)(PRIMARY_END - main_size);
    loader_memmove(relocated_main, compressed_main, main_size);

    /* Uncompress the compressed binary. */
    lz4_uncompress((void *)PRIMARY_START, PRIMARY_SIZE_MAX, relocated_main, main_size);

    /* Jump to the newly uncompressed binary. */
    ((void (*)(void))(PRIMARY_START))();

    /* We will never reach this point. */
    __builtin_unreachable();
}