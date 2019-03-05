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
 
#ifndef FUSEE_EXTKEYS_H
#define FUSEE_EXTKEYS_H

#include <string.h>
#include "utils.h"
#include "masterkey.h"

typedef struct {
    unsigned char tsec_root_keys[0x20][0x10];
    unsigned char master_keks[0x20][0x10];
} fusee_extkeys_t;

void parse_hex_key(unsigned char *key, const char *hex, unsigned int len);
void extkeys_initialize_keyset(fusee_extkeys_t *keyset, FILE *f);

#endif