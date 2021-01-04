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

#ifndef FUSEE_PACKAGE1_H
#define FUSEE_PACKAGE1_H

#include <stdio.h>
#include "key_derivation.h"

#define PACKAGE1LOADER_SIZE_MAX 0x40000

typedef struct {
    uint32_t package1loader_hash;
    uint32_t secmon_hash;
    uint32_t nx_bootloader_hash;
    uint32_t _0xC;
    char build_timestamp[0x0E];
    uint8_t _0x1E;
    uint8_t version;
} package1loader_header_t;

typedef struct {
    char magic[4];
    uint32_t warmboot_size;
    uint32_t warmboot_offset;
    uint32_t _0xC;
    uint32_t nx_bootloader_size;
    uint32_t nx_bootloader_offset;
    uint32_t secmon_size;
    uint32_t secmon_offset;
    uint8_t data[];
} package1_header_t;

typedef struct {
    uint8_t aes_mac[0x10];
    uint8_t rsa_sig[0x100];
    uint8_t salt[0x20];
    uint8_t hash[0x20];
    uint32_t bl_version;
    uint32_t bl_size;
    uint32_t bl_load_addr;
    uint32_t bl_entrypoint;
    uint8_t _0x160[0x10];
    uint8_t data[];
} pk11_mariko_oem_header_t;

bool package1_is_custom_public_key(const void *bct, bool mariko);

int package1_read_and_parse_boot0_erista(void **package1loader, size_t *package1loader_size, nx_keyblob_t *keyblobs, uint32_t *revision, FILE *boot0);
int package1_read_and_parse_boot0_mariko(void **package1loader, size_t *package1loader_size, FILE *boot0);

bool package1_get_tsec_fw(void **tsec_fw, const void *package1loader, size_t package1loader_size);
size_t package1_get_encrypted_package1(package1_header_t **package1, uint8_t *ctr, const void *package1loader, size_t package1loader_size);

/* Must be aligned to 16 bytes. */
bool package1_decrypt(package1_header_t *package1, size_t package1_size, const uint8_t *ctr);
void *package1_get_warmboot_fw(const package1_header_t *package1);

#endif
